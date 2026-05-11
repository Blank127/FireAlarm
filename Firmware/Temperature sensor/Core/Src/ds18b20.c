/**
 * @file    ds18b20.c
 * @brief   DS18B20 1-Wire temperature sensor driver implementation.
 *
 * @author  Aldrich Dias
 * @date    2026-02-11
 */

#include "ds18b20.h"
#include "main.h"

extern TIM_HandleTypeDef htim2;

/* 1-Wire Timing Constants -------------------------------------------------- */

/** @defgroup DS18B20_Timing 1-Wire Timing Values (microseconds)
 * @{
 */
#define DELAY_RESET_PULSE       480  /**< Reset pulse low time                        */
#define DELAY_PRESENCE_WAIT      70  /**< Wait after releasing before sampling presence */
#define DELAY_PRESENCE_READ     410  /**< Remaining presence window after sampling     */
#define DELAY_WRITE_1_LOW        10  /**< Write-1 slot: low pulse duration             */
#define DELAY_WRITE_1_HIGH       55  /**< Write-1 slot: recovery time                 */
#define DELAY_WRITE_0_LOW        65  /**< Write-0 slot: low pulse duration             */
#define DELAY_WRITE_0_HIGH        5  /**< Write-0 slot: recovery time                 */
#define DELAY_READ_LOW            5  /**< Read slot: master low pulse                 */
#define DELAY_READ_SAMPLE        10  /**< Read slot: sample point after release        */
#define DELAY_READ_HIGH          55  /**< Read slot: recovery time after sample        */
/** @} */

/* Private functions -------------------------------------------------------- */

/**
 * @brief  Busy-wait for the specified number of microseconds using TIM2.
 * @param  us  Delay duration in microseconds.
 */
static void delay_us(uint32_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

/**
 * @brief  Drive the 1-Wire bus low (assert).
 * @note   Pulls the data pin to GND.
 */
static void drive_low(void)
{
    HAL_GPIO_WritePin(one_wire_data_GPIO_Port, one_wire_data_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  Release the 1-Wire bus (deassert).
 * @note   Pin floats high via the external 4.7 kΩ pull-up resistor.
 */
static void release_line(void)
{
    HAL_GPIO_WritePin(one_wire_data_GPIO_Port, one_wire_data_Pin, GPIO_PIN_SET);
}

/**
 * @brief  Sample the current logic level on the 1-Wire bus.
 * @retval GPIO_PIN_SET (1) or GPIO_PIN_RESET (0).
 */
static uint8_t read_line(void)
{
    return HAL_GPIO_ReadPin(one_wire_data_GPIO_Port, one_wire_data_Pin);
}

/* Public functions --------------------------------------------------------- */

/**
 * @brief  Issue a 1-Wire reset pulse and detect device presence.
 * @retval true   Presence pulse detected — at least one device on the bus.
 * @retval false  No presence pulse — bus may be open or shorted.
 */
bool DS18B20_Reset(void)
{
    /* Pull bus low for reset pulse */
    drive_low();
    delay_us(DELAY_RESET_PULSE);

    /* Release and wait for device to assert presence */
    release_line();
    delay_us(DELAY_PRESENCE_WAIT);

    /* Sample presence pulse (active low, so invert) */
    uint8_t presence = !read_line();

    /* Wait for presence window to complete */
    delay_us(DELAY_PRESENCE_READ);

    return presence;
}

/**
 * @brief  Write a single bit onto the 1-Wire bus.
 * @param  bit  Bit value to write (0 or 1; only LSB is used).
 */
void DS18B20_WriteBit(uint8_t bit)
{
    if (bit)
    {
        /* Write 1: short low pulse then release */
        drive_low();
        delay_us(DELAY_WRITE_1_LOW);
        release_line();
        delay_us(DELAY_WRITE_1_HIGH);
    }
    else
    {
        /* Write 0: long low pulse then release */
        drive_low();
        delay_us(DELAY_WRITE_0_LOW);
        release_line();
        delay_us(DELAY_WRITE_0_HIGH);
    }
}

/**
 * @brief  Read a single bit from the 1-Wire bus.
 * @retval Bit value read (0 or 1).
 */
uint8_t DS18B20_ReadBit(void)
{
    /* Initiate read slot */
    drive_low();
    delay_us(DELAY_READ_LOW);

    /* Release and sample at the required point */
    release_line();
    delay_us(DELAY_READ_SAMPLE);
    uint8_t bit = read_line();

    /* Wait out the remainder of the read slot */
    delay_us(DELAY_READ_HIGH);

    return bit;
}

/**
 * @brief  Write one byte onto the 1-Wire bus, LSB first.
 * @param  byte  Byte to transmit.
 */
void DS18B20_WriteByte(uint8_t byte)
{
    for (int i = 0; i < 8; i++)
    {
        DS18B20_WriteBit(byte & 0x01);  /* send LSB */
        byte >>= 1;                     /* shift next bit into position */
    }
}

/**
 * @brief  Read one byte from the 1-Wire bus, LSB first.
 * @retval Byte received.
 */
uint8_t DS18B20_ReadByte(void)
{
    uint8_t data = 0;
    for (int i = 0; i < 8; i++)
    {
        data >>= 1;
        if (DS18B20_ReadBit())
        {
            data |= 0x80;
        }
    }
    return data;
}

/**
 * @brief  Initialise the DS18B20 and verify presence on the bus.
 * @retval true   Device detected and ready.
 * @retval false  No device detected on the 1-Wire bus.
 */
bool DS18B20_Init(void)
{
    return DS18B20_Reset();
}

/**
 * @brief  Trigger a temperature conversion (non-blocking).
 *
 * Sends SKIP_ROM followed by CONVERT_T. The conversion takes up to 750 ms
 * at 12-bit resolution — call @ref DS18B20_ReadTemperature only after that
 * delay has elapsed.
 */
void DS18B20_StartConversion(void)
{
    DS18B20_Reset();
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);    /* skip ROM — single sensor on bus */
    DS18B20_WriteByte(DS18B20_CMD_CONVERT_T);   /* start temperature conversion    */
}

/**
 * @brief  Read the most recent conversion result from the scratchpad.
 *
 * Sends SKIP_ROM followed by READ_SCRATCHPAD and reconstructs the
 * 12-bit twos-complement raw value from bytes 0 and 1. The raw value
 * is multiplied by 0.0625 (1/16) to convert to degrees Celsius.
 *
 * @note   Must be called at least 750 ms after @ref DS18B20_StartConversion
 *         to ensure the conversion is complete at 12-bit resolution.
 *
 * @retval Temperature in degrees Celsius (e.g. 23.5625).
 */
float DS18B20_ReadTemperature(void)
{
    uint8_t temp_lsb, temp_msb;
    int16_t temp_raw;
    float temp;

    DS18B20_Reset();
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);        /* skip ROM — single sensor on bus */
    DS18B20_WriteByte(DS18B20_CMD_READ_SCRATCHPAD); /* read scratchpad command          */

    temp_lsb = DS18B20_ReadByte();  /* scratchpad byte 0: temperature LSB */
    temp_msb = DS18B20_ReadByte();  /* scratchpad byte 1: temperature MSB */

    /* Reconstruct signed 16-bit raw value and convert to Celsius (LSB = 0.0625 °C) */
    temp_raw = (temp_msb << 8) | temp_lsb;
    temp = (float)temp_raw * 0.0625;

    return temp;
}