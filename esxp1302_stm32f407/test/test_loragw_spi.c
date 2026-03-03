/*
 * test_loragw_spi.c  –  SPI R/W stress test for SX1302 on STM32F407
 *
 * Ported from ESP32S3 bringup/test branch.
 * Uses STM32 HAL SPI + CMSIS-RTOS2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmsis_os.h"
#include "loragw_com.h"
#include "loragw_spi.h"
#include "loragw_gpio.h"
#include "loragw_aux.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

#define BUFF_SIZE           (1024 * 4)

#define SX1302_AGC_MCU_MEM  0x0000
#define SX1302_REG_COMMON   0x5600
#define SX1302_REG_AGC_MCU  0x5780

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

static uint8_t test_buff[BUFF_SIZE];
static uint8_t read_buff[BUFF_SIZE];

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTION ------------------------------------------------------ */

void test_loragw_spi(void)
{
    uint8_t data = 0;
    int cycle_number = 0;
    int i;
    uint16_t size;
    int rc;

    SPI_HandleTypeDef *spi = NULL;

    printf("Beginning of test for loragw_spi.c\n");

    /* Hardware reset the SX1302 */
    printf("Resetting SX1302...\n");
    lgw_reset();
    wait_ms(100);

    /* Open SPI */
    i = lgw_spi_open(&spi);
    if (i != 0) {
        printf("ERROR: failed to open SPI device\n");
        return;
    }

    /* Read SX1302 version register (COMMON + 6 = 0x5606), expected 0x10 */
    rc = lgw_spi_r(spi, LGW_SPI_MUX_TARGET_SX1302, SX1302_REG_COMMON + 6, &data);
    if (rc != LGW_SPI_SUCCESS) {
        printf("ERROR: lgw_spi_r(version) failed: %d\n", rc);
    } else {
        printf("SX1302 version: 0x%02X%s\n", data, data == 0x10 ? " (OK)" : " (ERROR: expected 0x10)");
    }

    /* Prepare AGC MCU for memory access */
    rc = lgw_spi_r(spi, LGW_SPI_MUX_TARGET_SX1302, SX1302_REG_AGC_MCU + 0, &data);
    if (rc != LGW_SPI_SUCCESS) {
        printf("ERROR: lgw_spi_r(AGC_MCU+0) failed: %d\n", rc);
    }
    rc = lgw_spi_w(spi, LGW_SPI_MUX_TARGET_SX1302, SX1302_REG_AGC_MCU + 0, 0x06);
    if (rc != LGW_SPI_SUCCESS) {
        printf("ERROR: lgw_spi_w(AGC_MCU+0) failed: %d\n", rc);
    }

    /* Databuffer R/W stress test */
    for (cycle_number = 0; cycle_number < 100; cycle_number++) {
        size = rand() % BUFF_SIZE;
        for (i = 0; i < size; ++i) {
            test_buff[i] = i & 0xFF;
        }
        printf("Cycle %i (size: %d) > ", cycle_number, size);

        lgw_spi_wb(spi, LGW_SPI_MUX_TARGET_SX1302, SX1302_AGC_MCU_MEM, test_buff, size);
        lgw_spi_rb(spi, LGW_SPI_MUX_TARGET_SX1302, SX1302_AGC_MCU_MEM, read_buff, size);

        for (i = 0; ((i < size) && (test_buff[i] == read_buff[i])); ++i);
        if (i != size) {
            printf("error during the buffer comparison\n");
            printf("Written values:\n");
            for (i = 0; i < size; ++i) {
                printf(" %02X ", test_buff[i]);
                if (i % 16 == 15)
                    printf("\n");
            }
            printf("\n");
            printf("Read values:\n");
            for (i = 0; i < size; ++i) {
                printf(" %02X ", read_buff[i]);
                if (i % 16 == 15)
                    printf("\n");
            }
            printf("\n");
            break;
        } else {
            printf("did a %i-byte R/W on a data buffer with no error\n", size);
            ++cycle_number;
        }
    }

    lgw_spi_close(spi);
    printf("End of test for loragw_spi.c\n");

    /* Stay alive */
    for (;;) {
        printf("test_spi: idle\n");
        osDelay(8000);
    }
}
