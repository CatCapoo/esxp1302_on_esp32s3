/*
 * loragw_i2c.h  –  STM32F407 I2C master wrapper (HAL I2C2)
 *
 * Provides the same interface as the ESP32S3 loragw_i2c.h but backed by
 * STM32 HAL.  CubeMX already calls MX_I2C2_Init() so lgw_i2c_open / close
 * are no-ops that just track the open state.
 *
 * Hardware: I2C2  PF0(SDA) / PF1(SCL)  100 kHz
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#define LGW_I2C_SUCCESS   0
#define LGW_I2C_ERROR    -1

/**
 * @brief Mark I2C port as open (CubeMX already inited it).
 * @return LGW_I2C_SUCCESS
 */
int lgw_i2c_open(void);

/**
 * @brief Mark I2C port as closed.
 * @return LGW_I2C_SUCCESS
 */
int lgw_i2c_close(void);

/**
 * @brief Read 1 byte from a register.
 * @param dev_addr  7-bit device address
 * @param reg_addr  register address
 * @param data      output byte
 */
int lgw_i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data);

/**
 * @brief Read 2 bytes from a register (MSB first).
 * @param dev_addr  7-bit device address
 * @param reg_addr  register address
 * @param data      2-byte output buffer
 */
int lgw_i2c_read_word(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data);

/**
 * @brief Write 1 byte to a register.
 * @param dev_addr  7-bit device address
 * @param reg_addr  register address
 * @param data      byte to write
 */
int lgw_i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);

/**
 * @brief Write a raw buffer (first byte is typically a control/register byte).
 * @param dev_addr  7-bit device address
 * @param data      buffer to transmit
 * @param size      number of bytes
 */
int lgw_i2c_write_buf(uint8_t dev_addr, const uint8_t *data, size_t size);
