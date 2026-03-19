/*
 * board_config.h  –  STM32F407 hardware pin assignments
 *
 * Matches CubeMX .ioc configuration:
 *   SPI1  PA5/PA6/PA7   (SCK/MISO/MOSI)  –  SX1302
 *   NSS   PA4  = GPIO_PIN_4  (software GPIO, active-low)
 *   RESET PC4  = GPIO_PIN_4  (GPIO output, active-high pulse)
 */

#pragma once

#include "main.h"          /* CubeMX-generated pin defines */
#include "spi.h"           /* hspi1 handle */
#include "i2c.h"           /* hi2c1 handle */

/* ---------------------------------------------------------------
 * SX1302 SPI bus  (directly uses CubeMX SPI1 + software CS)
 * --------------------------------------------------------------- */
#define SX1302_SPI_HANDLE     (&hspi1)

/* CS / NSS  –  active low, directly from CubeMX defines */
#define SX1302_NSS_PORT       SX1302_NSS_GPIO_Port   /* GPIOA */
#define SX1302_NSS_PIN        SX1302_NSS_Pin          /* GPIO_PIN_4 (PA4) */

/* ---------------------------------------------------------------
 * SX1302 control GPIO
 * --------------------------------------------------------------- */
#define SX1302_RESET_PORT     SX1302_RESET_GPIO_Port  /* GPIOC */
#define SX1302_RESET_PIN_NUM  SX1302_RESET_Pin        /* GPIO_PIN_4 (PC4) */

/* ---------------------------------------------------------------
 * LEDs  (directly from CubeMX defines)
 * --------------------------------------------------------------- */
/* LED0 = PF9, LED1 = PF10  already defined in main.h */

/* ---------------------------------------------------------------
 * Keys  (already defined in main.h)
 * --------------------------------------------------------------- */
/* KEY0 = PE4 (pull-up), KEY_UP = PA0 (pull-down) */

/* ---------------------------------------------------------------
 * I2C bus  (CubeMX I2C1  PB6-SCL / PB7-SDA  100 kHz)
 * --------------------------------------------------------------- */
#define I2C_HANDLE        (&hi2c1)
#define I2C_TIMEOUT_MS    100
