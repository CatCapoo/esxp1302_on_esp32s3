/*
 * loragw_i2c.c  –  STM32F407 I2C master wrapper (HAL I2C1)
 *
 * CubeMX already initialises I2C1 via MX_I2C1_Init() in main.c before
 * FreeRTOS starts.  This module just provides a thin adapter layer so the
 * upper-level drivers (LM75A, OLED …) stay platform-agnostic.
 *
 * IMPORTANT: I2C1 is shared between the OLED (SSD1306) and the temperature
 * sensor (LM75A).  The OLED is updated from the main pkt_fwd task while
 * thread_up calls lgw_receive() → lgw_get_temperature() → I2C read from
 * a higher-priority task.  STM32 HAL's __HAL_LOCK is a simple flag check
 * that is NOT safe under FreeRTOS pre-emption, so concurrent HAL I2C calls
 * can corrupt the peripheral state and cause silent failures (HAL_BUSY).
 *
 * Fix: a FreeRTOS mutex serialises every I2C1 operation.
 */

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "loragw_i2c.h"
#include "board_config.h"   /* I2C_HANDLE */
#include "i2c.h"            /* hi2c1 extern */

/* FreeRTOS mutex for I2C1 bus serialisation */
#include "FreeRTOS.h"
#include "semphr.h"

/* -----------------------------------------------------------------------
 * Private state
 * --------------------------------------------------------------------- */
static bool s_opened = false;
static SemaphoreHandle_t s_i2c_mtx = NULL;

/**
 * @brief Ensure the I2C mutex exists (lazy init, safe to call repeatedly).
 *
 * Called automatically before each I2C operation so the mutex is available
 * even if lgw_i2c_open() has not been called yet (e.g. oled_init() runs
 * before lgw_start()).
 */
static void i2c_mtx_ensure(void)
{
    if (s_i2c_mtx == NULL) {
        s_i2c_mtx = xSemaphoreCreateMutex();
    }
}

/* -----------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

int lgw_i2c_open(void)
{
    i2c_mtx_ensure();
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
    i2c_mtx_ensure();
    xSemaphoreTake(s_i2c_mtx, portMAX_DELAY);
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(
        I2C_HANDLE,
        (uint16_t)(dev_addr << 1),
        reg_addr, I2C_MEMADD_SIZE_8BIT,
        data, 1,
        I2C_TIMEOUT_MS);
    if (ret == HAL_BUSY || ret == HAL_TIMEOUT) {
        HAL_I2C_DeInit(I2C_HANDLE);
        HAL_I2C_Init(I2C_HANDLE);
        ret = HAL_I2C_Mem_Read(
            I2C_HANDLE,
            (uint16_t)(dev_addr << 1),
            reg_addr, I2C_MEMADD_SIZE_8BIT,
            data, 1,
            I2C_TIMEOUT_MS);
    }
    xSemaphoreGive(s_i2c_mtx);
    return (ret == HAL_OK) ? LGW_I2C_SUCCESS : LGW_I2C_ERROR;
}

int lgw_i2c_read_word(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data)
{
    if (!data) return LGW_I2C_ERROR;
    i2c_mtx_ensure();
    xSemaphoreTake(s_i2c_mtx, portMAX_DELAY);
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(
        I2C_HANDLE,
        (uint16_t)(dev_addr << 1),
        reg_addr, I2C_MEMADD_SIZE_8BIT,
        data, 2,
        I2C_TIMEOUT_MS);
    if (ret == HAL_BUSY || ret == HAL_TIMEOUT) {
        HAL_I2C_DeInit(I2C_HANDLE);
        HAL_I2C_Init(I2C_HANDLE);
        ret = HAL_I2C_Mem_Read(
            I2C_HANDLE,
            (uint16_t)(dev_addr << 1),
            reg_addr, I2C_MEMADD_SIZE_8BIT,
            data, 2,
            I2C_TIMEOUT_MS);
    }
    xSemaphoreGive(s_i2c_mtx);
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
    i2c_mtx_ensure();

    /* Dynamic timeout: at 100 kHz I2C each byte ≈ 0.09 ms.
     * The 1025-byte OLED refresh needs ~92 ms, barely under the
     * default 100 ms.  Add 1 ms per 5 bytes so preemption / ISR
     * jitter can't push us past the deadline. */
    uint32_t timeout = I2C_TIMEOUT_MS + (uint32_t)(size / 5);

    xSemaphoreTake(s_i2c_mtx, portMAX_DELAY);
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(
        I2C_HANDLE,
        (uint16_t)(dev_addr << 1),
        (uint8_t *)data, (uint16_t)size,
        timeout);
    if (ret != HAL_OK) {
        /* --- TEMPORARY DIAGNOSTIC --- */
        printf("[I2C] ERR: write 0x%02X len=%u HAL=%d, resetting...\r\n",
               dev_addr, (unsigned)size, (int)ret);
        /* Reset I2C peripheral and retry once */
        HAL_I2C_DeInit(I2C_HANDLE);
        HAL_I2C_Init(I2C_HANDLE);
        ret = HAL_I2C_Master_Transmit(
            I2C_HANDLE,
            (uint16_t)(dev_addr << 1),
            (uint8_t *)data, (uint16_t)size,
            timeout);
        if (ret != HAL_OK) {
            printf("[I2C] ERR: retry also failed (HAL=%d)\r\n", (int)ret);
        } else {
            printf("[I2C] OK: recovered after reset\r\n");
        }
    }
    xSemaphoreGive(s_i2c_mtx);
    return (ret == HAL_OK) ? LGW_I2C_SUCCESS : LGW_I2C_ERROR;
}
