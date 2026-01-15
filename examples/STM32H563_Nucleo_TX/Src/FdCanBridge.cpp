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
        : hfdcan_(hfdcan), canHub_(hub), txPending_(false) {}
    
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
                
                canHub_->send(b);
            }
        }
    }
    
    /// Call this from HAL TX complete interrupt callback
    void tx_interrupt_handler() {
        txPending_ = false;
    }
    
    /// Poll this from the main loop to transmit frames from OpenMRN to HAL
    void poll_tx() {
        // If TX is still pending, don't try to send
        if (txPending_) {
            return;
        }
        
        // Try to get a frame from the hub
        // Note: This is a simplified approach. A proper implementation would
        // use a flow-based approach, but that requires more OpenMRN infrastructure
        
        // For now, we'll check if there are any buffers to send
        // This would need to be connected to the hub's output
        // TODO: Implement proper TX flow integration
    }
    
    /// Direct TX for testing - call with a can_frame
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
    FDCAN_HandleTypeDef *hfdcan_;
    CanHubFlow *canHub_;
    volatile bool txPending_;
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

} // extern "C"
