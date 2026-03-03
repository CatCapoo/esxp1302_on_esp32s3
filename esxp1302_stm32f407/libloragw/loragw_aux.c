/*
 * loragw_aux.c  –  Auxiliary timing functions for STM32
 *
 * Uses CMSIS-RTOS2 osDelay for millisecond waits
 * and TIM2 free-running counter for microsecond timing.
 */

#include <stdio.h>

#include "cmsis_os.h"
#include "tim.h"

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
