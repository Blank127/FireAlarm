/**
 * @file    seven_seg.c
 * @brief   TM1650 4-digit 7-segment display driver implementation.
 *
 * @author  Aldrich Dias
 * @date    2026-02-12
 * @ref     https://github.com/DFRobot/DFRobot_LedDisplayModule
 */

#include "seven_seg.h"

/* Private defines ---------------------------------------------------------- */

/** @brief Bit 7 of a segment data byte — illuminates the decimal point. */
#define DECIMAL_POINT_BIT 0x80

/* Private data ------------------------------------------------------------- */

/**
 * @brief Segment bitmaps for digits 0–9.
 *
 * Bit mapping (LSB → MSB): A B C D E F G DP
 *
 * @code
 *      A
 *     ---
 *  F |   | B
 *     -G-
 *  E |   | C
 *     ---
 *      D    DP
 * @endcode
 */
static const uint8_t DIGIT_ENCODE[] =
{
    0x3F,  /* 0 */
    0x06,  /* 1 */
    0x5B,  /* 2 */
    0x4F,  /* 3 */
    0x66,  /* 4 */
    0x6D,  /* 5 */
    0x7D,  /* 6 */
    0x07,  /* 7 */
    0x7F,  /* 8 */
    0x6F   /* 9 */
};

/** @brief TM1650 digit register addresses, index 0 = leftmost digit. */
static const uint8_t DIGIT_ADDR[] =
{
    TM1650_DIG1_ADDR,
    TM1650_DIG2_ADDR,
    TM1650_DIG3_ADDR,
    TM1650_DIG4_ADDR
};

/* HAL-layer functions ------------------------------------------------------ */

/**
 * @brief  Write a command byte to the TM1650 control register.
 * @param  hi2c  Pointer to the I2C peripheral handle.
 * @param  cmd   Command byte to transmit.
 * @retval SevenSeg_HAL_Status_t
 */
SevenSeg_HAL_Status_t SevenSeg_HAL_WriteCommand(I2C_HandleTypeDef *hi2c, uint8_t cmd)
{
    HAL_StatusTypeDef hal_status;

    /* Send command to TM1650 command register (7-bit address shifted left by 1) */
    hal_status = HAL_I2C_Master_Transmit(hi2c,
                                         TM1650_CMD_ADDR << 1,
                                         &cmd,
                                         1,
                                         SEVEN_SEG_I2C_TIMEOUT);

    if (hal_status == HAL_OK)
    {
        return SEVEN_SEG_HAL_OK;
    }
    else if (hal_status == HAL_TIMEOUT)
    {
        return SEVEN_SEG_HAL_TIMEOUT;
    }
    else
    {
        return SEVEN_SEG_HAL_ERROR;
    }
}

/**
 * @brief  Write a segment data byte to a single digit register.
 * @param  hi2c        Pointer to the I2C peripheral handle.
 * @param  digit_addr  Target digit register address.
 * @param  data        Segment bitmap to write.
 * @retval SevenSeg_HAL_Status_t
 */
SevenSeg_HAL_Status_t SevenSeg_HAL_WriteDigit(I2C_HandleTypeDef *hi2c, uint8_t digit_addr, uint8_t data)
{
    HAL_StatusTypeDef hal_status;

    /* Send segment data to specific digit (7-bit address shifted left by 1) */
    hal_status = HAL_I2C_Master_Transmit(hi2c,
                                         digit_addr << 1,
                                         &data,
                                         1,
                                         SEVEN_SEG_I2C_TIMEOUT);

    if (hal_status == HAL_OK)
    {
        return SEVEN_SEG_HAL_OK;
    }
    else if (hal_status == HAL_TIMEOUT)
    {
        return SEVEN_SEG_HAL_TIMEOUT;
    }
    else
    {
        return SEVEN_SEG_HAL_ERROR;
    }
}

/* API-layer functions ------------------------------------------------------ */

/**
 * @brief  Initialise the display driver and power on the display.
 * @param  handle  Pointer to an uninitialised SevenSeg_Handle_t.
 * @param  hi2c    Pointer to the I2C peripheral handle to bind.
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_Init(SevenSeg_Handle_t *handle, I2C_HandleTypeDef *hi2c)
{
    if (handle == NULL || hi2c == NULL)
    {
        return SEVEN_SEG_INVALID_PARAM;
    }

    handle->hi2c = hi2c;
    handle->display_on = false;

    /* Clear display before enabling */
    SevenSeg_Clear(handle);

    /* Turn on display at maximum brightness (level 7) */
    uint8_t cmd = TM1650_DISPLAY_ON | (TM1650_MAX_BRIGHTNESS << TM1650_BRIGHTNESS_SHIFT);

    SevenSeg_HAL_Status_t status = SevenSeg_HAL_WriteCommand(handle->hi2c, cmd);

    if (status == SEVEN_SEG_HAL_OK)
    {
        handle->display_on = true;
        return SEVEN_SEG_OK;
    }
    else
    {
        return SEVEN_SEG_ERROR;
    }
}

