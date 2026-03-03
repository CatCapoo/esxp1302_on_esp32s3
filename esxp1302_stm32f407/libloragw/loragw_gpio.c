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

#include "loragw_gpio.h"
#include "loragw_aux.h"


void lgw_reset(void)
{
    /* GPIO is already configured by CubeMX MX_GPIO_Init().
       SX1302 RESET is active HIGH. */

    /* Assert RESET high */
    HAL_GPIO_WritePin(SX1302_RESET_PORT, SX1302_RESET_PIN_NUM, GPIO_PIN_SET);
    wait_ms(100);

    /* Release RESET low, wait 500ms for chip to fully boot */
    HAL_GPIO_WritePin(SX1302_RESET_PORT, SX1302_RESET_PIN_NUM, GPIO_PIN_RESET);
    wait_ms(500);
}
