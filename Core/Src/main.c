#include "main.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include <math.h>

SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim6;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM6_Init(void);

#define CS_LOW()				       HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, 0)
#define CS_HIGH()	             HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, 1)

int8_t Accel_Read_1Byte(uint8_t readAddr);
uint8_t SPI1_WriteRead(uint8_t byte);
void Accel_Write_1Byte(uint8_t writeAddr, uint8_t data);
void Accel_Init(void);
void Get_XYZ(int16_t value[3]);
void Transmit_to_PC(int16_t data16bit[3]);
void Get_Package_Data(int16_t data16bitAccel[3], uint8_t* package);
uint8_t Get_CRC(uint8_t* package, uint8_t len);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();
  MX_TIM6_Init();
  Accel_Init();

  HAL_TIM_Base_Start_IT(&htim6);

  int16_t res_mg[3] = {0, 0, 0};
  while (1)
  {
	  Get_XYZ(res_mg);
	  Transmit_to_PC(res_mg);
	  HAL_Delay(25);
  }
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 1679;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 49999;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PE3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PD13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

//a function that is automatically called when an interrupt from the timer appears 6
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM6) //check if the interrupt comes from TIM1
	{
		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
	}
}

//reads and sends one byte to the accelerometer
uint8_t SPI1_WriteRead(uint8_t byte) {
  uint8_t rxData = 0;
  HAL_SPI_TransmitReceive(&hspi1, &byte, &rxData, 1, 1000);
  return rxData;
}

//reads one byte from the accelerometer
int8_t Accel_Read_1Byte(uint8_t readAddr) {
  uint8_t rx = 0;
  readAddr |= 0x80; //1000 0000 enabling reading from the accelerometer
  CS_LOW();
  SPI1_WriteRead(readAddr);
  rx = SPI1_WriteRead(0);
  CS_HIGH();
  return rx;
}

//sends one byte to the accelerometer
void Accel_Write_1Byte(uint8_t writeAddr, uint8_t data) {
  CS_LOW();
  SPI1_WriteRead(writeAddr);
  SPI1_WriteRead(data);
  CS_HIGH();
}

//initial setup of the LIS302DLH accelerometer
void Accel_Init(void) {
	uint8_t ctrl_reg1 = 0x20;
	uint8_t value = 0x00;
	value |= 0x40 | 0x7;
	Accel_Write_1Byte(ctrl_reg1, value);
}

//reads the acceleration value along 3 axes
//returns values by pointer
void Get_XYZ(int16_t value[3]) {
	int16_t sens = 18; // mg/digit
	for (size_t i = 0; i < 3; i++) {
		value[i] = round(Accel_Read_1Byte(0x29 + 2*i) * sens); //g
	}
}

//transfer counter
uint8_t num_of_package = 0;

//transfers 1 data packet to the computer via USB
void Transmit_to_PC(int16_t data16bit[3]) {
	uint8_t package[10];
	Get_Package_Data(data16bit, package);
	CDC_Transmit_FS(package, 10);
	num_of_package++;
}

//generates a data package
//the first two bytes are initial
//then 6 bytes for acceleration
//2 bytes for the high bits and 2 for the low bits
//then a byte transfer counter, 8 bits CRC
void Get_Package_Data(int16_t data16bitAccel[3], uint8_t* package) {
	//bytes indicating the start of transmission
	package[0] = 0xCE;
	package[1] = 0xFA;

	//XYZ-axis acceleration
	size_t i = 2, j = 0;
	while(j < 3) {
		package[i] = data16bitAccel[j] >> 8; //high
		package[i+1] = data16bitAccel[j] & 0x00FF; //low
		i += 2;
		j++;
	}
	package[8] = num_of_package;
	package[9] = Get_CRC(package, 9);
}

//generates an 8 bit CRC for 9 bytes of data
uint8_t Get_CRC(uint8_t* package, uint8_t len) {
	uint8_t crc = 0x00, poly = 0x07;
	while(len--) {
		crc ^= *package++;
		for (size_t i = 0; i < 8; i++) {
			crc = crc&0x80 ? (crc << 1)^poly : crc<<1;
		}
	}
	return crc;
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