/**
 * @brief  Turn the display on at maximum brightness.
 * @param  handle  Pointer to an initialised SevenSeg_Handle_t.
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_DisplayOn(SevenSeg_Handle_t *handle)
{
    if (handle == NULL)
    {
        return SEVEN_SEG_INVALID_PARAM;
    }

    /* Always use maximum brightness */
    uint8_t cmd = TM1650_DISPLAY_ON | (TM1650_MAX_BRIGHTNESS << TM1650_BRIGHTNESS_SHIFT);

    SevenSeg_HAL_Status_t status = SevenSeg_HAL_WriteCommand(handle->hi2c, cmd);

    if (status == SEVEN_SEG_HAL_OK)
    {
        handle->display_on = true;
        return SEVEN_SEG_OK;
    }
    else
    {
        return SEVEN_SEG_ERROR;
    }
}

/**
 * @brief  Turn the display off (retains digit data).
 * @param  handle  Pointer to an initialised SevenSeg_Handle_t.
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_DisplayOff(SevenSeg_Handle_t *handle)
{
    if (handle == NULL)
    {
        return SEVEN_SEG_INVALID_PARAM;
    }

    uint8_t cmd = TM1650_DISPLAY_OFF;

    SevenSeg_HAL_Status_t status = SevenSeg_HAL_WriteCommand(handle->hi2c, cmd);

    if (status == SEVEN_SEG_HAL_OK)
    {
        handle->display_on = false;
        return SEVEN_SEG_OK;
    }
    else
    {
        return SEVEN_SEG_ERROR;
    }
}

/**
 * @brief  Blank all four digits without powering the display off.
 * @param  handle  Pointer to an initialised SevenSeg_Handle_t.
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_Clear(SevenSeg_Handle_t *handle)
{
    if (handle == NULL)
    {
        return SEVEN_SEG_INVALID_PARAM;
    }

    /* Write 0x00 to all 4 digit registers */
    for (uint8_t i = 0; i < 4; i++)
    {
        SevenSeg_HAL_Status_t status = SevenSeg_HAL_WriteDigit(handle->hi2c, DIGIT_ADDR[i], 0x00);
        if (status != SEVEN_SEG_HAL_OK)
        {
            return SEVEN_SEG_ERROR;
        }
    }

    return SEVEN_SEG_OK;
}

/**
 * @brief  Display a 4-digit unsigned integer (0–9999).
 * @param  handle         Pointer to an initialised SevenSeg_Handle_t.
 * @param  number         Value to display (0–9999).
 * @param  leading_zeros  If true, pad with leading zeros (e.g. 42 → "0042").
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_DisplayNumber(SevenSeg_Handle_t *handle, uint16_t number, bool leading_zeros)
{
    if (handle == NULL)
    {
        return SEVEN_SEG_INVALID_PARAM;
    }

    if (number > 9999)
    {
        return SEVEN_SEG_INVALID_PARAM;
    }

    uint8_t digits[4];

    /* Decompose into individual digits, most-significant first */
    digits[3] = number % 10;          /* ones      */
    digits[2] = (number / 10) % 10;   /* tens      */
    digits[1] = (number / 100) % 10;  /* hundreds  */
    digits[0] = (number / 1000) % 10; /* thousands */

    /* Write each digit, blanking leading zero positions when required */
    for (uint8_t i = 0; i < 4; i++)
    {
        uint8_t segment_data;

        /* Blank leading zeros based on the magnitude of number */
        if (!leading_zeros && number < 1000 && i == 0 && digits[0] == 0)
        {
            segment_data = 0x00;  /* blank thousands */
        }
        else if (!leading_zeros && number < 100 && i <= 1 && digits[i] == 0)
        {
            segment_data = 0x00;  /* blank hundreds  */
        }
        else if (!leading_zeros && number < 10 && i <= 2 && digits[i] == 0)
        {
            segment_data = 0x00;  /* blank tens      */
        }
        else
        {
            segment_data = DIGIT_ENCODE[digits[i]];
        }

        SevenSeg_HAL_Status_t status = SevenSeg_HAL_WriteDigit(handle->hi2c,
                                                               DIGIT_ADDR[i],
                                                               segment_data);
        if (status != SEVEN_SEG_HAL_OK) {
            return SEVEN_SEG_ERROR;
        }
    }

    return SEVEN_SEG_OK;
}

