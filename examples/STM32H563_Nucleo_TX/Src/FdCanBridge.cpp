/** \copyright
 * Copyright (c) 2025
 * All rights reserved.
 *
 * \file FdCanBridge.cpp
 * Bridge between STM32 HAL FDCAN driver and OpenMRNLite CAN hub.
 *
 * @date January 2026
 */

#include "main.h"
#include "app_azure_rtos.h"
#include <OpenMRNLite.h>
#include "can_frame.h"
#include <cstring>

extern FDCAN_HandleTypeDef hfdcan1; // From main.c

class FdCanBridge {
public:
    FdCanBridge(FDCAN_HandleTypeDef *hfdcan, CanHubFlow *hub)
        : hfdcan_(hfdcan), canHub_(hub), txPending_(false), writePort_(this) {}
    
    void init() {
        // Configure FDCAN to start receiving
        if (HAL_FDCAN_Start(hfdcan_) != HAL_OK) {
            Error_Handler();
        }
        
        // Enable RX FIFO 0 new message interrupt
        if (HAL_FDCAN_ActivateNotification(hfdcan_, 
                FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) {
            Error_Handler();
        }
        
        // Enable TX complete interrupt
        if (HAL_FDCAN_ActivateNotification(hfdcan_, 
                FDCAN_IT_TX_COMPLETE, 0) != HAL_OK) {
            Error_Handler();
        }
        
        // Register the write port with the CAN hub to receive outgoing frames
        // DEBUG: Set breakpoint here to verify this is called
        volatile bool registering = true;
        canHub_->register_port(&writePort_);
        registering = false; // Set breakpoint and check canHub_ is valid
        (void)registering;
    }
    
    /// Call this from HAL RX interrupt callback
    void rx_interrupt_handler() {
        FDCAN_RxHeaderTypeDef rxHeader;
        uint8_t rxData[64];
        
        if (HAL_FDCAN_GetRxMessage(hfdcan_, FDCAN_RX_FIFO0, 
                                    &rxHeader, rxData) == HAL_OK) {
            // Send to OpenMRN hub (this is interrupt context safe)
            auto *b = canHub_->alloc();
            if (b) {
                // Get the embedded can_frame and initialize it
                struct can_frame *frame = b->data()->mutable_frame();
                
                // Set the CAN ID
                if (rxHeader.IdType == FDCAN_EXTENDED_ID) {
                    SET_CAN_FRAME_ID_EFF(*frame, rxHeader.Identifier);
                    SET_CAN_FRAME_EFF(*frame);
                } else {
                    SET_CAN_FRAME_ID(*frame, rxHeader.Identifier);
                    CLR_CAN_FRAME_EFF(*frame);
                }
                
                // Convert DLC from FDCAN format to standard CAN DLC
                uint32_t dlc_code = rxHeader.DataLength >> 16;
                if (dlc_code <= 8) {
                    frame->can_dlc = dlc_code;
                } else {
                    // For FDCAN, DLC > 8 are special codes, map to 8 for classic CAN
                    frame->can_dlc = 8;
                }
                
                memcpy(frame->data, rxData, frame->can_dlc);
                
                // Skip our own write port to prevent loopback
                b->data()->skipMember_ = &writePort_;
                canHub_->send(b);
            }
        }
    }
    
    /// Call this from HAL TX complete interrupt callback
    void tx_interrupt_handler() {
        txPending_ = false;
        // Notify the write port that we can transmit again
        writePort_.notify();
    }
    
    /// Transmit a frame via FDCAN hardware
    /// @return true if frame was queued successfully
    bool try_tx_frame(const struct can_frame *frame) {
        if (txPending_) {
            return false;
        }
        
        FDCAN_TxHeaderTypeDef txHeader;
        
        // Determine if extended or standard ID
        if (IS_CAN_FRAME_EFF(*frame)) {
            txHeader.Identifier = GET_CAN_FRAME_ID_EFF(*frame);
            txHeader.IdType = FDCAN_EXTENDED_ID;
        } else {
            txHeader.Identifier = GET_CAN_FRAME_ID(*frame);
            txHeader.IdType = FDCAN_STANDARD_ID;
        }
        txHeader.TxFrameType = FDCAN_DATA_FRAME;
        
        // Convert DLC to FDCAN format
        if (frame->can_dlc <= 8) {
            txHeader.DataLength = frame->can_dlc << 16;
        } else {
            txHeader.DataLength = 8 << 16;
        }
        
        txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
        txHeader.BitRateSwitch = FDCAN_BRS_OFF;
        txHeader.FDFormat = FDCAN_CLASSIC_CAN;
        txHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
        txHeader.MessageMarker = 0;
        
        if (HAL_FDCAN_AddMessageToTxFifoQ(hfdcan_, &txHeader, 
                                          (uint8_t*)frame->data) == HAL_OK) {
            txPending_ = true;
            return true;
        }
        
        return false;
    }

private:
    friend class WritePort;
    
    /// State flow port for receiving frames from the OpenMRN hub
    class WritePort : public CanHubPort
    {
    public:
        WritePort(FdCanBridge *parent)
            : CanHubPort(parent->canHub_->service())
            , parent_(parent)
        {
        }

        Action entry() override
        {
            // Attempt to transmit the frame
            const struct can_frame &frame = message()->data()->frame();
            
            // DEBUG: Add breakpoint here to see frame details
            volatile uint32_t can_id = IS_CAN_FRAME_EFF(frame) ? 
                GET_CAN_FRAME_ID_EFF(frame) : GET_CAN_FRAME_ID(frame);
            volatile uint8_t dlc = frame.can_dlc;
            (void)can_id; // Prevent unused warning
            (void)dlc;
            
            if (parent_->try_tx_frame(&frame)) {
                // Frame queued successfully, wait for TX complete
                return wait_and_call(STATE(tx_done));
            } else {
                // TX is busy, yield and retry later
                return yield_and_call(STATE(entry));
            }
        }

        Action tx_done()
        {
            return release_and_exit();
        }

    private:
        FdCanBridge *parent_;
    };

    FDCAN_HandleTypeDef *hfdcan_;
    CanHubFlow *canHub_;
    volatile bool txPending_;
    WritePort writePort_;
};

static FdCanBridge *canBridge = nullptr;

extern "C" {

/// HAL callback for RX FIFO 0 new message
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
    if (canBridge && (RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE)) {
        canBridge->rx_interrupt_handler();
    }
}

/// HAL callback for TX complete
void HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t BufferIndexes) {
    if (canBridge) {
        canBridge->tx_interrupt_handler();
    }
}

