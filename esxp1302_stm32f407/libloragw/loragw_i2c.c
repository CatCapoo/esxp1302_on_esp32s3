/*
 * loragw_i2c.c  –  STM32F407 I2C master wrapper (HAL I2C2)
 *
 * CubeMX already initialises I2C2 via MX_I2C2_Init() in main.c before
 * FreeRTOS starts.  This module just provides a thin adapter layer so the
 * upper-level drivers (LM75A, OLED …) stay platform-agnostic.
 */

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "loragw_i2c.h"
#include "board_config.h"   /* I2C_HANDLE */
#include "i2c.h"            /* hi2c2 extern */

/* -----------------------------------------------------------------------
 * Private state
 * --------------------------------------------------------------------- */
static bool s_opened = false;

/* -----------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

int lgw_i2c_open(void)
{
    s_opened = true;
    return LGW_I2C_SUCCESS;
}

int lgw_i2c_close(void)
{
    s_opened = false;
    return LGW_I2C_SUCCESS;
}

int lgw_i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data)
{
    if (!data) return LGW_I2C_ERROR;
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(
        I2C_HANDLE,
        (uint16_t)(dev_addr << 1),
        reg_addr, I2C_MEMADD_SIZE_8BIT,
        data, 1,
        I2C_TIMEOUT_MS);
    return (ret == HAL_OK) ? LGW_I2C_SUCCESS : LGW_I2C_ERROR;
}

int lgw_i2c_read_word(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data)
{
    if (!data) return LGW_I2C_ERROR;
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(
        I2C_HANDLE,
        (uint16_t)(dev_addr << 1),
        reg_addr, I2C_MEMADD_SIZE_8BIT,
        data, 2,
        I2C_TIMEOUT_MS);
    return (ret == HAL_OK) ? LGW_I2C_SUCCESS : LGW_I2C_ERROR;
}

int lgw_i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    uint8_t buf[2] = { reg_addr, data };
    return lgw_i2c_write_buf(dev_addr, buf, 2);
}

int lgw_i2c_write_buf(uint8_t dev_addr, const uint8_t *data, size_t size)
{
    if (!data || size == 0) return LGW_I2C_ERROR;
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(
        I2C_HANDLE,
        (uint16_t)(dev_addr << 1),
        (uint8_t *)data, (uint16_t)size,
        I2C_TIMEOUT_MS);
    return (ret == HAL_OK) ? LGW_I2C_SUCCESS : LGW_I2C_ERROR;
}
