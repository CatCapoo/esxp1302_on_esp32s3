/*
 * loragw_lm75a.h  –  NXP LM75A temperature sensor driver
 *
 * Ported from ESP32S3 bringup/test branch.
 *
 * LM75A 7-bit I2C addresses (A2/A1/A0 pin strapping):
 *   0x48  A2=GND A1=GND A0=GND  (all pulled-down)
 *   0x49  A2=GND A1=GND A0=VCC  (reference design default)
 *   0x4A  A2=GND A1=VCC A0=GND
 *   0x4B  A2=GND A1=VCC A0=VCC
 *   0x4C  A2=VCC A1=GND A0=GND
 *   0x4D  A2=VCC A1=GND A0=VCC
 *   0x4E  A2=VCC A1=VCC A0=GND
 *   0x4F  A2=VCC A1=VCC A0=VCC
 */

#pragma once

#include <stdint.h>

/* Default address – check your board: A0=GND→0x48, A0=VCC→0x49 */
#define LM75A_I2C_ADDR_DEFAULT  0x48

/**
 * @brief Configure LM75A: normal mode, OS comparator, active-low.
 * @param i2c_addr  7-bit I2C address
 * @return LGW_I2C_SUCCESS / LGW_I2C_ERROR
 */
int lm75a_configure(uint8_t i2c_addr);

/**
 * @brief Read temperature from LM75A.
 * @param i2c_addr    7-bit I2C address
 * @param temperature pointer to float result (°C, 0.125 °C resolution)
 * @return LGW_I2C_SUCCESS / LGW_I2C_ERROR
 */
int lm75a_get_temperature(uint8_t i2c_addr, float *temperature);
