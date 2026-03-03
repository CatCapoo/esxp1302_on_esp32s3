/*
 * loragw_spi.c  –  STM32 HAL SPI driver for SX1302
 *
 * Ported from ESP-IDF version.  Uses STM32 HAL SPI with manual CS control.
 *
 * SX1302 SPI protocol:
 *   Write single: [mux | WR|addr_hi | addr_lo | data]
 *   Read  single: [mux | RD|addr_hi | addr_lo | NOP] → rx[3]=data
 *   Write burst:  [mux | WR|addr_hi | addr_lo | d0 d1 …]
 *   Read  burst:  [mux | RD|addr_hi | addr_lo | NOP | d0 d1 …]
 *   Radio write:  [mux | WR|opcode  | d0 d1 …]
 *   Radio read:   [mux | RD|opcode  | d0 d1 …]
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "loragw_spi.h"
#include "loragw_aux.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#if DEBUG_SPI == 1
    #define DEBUG_MSG(str)                printf(str)
    #define DEBUG_PRINTF(fmt, ...)        printf("%s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
    #define CHECK_NULL(a)                 if((a)==NULL){printf("%s:%d: ERROR: NULL POINTER\n",__FUNCTION__,__LINE__);return LGW_SPI_ERROR;}
#else
    #define DEBUG_MSG(str)
    #define DEBUG_PRINTF(fmt, ...)
    #define CHECK_NULL(a)                 if((a)==NULL){return LGW_SPI_ERROR;}
#endif

#define READ_ACCESS     0x0000
#define WRITE_ACCESS    0x8000
#define ADDR_MASK       0x7FFF

/* SPI timeout in milliseconds */
#define SPI_TIMEOUT_MS  1000

/* -------------------------------------------------------------------------- */
/* --- CS HELPERS ----------------------------------------------------------- */

static inline void cs_select(void)
{
    HAL_GPIO_WritePin(SX1302_NSS_PORT, SX1302_NSS_PIN, GPIO_PIN_RESET);
}

static inline void cs_deselect(void)
{
    HAL_GPIO_WritePin(SX1302_NSS_PORT, SX1302_NSS_PIN, GPIO_PIN_SET);
}

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */

int lgw_spi_open(SPI_HandleTypeDef **spi_target)
{
    /* On STM32 the SPI peripheral is already initialised by MX_SPI2_Init().
       We just hand back a pointer to the global handle. */
    *spi_target = SX1302_SPI_HANDLE;
    cs_deselect(); /* make sure NSS is high */
    printf("SPI link opened (STM32 HAL)\n");
    return LGW_SPI_SUCCESS;
}

int lgw_spi_close(SPI_HandleTypeDef *spi)
{
    CHECK_NULL(spi);
    cs_deselect();
    printf("SPI link closed\n");
    return LGW_SPI_SUCCESS;
}

/* ---- Single-byte write -------------------------------------------------- */

int lgw_spi_w(SPI_HandleTypeDef *spi, uint8_t spi_mux_target,
              uint16_t address, uint8_t data)
{
    HAL_StatusTypeDef rc;
    uint8_t tx[4];

    CHECK_NULL(spi);

    tx[0] = spi_mux_target;
    tx[1] = (uint8_t)((WRITE_ACCESS | (address & ADDR_MASK)) >> 8);
    tx[2] = (uint8_t)(address & 0xFF);
    tx[3] = data;

    cs_select();
    rc = HAL_SPI_Transmit(spi, tx, 4, SPI_TIMEOUT_MS);
    cs_deselect();

    return (rc == HAL_OK) ? LGW_SPI_SUCCESS : LGW_SPI_ERROR;
}

/* ---- Single-byte read --------------------------------------------------- */

int lgw_spi_r(SPI_HandleTypeDef *spi, uint8_t spi_mux_target,
              uint16_t address, uint8_t *data)
{
    HAL_StatusTypeDef rc;
    uint8_t tx[5], rx[5];

    CHECK_NULL(spi);
    CHECK_NULL(data);

    tx[0] = spi_mux_target;
    tx[1] = (uint8_t)((READ_ACCESS | (address & ADDR_MASK)) >> 8);
    tx[2] = (uint8_t)(address & 0xFF);
    tx[3] = 0x00; /* NOP / dummy */
    tx[4] = 0x00; /* will clock out the data byte */

    cs_select();
    rc = HAL_SPI_TransmitReceive(spi, tx, rx, 5, SPI_TIMEOUT_MS);
    cs_deselect();

    if (rc != HAL_OK)
        return LGW_SPI_ERROR;

    *data = rx[4]; /* the data byte appears at rx[4] */

#if DEBUG_SPI == 1
    for (int i = 0; i < 5; i++) printf("0x%02X ", rx[i]);
    printf("\n");
#endif

    return LGW_SPI_SUCCESS;
}

/* ---- Read-modify-write -------------------------------------------------- */

int lgw_spi_rmw(SPI_HandleTypeDef *spi, uint8_t spi_mux_target,
                uint16_t address, uint8_t offs, uint8_t leng, uint8_t data)
{
    int spi_stat = LGW_SPI_SUCCESS;
    uint8_t buf[4] = {0};

    /* Read */
    spi_stat += lgw_spi_r(spi, spi_mux_target, address, &buf[0]);

    /* Modify */
    buf[1] = ((1 << leng) - 1) << offs;          /* bit mask */
    buf[2] = ((uint8_t)data) << offs;             /* new data offsetted */
    buf[3] = (~buf[1] & buf[0]) | (buf[1] & buf[2]); /* mix old & new */

    /* Write */
    spi_stat += lgw_spi_w(spi, spi_mux_target, address, buf[3]);

    return spi_stat;
}

/* ---- Burst write -------------------------------------------------------- */

int lgw_spi_wb(SPI_HandleTypeDef *spi, uint8_t spi_mux_target,
               uint16_t address, const uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef rc;
    uint8_t hdr[3];

    CHECK_NULL(spi);
    CHECK_NULL(data);

    hdr[0] = spi_mux_target;
    hdr[1] = (uint8_t)((WRITE_ACCESS | (address & ADDR_MASK)) >> 8);
    hdr[2] = (uint8_t)(address & 0xFF);

    cs_select();

    /* Send header */
    rc = HAL_SPI_Transmit(spi, hdr, 3, SPI_TIMEOUT_MS);
    if (rc != HAL_OK) {
        cs_deselect();
        return LGW_SPI_ERROR;
    }

    /* Send data in chunks */
    uint16_t size_to_do = size;
    uint16_t offset = 0;
    while (size_to_do > 0) {
        uint16_t chunk = (size_to_do < LGW_BURST_CHUNK) ? size_to_do : LGW_BURST_CHUNK;
        rc = HAL_SPI_Transmit(spi, (uint8_t *)(data + offset), chunk, SPI_TIMEOUT_MS);
        if (rc != HAL_OK) {
            cs_deselect();
            return LGW_SPI_ERROR;
        }
        offset += chunk;
        size_to_do -= chunk;
        DEBUG_PRINTF("BURST WRITE: to_do %d chunk %d transferred %d\n", size_to_do, chunk, offset);
    }

    cs_deselect();
    return LGW_SPI_SUCCESS;
}

/* ---- Burst read --------------------------------------------------------- */

int lgw_spi_rb(SPI_HandleTypeDef *spi, uint8_t spi_mux_target,
               uint16_t address, uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef rc;
    uint8_t hdr[4]; /* mux + addr(2) + NOP */
    uint8_t dummy[4];

    CHECK_NULL(spi);
    CHECK_NULL(data);

    hdr[0] = spi_mux_target;
    hdr[1] = (uint8_t)((READ_ACCESS | (address & ADDR_MASK)) >> 8);
    hdr[2] = (uint8_t)(address & 0xFF);
    hdr[3] = 0x00; /* NOP byte */

    cs_select();

    /* Send header (with dummy-receive) */
    rc = HAL_SPI_TransmitReceive(spi, hdr, dummy, 4, SPI_TIMEOUT_MS);
    if (rc != HAL_OK) {
        cs_deselect();
        return LGW_SPI_ERROR;
    }

    /* Receive data in chunks – transmit zeros to clock out */
    uint16_t size_to_do = size;
    uint16_t offset = 0;
    uint8_t zeros[LGW_BURST_CHUNK];
    memset(zeros, 0, sizeof(zeros));

    while (size_to_do > 0) {
        uint16_t chunk = (size_to_do < LGW_BURST_CHUNK) ? size_to_do : LGW_BURST_CHUNK;
        rc = HAL_SPI_TransmitReceive(spi, zeros, data + offset, chunk, SPI_TIMEOUT_MS);
        if (rc != HAL_OK) {
            cs_deselect();
            return LGW_SPI_ERROR;
        }
        offset += chunk;
        size_to_do -= chunk;
        DEBUG_PRINTF("BURST READ: to_do %d chunk %d transferred %d\n", size_to_do, chunk, offset);
    }

    cs_deselect();
    return LGW_SPI_SUCCESS;
}

/* ---- Radio burst write (op-code based) ---------------------------------- */

int radio_spi_wb(SPI_HandleTypeDef *spi, uint8_t spi_mux_target,
                 uint8_t op_code, const uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef rc;
    uint8_t hdr[2];

    CHECK_NULL(spi);
    CHECK_NULL(data);

    if (2 + size > LGW_BURST_CHUNK) {
        DEBUG_PRINTF("size (%d) too big for radio burst write!\n", size);
        return LGW_SPI_ERROR;
    }

    hdr[0] = spi_mux_target;
    hdr[1] = op_code;  /* SX1250 write opcodes all have bit7=1 naturally */

    cs_select();

    rc = HAL_SPI_Transmit(spi, hdr, 2, SPI_TIMEOUT_MS);
    if (rc != HAL_OK) { cs_deselect(); return LGW_SPI_ERROR; }

    rc = HAL_SPI_Transmit(spi, (uint8_t *)data, size, SPI_TIMEOUT_MS);
    if (rc != HAL_OK) { cs_deselect(); return LGW_SPI_ERROR; }

    cs_deselect();
    return LGW_SPI_SUCCESS;
}

/* ---- Radio burst read (op-code based) ----------------------------------- */

int radio_spi_rb(SPI_HandleTypeDef *spi, uint8_t spi_mux_target,
                 uint8_t op_code, uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef rc;
    uint8_t hdr[2];
    uint8_t dummy[2];

    CHECK_NULL(spi);
    CHECK_NULL(data);

    if (2 + size > LGW_BURST_CHUNK) {
        DEBUG_PRINTF("size (%d) too big for radio burst read!\n", size);
        return LGW_SPI_ERROR;
    }

    hdr[0] = spi_mux_target;
    hdr[1] = op_code;  /* send opcode verbatim, bit7 is part of the SX1250 opcode */

    cs_select();

    rc = HAL_SPI_TransmitReceive(spi, hdr, dummy, 2, SPI_TIMEOUT_MS);
    if (rc != HAL_OK) { cs_deselect(); return LGW_SPI_ERROR; }

    /* data buffer carries TX bytes (addr/NOP for READ_REGISTER etc.)
       and receives response in-place */
    uint8_t tx_buf[LGW_BURST_CHUNK];
    memcpy(tx_buf, data, size);
    rc = HAL_SPI_TransmitReceive(spi, tx_buf, data, size, SPI_TIMEOUT_MS);
    if (rc != HAL_OK) { cs_deselect(); return LGW_SPI_ERROR; }

    cs_deselect();
    return LGW_SPI_SUCCESS;
}

/* ---- SX1261 burst write ------------------------------------------------- */

int sx1261_spi_wb(SPI_HandleTypeDef *spi, uint8_t op_code,
                  const uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef rc;
    uint8_t hdr[1];

    CHECK_NULL(spi);
    CHECK_NULL(data);

    hdr[0] = (uint8_t)(WRITE_ACCESS >> 8) | (op_code & 0x7F);

    cs_select();

    rc = HAL_SPI_Transmit(spi, hdr, 1, SPI_TIMEOUT_MS);
    if (rc != HAL_OK) { cs_deselect(); return LGW_SPI_ERROR; }

    rc = HAL_SPI_Transmit(spi, (uint8_t *)data, size, SPI_TIMEOUT_MS);
    if (rc != HAL_OK) { cs_deselect(); return LGW_SPI_ERROR; }

    cs_deselect();
    return LGW_SPI_SUCCESS;
}

/* ---- SX1261 burst read -------------------------------------------------- */

int sx1261_spi_rb(SPI_HandleTypeDef *spi, uint8_t op_code,
                  uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef rc;
    uint8_t hdr[2];
    uint8_t dummy[2];

    CHECK_NULL(spi);
    CHECK_NULL(data);

    hdr[0] = (op_code & 0x7F);
    hdr[1] = 0x00; /* NOP */

    cs_select();

    rc = HAL_SPI_TransmitReceive(spi, hdr, dummy, 2, SPI_TIMEOUT_MS);
    if (rc != HAL_OK) { cs_deselect(); return LGW_SPI_ERROR; }

    uint8_t zeros[LGW_BURST_CHUNK];
    memset(zeros, 0, size);
    rc = HAL_SPI_TransmitReceive(spi, zeros, data, size, SPI_TIMEOUT_MS);
    if (rc != HAL_OK) { cs_deselect(); return LGW_SPI_ERROR; }

    cs_deselect();
    return LGW_SPI_SUCCESS;
}

/* -------------------------------------------------------------------------- */

uint16_t lgw_spi_chunk_size(void) {
    return (uint16_t)LGW_BURST_CHUNK;
}
