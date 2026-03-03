/*
 * test_loragw_spi_sx1250.c  –  SX1250 register R/W test via SX1302 on STM32F407
 *
 * Ported from ESP32S3 bringup/test branch.
 * Uses STM32 HAL SPI + CMSIS-RTOS2.
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmsis_os.h"
#include "loragw_spi.h"
#include "loragw_aux.h"
#include "loragw_reg.h"
#include "loragw_hal.h"
#include "loragw_sx1250.h"
#include "loragw_sx1302.h"
#include "loragw_gpio.h"
#include "loragw_debug.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

#define BUFF_SIZE   16

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTION ------------------------------------------------------ */

void test_loragw_spi_sx1250(void)
{
    uint8_t test_buff[BUFF_SIZE];
    uint8_t read_buff[BUFF_SIZE];
    uint32_t test_val, read_val;
    int cycle_number = 0;
    int i, x;

    printf("Beginning of test for loragw_spi_sx1250.c\n");

    /* Initialise random payload generator */
    dbg_init_random();

    /* Hardware reset the SX1302 */
    printf("Resetting SX1302...\n");
    lgw_reset();
    wait_ms(500);

    /* Connect to the concentrator via register abstraction layer */
    x = lgw_connect(LGW_COM_SPI, "spi");
    if (x != LGW_REG_SUCCESS) {
        printf("ERROR: Failed to connect to the concentrator using SPI\n");
        return;
    }
    printf("LoRa gateway connected\n");

    /* Reset radios */
    for (i = 0; i < LGW_RF_CHAIN_NB; i++) {
        sx1302_radio_reset(i, LGW_RADIO_TYPE_SX1250);
        sx1302_radio_set_mode(i, LGW_RADIO_TYPE_SX1250);
    }

    /* Select the radio which provides the clock to the sx1302 */
    sx1302_radio_clock_select(0);

    /* Ensure we can control the radio from host */
    lgw_reg_w(SX1302_REG_COMMON_CTRL0_HOST_RADIO_CTRL, 0x01);

    /* Ensure PA/LNA are disabled */
    lgw_reg_w(SX1302_REG_AGC_MCU_CTRL_FORCE_HOST_FE_CTRL, 1);
    lgw_reg_w(SX1302_REG_AGC_MCU_RF_EN_A_PA_EN, 0);
    lgw_reg_w(SX1302_REG_AGC_MCU_RF_EN_A_LNA_EN, 0);

    /* Set Radio in Standby mode (XOSC) */
    test_buff[0] = (uint8_t)STDBY_XOSC;
    sx1250_reg_w(SET_STANDBY, test_buff, 1, 0);
    sx1250_reg_w(SET_STANDBY, test_buff, 1, 1);
    wait_ms(10);

    /* Read radio status */
    test_buff[0] = 0x00;
    sx1250_reg_r(GET_STATUS, test_buff, 1, 0);
    printf("Radio0: get_status: 0x%02X\n", test_buff[0]);
    sx1250_reg_r(GET_STATUS, test_buff, 1, 1);
    printf("Radio1: get_status: 0x%02X\n", test_buff[0]);

    /* Register R/W stress test – write RF frequency and read it back */
    while (cycle_number < 20) {
        test_buff[0] = rand() & 0x7F;
        test_buff[1] = rand() & 0xFF;
        test_buff[2] = rand() & 0xFF;
        test_buff[3] = rand() & 0xFF;
        test_val = ((uint32_t)test_buff[0] << 24) |
                   ((uint32_t)test_buff[1] << 16) |
                   ((uint32_t)test_buff[2] << 8)  |
                   ((uint32_t)test_buff[3] << 0);

        sx1250_reg_w(SET_RF_FREQUENCY, test_buff, 4, 0);
        wait_ms(1);

        /* Read back from SX1250 internal register 0x088B (RF frequency) */
        read_buff[0] = 0x08;
        read_buff[1] = 0x8B;
        read_buff[2] = 0x00;
        read_buff[3] = 0x00;
        read_buff[4] = 0x00;
        read_buff[5] = 0x00;
        read_buff[6] = 0x00;
        sx1250_reg_r(READ_REGISTER, read_buff, 7, 0);
        read_val = ((uint32_t)read_buff[3] << 24) |
                   ((uint32_t)read_buff[4] << 16) |
                   ((uint32_t)read_buff[5] << 8)  |
                   ((uint32_t)read_buff[6] << 0);

        printf("Cycle %i > ", cycle_number);
        if (read_val != test_val) {
            printf("error during the buffer comparison\n");
            printf("Written value: %08" PRIX32 "\n", test_val);
            printf("Read value:    %08" PRIX32 "\n", read_val);
            break;
        } else {
            printf("did a %i-byte R/W on a register with no error\n", 4);
            ++cycle_number;
        }
    }

    lgw_disconnect();
    printf("End of test for loragw_spi_sx1250.c\n");

    /* Stay alive */
    for (;;) {
        printf("test_sx1250: idle\n");
        osDelay(8000);
    }
}
