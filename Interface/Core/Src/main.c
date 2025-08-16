/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdbool.h"
#include "i2c-lcd.h"
#include <DataFrame.h>
#include <CANparser/CANparser.h>
#include <stdio.h>
#include <string.h>
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
FDCAN_HandleTypeDef hfdcan1;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef hlpuart1;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

uint8_t TxData[8];
FDCAN_TxHeaderTypeDef screenHeader;
FDCAN_TxHeaderTypeDef WiFiConfigModeControl;
FDCAN_RxHeaderTypeDef RxHeader;

DataFrame data;

// I/O
#define menuItems 2
uint32_t menuSelect = 0;
uint32_t counter = 0;

bool blockbtn = false;
#define debounceTime 200
uint32_t lastPress = 0;

//wifiConfig button
bool requestWifiConfigMode = false;
bool sendRequestWifiConfigMode = false;

bool blockWifiBtn = false;
#define wifiDebounceTime 500
uint32_t lastWifiPress = 0;

unsigned long lastRefresh = 0;
char screenStr[80];
int screenCharSize;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C1_Init(void);
static void MX_FDCAN1_Init(void);
/* USER CODE BEGIN PFP */

void drawDataScreen(int screencode);

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
	uint8_t RxData[8];
	HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData);
	if (RxHeader.Identifier == 0x753) {
		requestWifiConfigMode = 0;
	}
	CAN_parseMessage(RxHeader.Identifier, RxData, &data);
}

void toggleWifiConfigMode(FDCAN_HandleTypeDef* hfdcan1, bool requestWifiConfigMode) {
	uint8_t txBuf[8] = {};
	txBuf[0] = requestWifiConfigMode;
	HAL_FDCAN_AddMessageToTxFifoQ(hfdcan1, &WiFiConfigModeControl, txBuf);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	menuSelect = (__HAL_TIM_GET_COUNTER(htim) / 4) % menuItems;
	drawDataScreen(menuSelect);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_PIN) {
	if (GPIO_PIN == GPIO_PIN_3 && !blockbtn ) {
		counter++;
		blockbtn = true;
	}
    if(GPIO_PIN == GPIO_PIN_4 && !blockWifiBtn) // If The INT Source Is EXTI Line9 (A9 Pin)
    {
    	sendRequestWifiConfigMode = true;
		requestWifiConfigMode = !requestWifiConfigMode;
		blockWifiBtn = true;
    }
}

float float_overflowCheck(float val, float replace) {
	if (val > replace) {
		return val = replace;
	}
	else return val;
}

uint8_t uint8_overflowCheck(uint8_t val, uint8_t replace) {
	if (val > replace) {
		return val = replace;
	}
	else return val;
}

uint16_t uint16_overflowCheck(uint16_t val, uint16_t replace) {
	if (val > replace) {
		return val = replace;
	}
	else return val;
}

