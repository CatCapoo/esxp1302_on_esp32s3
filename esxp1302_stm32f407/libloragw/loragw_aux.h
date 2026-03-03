/*
 * loragw_aux.h  –  Auxiliary functions (timing, macros)
 *
 * Ported from ESP-IDF to STM32 HAL + CMSIS-RTOS2.
 */

#ifndef _LORAGW_AUX_H
#define _LORAGW_AUX_H

#include <stdint.h>
#include <stdbool.h>

#include "config.h"

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

#define DEBUG_PERF 0

/* -------------------------------------------------------------------------- */
/* --- PUBLIC MACROS -------------------------------------------------------- */

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#define TAKE_N_BITS_FROM(b, p, n) (((b) >> (p)) & ((1 << (n)) - 1))

/* -------------------------------------------------------------------------- */
/* --- PERFORMANCE MEASUREMENT STUBS --------------------------------------- */
/* The original code uses gettimeofday() / struct timeval for profiling.     */
/* On STM32 we stub these out when DEBUG_PERF == 0.                          */

#include <sys/time.h>   /* struct timeval — provided by newlib */

#if DEBUG_PERF
void _meas_time_start(struct timeval *tm);
void _meas_time_stop(int debug_level, struct timeval start_time, const char *str);
#else
#define _meas_time_start(x)          ((void)0)
#define _meas_time_stop(n, x, y)     ((void)0)
#endif

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */

void wait_ms(unsigned long t);
void wait_us(unsigned long t);

/**
 * @brief Get current microsecond timestamp from TIM2
 */
uint32_t timestamp_us(void);

/**
 * @brief Compute LoRa packet time on air in microseconds
 */
unsigned int lora_packet_time_on_air(const uint8_t bw, const uint8_t sf,
                                     const uint8_t cr,
                                     const uint16_t n_symbol_preamble,
                                     const bool no_header, const bool no_crc,
                                     const uint8_t size,
                                     double *nb_symbols,
                                     unsigned int *nb_symbols_payload,
                                     uint16_t *t_symbol_us);

/**
 * @brief Get the current time for later timeout check
 * @param start contains the current time (struct timeval)
 */
void timeout_start(struct timeval *start);

/**
 * @brief Check if the given timeout in milliseconds has elapsed
 * @param start  start time from timeout_start()
 * @param timeout_ms  timeout threshold in milliseconds
 * @return -1 if the timeout has elapsed, 0 otherwise
 */
int timeout_check(struct timeval start, unsigned int timeout_ms);

#endif /* _LORAGW_AUX_H */
