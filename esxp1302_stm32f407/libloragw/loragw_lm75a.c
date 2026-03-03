/*
 * loragw_lm75a.c  –  NXP LM75A temperature sensor driver
 *
 * Ported from ESP32S3 bringup/test branch; uses STM32 lgw_i2c_* API.
 */

#include <stdint.h>
#include <stdio.h>
#include "loragw_i2c.h"
#include "loragw_lm75a.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

/* LM75A register addresses */
#define LM75A_REG_TEMP   0x00   /* Temperature register (read-only, 16-bit) */
#define LM75A_REG_CONF   0x01   /* Configuration register */
#define LM75A_REG_THYST  0x02   /* Hysteresis register */
#define LM75A_REG_TOS    0x03   /* Over-temperature shutdown threshold */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */

int lm75a_configure(uint8_t i2c_addr)
{
    int err;
    uint8_t val;

    /* Read configuration register to verify device is present */
    err = lgw_i2c_read(i2c_addr, LM75A_REG_CONF, &val);
    if (err != LGW_I2C_SUCCESS) {
        printf("ERROR: LM75A not found at 0x%02X\n", i2c_addr);
        return LGW_I2C_ERROR;
    }
    printf("INFO: LM75A config reg = 0x%02X\n", val);

    /*
     * Configure: normal operation, OS comparator, OS active-low,
     * fault queue = 1 (all reset defaults → write 0x00).
     */
    err = lgw_i2c_write(i2c_addr, LM75A_REG_CONF, 0x00);
    if (err != LGW_I2C_SUCCESS) {
        printf("ERROR: failed to configure LM75A at 0x%02X\n", i2c_addr);
        return LGW_I2C_ERROR;
    }

    printf("INFO: LM75A configured at 0x%02X\n", i2c_addr);
    return LGW_I2C_SUCCESS;
}

int lm75a_get_temperature(uint8_t i2c_addr, float *temperature)
{
    int err;
    uint8_t buf[2];
    int16_t raw;

    if (!temperature) return LGW_I2C_ERROR;

    /*
     * LM75A temperature register (0x00) is 16-bit, MSB first:
     *   Byte0[7:0] = D10..D3
     *   Byte1[7:5] = D2..D0, bits[4:0] are always 0
     *
     * 11-bit two's complement → divide combined 16-bit value by 256
     * gives temperature in °C with 0.125 °C resolution.
     */
    err = lgw_i2c_read_word(i2c_addr, LM75A_REG_TEMP, buf);
    if (err != LGW_I2C_SUCCESS) {
        printf("ERROR: failed to read LM75A temperature (0x%02X)\n", i2c_addr);
        return LGW_I2C_ERROR;
    }

    raw = (int16_t)((buf[0] << 8) | buf[1]);
    *temperature = raw / 256.0f;
    return LGW_I2C_SUCCESS;
}
