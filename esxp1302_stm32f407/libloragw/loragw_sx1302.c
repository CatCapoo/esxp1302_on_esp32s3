/*
 * loragw_sx1302.c  –  Minimal SX1302 functions for STM32 bringup/test
 *
 * Only the radio control functions needed for SPI and SX1250 testing
 * are included.  The full HAL (AGC firmware, RX/TX, etc.) will be ported later.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "loragw_reg.h"
#include "loragw_aux.h"
#include "loragw_hal.h"
#include "loragw_sx1302.h"
#include "loragw_sx1250.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#if DEBUG_SX1302 == 1
    #define DEBUG_MSG(str)                printf(str)
    #define DEBUG_PRINTF(fmt, ...)        printf("%s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
    #define DEBUG_MSG(str)
    #define DEBUG_PRINTF(fmt, ...)
#endif

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS : radio clock & reset ------------------------------- */

int sx1302_radio_clock_select(uint8_t rf_chain) {
    int err = LGW_REG_SUCCESS;

    if (rf_chain >= LGW_RF_CHAIN_NB) {
        DEBUG_MSG("ERROR: invalid RF chain\n");
        return LGW_REG_ERROR;
    }

    switch (rf_chain) {
        case 0:
            DEBUG_MSG("Select Radio A clock\n");
            err |= lgw_reg_w(SX1302_REG_CLK_CTRL_CLK_SEL_CLK_RADIO_A_SEL, 0x01);
            err |= lgw_reg_w(SX1302_REG_CLK_CTRL_CLK_SEL_CLK_RADIO_B_SEL, 0x00);
            break;
        case 1:
            DEBUG_MSG("Select Radio B clock\n");
            err |= lgw_reg_w(SX1302_REG_CLK_CTRL_CLK_SEL_CLK_RADIO_A_SEL, 0x00);
            err |= lgw_reg_w(SX1302_REG_CLK_CTRL_CLK_SEL_CLK_RADIO_B_SEL, 0x01);
            break;
        default:
            return LGW_REG_ERROR;
    }

    err |= lgw_reg_w(SX1302_REG_CLK_CTRL_CLK_SEL_CLKDIV_EN, 0x01);
    err |= lgw_reg_w(SX1302_REG_COMMON_CTRL0_CLK32_RIF_CTRL, 0x01);

    return err;
}

int sx1302_radio_reset(uint8_t rf_chain, lgw_radio_type_t type) {
    uint16_t reg_radio_en;
    uint16_t reg_radio_rst;
    int err = LGW_REG_SUCCESS;

    if (rf_chain >= LGW_RF_CHAIN_NB) {
        DEBUG_MSG("ERROR: invalid RF chain\n");
        return LGW_REG_ERROR;
    }
    if ((type != LGW_RADIO_TYPE_SX1255) && (type != LGW_RADIO_TYPE_SX1257) && (type != LGW_RADIO_TYPE_SX1250)) {
        DEBUG_MSG("ERROR: invalid radio type\n");
        return LGW_REG_ERROR;
    }

    /* Switch to SPI clock before resetting the radio */
    err |= lgw_reg_w(SX1302_REG_COMMON_CTRL0_CLK32_RIF_CTRL, 0x00);

    /* Enable the radio */
    reg_radio_en = REG_SELECT(rf_chain, SX1302_REG_AGC_MCU_RF_EN_A_RADIO_EN, SX1302_REG_AGC_MCU_RF_EN_B_RADIO_EN);
    err |= lgw_reg_w(reg_radio_en, 0x01);

    /* Reset sequence */
    reg_radio_rst = REG_SELECT(rf_chain, SX1302_REG_AGC_MCU_RF_EN_A_RADIO_RST, SX1302_REG_AGC_MCU_RF_EN_B_RADIO_RST);
    err |= lgw_reg_w(reg_radio_rst, 0x01);
    wait_ms(500);
    err |= lgw_reg_w(reg_radio_rst, 0x00);
    wait_ms(10);

    /* For SX1250: assert RST again to trigger internal auto-calibration */
    if (type == LGW_RADIO_TYPE_SX1250) {
        err |= lgw_reg_w(reg_radio_rst, 0x01);
        wait_ms(10); /* wait for auto calibration to complete */
    }

    DEBUG_PRINTF("Radio %u reset done (type=%d)\n", rf_chain, type);

    return err;
}

int sx1302_radio_set_mode(uint8_t rf_chain, lgw_radio_type_t type) {
    uint16_t reg;
    int err;

    if (rf_chain >= LGW_RF_CHAIN_NB) {
        DEBUG_MSG("ERROR: invalid RF chain\n");
        return LGW_REG_ERROR;
    }
    if ((type != LGW_RADIO_TYPE_SX1255) && (type != LGW_RADIO_TYPE_SX1257) && (type != LGW_RADIO_TYPE_SX1250)) {
        DEBUG_MSG("ERROR: invalid radio type\n");
        return LGW_REG_ERROR;
    }

    reg = REG_SELECT(rf_chain, SX1302_REG_COMMON_CTRL0_SX1261_MODE_RADIO_A,
                                SX1302_REG_COMMON_CTRL0_SX1261_MODE_RADIO_B);
    switch (type) {
        case LGW_RADIO_TYPE_SX1250:
            DEBUG_PRINTF("Setting rf_chain_%u in sx1250 mode\n", rf_chain);
            err = lgw_reg_w(reg, 0x01);
            break;
        default:
            DEBUG_PRINTF("Setting rf_chain_%u in sx125x mode\n", rf_chain);
            err = lgw_reg_w(reg, 0x00);
            break;
    }
    if (err != LGW_REG_SUCCESS) {
        printf("ERROR: failed to set mode for radio %u\n", rf_chain);
        return LGW_REG_ERROR;
    }

    return LGW_REG_SUCCESS;
}