int _write(int file, char *ptr, int len)
{
  /* Implement your write code here, this is used by puts and printf for example */
  int i=0;
  for(i=0 ; i<len ; i++)
    ITM_SendChar((*ptr++));
  return len;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void screen0() {
	float Pmotor = (data.motor.battery_voltage * data.motor.battery_current);
	screenCharSize = sprintf(screenStr,
		"Pin %5i Pou %6.0fRPM %16.2fVEL%6.2f Vba %6.2fTmc%6.2f Tba %6.2f",
		uint16_overflowCheck(data.mppt.power, (uint16_t) 999999),
		float_overflowCheck(Pmotor, 999999),
		float_overflowCheck(data.motor.rpm, 99999999.99),
		float_overflowCheck(data.gps.speed, 999.99),
		float_overflowCheck(data.motor.battery_voltage, 999.99),
		float_overflowCheck(data.motor.controller_temp, 999.99),
		float_overflowCheck(data.bms.cells_temp, 999.99)
	);
	for (int i = 0; i < screenCharSize; i++) {
		lcd_send_data(screenStr[i]);
	}
	for (int i = screenCharSize; i < 80; i++) {
		lcd_send_data(' ');
	}
}
void screen1() {
	//format, numbers give the max digits allowed
	//S = state, W = warning, F = failure
	//motor:W (2) F (7)
	//mppt:	S (1) F (3)
	//fix (1) antenna (1)
	//WiFi (1) time (5)
	uint8_t seconds = (int) (data.telemetry.unixTime % 60);
	screenCharSize = sprintf(screenStr,
		"motor:W %2i F %7ifix %2i %4 antenna %imppt :S %2i ERR %5iWiFi %i %3  time %4i",
		uint8_overflowCheck(data.motor.warning, (uint8_t) 99),
		uint8_overflowCheck(data.motor.failures, (uint8_t) 9999999),
		uint8_overflowCheck(data.gps.fix, (uint8_t) 9),
		uint8_overflowCheck(data.gps.antenna, (uint8_t) 9),
		uint8_overflowCheck(data.mppt.cs, (uint8_t) 9),
		uint8_overflowCheck(data.mppt.error, (uint8_t) 999),
		uint8_overflowCheck(data.esp.status, (uint8_t) 9),
		seconds
	);
	for (int i = 0; i < screenCharSize; i++) {
		lcd_send_data(screenStr[i]);
	}
}

void screen2() {
	screenCharSize = sprintf(screenStr, "- Wifi Setup Mode - Hi-Horizon TelemetryConnect to HotSpot: To send new config");
	for (int i = 0; i < screenCharSize; i++) {
		lcd_send_data(screenStr[i]);
	}
	//clear the rest of the chars
	for (int i = 0; i <  80 - screenCharSize; i++) {
		lcd_send_data(' ');
	}
}

void drawDataScreen(int screencode) {
  lcd_send_cmd (0x80|0x00);
  switch (screencode) {
    case 0:
    	screen0();
    	break;
    case 1:
    	screen1();
    	break;
    case 2:
    	screen2();
    	break;
  }
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
  MX_LPUART1_UART_Init();
  MX_TIM2_Init();
  MX_I2C1_Init();
  MX_FDCAN1_Init();
  /* USER CODE BEGIN 2 */

  screenHeader.Identifier 	= 0x731;
  screenHeader.IdType 		= FDCAN_STANDARD_ID;
  screenHeader.TxFrameType 	= FDCAN_DATA_FRAME;
  screenHeader.DataLength 	= FDCAN_DLC_BYTES_8;
  screenHeader.FDFormat		= FDCAN_CLASSIC_CAN;

  WiFiConfigModeControl.Identifier 		= 0x754;
  WiFiConfigModeControl.IdType 			= FDCAN_STANDARD_ID;
  WiFiConfigModeControl.TxFrameType 	= FDCAN_DATA_FRAME;
  WiFiConfigModeControl.DataLength 		= FDCAN_DLC_BYTES_8;
  WiFiConfigModeControl.FDFormat		= FDCAN_CLASSIC_CAN;

  lcd_init();
  HAL_TIM_Encoder_Start_IT(&htim2, TIM_CHANNEL_ALL);

  HAL_FDCAN_Start(&hfdcan1);

   //activates interrupt for a new can Frame
  HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);

  data.mppt.power = (uint16_t) 65535;
  data.gps.speed = 9999999;
  data.motor.rpm = 9999999;
  data.motor.battery_voltage = 9999999;
  data.motor.battery_current = 9999999;
  data.motor.controller_temp = (uint8_t) 255;
  data.bms.cells_temp = 9999999;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	if (HAL_GetTick() - lastPress > debounceTime) {
		blockbtn = false;
		lastPress = HAL_GetTick();
	}

	if (HAL_GetTick() - lastWifiPress > wifiDebounceTime) {
		blockWifiBtn = false;
		lastWifiPress = HAL_GetTick();
	}

//	if (sendRequestWifiConfigMode) {
//		toggleWifiConfigMode(&hfdcan1);
//		sendRequestWifiConfigMode=false;
//	}

	if (HAL_GetTick() - lastRefresh > 1000L) {
		toggleWifiConfigMode(&hfdcan1, requestWifiConfigMode);

		if (requestWifiConfigMode == 1) drawDataScreen(2);
		else 					   drawDataScreen(menuSelect);
		lastRefresh = HAL_GetTick();
		//send screen status through CAN
		int32_t ind = 0;
		buffer_append_uint8(TxData, 0, &ind); //status
		buffer_append_uint8(TxData, 0, &ind); //fans
		buffer_append_uint8(TxData, (uint8_t) requestWifiConfigMode, &ind); //wifiSetup

		HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &screenHeader, TxData);
	}
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV6;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
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
  hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
  hfdcan1.Init.AutoRetransmission = ENABLE;
  hfdcan1.Init.TransmitPause = DISABLE;
  hfdcan1.Init.ProtocolException = DISABLE;
  hfdcan1.Init.NominalPrescaler = 17;
  hfdcan1.Init.NominalSyncJumpWidth = 13;
  hfdcan1.Init.NominalTimeSeg1 = 39;
  hfdcan1.Init.NominalTimeSeg2 = 40;
  hfdcan1.Init.DataPrescaler = 1;
  hfdcan1.Init.DataSyncJumpWidth = 12;
  hfdcan1.Init.DataTimeSeg1 = 12;
  hfdcan1.Init.DataTimeSeg2 = 12;
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x30A0A7FB;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65536;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PC3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : wificonfig_Pin */
  GPIO_InitStruct.Pin = wificonfig_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(wificonfig_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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

#ifdef  USE_FULL_ASSERT
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