/// Setup function to be called from application
void setup_can_bridge(openmrn_arduino::OpenMRN *openmrn) {
    canBridge = new FdCanBridge(&hfdcan1, openmrn->stack()->can_hub());
    canBridge->init();
}

/// Get the bridge instance for direct access if needed
FdCanBridge* get_can_bridge() {
    return canBridge;
}

/// Test function to send a frame through the OpenMRN hub
void test_send_can_frame(openmrn_arduino::OpenMRN *openmrn) {
    if (!openmrn || !openmrn->stack()) return;
    
    // DEBUG: Set breakpoint here
    volatile bool sending = true;
    
    auto *b = openmrn->stack()->can_hub()->alloc();
    if (b) {
        struct can_frame *frame = b->data()->mutable_frame();
        
        // Create a test frame: standard ID 0x123, 8 bytes of data
        CLR_CAN_FRAME_EFF(*frame);
        SET_CAN_FRAME_ID(*frame, 0x123);
        frame->can_dlc = 8;
        for (int i = 0; i < 8; i++) {
            frame->data[i] = i;
        }
        
        // Don't set skipMember - we want this to go to our WritePort
        b->data()->skipMember_ = nullptr;
        
        openmrn->stack()->can_hub()->send(b);
        sending = false; // Breakpoint: verify send was called
    }
    (void)sending;
}

} // extern "C"