/**
 * @brief  Write a raw segment bitmap to a single digit position.
 * @param  handle         Pointer to an initialised SevenSeg_Handle_t.
 * @param  digit          Digit position (0–3, left to right).
 * @param  segments       Segment bitmap byte.
 * @param  decimal_point  If true, illuminate the decimal point for this digit.
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_DisplayRaw(SevenSeg_Handle_t *handle, uint8_t digit, uint8_t segments, bool decimal_point)
{
    if (handle == NULL)
    {
        return SEVEN_SEG_INVALID_PARAM;
    }

    if (digit > 3)
    {
        return SEVEN_SEG_INVALID_PARAM;
    }

    uint8_t data = segments;
    if (decimal_point)
    {
        data |= DECIMAL_POINT_BIT;
    }

    SevenSeg_HAL_Status_t status = SevenSeg_HAL_WriteDigit(handle->hi2c,
                                                           DIGIT_ADDR[digit],
                                                           data);

    if (status == SEVEN_SEG_HAL_OK)
    {
        return SEVEN_SEG_OK;
    }
    else
    {
        return SEVEN_SEG_ERROR;
    }
}

/**
 * @brief  Display a value with an implied single decimal place (e.g. 752 → "75.2").
 * @param  handle  Pointer to an initialised SevenSeg_Handle_t.
 * @param  value   Raw integer value representing tenths (e.g. 752 = 75.2).
 * @retval SevenSeg_Status_t
 */
SevenSeg_Status_t SevenSeg_DisplayDecimal(SevenSeg_Handle_t *handle, uint16_t value)
{
    if (handle == NULL)
    {
        return SEVEN_SEG_INVALID_PARAM;
    }

    if (value > 9999)
    {
        return SEVEN_SEG_INVALID_PARAM;
    }

    /* Decompose value into display positions:
     *   digit0 = hundreds  (pos 0, blank if zero)
     *   digit1 = tens      (pos 1, blank if no significant digit yet)
     *   digit2 = ones      (pos 2, decimal point lit)
     *   digit3 = tenths    (pos 3)
     */
    uint8_t digit3 = value % 10;           /* tenths place (after decimal)  */
    uint8_t digit2 = (value / 10) % 10;    /* ones place   (before decimal) */
    uint8_t digit1 = (value / 100) % 10;   /* tens place                    */
    uint8_t digit0 = (value / 1000) % 10;  /* hundreds place                */

    /* pos 0: hundreds — blank if zero */
    uint8_t segment_data;
    if (digit0 > 0)
    {
        segment_data = DIGIT_ENCODE[digit0];
    }
    else
    {
        segment_data = 0x00;  /* blank */
    }

    SevenSeg_HAL_Status_t status = SevenSeg_HAL_WriteDigit(handle->hi2c, DIGIT_ADDR[0], segment_data);
    if (status != SEVEN_SEG_HAL_OK)
    {
        return SEVEN_SEG_ERROR;
    }

    /* pos 1: tens — blank if no significant digit yet */
    if (digit1 > 0 || digit0 > 0)
    {
        segment_data = DIGIT_ENCODE[digit1];
    }
    else
    {
        segment_data = 0x00;  /* blank */
    }

    status = SevenSeg_HAL_WriteDigit(handle->hi2c, DIGIT_ADDR[1], segment_data);
    if (status != SEVEN_SEG_HAL_OK)
    {
        return SEVEN_SEG_ERROR;
    }

    /* pos 2: ones — decimal point always lit here */
    segment_data = DIGIT_ENCODE[digit2] | DECIMAL_POINT_BIT;
    status = SevenSeg_HAL_WriteDigit(handle->hi2c, DIGIT_ADDR[2], segment_data);
    if (status != SEVEN_SEG_HAL_OK)
    {
        return SEVEN_SEG_ERROR;
    }

    /* pos 3: tenths */
    segment_data = DIGIT_ENCODE[digit3];
    status = SevenSeg_HAL_WriteDigit(handle->hi2c, DIGIT_ADDR[3], segment_data);
    if (status != SEVEN_SEG_HAL_OK)
    {
        return SEVEN_SEG_ERROR;
    }

    return SEVEN_SEG_OK;
}