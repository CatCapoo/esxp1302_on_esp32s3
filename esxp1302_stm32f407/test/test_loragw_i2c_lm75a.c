/*
 * test_loragw_i2c_lm75a.c  –  LM75A temperature sensor test on STM32F407
 *
 * Run with LM75A connected to I2C2 (PF0-SDA / PF1-SCL).
 * Scans addresses 0x48–0x4F and configures the first device found,
 * then reads temperature every second for 20 cycles.
 */

#include <stdio.h>
#include "cmsis_os.h"
#include "loragw_i2c.h"
#include "loragw_lm75a.h"
#include "loragw_aux.h"

/* LM75A address range */
#define LM75A_ADDR_MIN  0x48
#define LM75A_ADDR_MAX  0x4F
#define READ_CYCLES     20

void test_loragw_i2c_lm75a(void)
{
    int     err;
    int     cycle = 0;
    float   temperature;
    uint8_t found_addr = 0;
    uint8_t dummy;

    printf("Beginning of test for LM75A temperature sensor\n");

    lgw_i2c_open();

    /* -----------------------------------------------------------------------
     * Scan I2C bus for LM75A (addresses 0x48–0x4F)
     * --------------------------------------------------------------------- */
    printf("Scanning I2C bus for LM75A (0x48–0x4F)...\n");
    for (uint8_t addr = LM75A_ADDR_MIN; addr <= LM75A_ADDR_MAX; addr++) {
        err = lgw_i2c_read(addr, 0x01, &dummy);  /* read config reg to probe */
        if (err == LGW_I2C_SUCCESS) {
            printf("  Found device at 0x%02X\n", addr);
            if (found_addr == 0) found_addr = addr;
        }
    }

    if (found_addr == 0) {
        printf("ERROR: No LM75A found on I2C bus\n");
        goto idle;
    }
    printf("Using LM75A at 0x%02X\n", found_addr);

    /* -----------------------------------------------------------------------
     * Configure LM75A
     * --------------------------------------------------------------------- */
    err = lm75a_configure(found_addr);
    if (err != LGW_I2C_SUCCESS) {
        printf("ERROR: lm75a_configure failed\n");
        goto idle;
    }

    /* -----------------------------------------------------------------------
     * Read temperature loop
     * --------------------------------------------------------------------- */
    printf("Reading temperature (%d samples, 1 s interval):\n", READ_CYCLES);
    while (cycle < READ_CYCLES) {
        err = lm75a_get_temperature(found_addr, &temperature);
        if (err == LGW_I2C_SUCCESS) {
            /* Print with 3 decimal places (0.125 °C resolution) */
            int  t_int  = (int)temperature;
            int  t_frac = (int)((temperature - (float)t_int) * 1000.0f);
            if (t_frac < 0) t_frac = -t_frac;
            printf("  [%2d] Temperature: %d.%03d C\n", cycle, t_int, t_frac);
        } else {
            printf("  [%2d] ERROR: failed to read temperature\n", cycle);
        }
        cycle++;
        wait_ms(1000);
    }

    printf("End of test for LM75A temperature sensor\n");

idle:
    lgw_i2c_close();
    for (;;) {
        printf("test_lm75a: idle\n");
        osDelay(10000);
    }
}
