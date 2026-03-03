/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2019 Semtech

Description:
    LoRa concentrator debug functions

License: Revised BSD License, see LICENSE.TXT file include in the project
*/


/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <stdio.h>      /* printf */
#include <string.h>     /* memcmp */

#include "loragw_aux.h"
#include "loragw_debug.h"

#include "tinymt32.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

static tinymt32_t tinymt;

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS DEFINITION ------------------------------------------ */

void dbg_init_random(void) {
    tinymt.mat1 = 0x8f7011ee;
    tinymt.mat2 = 0xfc78ff1f;
    tinymt.tmat = 0x3793fdff;
}

/* Stubs for functions that need FILE* — not available on bare-metal STM32 */

void dbg_log_buffer_to_file(FILE * file, uint8_t * buffer, uint16_t size) {
    (void)file;
    int i;
    printf("Buffer (%u bytes): ", size);
    for (i = 0; i < size; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

void dbg_log_payload_diff_to_file(FILE * file, uint8_t * buffer1, uint8_t * buffer2, uint16_t size) {
    (void)file;
    (void)buffer1;
    (void)buffer2;
    (void)size;
}

void dbg_generate_random_payload(unsigned int pkt_cnt, uint8_t * buffer_expected, uint8_t size) {
    int k;
    tinymt32_init(&tinymt, (int)pkt_cnt);
    buffer_expected[4] = (uint8_t)(pkt_cnt >> 24);
    buffer_expected[5] = (uint8_t)(pkt_cnt >> 16);
    buffer_expected[6] = (uint8_t)(pkt_cnt >> 8);
    buffer_expected[7] = (uint8_t)(pkt_cnt >> 0);
    tinymt32_generate_uint32(&tinymt);
    for (k = 8; k < (int)size; k++) {
        buffer_expected[k] = (uint8_t)tinymt32_generate_uint32(&tinymt);
    }
}
