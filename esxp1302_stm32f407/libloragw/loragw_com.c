/*
 * loragw_com.c  –  Communication abstraction (SPI-only for STM32)
 *
 * Ported from the ESP-IDF version.  USB paths are stubbed out.
 */

#include <stdint.h>
#include <stdio.h>

#include "loragw_com.h"
#include "loragw_spi.h"
#include "loragw_aux.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#if DEBUG_COM == 1
    #define DEBUG_MSG(str)                printf(str)
    #define DEBUG_PRINTF(fmt, ...)        printf("%s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
    #define CHECK_NULL(a)                 if((a)==NULL){printf("%s:%d: ERROR: NULL POINTER\n",__FUNCTION__,__LINE__);return LGW_COM_ERROR;}
#else
    #define DEBUG_MSG(str)
    #define DEBUG_PRINTF(fmt, ...)
    #define CHECK_NULL(a)                 if((a)==NULL){return LGW_COM_ERROR;}
#endif

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

static lgw_com_type_t  _lgw_com_type   = LGW_COM_UNKNOWN;
static void           *_lgw_com_target = NULL;

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */

int lgw_com_open(lgw_com_type_t com_type, const char *com_path) {
    int com_stat;

    CHECK_NULL(com_path);
    if (com_type != LGW_COM_SPI) {
        printf("ERROR: only SPI is supported on STM32\n");
        return LGW_COM_ERROR;
    }

    if (_lgw_com_target != NULL) {
        DEBUG_MSG("WARNING: concentrator was already connected\n");
        lgw_com_close();
    }

    _lgw_com_type = com_type;

    printf("Opening SPI communication interface\n");
    com_stat = lgw_spi_open((SPI_HandleTypeDef **)&_lgw_com_target);

    return com_stat;
}

int lgw_com_close(void) {
    int com_stat;

    if (_lgw_com_target == NULL) {
        printf("ERROR: concentrator is not connected\n");
        return -1;
    }

    printf("Closing SPI communication interface\n");
    com_stat = lgw_spi_close((SPI_HandleTypeDef *)_lgw_com_target);
    _lgw_com_target = NULL;

    return com_stat;
}

int lgw_com_w(uint8_t spi_mux_target, uint16_t address, uint8_t data) {
    CHECK_NULL(_lgw_com_target);
    return lgw_spi_w((SPI_HandleTypeDef *)_lgw_com_target, spi_mux_target, address, data);
}

int lgw_com_r(uint8_t spi_mux_target, uint16_t address, uint8_t *data) {
    CHECK_NULL(_lgw_com_target);
    CHECK_NULL(data);
    return lgw_spi_r((SPI_HandleTypeDef *)_lgw_com_target, spi_mux_target, address, data);
}

int lgw_com_rmw(uint8_t spi_mux_target, uint16_t address, uint8_t offs, uint8_t leng, uint8_t data) {
    CHECK_NULL(_lgw_com_target);
    return lgw_spi_rmw((SPI_HandleTypeDef *)_lgw_com_target, spi_mux_target, address, offs, leng, data);
}

int lgw_com_wb(uint8_t spi_mux_target, uint16_t address, const uint8_t *data, uint16_t size) {
    CHECK_NULL(_lgw_com_target);
    CHECK_NULL(data);
    return lgw_spi_wb((SPI_HandleTypeDef *)_lgw_com_target, spi_mux_target, address, data, size);
}

int lgw_com_rb(uint8_t spi_mux_target, uint16_t address, uint8_t *data, uint16_t size) {
    CHECK_NULL(_lgw_com_target);
    CHECK_NULL(data);
    return lgw_spi_rb((SPI_HandleTypeDef *)_lgw_com_target, spi_mux_target, address, data, size);
}

int lgw_com_set_write_mode(lgw_com_write_mode_t write_mode) {
    (void)write_mode;
    return LGW_COM_SUCCESS; /* only single mode on SPI */
}

int lgw_com_flush(void) {
    return LGW_COM_SUCCESS;
}

uint16_t lgw_com_chunk_size(void) {
    return lgw_spi_chunk_size();
}

void* lgw_com_target(void) {
    return _lgw_com_target;
}

lgw_com_type_t lgw_com_type(void) {
    return _lgw_com_type;
}
