/* USER CODE BEGIN Header */
/**
 * @file    main.c
 * @brief   Seven-segment display test application for STM32G0.
 *
 * Initialises two TM1650-based 7-segment displays on I2C1 and I2C2,
 * runs a self-test sequence, then counts 0–9999 on both displays in
 * opposite directions. Debug output is streamed over USART2 at 115200 baud.
 *
 * @copyright Copyright (c) 2026 STMicroelectronics. All rights reserved.
 *            Licensed under terms found in the LICENSE file in the root directory.
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "seven_seg.h"
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
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
SevenSeg_Handle_t display1;  /**< Display handle bound to I2C1 */
SevenSeg_Handle_t display2;  /**< Display handle bound to I2C2 */

char uart_buffer[100];  /**< Scratch buffer for UART debug messages */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */
void UART_Print(const char *msg);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief  Transmit a null-terminated string over USART2 (blocking).
 * @param  msg  String to transmit.
 */
void UART_Print(const char *msg)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

/* USER CODE END 0 */

/**
 * @brief  Application entry point.
 * @retval int  Never returns.
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
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */

  UART_Print("\r\n===== Seven Segment Display Test =====\r\n");

    /* Initialise Display 1 on I2C1 */
    UART_Print("Initializing Display 1 (I2C1)...\r\n");
    SevenSeg_Status_t status1 = SevenSeg_Init(&display1, &hi2c1);

    if (status1 == SEVEN_SEG_OK)
    {
        UART_Print("Display 1: OK\r\n");
    }
    else
    {
        UART_Print("Display 1: FAILED!\r\n");
    }

    /* Initialise Display 2 on I2C2 */
    UART_Print("Initializing Display 2 (I2C2)...\r\n");
    SevenSeg_Status_t status2 = SevenSeg_Init(&display2, &hi2c2);

    if (status2 == SEVEN_SEG_OK)
    {
        UART_Print("Display 2: OK\r\n");
    } else
    {
        UART_Print("Display 2: FAILED!\r\n");
    }

    UART_Print("\r\nStarting display test...\r\n\r\n");

    /* Test 1: Static numbers */
    UART_Print("Test 1: Static numbers\r\n");
    SevenSeg_DisplayNumber(&display1, 1234, false);  /* "1234" on display 1 */
    SevenSeg_DisplayNumber(&display2, 5678, false);  /* "5678" on display 2 */
    HAL_Delay(3000);

    /* Test 2: Leading zeros */
    UART_Print("Test 2: Leading zeros\r\n");
    SevenSeg_DisplayNumber(&display1, 42, true);    /* "0042" on display 1 */
    SevenSeg_DisplayNumber(&display2, 7, true);     /* "0007" on display 2 */
    HAL_Delay(3000);

    /* Test 3: All segments lit (8888) */
    UART_Print("Test 3: All segments (8888)\r\n");
    SevenSeg_DisplayNumber(&display1, 8888, true);
    SevenSeg_DisplayNumber(&display2, 8888, true);
    HAL_Delay(3000);

    /* Test 4: Clear displays */
    UART_Print("Test 4: Clear displays\r\n");
    SevenSeg_Clear(&display1);
    SevenSeg_Clear(&display2);
    HAL_Delay(2000);

    /* Test 5: Power off then on */
    UART_Print("Test 5: Display OFF\r\n");
    SevenSeg_DisplayNumber(&display1, 1111, false);
    SevenSeg_DisplayNumber(&display2, 2222, false);
    HAL_Delay(1000);

    UART_Print("Turning OFF...\r\n");
    SevenSeg_DisplayOff(&display1);
    SevenSeg_DisplayOff(&display2);
    HAL_Delay(2000);

    UART_Print("Turning ON...\r\n");
    SevenSeg_DisplayOn(&display1);
    SevenSeg_DisplayOn(&display2);
    HAL_Delay(2000);

    UART_Print("\r\nTest complete! Starting counter...\r\n\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint16_t counter = 0;
  while (1)
  {
    /* USER CODE END WHILE */

	  /* Display counter on both displays (opposite directions) */
	      SevenSeg_DisplayNumber(&display1, counter, false);
	      SevenSeg_DisplayNumber(&display2, 9999 - counter, false);

	      /* Log to UART every 10 counts to avoid flooding the terminal */
	      if (counter % 10 == 0) {
	          sprintf(uart_buffer, "Display 1: %4d | Display 2: %4d\r\n",
	                  counter, 9999 - counter);
	          UART_Print(uart_buffer);
	      }

	      /* Increment counter, wrap at 9999 */
	      counter++;
	      if (counter > 9999) {
	          counter = 0;
	          UART_Print("\r\n--- Counter Reset ---\r\n\r\n");
	      }

	      /* Blink LED to show program is running */
	      HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);

	      HAL_Delay(100);  /* Update every 100 ms */
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief  Configure system clocks.
 *
 * Source: HSI @ 16 MHz, no PLL. SYSCLK / HCLK / PCLK1 all at 16 MHz,
 * flash latency 0.
 *
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief  Initialise I2C1.
 *
 * Timing register 0x00300617 targets ~100 kHz standard mode on a 16 MHz HSI
 * clock. Analogue filter enabled, digital filter off.
 *
 * @retval None
 */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00300617;
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
 * @brief  Initialise I2C2.
 *
 * Identical configuration to I2C1 — same timing, analogue filter enabled,
 * digital filter off.
 *
 * @retval None
 */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00300617;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
 * @brief  Initialise USART2 — 115200 8N1, no hardware flow control, FIFO off.
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
 * @brief  Initialise GPIO pins.
 *
 * - B1  (PC13): input, rising-edge interrupt, no pull.
 * - LED (PA5):  push-pull output, high speed, initialised low.
 *
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_GREEN_Pin */
  GPIO_InitStruct.Pin = LED_GREEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED_GREEN_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  Fatal error handler — disables interrupts and halts.
 *
 * Called by HAL on any peripheral initialisation failure. Extend this
 * function to add LED blink patterns or UART fault reporting if needed.
 *
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  assert_param failure callback.
 * @param  file  Source file where the assertion failed.
 * @param  line  Line number of the failed assertion.
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add their own implementation to report the file name and line number,
     e.g.: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */