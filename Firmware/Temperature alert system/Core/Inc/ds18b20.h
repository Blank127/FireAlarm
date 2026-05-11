/**
 * @file    ds18b20.h
 * @brief   DS18B20 1-Wire temperature sensor driver for STM32G0.
 *
 * Provides 1-Wire bus primitives and a higher-level API for initialising
 * the DS18B20, triggering a temperature conversion, and reading the result.
 *
 * @author  Aldrich Dias
 * @date    2026-02-11
 */

#ifndef DS18B20_H
#define DS18B20_H

#include <stdbool.h>
#include <stdint.h>

/* DS18B20 1-Wire Commands -------------------------------------------------- */

/** @defgroup DS18B20_Commands DS18B20 ROM and Function Commands
 * @{
 */
#define DS18B20_CMD_SKIP_ROM         0xCC  /**< Skip ROM — address all devices on the bus */
#define DS18B20_CMD_CONVERT_T        0x44  /**< Initiate a temperature conversion          */
#define DS18B20_CMD_READ_SCRATCHPAD  0xBE  /**< Read 9 bytes from the scratchpad register  */
/** @} */

/* 1-Wire Bus Primitives ---------------------------------------------------- */

/**
 * @brief  Issue a 1-Wire reset pulse and detect device presence.
 * @retval true   At least one device pulled the bus low (presence detected).
 * @retval false  No presence pulse received — bus may be open or shorted.
 */
bool DS18B20_Reset(void);

/**
 * @brief  Write a single bit onto the 1-Wire bus.
 * @param  bit  Bit value to write (0 or 1; only LSB is used).
 */
void DS18B20_WriteBit(uint8_t bit);

/**
 * @brief  Read a single bit from the 1-Wire bus.
 * @retval Bit value read (0 or 1).
 */
uint8_t DS18B20_ReadBit(void);

/**
 * @brief  Write one byte onto the 1-Wire bus, LSB first.
 * @param  byte  Byte to transmit.
 */
void DS18B20_WriteByte(uint8_t byte);

/**
 * @brief  Read one byte from the 1-Wire bus, LSB first.
 * @retval Byte received.
 */
uint8_t DS18B20_ReadByte(void);

/* API-Layer Functions ------------------------------------------------------ */

/**
 * @brief  Initialise the DS18B20 and verify presence on the bus.
 *
 * Issues a reset pulse and checks for a presence pulse. Call once at
 * startup before any conversion or read operation.
 *
 * @retval true   Device detected and ready.
 * @retval false  No device detected on the 1-Wire bus.
 */
bool DS18B20_Init(void);

/**
 * @brief  Trigger a temperature conversion (non-blocking).
 *
 * Sends SKIP_ROM followed by CONVERT_T. The conversion takes up to 750 ms
 * at 12-bit resolution — call @ref DS18B20_ReadTemperature only after that
 * delay has elapsed.
 */
void DS18B20_StartConversion(void);

/**
 * @brief  Read the most recent conversion result from the scratchpad.
 *
 * Sends SKIP_ROM followed by READ_SCRATCHPAD and returns the temperature
 * computed from the first two scratchpad bytes.
 *
 * @note   Must be called at least 750 ms after @ref DS18B20_StartConversion
 *         to ensure the conversion is complete at 12-bit resolution.
 *
 * @retval Temperature in degrees Celsius (e.g. 23.5625).
 *         Returns 0.0f if the reset before reading fails.
 */
float DS18B20_ReadTemperature(void);

#endif /* DS18B20_H */