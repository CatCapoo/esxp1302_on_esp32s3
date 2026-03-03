/*
 * test_loragw_reg.c  –  SX1302 register default-value & R/W test on STM32F407
 *
 * Ported from ESP32S3: main/libloragw-test/test_loragw_reg.c
 * (C)2019 Semtech – Revised BSD License
 *
 * Tests:
 *   TEST#1  Read every non-read-only register and verify its reset-default value.
 *   TEST#2  Write random values to every writable register, read them back and
 *           compare.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmsis_os.h"
#include "loragw_com.h"
#include "loragw_reg.h"
#include "loragw_aux.h"
#include "loragw_gpio.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

/* loregs[] is defined in loragw_reg.c and carries metadata for every register */
extern const struct lgw_reg_s loregs[LGW_TOTALREGS + 1];

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTION ------------------------------------------------------ */

void test_loragw_reg(void)
{
    int      i, x;
    int32_t  val;
    bool     error_found;
    uint8_t  rand_values[LGW_TOTALREGS];
    bool     reg_ignored[LGW_TOTALREGS];
    uint8_t  reg_val;
    uint8_t  reg_max;

    printf("Beginning of test for loragw_reg.c\n");

    /* Hardware reset */
    printf("Resetting SX1302...\n");
    lgw_reset();
    wait_ms(500);

    /* Connect via SPI */
    x = lgw_connect(LGW_COM_SPI, "spi");
    if (x != LGW_REG_SUCCESS) {
        printf("ERROR: Failed to connect\n");
        return;
    }
    printf("LoRa gateway connected\n");

    /* -----------------------------------------------------------------------
     * Mark registers that should NOT be modified during testing.
     * Setting CLK32_RIF_CTRL to 1 locks the SPI bus and makes all
     * subsequent accesses fail.
     * --------------------------------------------------------------------- */
    memset(reg_ignored, 0, sizeof reg_ignored);
    reg_ignored[SX1302_REG_COMMON_CTRL0_CLK32_RIF_CTRL] = true;

    /* -----------------------------------------------------------------------
     * TEST#1 – Read all non-read-only registers and check default values
     * --------------------------------------------------------------------- */
    printf("## TEST#1: read all registers and check default values\n");
    error_found = false;
    for (i = 0; i < LGW_TOTALREGS; i++) {
        if (loregs[i].rdon == 0) {
            x = lgw_reg_r(i, &val);
            if (x != LGW_REG_SUCCESS) {
                printf("ERROR: failed to read register %d\n", i);
                goto disconnect;
            }
            if (val != loregs[i].dflt) {
                printf("ERROR: reg[%d] default=%d, read=%d\n",
                       i, (int)loregs[i].dflt, (int)val);
                error_found = true;
            }
        }
    }
    printf("------------------\n");
    printf(" TEST#1 %s\n", error_found ? "FAILED" : "PASSED");
    printf("------------------\n\n");

    /* -----------------------------------------------------------------------
     * TEST#2 – Write random values, then read them back
     * --------------------------------------------------------------------- */
    printf("## TEST#2: R/W test on all writable registers\n");
    error_found = false;

    /* Write phase */
    for (i = 0; i < LGW_TOTALREGS; i++) {
        if ((loregs[i].rdon == 0) && (reg_ignored[i] == false)) {
            /* Maximum unsigned value for this field width */
            reg_max = (uint8_t)((1u << loregs[i].leng) - 1u);

            if (loregs[i].leng == 1) {
                /* Single-bit: flip the default */
                reg_val = (uint8_t)(!loregs[i].dflt);
            } else {
                /* Pick a random value different from the reset default */
                do {
                    if (loregs[i].sign) {
                        reg_val = (uint8_t)(rand() % (reg_max / 2));
                    } else {
                        reg_val = (uint8_t)(rand() % reg_max);
                    }
                } while (reg_val == (uint8_t)loregs[i].dflt);
            }

            x = lgw_reg_w(i, reg_val);
            if (x != LGW_REG_SUCCESS) {
                printf("ERROR: failed to write register %d\n", i);
                goto disconnect;
            }
            rand_values[i] = reg_val;
        }
    }

    /* Read & verify phase */
    for (i = 0; i < LGW_TOTALREGS; i++) {
        if ((loregs[i].rdon == 0) && (loregs[i].chck == 1) && (reg_ignored[i] == false)) {
            x = lgw_reg_r(i, &val);
            if (x != LGW_REG_SUCCESS) {
                printf("ERROR: failed to read register %d\n", i);
                goto disconnect;
            }
            if ((uint8_t)val != rand_values[i]) {
                printf("ERROR: reg[%d] wrote=0x%02X read=0x%02X\n",
                       i, rand_values[i], (uint8_t)val);
                error_found = true;
            }
        }
    }
    printf("------------------\n");
    printf(" TEST#2 %s\n", error_found ? "FAILED" : "PASSED");
    printf("------------------\n\n");

disconnect:
    x = lgw_disconnect();
    if (x != LGW_REG_SUCCESS) {
        printf("ERROR: failed to disconnect\n");
    }

    printf("End of test for loragw_reg.c\n");

    /* Idle loop so the task stays alive */
    for (;;) {
        printf("test_loragw_reg: idle\n");
        osDelay(10000);
    }
}
