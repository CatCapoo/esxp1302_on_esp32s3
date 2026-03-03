/*
 * loragw_aux.c  –  Auxiliary timing functions for STM32
 *
 * Uses CMSIS-RTOS2 osDelay for millisecond waits
 * and TIM2 free-running counter for microsecond timing.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <sys/time.h>

#include "cmsis_os.h"
#include "tim.h"

#include "loragw_hal.h"
#include "loragw_aux.h"

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#if DEBUG_AUX == 1
    #define DEBUG_MSG(str)                printf(str)
    #define DEBUG_PRINTF(fmt, ...)        printf("%s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
    #define DEBUG_MSG(str)
    #define DEBUG_PRINTF(fmt, ...)
#endif

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */

void wait_ms(unsigned long delay_ms) {
    if (delay_ms == 0) return;
    osDelay(delay_ms);
}

void wait_us(unsigned long delay_us) {
    /* For very short delays, use a busy-wait on TIM2 (1 µs resolution) */
    uint32_t start = __HAL_TIM_GET_COUNTER(&htim2);
    while ((__HAL_TIM_GET_COUNTER(&htim2) - start) < delay_us) {
        /* spin */
    }
}

uint32_t timestamp_us(void) {
    return __HAL_TIM_GET_COUNTER(&htim2);
}

/* --- LoRa packet time on air (ported from ESP32 loragw_aux.c) --- */

unsigned int lora_packet_time_on_air(const uint8_t bw, const uint8_t sf,
                                     const uint8_t cr,
                                     const uint16_t n_symbol_preamble,
                                     const bool no_header, const bool no_crc,
                                     const uint8_t size,
                                     double *out_nb_symbols,
                                     unsigned int *out_nb_symbols_payload,
                                     uint16_t *out_t_symbol_us) {
    uint8_t H, DE, n_bit_crc;
    uint8_t bw_pow;
    uint16_t t_symbol_us;
    double n_symbol;
    unsigned int toa_us, n_symbol_payload;

    if (IS_LORA_DR(sf) == false) {
        printf("ERROR: wrong datarate - %s\n", __FUNCTION__);
        return 0;
    }
    if (IS_LORA_BW(bw) == false) {
        printf("ERROR: wrong bandwidth - %s\n", __FUNCTION__);
        return 0;
    }
    if (IS_LORA_CR(cr) == false) {
        printf("ERROR: wrong coding rate - %s\n", __FUNCTION__);
        return 0;
    }

    switch (bw) {
        case BW_125KHZ: bw_pow = 1; break;
        case BW_250KHZ: bw_pow = 2; break;
        case BW_500KHZ: bw_pow = 4; break;
        default:
            printf("ERROR: unsupported bandwidth 0x%02X (%s)\n", bw, __FUNCTION__);
            return 0;
    }

    t_symbol_us = (1 << sf) * 8 / bw_pow;

    H = (no_header == false) ? 1 : 0;
    DE = (sf >= 11) ? 1 : 0;
    n_bit_crc = (no_crc == false) ? 16 : 0;

    n_symbol_payload = (unsigned int)ceil(
        MAX((double)(8 * size + n_bit_crc - 4 * sf + ((sf >= 7) ? 8 : 0) + 20 * H), 0.0) /
        (double)(4 * (sf - 2 * DE))
    ) * (cr + 4);

    n_symbol = (double)n_symbol_preamble + ((sf >= 7) ? 4.25 : 6.25) + 8.0 + (double)n_symbol_payload;
    toa_us = (unsigned int)((double)n_symbol * (double)t_symbol_us);

    DEBUG_PRINTF("INFO: LoRa packet ToA: %u us (n_symbol:%f, t_symbol_us:%u)\n", toa_us, n_symbol, t_symbol_us);

    if (out_nb_symbols != NULL) *out_nb_symbols = n_symbol;
    if (out_nb_symbols_payload != NULL) *out_nb_symbols_payload = n_symbol_payload;
    if (out_t_symbol_us != NULL) *out_t_symbol_us = t_symbol_us;

    return toa_us;
}

/* --- Timeout helpers (ported from ESP32 loragw_aux.c) --- */

void timeout_start(struct timeval *start) {
    /* Use HAL_GetTick (ms) mapped to struct timeval */
    uint32_t ms = HAL_GetTick();
    start->tv_sec  = (long)(ms / 1000);
    start->tv_usec = (long)((ms % 1000) * 1000);
}

int timeout_check(struct timeval start, unsigned int timeout_ms) {
    uint32_t now_ms = HAL_GetTick();
    uint32_t start_ms = (uint32_t)(start.tv_sec * 1000 + start.tv_usec / 1000);
    if ((now_ms - start_ms) >= timeout_ms) {
        return -1;  /* timeout */
    }
    return 0;
}
