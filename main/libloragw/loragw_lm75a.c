/*
Description:
    Basic driver for NXP LM75A temperature sensor
    (Replacement for ST STTS751 temperature sensor)
*/


/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <stdio.h>      /* printf fprintf */

#include "loragw_i2c.h"
#include "loragw_lm75a.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#if DEBUG_I2C == 1
    #define DEBUG_MSG(str)              fprintf(stdout, str)
    #define DEBUG_PRINTF(fmt, args...)  fprintf(stdout,"%s:%d: "fmt, __FUNCTION__, __LINE__, args)
    #define CHECK_NULL(a)               if(a==NULL){fprintf(stderr,"%s:%d: ERROR: NULL POINTER AS ARGUMENT\n", __FUNCTION__, __LINE__);return LGW_I2C_ERROR;}
#else
    #define DEBUG_MSG(str)
    #define DEBUG_PRINTF(fmt, args...)
    #define CHECK_NULL(a)               if(a==NULL){return LGW_I2C_ERROR;}
#endif

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

/* LM75A register addresses */
#define LM75A_REG_TEMP          0x00    /* Temperature register (read-only) */
#define LM75A_REG_CONF          0x01    /* Configuration register */
#define LM75A_REG_THYST         0x02    /* Hysteresis register */
#define LM75A_REG_TOS           0x03    /* Over-temperature shutdown threshold register */

/* LM75A Configuration register bits */
#define LM75A_CONF_SHUTDOWN     0x01    /* Shutdown mode */
#define LM75A_CONF_OS_COMP      0x00    /* OS comparator mode */
#define LM75A_CONF_OS_INT       0x02    /* OS interrupt mode */
#define LM75A_CONF_OS_POL_LOW   0x00    /* OS polarity active-low */
#define LM75A_CONF_OS_POL_HIGH  0x04    /* OS polarity active-high */

/* LM75A temperature resolution: 0.125°C per LSB (11-bit) */
#define LM75A_TEMP_RESOLUTION   0.125f

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */

int lm75a_configure(uint8_t i2c_addr)
{
    int err;
    uint8_t val;

    DEBUG_PRINTF("INFO: configuring LM75A temperature sensor on 0x%02X...\n", i2c_addr);

    /* Read configuration register to verify the device is present and accessible */
    err = i2c_esp32_read(i2c_addr, LM75A_REG_CONF, &val);
    if (err != 0) {
        DEBUG_PRINTF("ERROR: failed to read I2C device 0x%02X (err=%i)\n", i2c_addr, err);
        return LGW_I2C_ERROR;
    }
    DEBUG_PRINTF("INFO: LM75A config register: 0x%02X\n", val);

    /*
     * Configure LM75A:
     * - Normal operation mode (not shutdown)
     * - OS comparator mode
     * - OS active low
     * - Default fault queue (1 fault)
     */
    err = i2c_esp32_write(i2c_addr, LM75A_REG_CONF, 0x00);
    if (err != 0) {
        DEBUG_PRINTF("ERROR: failed to write I2C device 0x%02X (err=%i)\n", i2c_addr, err);
        return LGW_I2C_ERROR;
    }

    printf("INFO: LM75A temperature sensor configured on 0x%02X\n", i2c_addr);

    return LGW_I2C_SUCCESS;
}

int lm75a_get_temperature(uint8_t i2c_addr, float * temperature)
{
    int err;
    uint8_t buf[2];
    int16_t raw;

    CHECK_NULL(temperature);

    /*
     * LM75A temperature register (0x00) is 16 bits:
     *   - Byte 0 (MSB): D10..D3 (integer part + sign)
     *   - Byte 1 (LSB): D2..D0 in bits[7:5], bits[4:0] are zero
     *
     * The 11-bit two's complement value has a resolution of 0.125°C.
     * To get the temperature: raw_16bit = (MSB << 8 | LSB), then divide by 256.0
     */

    /* Read Temperature MSB and LSB (2 bytes from register 0x00) */
    err = i2c_esp32_read_word(i2c_addr, LM75A_REG_TEMP, buf);
    if (err != 0) {
        printf("ERROR: failed to read I2C device 0x%02X (err=%i)\n", i2c_addr, err);
        return LGW_I2C_ERROR;
    }

    /* Combine MSB and LSB into a signed 16-bit value, then divide by 256.0
     * This gives temperature with 0.125°C resolution (11-bit) */
    raw = (int16_t)((buf[0] << 8) | buf[1]);
    *temperature = raw / 256.0f;

    DEBUG_PRINTF("Temperature: %f C (h:0x%02X l:0x%02X)\n", *temperature, buf[0], buf[1]);

    return LGW_I2C_SUCCESS;
}
