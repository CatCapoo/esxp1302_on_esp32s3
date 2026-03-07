/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2019 Semtech

Description:
    Minimum test program for the loragw_i2c module

License: Revised BSD License, see LICENSE.TXT file include in the project
*/


/* Fix an issue between POSIX and C99 */
#if __STDC_VERSION__ >= 199901L
    #define _XOPEN_SOURCE 600
#else
    #define _XOPEN_SOURCE 500
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     /* getopt, access */
#include <time.h>

#include "loragw_i2c.h"
#include "loragw_lm75a.h"
#include "loragw_aux.h"
#include "loragw_hal.h"


#define LM75A_REG_TEMP          0x00
#define LM75A_REG_CONF          0x01
#define LM75A_I2C_ADDR          0x48    /* LM75A default 7-bit address (A2=A1=A0=0) */


void app_main(void)
{
    int i, err;
    uint8_t val;
    uint8_t buf[2];
    float temperature;

    printf( "+++ Start of I2C test program (LM75A) +++\n" );

    // I've two choices to skip when something goes wrong before the 'for()' loop:
    //   1. use 'goto'.
    //   2. put all of i2c_esp32_open/read/write() in a big while loop and use 'break'.
    // Since I don't like add too many indents, I choose the 1st one here.

    /* Open I2C port expander */
    err = i2c_esp32_open();
    if (err != 0)
    {
        printf( "ERROR: failed to open I2C port(err=%i)\n", err);
        goto out;
    }

    /* Read LM75A configuration register to verify device is present */
    err = i2c_esp32_read(LM75A_I2C_ADDR, LM75A_REG_CONF, &val);
    if ( err != 0 )
    {
        printf( "ERROR: failed to read I2C device 0x%x (err=%i)\n", LM75A_I2C_ADDR, err );
        goto out;
    }
    printf("INFO: LM75A config register: 0x%02X\n", val);

    /* Configure LM75A: normal mode, OS comparator, active-low */
    err = i2c_esp32_write(LM75A_I2C_ADDR, LM75A_REG_CONF, 0x00);
    if ( err != 0 )
    {
        printf( "ERROR: failed to write I2C device 0x%02X (err=%i)\n", LM75A_I2C_ADDR, err );
        goto out;
    }

    for(i=0; i<100; i++) {
        /* Read Temperature (2 bytes from register 0x00) */
        err = i2c_esp32_read_word(LM75A_I2C_ADDR, LM75A_REG_TEMP, buf);
        if ( err != 0 )
        {
            printf( "ERROR: failed to read I2C device 0x%02X (err=%i)\n", LM75A_I2C_ADDR, err );
            break;
        }

        /* LM75A: 11-bit signed value, resolution 0.125°C */
        int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
        temperature = raw / 256.0f;

        printf( "Temperature: %f C (h:0x%02X l:0x%02X)\n", temperature, buf[0], buf[1] );
        wait_ms( 1000 );
    }

    /* Terminate */
    printf( "+++ End of I2C test program +++\n" );

    err = i2c_esp32_close();
    if ( err != 0 )
    {
        printf( "ERROR: failed to close I2C device (err=%i)\n", err );
    }

out:
    while(true){
        printf("end\n");
        vTaskDelay(8000 / portTICK_PERIOD_MS);
    }

    return;
}
