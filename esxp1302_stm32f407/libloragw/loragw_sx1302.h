/*
 * loragw_sx1302.h  –  Minimal SX1302 functions for STM32 bringup/test
 */

#ifndef _LORAGW_SX1302_H
#define _LORAGW_SX1302_H

#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "loragw_hal.h"

/* -------------------------------------------------------------------------- */
/* --- PUBLIC MACROS -------------------------------------------------------- */

#define REG_SELECT(rf_chain, a, b) ((rf_chain == 0) ? a : b)

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */

int sx1302_radio_clock_select(uint8_t rf_chain);
int sx1302_radio_reset(uint8_t rf_chain, lgw_radio_type_t type);
int sx1302_radio_set_mode(uint8_t rf_chain, lgw_radio_type_t type);

#endif
