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
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */

void wait_ms(unsigned long t);
void wait_us(unsigned long t);

/**
 * @brief Get current microsecond timestamp from TIM2
 */
uint32_t timestamp_us(void);

#endif /* _LORAGW_AUX_H */
