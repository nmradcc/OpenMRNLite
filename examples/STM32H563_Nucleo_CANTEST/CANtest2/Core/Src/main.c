/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32h5xx_nucleo.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;
__IO uint32_t BspButtonState = BUTTON_RELEASED;

FDCAN_HandleTypeDef hfdcan1;
FDCAN_HandleTypeDef hfdcan2;

/* USER CODE BEGIN PV */

static volatile uint8_t can_rx_complete = 0;
static volatile uint8_t can_tx_complete = 0;
static volatile FDCAN_RxHeaderTypeDef can_rx_hdr;
static volatile uint8_t can_rx_buf[8];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_FDCAN2_Init(void);
static void MX_ICACHE_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static uint8_t can_test_number = 0;  /* 0 = CAN1 Loopback, 1 = CAN2 Loopback, 2 = Direct Transceiver Loopback (bidirectional) */

/**
  * @brief  Perform CAN tests with different modes
  * @param  None
  * @retval None
  */
static void CAN_LoopbackTest(void)
{
  HAL_StatusTypeDef status;
  FDCAN_TxHeaderTypeDef TxHeader;
  uint8_t TxData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  /* RX handled via interrupt callback into can_rx_hdr/can_rx_buf */
  uint32_t timeout = 0;
  uint8_t test_passed = 0;
  uint8_t i;
  FDCAN_HandleTypeDef *TestHandle;
  FDCAN_HandleTypeDef *OtherHandle;
  const char *TestName;
  const char *OtherName;
  const char *TestDesc;

  printf("\n\r========== CAN Test %d Start ==========\n\r", can_test_number + 1);

  /* Determine which test to run */
  switch (can_test_number)
  {
    case 0:
      /* Test 1: CAN1 Internal Loopback */
      TestHandle = &hfdcan1;
      OtherHandle = NULL;
      TestName = "CAN1";
      OtherName = NULL;
      TestDesc = "Internal Loopback with Interrupts";
      printf("Testing: CAN1 (Internal Loopback with Interrupts)\n\r");
      break;
    case 1:
      /* Test 2: CAN2 Internal Loopback */
      TestHandle = &hfdcan2;
      OtherHandle = NULL;
      TestName = "CAN2";
      OtherName = NULL;
      TestDesc = "Internal Loopback with Interrupts";
      printf("Testing: CAN2 (Internal Loopback with Interrupts)\n\r");
      break;
    case 2:
      /* Test 3: Direct Transceiver Loopback via External Connection - Bidirectional */
      TestHandle = &hfdcan1;
      OtherHandle = &hfdcan2;
      TestName = "CAN1";
      OtherName = "CAN2";
      TestDesc = "Direct Transceiver Loopback (Bidirectional)";
      printf("Testing: Bidirectional Direct Transceiver Loopback\n\r");
      printf("Note: Requires CAN1 and CAN2 connected on external CAN bus\n\r");
      break;
    default:
      can_test_number = 0;
      return;
  }

  /* Stop any existing operation */
  HAL_FDCAN_Stop(TestHandle);
  if (OtherHandle != NULL)
  {
    HAL_FDCAN_Stop(OtherHandle);
  }
  HAL_Delay(10);

  /* Reset flags */
  can_rx_complete = 0;
  can_tx_complete = 0;

  /* Configure the test based on test number */
  if (can_test_number == 2)
  {
    /* Test 3: Reconfigure to Normal Mode for direct transceiver loopback */
    printf("[DEBUG] Reconfiguring to Normal Mode...\n\r");
    TestHandle->Init.Mode = FDCAN_MODE_NORMAL;
    TestHandle->Init.StdFiltersNbr = 2;  /* Need 2 filters for bidirectional test */
    OtherHandle->Init.Mode = FDCAN_MODE_NORMAL;
    OtherHandle->Init.StdFiltersNbr = 2;  /* Need 2 filters for bidirectional test */
    if (HAL_FDCAN_Init(TestHandle) != HAL_OK)
    {
      printf("ERROR: Failed to reconfigure %s to Normal Mode\n\r", TestName);
      goto test_end;
    }
    if (HAL_FDCAN_Init(OtherHandle) != HAL_OK)
    {
      printf("ERROR: Failed to reconfigure %s to Normal Mode\n\r", OtherName);
      goto test_end;
    }
    printf("Reconfigured %s and %s to Normal Mode\n\r", TestName, OtherName);
  }

  /* Add a filter to accept the test message (must be done AFTER Init) */
  FDCAN_FilterTypeDef sFilterConfig;
  sFilterConfig.IdType = FDCAN_STANDARD_ID;
  sFilterConfig.FilterIndex = 0;
  sFilterConfig.FilterType = FDCAN_FILTER_MASK;
  sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  sFilterConfig.FilterID1 = 0x123;
  sFilterConfig.FilterID2 = 0x7FF;  /* Mask for all 11-bit identifiers */
  
  if (HAL_FDCAN_ConfigFilter(TestHandle, &sFilterConfig) != HAL_OK)
  {
    printf("ERROR: Failed to configure filter on %s\n\r", TestName);
  }
  
  /* For test 3, add filter for reverse direction ID on CAN1 */
  if (can_test_number == 2)
  {
    FDCAN_FilterTypeDef sFilterConfig1 = sFilterConfig;
    sFilterConfig1.FilterID1 = 0x234;  /* Accept reverse direction ID */
    sFilterConfig1.FilterIndex = 1;  /* Use second filter slot */
    if (HAL_FDCAN_ConfigFilter(TestHandle, &sFilterConfig1) != HAL_OK)
    {
      printf("ERROR: Failed to configure second filter on %s\n\r", TestName);
    }
    printf("[DEBUG] %s filters configured for IDs 0x%03X and 0x%03X\n\r", 
           TestName, (unsigned int)sFilterConfig.FilterID1, (unsigned int)sFilterConfig1.FilterID1);
  }
  else
  {
    printf("[DEBUG] %s filter configured for ID 0x%03X\n\r", TestName, (unsigned int)sFilterConfig.FilterID1);
  }
  
  if (OtherHandle != NULL)
  {
    /* For test 3, also configure filter for reverse direction ID */
    FDCAN_FilterTypeDef sFilterConfig2 = sFilterConfig;
    sFilterConfig2.FilterID1 = 0x234;  /* Accept reverse direction ID */
    sFilterConfig2.FilterIndex = 1;  /* Use second filter slot */
    if (HAL_FDCAN_ConfigFilter(OtherHandle, &sFilterConfig) != HAL_OK)
    {
      printf("ERROR: Failed to configure filter on %s\n\r", OtherName);
    }
    if (HAL_FDCAN_ConfigFilter(OtherHandle, &sFilterConfig2) != HAL_OK)
    {
      printf("ERROR: Failed to configure second filter on %s\n\r", OtherName);
    }
    printf("[DEBUG] %s filters configured for IDs 0x%03X and 0x%03X\n\r", 
           OtherName, (unsigned int)sFilterConfig.FilterID1, (unsigned int)sFilterConfig2.FilterID1);
  }

  /* Start FDCAN controllers */
  if (HAL_FDCAN_Start(TestHandle) != HAL_OK)
  {
    printf("ERROR: Failed to start %s\n\r", TestName);
    goto test_end;
  }
  printf("[DEBUG] %s started successfully\n\r", TestName);
  
  if (OtherHandle != NULL)
  {
    if (HAL_FDCAN_Start(OtherHandle) != HAL_OK)
    {
      printf("ERROR: Failed to start %s\n\r", OtherName);
      goto test_end;
    }
    printf("[DEBUG] %s started successfully\n\r", OtherName);
  }

  /* Configure RX FIFO0 interrupt (after start) */
  HAL_FDCAN_ActivateNotification(TestHandle, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
  HAL_FDCAN_ActivateNotification(TestHandle, FDCAN_IT_TX_COMPLETE, 0);
  
  if (OtherHandle != NULL)
  {
    HAL_FDCAN_ActivateNotification(OtherHandle, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_ActivateNotification(OtherHandle, FDCAN_IT_TX_COMPLETE, 0);
  }

  /* Configure global filter: accept non-matching standard IDs to RX FIFO0, reject extended */
  HAL_FDCAN_ConfigGlobalFilter(TestHandle,
                               FDCAN_ACCEPT_IN_RX_FIFO0,
                               FDCAN_REJECT,
                               FDCAN_REJECT_REMOTE,
                               FDCAN_REJECT_REMOTE);
  
  if (OtherHandle != NULL)
  {
    HAL_FDCAN_ConfigGlobalFilter(OtherHandle,
                                 FDCAN_ACCEPT_IN_RX_FIFO0,
                                 FDCAN_REJECT,
                                 FDCAN_REJECT_REMOTE,
                                 FDCAN_REJECT_REMOTE);
  }

  /* Configure TX header */
  TxHeader.Identifier = 0x123;                  /* Test ID */
  TxHeader.IdType = FDCAN_STANDARD_ID;
  TxHeader.TxFrameType = FDCAN_DATA_FRAME;
  TxHeader.DataLength = FDCAN_DLC_BYTES_8;
  TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
  TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
  TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  TxHeader.MessageMarker = 0;

  /* Transmit test message from primary controller */
  status = HAL_FDCAN_AddMessageToTxFifoQ(TestHandle, &TxHeader, TxData);
  if (status != HAL_OK)
  {
    printf("ERROR: Failed to add message to %s TX FIFO\n\r", TestName);
    printf("Status: %d\n\r", status);
    goto test_end;
  }
  printf("%s TX: Message queued (ID: 0x%03X, Data: ", TestName, (unsigned int)TxHeader.Identifier);
  for (i = 0; i < 8; i++)
  {
    printf("%02X ", TxData[i]);
  }
  printf(")\n\r");
  
  /* For tests 1 & 2, wait shorter; for test 3 (external), wait longer */
  uint32_t wait_time = (can_test_number == 2) ? 100 : 5;
  HAL_Delay(wait_time);
  
  /* Check RX FIFO levels */
  if (can_test_number == 2)
  {
    uint32_t fifo_level = HAL_FDCAN_GetRxFifoFillLevel(OtherHandle, FDCAN_RX_FIFO0);
    printf("[DEBUG] %s RX FIFO0 level: %u\n\r", OtherName, (unsigned int)fifo_level);
  }
  
  /* Check protocol status and error counters */
  FDCAN_ProtocolStatusTypeDef protocolStatus;
  FDCAN_ErrorCountersTypeDef errorCounters;
  HAL_FDCAN_GetProtocolStatus(TestHandle, &protocolStatus);
  HAL_FDCAN_GetErrorCounters(TestHandle, &errorCounters);
  printf("%s TEC: %u, REC: %u, ErrorLogging: %u\n\r", TestName,
    (unsigned int)errorCounters.TxErrorCnt,
    (unsigned int)errorCounters.RxErrorCnt,
    (unsigned int)errorCounters.ErrorLogging);
  printf("[DEBUG] %s LastErrorCode: 0x%02X, BusOff: %u, ErrorPassive: %u\n\r", TestName,
    (unsigned int)protocolStatus.LastErrorCode,
    (unsigned int)protocolStatus.BusOff,
    (unsigned int)protocolStatus.ErrorPassive);
  
  if (OtherHandle != NULL)
  {
    HAL_FDCAN_GetProtocolStatus(OtherHandle, &protocolStatus);
    HAL_FDCAN_GetErrorCounters(OtherHandle, &errorCounters);
    printf("%s TEC: %u, REC: %u, ErrorLogging: %u\n\r", OtherName,
      (unsigned int)errorCounters.TxErrorCnt,
      (unsigned int)errorCounters.RxErrorCnt,
      (unsigned int)errorCounters.ErrorLogging);
    printf("[DEBUG] %s LastErrorCode: 0x%02X, BusOff: %u, ErrorPassive: %u\n\r", OtherName,
      (unsigned int)protocolStatus.LastErrorCode,
      (unsigned int)protocolStatus.BusOff,
      (unsigned int)protocolStatus.ErrorPassive);
  }

  /* Wait for message reception with interrupt */
  timeout = 0;
  uint32_t max_timeout = (can_test_number == 2) ? 2000 : 1000;  /* Longer timeout for external tests */
  printf("[DEBUG] Waiting for RX (max %ums)...\n\r", (unsigned int)max_timeout);
  while (can_rx_complete == 0 && timeout < max_timeout)
  {
    HAL_Delay(1);
    timeout++;
    /* Check for TX complete */
    if (can_test_number == 2 && timeout == 50 && can_tx_complete)
    {
      printf("[DEBUG] TX complete confirmed\n\r");
    }
  }

  if (can_rx_complete)
  {
      printf("%s RX: Message received (ID: 0x%03X, Data: ", 
             (OtherHandle != NULL && can_test_number == 2) ? OtherName : TestName,
             (unsigned int)can_rx_hdr.Identifier);
      for (i = 0; i < 8; i++)
      {
        printf("%02X ", can_rx_buf[i]);
      }
      printf(")\n\r");

      /* Verify the received message */
      if (can_rx_hdr.Identifier == TxHeader.Identifier)
      {
        test_passed = 1;
        for (i = 0; i < 8; i++)
        {
          if (can_rx_buf[i] != TxData[i])
          {
            test_passed = 0;
            break;
          }
        }
      }

      if (!test_passed)
      {
        printf("\n\r*** CAN TEST %d FAILED: Data Mismatch ***\n\r", can_test_number + 1);
        BSP_LED_On(LED_RED);
        goto test_end;
      }
  }
  else
  {
    printf("ERROR: No message received (timeout after %ums)\n\r", (unsigned int)timeout);
    if (can_test_number == 2)
    {
      printf("[DEBUG] TX complete flag: %u\n\r", can_tx_complete);
      printf("[DEBUG] RX complete flag: %u\n\r", can_rx_complete);
      
      /* Check FIFO levels */
      uint32_t fifo_level_can1 = HAL_FDCAN_GetRxFifoFillLevel(TestHandle, FDCAN_RX_FIFO0);
      uint32_t fifo_level_can2 = HAL_FDCAN_GetRxFifoFillLevel(OtherHandle, FDCAN_RX_FIFO0);
      printf("[DEBUG] %s RX FIFO0 level: %u, %s RX FIFO0 level: %u\n\r", 
             TestName, (unsigned int)fifo_level_can1, OtherName, (unsigned int)fifo_level_can2);
      
      /* Re-check error counters */
      HAL_FDCAN_GetErrorCounters(TestHandle, &errorCounters);
      HAL_FDCAN_GetProtocolStatus(TestHandle, &protocolStatus);
      printf("[DEBUG] %s Final TEC: %u, REC: %u, LastError: 0x%02X\n\r", TestName,
        (unsigned int)errorCounters.TxErrorCnt,
        (unsigned int)errorCounters.RxErrorCnt,
        (unsigned int)protocolStatus.LastErrorCode);
      if (OtherHandle != NULL)
      {
        HAL_FDCAN_GetErrorCounters(OtherHandle, &errorCounters);
        HAL_FDCAN_GetProtocolStatus(OtherHandle, &protocolStatus);
        printf("[DEBUG] %s Final TEC: %u, REC: %u, LastError: 0x%02X\n\r", OtherName,
          (unsigned int)errorCounters.TxErrorCnt,
          (unsigned int)errorCounters.RxErrorCnt,
          (unsigned int)protocolStatus.LastErrorCode);
      }
      printf("Hint for Test 3:\n\r");
      printf("  1. Verify CAN1 and CAN2 are physically connected via transceivers\n\r");
      printf("  2. Check CAN bus termination (120 ohm resistors at both ends)\n\r");
      printf("  3. Verify transceiver power and connections (CANH, CANL, GND)\n\r");
      printf("  4. TEC increasing = No ACK (no device on bus or bus error)\n\r");
      printf("  5. If no hardware: Test 3 requires external CAN transceivers\n\r");
    }
    BSP_LED_On(LED_RED);
    goto test_end;
  }

  /* For Test 3, perform bidirectional test (reverse direction) */
  if (can_test_number == 2)
  {
    printf("\n\r--- Bidirectional Test: Reverse Direction ---\n\r");
    
    /* Reset flags for second direction */
    can_rx_complete = 0;
    can_tx_complete = 0;
    
    /* Prepare different data for reverse test */
    uint8_t TxData2[8] = {0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
    TxHeader.Identifier = 0x234;  /* Different ID for reverse direction */

    /* Transmit from OtherHandle (CAN2) */
    status = HAL_FDCAN_AddMessageToTxFifoQ(OtherHandle, &TxHeader, TxData2);
    if (status != HAL_OK)
    {
      printf("ERROR: Failed to add message to %s TX FIFO\n\r", OtherName);
      printf("Status: %d\n\r", status);
      goto test_end;
    }
    printf("%s TX: Message queued (ID: 0x%03X, Data: ", OtherName, (unsigned int)TxHeader.Identifier);
    for (i = 0; i < 8; i++)
    {
      printf("%02X ", TxData2[i]);
    }
    printf(")\n\r");
    
    HAL_Delay(100);
    
    /* Wait for message reception on TestHandle (CAN1) */
    timeout = 0;
    printf("[DEBUG] Waiting for reverse direction RX...\n\r");
    while (can_rx_complete == 0 && timeout < 2000)
    {
      HAL_Delay(1);
      timeout++;
      /* Check for TX complete in reverse direction */
      if (timeout == 50 && can_tx_complete)
      {
        printf("[DEBUG] Reverse TX complete confirmed\n\r");
      }
    }

    if (can_rx_complete)
    {
        printf("%s RX: Message received (ID: 0x%03X, Data: ", TestName, (unsigned int)can_rx_hdr.Identifier);
        for (i = 0; i < 8; i++)
        {
          printf("%02X ", can_rx_buf[i]);
        }
        printf(")\n\r");

        /* Verify the received message */
        if (can_rx_hdr.Identifier == TxHeader.Identifier)
        {
          test_passed = 1;
          for (i = 0; i < 8; i++)
          {
            if (can_rx_buf[i] != TxData2[i])
            {
              test_passed = 0;
              break;
            }
          }
        }

        if (test_passed)
        {
          printf("\n\r*** CAN TEST %d PASSED (Bidirectional) ***\n\r", can_test_number + 1);
          BSP_LED_On(LED_YELLOW);
          HAL_Delay(500);
          BSP_LED_Off(LED_YELLOW);
        }
        else
        {
          printf("\n\r*** CAN TEST %d FAILED: Reverse Direction Data Mismatch ***\n\r", can_test_number + 1);
          BSP_LED_On(LED_RED);
        }
    }
    else
    {
      printf("ERROR: No message received on %s in reverse direction (timeout after %ums)\n\r", TestName, (unsigned int)timeout);
      printf("[DEBUG] Reverse TX complete flag: %u\n\r", can_tx_complete);
      /* Check error counters for reverse direction failure */
      HAL_FDCAN_GetErrorCounters(OtherHandle, &errorCounters);
      HAL_FDCAN_GetProtocolStatus(OtherHandle, &protocolStatus);
      printf("[DEBUG] %s Reverse TEC: %u, REC: %u, LastError: 0x%02X\n\r", OtherName,
        (unsigned int)errorCounters.TxErrorCnt,
        (unsigned int)errorCounters.RxErrorCnt,
        (unsigned int)protocolStatus.LastErrorCode);
      BSP_LED_On(LED_RED);
    }
  }
  else
  {
    /* For tests 1 and 2, single direction is enough */
    printf("\n\r*** CAN TEST %d PASSED (%s) ***\n\r", can_test_number + 1, TestDesc);
    BSP_LED_On(LED_YELLOW);
    HAL_Delay(500);
    BSP_LED_Off(LED_YELLOW);
  }

test_end:
  /* Move to next test */
  can_test_number = (can_test_number + 1) % 3;
  
  /* Stop FDCAN controllers */
  HAL_FDCAN_Stop(TestHandle);
  if (OtherHandle != NULL)
  {
    HAL_FDCAN_Stop(OtherHandle);
  }
  
  /* Re-initialize to loopback mode if we're moving back to tests 1 or 2 */
  if (can_test_number == 0 || can_test_number == 1)
  {
    FDCAN_HandleTypeDef *handle = (can_test_number == 0) ? &hfdcan1 : &hfdcan2;
    handle->Init.Mode = FDCAN_MODE_INTERNAL_LOOPBACK;
    HAL_FDCAN_Init(handle);
  }
  
  printf("========== CAN Test %d End ==========\n\r\n\r", (can_test_number == 0) ? 3 : can_test_number);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FDCAN1_Init();
  MX_FDCAN2_Init();
  MX_ICACHE_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Initialize leds */
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_YELLOW);
  BSP_LED_Init(LED_RED);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN BSP */
  /* -- Sample board code to send message over COM1 port ---- */
  printf("Welcome to STM32 world !\n\r");
  printf("CAN Bus Tests - Press user button to cycle through tests:\n\r");
  printf("  Test 1: CAN1 Internal Loopback\n\r");
  printf("  Test 2: CAN2 Internal Loopback\n\r");
  printf("  Test 3: Direct Transceiver Loopback (Bidirectional)\n\r");
  /* -- Sample board code to switch on leds ---- */
  BSP_LED_On(LED_GREEN);
  /* USER CODE END BSP */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* -- Sample board code for User push-button in interrupt mode ---- */
    if (BspButtonState == BUTTON_PRESSED)
    {
      /* Update button state */
      BspButtonState = BUTTON_RELEASED;
      /* Perform CAN loopback test */
      CAN_LoopbackTest();
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS_DIGITAL;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLL1_SOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 250;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1_VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the programming delay
  */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_2);
}

/**
  * @brief FDCAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_FDCAN1_Init(void)
{

  /* USER CODE BEGIN FDCAN1_Init 0 */

  /* USER CODE END FDCAN1_Init 0 */

  /* USER CODE BEGIN FDCAN1_Init 1 */

  /* USER CODE END FDCAN1_Init 1 */
  hfdcan1.Instance = FDCAN1;
  hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
  hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan1.Init.Mode = FDCAN_MODE_INTERNAL_LOOPBACK;
  hfdcan1.Init.AutoRetransmission = DISABLE;
  hfdcan1.Init.TransmitPause = DISABLE;
  hfdcan1.Init.ProtocolException = DISABLE;
  hfdcan1.Init.NominalPrescaler = 4;
  hfdcan1.Init.NominalSyncJumpWidth = 1;
  hfdcan1.Init.NominalTimeSeg1 = 11;
  hfdcan1.Init.NominalTimeSeg2 = 4;
  hfdcan1.Init.DataPrescaler = 1;
  hfdcan1.Init.DataSyncJumpWidth = 1;
  hfdcan1.Init.DataTimeSeg1 = 1;
  hfdcan1.Init.DataTimeSeg2 = 1;
  hfdcan1.Init.StdFiltersNbr = 1;
  hfdcan1.Init.ExtFiltersNbr = 0;
  hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FDCAN1_Init 2 */

  /* USER CODE END FDCAN1_Init 2 */

}

/**
  * @brief FDCAN2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_FDCAN2_Init(void)
{

  /* USER CODE BEGIN FDCAN2_Init 0 */

  /* USER CODE END FDCAN2_Init 0 */

  /* USER CODE BEGIN FDCAN2_Init 1 */

  /* USER CODE END FDCAN2_Init 1 */
  hfdcan2.Instance = FDCAN2;
  hfdcan2.Init.ClockDivider = FDCAN_CLOCK_DIV1;
  hfdcan2.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan2.Init.Mode = FDCAN_MODE_INTERNAL_LOOPBACK;
  hfdcan2.Init.AutoRetransmission = DISABLE;
  hfdcan2.Init.TransmitPause = DISABLE;
  hfdcan2.Init.ProtocolException = DISABLE;
  hfdcan2.Init.NominalPrescaler = 4;
  hfdcan2.Init.NominalSyncJumpWidth = 1;
  hfdcan2.Init.NominalTimeSeg1 = 11;
  hfdcan2.Init.NominalTimeSeg2 = 4;
  hfdcan2.Init.DataPrescaler = 1;
  hfdcan2.Init.DataSyncJumpWidth = 1;
  hfdcan2.Init.DataTimeSeg1 = 1;
  hfdcan2.Init.DataTimeSeg2 = 1;
  hfdcan2.Init.StdFiltersNbr = 1;
  hfdcan2.Init.ExtFiltersNbr = 0;
  hfdcan2.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  if (HAL_FDCAN_Init(&hfdcan2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FDCAN2_Init 2 */

  /* USER CODE END FDCAN2_Init 2 */

}

/**
  * @brief ICACHE Initialization Function
  * @param None
  * @retval None
  */
static void MX_ICACHE_Init(void)
{

  /* USER CODE BEGIN ICACHE_Init 0 */

  /* USER CODE END ICACHE_Init 0 */

  /* USER CODE BEGIN ICACHE_Init 1 */

  /* USER CODE END ICACHE_Init 1 */

  /** Enable instruction cache (default 2-ways set associative cache)
  */
  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ICACHE_Init 2 */

  /* USER CODE END ICACHE_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pins : RMII_MDC_Pin RMII_RXD0_Pin RMII_RXD1_Pin */
  GPIO_InitStruct.Pin = RMII_MDC_Pin|RMII_RXD0_Pin|RMII_RXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_REF_CLK_Pin RMII_MDIO_Pin RMII_CRS_DV_Pin */
  GPIO_InitStruct.Pin = RMII_REF_CLK_Pin|RMII_MDIO_Pin|RMII_CRS_DV_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : VBUS_SENSE_Pin */
  GPIO_InitStruct.Pin = VBUS_SENSE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(VBUS_SENSE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : UCPD_CC1_Pin UCPD_CC2_Pin */
  GPIO_InitStruct.Pin = UCPD_CC1_Pin|UCPD_CC2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : RMII_TXD1_Pin */
  GPIO_InitStruct.Pin = RMII_TXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(RMII_TXD1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : UCPD_FLT_Pin */
  GPIO_InitStruct.Pin = UCPD_FLT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(UCPD_FLT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_FS_N_Pin USB_FS_P_Pin */
  GPIO_InitStruct.Pin = USB_FS_N_Pin|USB_FS_P_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_USB;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_TXT_EN_Pin RMI_TXD0_Pin */
  GPIO_InitStruct.Pin = RMII_TXT_EN_Pin|RMI_TXD0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_D1_TX_Pin ARD_D0_RX_Pin */
  GPIO_InitStruct.Pin = ARD_D1_TX_Pin|ARD_D0_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF8_LPUART1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
  * @brief  RX FIFO 0 callback.
  * @param  hfdcan pointer to an FDCAN_HandleTypeDef structure that contains
  *         the configuration information for the specified FDCAN.
  * @param  RxFifo0ITs indicates which RX FIFO 0 interrupts are signaled.
  * @retval None
  */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0)
  {
    /* Read the message into a global buffer and set flag */
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0,
                               (FDCAN_RxHeaderTypeDef*)&can_rx_hdr,
                               (uint8_t*)can_rx_buf) == HAL_OK)
    {
      can_rx_complete = 1;
    }
  }
}

/**
  * @brief  TX Buffer complete callback.
  * @param  hfdcan pointer to an FDCAN_HandleTypeDef structure that contains
  *         the configuration information for the specified FDCAN.
  * @param  BufferIndexes Indexes of the transmitted buffers.
  * @retval None
  */
void HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t BufferIndexes)
{
  /* Set flag indicating message transmitted */
  can_tx_complete = 1;
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  BSP Push Button callback
  * @param  Button Specifies the pressed button
  * @retval None
  */
void BSP_PB_Callback(Button_TypeDef Button)
{
  if (Button == BUTTON_USER)
  {
    BspButtonState = BUTTON_PRESSED;
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
