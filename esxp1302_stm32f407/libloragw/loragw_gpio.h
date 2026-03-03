/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2019 Semtech

Description:
    Functions to reset LoRa concentrator from GPIO Pins.

License: Revised BSD License, see LICENSE.TXT file include in the project
*/

#ifndef _LORAGW_GPIO_H
#define _LORAGW_GPIO_H

#include "board_config.h"

/* Reset the SX1302 gateway via RESET GPIO pin */
void lgw_reset(void);

#endif
