/* USER CODE BEGIN Header */
/**
 * @file    main.c
 * @brief   Temperature sensor alarm application for STM32G0.
 *
 * Reads temperature from a DS18B20 1-Wire sensor and displays the current
 * reading and alarm threshold on two TM1650 7-segment displays over I2C.
 * An external LED is activated when the temperature exceeds the threshold.
 * The onboard B1 button acknowledges and clears the alarm.
 *
 * @copyright Copyright (c) 2026 STMicroelectronics. All rights reserved.
 *            Licensed under terms found in the LICENSE file in the root directory.
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ds18b20.h"
#include "seven_seg.h"
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/**
 * @brief Alarm state.
 */
typedef enum
{
    ALARM_STATE_OFF,  /**< Alarm inactive — temperature below threshold */
    ALARM_STATE_ON    /**< Alarm active   — temperature exceeded threshold */
} AlarmState_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define THRESHOLD_TEMP          30.0f   /**< Default alarm threshold in degrees Celsius */
#define TEMP_SAMPLE_PERIOD_MS   1000    /**< Total loop period in ms (conversion + idle) */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

SevenSeg_Handle_t display_current;    /**< Display handle for current temperature (I2C1) */
SevenSeg_Handle_t display_threshold;  /**< Display handle for alarm threshold     (I2C2) */

float temp_current  = 0.0f;            /**< Most recent valid temperature reading in °C  */
float temp_threshold = THRESHOLD_TEMP; /**< Active alarm threshold in °C                 */
AlarmState_t alarm_state = ALARM_STATE_OFF; /**< Current alarm state                     */

volatile bool button_pressed = false;  /**< Set by EXTI ISR; cleared in main loop        */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

static void LED_SetState(bool on);
static void DisplayTemperature(SevenSeg_Handle_t *display, float temp);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief  Drive the external alarm LED on or off.
 * @param  on  true to illuminate the LED, false to extinguish it.
 */
static void LED_SetState(bool on)
{
    if (on)
    {
        HAL_GPIO_WritePin(ext_led_GPIO_Port, ext_led_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(ext_led_GPIO_Port, ext_led_Pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief  Format a float temperature value and write it to a 7-segment display.
 *
 * Multiplies @p temp by 10, rounds to the nearest integer, and calls
 * SevenSeg_DisplayDecimal() to show one decimal place (e.g. 25.3 °C → "25.3").
 *
 * @param  display  Pointer to an initialised SevenSeg_Handle_t.
 * @param  temp     Temperature in degrees Celsius.
 */
static void DisplayTemperature(SevenSeg_Handle_t *display, float temp)
{
    uint16_t value = (uint16_t)roundf(temp * 10.0f);
    SevenSeg_DisplayDecimal(display, value);
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
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  /* Start TIM2 as a free-running microsecond counter for the DS18B20 1-Wire driver */
  HAL_TIM_Base_Start(&htim2);

  /* Initialise DS18B20 — halt on failure (sensor required for operation) */
  if (!DS18B20_Init())
  {
      Error_Handler();
  }

  /* Initialise current temperature display on I2C1 */
  if (SevenSeg_Init(&display_current, &hi2c1) != SEVEN_SEG_OK)
  {
      Error_Handler();
  }

  /* Initialise threshold display on I2C2 */
  if (SevenSeg_Init(&display_threshold, &hi2c2) != SEVEN_SEG_OK)
  {
      Error_Handler();
  }

  /* Show the default threshold on startup */
  DisplayTemperature(&display_threshold, temp_threshold);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* ----------------------------------------------------------------
       * Button handling — acknowledge alarm on B1 press
       * button_pressed is set by the EXTI ISR (HAL_GPIO_EXTI_Falling_Callback)
       * ---------------------------------------------------------------- */
      if (button_pressed)
      {
          button_pressed = false;  /* Clear ISR flag */

          /* Debounce delay */
          HAL_Delay(50);

          /* Confirm button is still held (active low) */
          if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET)
          {
              /* Acknowledge alarm — turn off LED and return to idle state */
              if (alarm_state == ALARM_STATE_ON)
              {
                  alarm_state = ALARM_STATE_OFF;
                  LED_SetState(false);
              }
          }
      }

      /* ----------------------------------------------------------------
       * Temperature measurement
       * ---------------------------------------------------------------- */

      /* Trigger conversion and wait for 12-bit result (max 750 ms) */
      DS18B20_StartConversion();
      HAL_Delay(750);

      float temp_new = DS18B20_ReadTemperature();

      /* Accept only readings within the DS18B20 valid range (-55 to +125 °C) */
      if (temp_new > -100.0f && temp_new < 125.0f)
      {
          temp_current = temp_new;

          /* Refresh the current temperature display */
          DisplayTemperature(&display_current, temp_current);

          /* Trigger alarm if threshold is exceeded and alarm is not already active */
          if (alarm_state == ALARM_STATE_OFF)
          {
              if (temp_current > temp_threshold)
              {
                  alarm_state = ALARM_STATE_ON;
                  LED_SetState(true);
              }
          }
      }

      /* Idle for the remainder of the sample period */
      HAL_Delay(TEMP_SAMPLE_PERIOD_MS - 750);

    /* USER CODE END WHILE */

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
 * @brief  Initialise TIM2 as a free-running microsecond counter.
 *
 * Prescaler of 15 on a 16 MHz HSI clock gives a 1 MHz tick (1 µs resolution).
 * Period set to 65535 (max 16-bit). Used exclusively by the DS18B20 1-Wire driver.
 *
 * @retval None
 */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 15;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
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
 * - B1            (PC13): input, falling-edge EXTI, internal pull-up (alarm acknowledge).
 * - one_wire_data (PA):   open-drain output, low speed (DS18B20 1-Wire bus).
 * - ext_led       (PA):   push-pull output, low speed, pull-up (external alarm LED).
 * - LED_GREEN     (PA5):  push-pull output, high speed (onboard status LED).
 *
 * EXTI4_15 interrupt enabled at priority 0 for the B1 button.
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
  HAL_GPIO_WritePin(GPIOA, one_wire_data_Pin|ext_led_Pin|LED_GREEN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : one_wire_data_Pin */
  GPIO_InitStruct.Pin = one_wire_data_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(one_wire_data_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ext_led_Pin */
  GPIO_InitStruct.Pin = ext_led_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ext_led_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_GREEN_Pin */
  GPIO_InitStruct.Pin = LED_GREEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED_GREEN_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
 * @brief  EXTI falling-edge callback — handles B1 button press.
 *
 * Sets the @c button_pressed flag for processing in the main loop.
 * HAL_Delay must not be called from within an ISR context.
 *
 * @param  GPIO_Pin  Pin that triggered the interrupt.
 */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == B1_Pin)
    {
        button_pressed = true;
    }
}

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