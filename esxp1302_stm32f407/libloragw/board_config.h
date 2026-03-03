/*
 * board_config.h  –  STM32F407 hardware pin assignments
 *
 * Matches CubeMX .ioc configuration:
 *   SPI3  PC10/PC11/PC12   (SCK/MISO/MOSI)
 *   NSS   PD2              (software GPIO)
 *   RESET PA8              (GPIO output)
 */

#pragma once

#include "main.h"          /* CubeMX-generated pin defines */
#include "spi.h"           /* hspi3 handle */

/* ---------------------------------------------------------------
 * SX1302 SPI bus  (directly uses CubeMX SPI3 + software CS)
 * --------------------------------------------------------------- */
#define SX1302_SPI_HANDLE     (&hspi3)

/* CS / NSS  –  active low, directly from CubeMX defines */
#define SX1302_NSS_PORT       SX1302_NSS_GPIO_Port   /* GPIOB */
#define SX1302_NSS_PIN        SX1302_NSS_Pin          /* GPIO_PIN_12 */

/* ---------------------------------------------------------------
 * SX1302 control GPIO
 * --------------------------------------------------------------- */
#define SX1302_RESET_PORT     SX1302_RESET_GPIO_Port  /* GPIOD */
#define SX1302_RESET_PIN_NUM  SX1302_RESET_Pin        /* GPIO_PIN_9 */

/* ---------------------------------------------------------------
 * LEDs  (directly from CubeMX defines)
 * --------------------------------------------------------------- */
/* LED0 = PF9, LED1 = PF10  already defined in main.h */

/* ---------------------------------------------------------------
 * Keys  (already defined in main.h)
 * --------------------------------------------------------------- */
/* KEY0 = PE4 (pull-up), KEY_UP = PA0 (pull-down) */
