/*
Description:
    Basic driver for NXP LM75A temperature sensor
    (Replacement for ST STTS751 temperature sensor)
*/

#ifndef _LORAGW_LM75A_H
#define _LORAGW_LM75A_H

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include "config.h"     /* library configuration options (dynamically generated) */

/*
  LM75A 7-bit I2C addresses (depends on A0/A1/A2 pin strapping):
  0x48: A2=GND A1=GND A0=GND
  0x49: A2=GND A1=GND A0=VCC  (reference design default: A0=VCC, A1=A2=GND)
  0x4A: A2=GND A1=VCC A0=GND
  0x4B: A2=GND A1=VCC A0=VCC
  0x4C: A2=VCC A1=GND A0=GND
  0x4D: A2=VCC A1=GND A0=VCC
  0x4E: A2=VCC A1=VCC A0=GND
  0x4F: A2=VCC A1=VCC A0=VCC
*/
static const uint8_t I2C_PORT_TEMP_SENSOR[] = {0x48, 0x49, 0x4A, 0x4B};

/**
@brief Configure the temperature sensor (NXP LM75A)
@param i2c_addr the I2C device address of the sensor
@return LGW_I2C_ERROR if fails, LGW_I2C_SUCCESS otherwise
*/
int lm75a_configure(uint8_t i2c_addr);

/**
@brief Get the temperature from the sensor
@param i2c_addr the I2C device address of the sensor
@param temperature pointer to store the temperature read from sensor
@return LGW_I2C_ERROR if fails, LGW_I2C_SUCCESS otherwise
*/
int lm75a_get_temperature(uint8_t i2c_addr, float * temperature);

#endif
