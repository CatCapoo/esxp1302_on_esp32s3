/*
 * loragw_spi.h  –  STM32 HAL SPI driver for SX1302
 *
 * Ported from ESP-IDF version to STM32 HAL.
 * Uses SPI_HandleTypeDef* instead of spi_device_handle_t*.
 */

#ifndef _LORAGW_SPI_H
#define _LORAGW_SPI_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "config.h"
#include "board_config.h"

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

#define LGW_SPI_SUCCESS     0
#define LGW_SPI_ERROR       -1
#define LGW_BURST_CHUNK     1024

#define SPI_SPEED           2000000  /* informational; actual speed set by CubeMX prescaler */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */

/**
 * @brief Open the SPI link (on STM32, SPI is already initialised by CubeMX)
 * @param spi_target  Returns a pointer to the SPI handle
 */
int lgw_spi_open(SPI_HandleTypeDef **spi_target);

/**
 * @brief Close the SPI link
 */
int lgw_spi_close(SPI_HandleTypeDef *spi);

/**
 * @brief Single-byte write
 */
int lgw_spi_w(SPI_HandleTypeDef *spi, uint8_t spi_mux_target, uint16_t address, uint8_t data);

/**
 * @brief Single-byte read
 */
int lgw_spi_r(SPI_HandleTypeDef *spi, uint8_t spi_mux_target, uint16_t address, uint8_t *data);

/**
 * @brief Single-byte read-modify-write
 */
int lgw_spi_rmw(SPI_HandleTypeDef *spi, uint8_t spi_mux_target, uint16_t address, uint8_t offs, uint8_t leng, uint8_t data);

/**
 * @brief Burst (multiple-byte) write
 */
int lgw_spi_wb(SPI_HandleTypeDef *spi, uint8_t spi_mux_target, uint16_t address, const uint8_t *data, uint16_t size);

/**
 * @brief Burst (multiple-byte) read
 */
int lgw_spi_rb(SPI_HandleTypeDef *spi, uint8_t spi_mux_target, uint16_t address, uint8_t *data, uint16_t size);

/**
 * @brief Radio SPI burst write (op_code based)
 */
int radio_spi_wb(SPI_HandleTypeDef *spi, uint8_t spi_mux_target, uint8_t op_code, const uint8_t *data, uint16_t size);

/**
 * @brief Radio SPI burst read (op_code based)
 */
int radio_spi_rb(SPI_HandleTypeDef *spi, uint8_t spi_mux_target, uint8_t op_code, uint8_t *data, uint16_t size);

/**
 * @brief SX1261 SPI burst write
 */
int sx1261_spi_wb(SPI_HandleTypeDef *spi, uint8_t op_code, const uint8_t *data, uint16_t size);

/**
 * @brief SX1261 SPI burst read
 */
int sx1261_spi_rb(SPI_HandleTypeDef *spi, uint8_t op_code, uint8_t *data, uint16_t size);

/**
 * @brief Return the SPI burst chunk size
 */
uint16_t lgw_spi_chunk_size(void);

#endif /* _LORAGW_SPI_H */
