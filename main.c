/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "NMEA.h"
#include "uartRingBuffer.h"
#include "fonts.h"
#include "ssd1306.h"
#include "stdio.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DHT11_PIN GPIO_PIN_0
#define DHT11_PORT GPIOA
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
void delay_us(uint32_t us) {
    us *= (SystemCoreClock / 1000000) / 9;
    while (us--) {
        __NOP();
    }
}

void delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

void DHT11_Start(void) {
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    delay_ms(18);

    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    delay_us(30);

    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

uint8_t DHT11_Check_Response(void) {
    uint8_t response = 0;
    delay_us(40);

    if (!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) {
        delay_us(80);

        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) {
            response = 1;
        }
    }

    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN));

    return response;
}

uint8_t DHT11_Read_Bit(void) {
    uint8_t bit = 0;

    while (!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN));
    delay_us(30);

    if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) {
        bit = 1;
    }

    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN));

    return bit;
}

uint8_t DHT11_Read_Byte(void) {
    uint8_t byte = 0;

    for (int i = 0; i < 8; i++) {
        byte |= (DHT11_Read_Bit() << (7 - i));
    }

    return byte;
}

void DHT11_Read_Data(uint8_t* temperature, uint8_t* humidity) {
    uint8_t data[5];

    DHT11_Start();

    if (DHT11_Check_Response()) {
        for (int i = 0; i < 5; i++) {
            data[i] = DHT11_Read_Byte();
        }

        if ((data[0] + data[1] + data[2] + data[3]) == data[4]) {
            *humidity = data[0];
            *temperature = data[2];
        }
    }
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
	char GGA[100];
	char RMC[100];

	GPSSTRUCT gpsData;


	int flagGGA = 0, flagRMC = 0;
	char TimeBuffer[50];
	char LocationBuffer[50];

	//int VCCTimeout = 5000;
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
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  SSD1306_Init();

  Ringbuf_init();
  HAL_Delay(500);

  uint8_t temperature, humidity;
  char obuf[50];
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  if(Wait_for("GGA") == 1)
	  {
		  

		  Copy_upto("*", GGA);
		  if (decodeGGA(GGA, &gpsData.ggastruct) == 0) flagGGA = 2;  //valid
		 		  else flagGGA = 1;  // invalid
	  }
	  if (Wait_for("RMC") == 1)
	  	  {

	  		 

	  		  Copy_upto("*", RMC);
	  		  if (decodeRMC(RMC, &gpsData.rmcstruct) == 0) flagRMC = 2; // valid
	  		  else flagRMC = 1;  // invalid
	  	  }
	  if ((flagGGA == 2) | (flagRMC == 2))
		{
		  	//SSD1306_Clear();

			SSD1306_GotoXY(0, 10);
			sprintf(TimeBuffer,"%02d:%02d:%02d, %02d%02d%02d",gpsData.ggastruct.tim.hour, \
					  gpsData.ggastruct.tim.min, gpsData.ggastruct.tim.sec, gpsData.rmcstruct.date.Day, \
					  gpsData.rmcstruct.date.Mon, gpsData.rmcstruct.date.Yr);
			SSD1306_Puts(TimeBuffer, &Font_7x10, 1);

			SSD1306_GotoXY(0, 20);
			sprintf(LocationBuffer, "%.2f%c, %.2f%c", gpsData.ggastruct.lcation.latitude, gpsData.ggastruct.lcation.NS,
			            gpsData.ggastruct.lcation.longitude, gpsData.ggastruct.lcation.EW);
			SSD1306_Puts(LocationBuffer, &Font_7x10, 1);

	  	  	  DHT11_Read_Data(&temperature, &humidity);
	  	  		 		  	  HAL_Delay(1000);

	  	  	 SSD1306_GotoXY(0, 40);
	  	  		 sprintf(obuf, "Temperature: %dC", temperature);
	  	  		 SSD1306_Puts(obuf, &Font_7x10, 1);
				 SSD1306_GotoXY(0, 50);
	  	 		 sprintf(obuf, "Humidity: %d%%", humidity);
	  	  		 SSD1306_Puts(obuf, &Font_7x10, 1);


			SSD1306_UpdateScreen();
		}

	  else if ((flagGGA == 1) | (flagRMC == 1)) {
	      
	      SSD1306_Clear(); // Clear 
	      SSD1306_GotoXY(0, 10); // line 1
	      SSD1306_Puts("     NO FIX     ", &Font_7x10, 1); // print
	      SSD1306_GotoXY(0, 25); //line 2
	      SSD1306_Puts("   Please wait   ", &Font_7x10, 1); // print


	  }
/*
	  if (VCCTimeout <= 0) {
	      VCCTimeout = 5000;  // Reset the timeout

	      // Reset flags
	      flagGGA = flagRMC = 0;

	      // Display VCC issue message
	     // SSD1306_Clear(); // Clear the display
	      SSD1306_GotoXY(0, 10); // Set cursor position for line 1
	      SSD1306_Puts("    VCC Issue   ", &Font_7x10, 1); // Print text
	      SSD1306_GotoXY(0, 25); // Set cursor position for line 2
	      SSD1306_Puts("Check Connection", &Font_7x10, 1); // Print text


	  }

*/	  	  	  	  

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
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
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
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
