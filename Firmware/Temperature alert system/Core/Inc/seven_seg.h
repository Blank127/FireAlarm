/**
 * @file    seven_seg.h
 * @brief   TM1650-based 4-digit 7-segment display driver for STM32G0.
 *
 * Provides HAL-level I2C primitives and a higher-level API for controlling
 * a TM1650 display module over I2C. Based on the DFRobot LedDisplayModule
 * library (https://github.com/DFRobot/DFRobot_LedDisplayModule).
 *
 * @author  Aldrich Dias
 * @date    2026-02-12
 */

#ifndef INC_SEVEN_SEG_H_
#define INC_SEVEN_SEG_H_

#include <stdint.h>
#include <stdbool.h>
#include <stm32g0xx_hal.h>

/* TM1650 I2C Register Addresses -------------------------------------------- */

/** @defgroup TM1650_Addresses TM1650 I2C Register Addresses
 * @{
 */
#define TM1650_CMD_ADDR          0x24  /**< Command/control register address  */
#define TM1650_DIG1_ADDR         0x34  /**< Digit 1 data register address     */
#define TM1650_DIG2_ADDR         0x35  /**< Digit 2 data register address     */
#define TM1650_DIG3_ADDR         0x36  /**< Digit 3 data register address     */
#define TM1650_DIG4_ADDR         0x37  /**< Digit 4 data register address     */
/** @} */

/* TM1650 Display Control --------------------------------------------------- */

/** @defgroup TM1650_Control TM1650 Display Control Defines
 * @{
 */
#define TM1650_DISPLAY_ON        0x01  /**< Value written to CMD to enable display  */
#define TM1650_DISPLAY_OFF       0x00  /**< Value written to CMD to disable display */
#define TM1650_BRIGHTNESS_SHIFT  4     /**< Bit position of brightness field (bits 4–6) */
#define TM1650_MAX_BRIGHTNESS    7     /**< Maximum brightness level (0–7)          */
/** @} */

/* Driver Configuration ----------------------------------------------------- */

#define SEVEN_SEG_I2C_TIMEOUT    100   /**< I2C transaction timeout in milliseconds */

/* Types -------------------------------------------------------------------- */

/**
 * @brief HAL-layer status codes returned by low-level I2C functions.
 */
typedef enum {
    SEVEN_SEG_HAL_OK = 0,  /**< Transaction completed successfully */
    SEVEN_SEG_HAL_ERROR,   /**< HAL reported a bus or peripheral error */
    SEVEN_SEG_HAL_TIMEOUT  /**< Transaction did not complete within the timeout */
} SevenSeg_HAL_Status_t;

/**
 * @brief Display driver handle.
 *
 * Must be initialised with @ref SevenSeg_Init before any other API call.
 */
typedef struct {
    I2C_HandleTypeDef *hi2c;  /**< Pointer to the I2C peripheral handle */
    bool display_on;          /**< Tracks current display ON/OFF state  */
} SevenSeg_Handle_t;

/**
 * @brief API-layer status codes returned by higher-level driver functions.
 */
typedef enum {
    SEVEN_SEG_OK = 0,       /**< Operation completed successfully  */
    SEVEN_SEG_ERROR,        /**< Internal or HAL error occurred    */
    SEVEN_SEG_INVALID_PARAM /**< A supplied parameter was invalid  */
} SevenSeg_Status_t;

/* HAL-Layer Prototypes ----------------------------------------------------- */

/**
 * @brief  Write a command byte to the TM1650 control register.
 * @param  hi2c  Pointer to the I2C peripheral handle.
 * @param  cmd   Command byte to transmit.
 * @retval SevenSeg_HAL_Status_t
 */
SevenSeg_HAL_Status_t SevenSeg_HAL_WriteCommand(I2C_HandleTypeDef *hi2c, uint8_t cmd);

/**
 * @brief  Write a segment data byte to a single digit register.
 * @param  hi2c        Pointer to the I2C peripheral handle.
 * @param  digit_addr  Target digit register address (@ref TM1650_Addresses).
 * @param  data        Segment bitmap to write.
 * @retval SevenSeg_HAL_Status_t
 */
SevenSeg_HAL_Status_t SevenSeg_HAL_WriteDigit(I2C_HandleTypeDef *hi2c, uint8_t digit_addr, uint8_t data);

/* API-Layer Prototypes ----------------------------------------------------- */

/**
 * @brief  Initialise the display driver and power on the display.
 * @param  handle  Pointer to an uninitialised @ref SevenSeg_Handle_t.
 * @param  hi2c    Pointer to the I2C peripheral handle to bind.
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_Init(SevenSeg_Handle_t *handle, I2C_HandleTypeDef *hi2c);

/**
 * @brief  Turn the display on.
 * @param  handle  Pointer to an initialised @ref SevenSeg_Handle_t.
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_DisplayOn(SevenSeg_Handle_t *handle);

/**
 * @brief  Turn the display off (retains digit data).
 * @param  handle  Pointer to an initialised @ref SevenSeg_Handle_t.
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_DisplayOff(SevenSeg_Handle_t *handle);

/**
 * @brief  Blank all four digits without powering the display off.
 * @param  handle  Pointer to an initialised @ref SevenSeg_Handle_t.
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_Clear(SevenSeg_Handle_t *handle);

/**
 * @brief  Display a 4-digit unsigned integer (0–9999).
 * @param  handle         Pointer to an initialised @ref SevenSeg_Handle_t.
 * @param  number         Value to display (0–9999).
 * @param  leading_zeros  If @c true, pad with leading zeros (e.g. 0042).
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_DisplayNumber(SevenSeg_Handle_t *handle, uint16_t number, bool leading_zeros);

/**
 * @brief  Write a raw segment bitmap to a single digit position.
 * @param  handle         Pointer to an initialised @ref SevenSeg_Handle_t.
 * @param  digit          Digit position (0–3, left to right).
 * @param  segments       Segment bitmap byte.
 * @param  decimal_point  If @c true, illuminate the decimal point for this digit.
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_DisplayRaw(SevenSeg_Handle_t *handle, uint8_t digit, uint8_t segments, bool decimal_point);

/**
 * @brief  Display a value with an implied single decimal place (e.g. 253 → "25.3").
 * @param  handle  Pointer to an initialised @ref SevenSeg_Handle_t.
 * @param  value   Raw integer value; divided by 10 for display.
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_DisplayDecimal(SevenSeg_Handle_t *handle, uint16_t value);

#endif /* INC_SEVEN_SEG_H_ */