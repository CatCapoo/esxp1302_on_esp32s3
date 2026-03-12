/*
 * board_config.h  –  STM32F407 hardware pin assignments
 *
 * Matches CubeMX .ioc configuration:
 *   SPI3  PC10/PC11/PC12   (SCK/MISO/MOSI)
 *   NSS   PD2  = GPIO_PIN_2  (software GPIO, active-low)
 *   RESET PA8  = GPIO_PIN_8  (GPIO output, active-high pulse)
 */

#pragma once

#include "main.h"          /* CubeMX-generated pin defines */
#include "spi.h"           /* hspi3 handle */
#include "i2c.h"           /* hi2c2 handle */

/* ---------------------------------------------------------------
 * SX1302 SPI bus  (directly uses CubeMX SPI3 + software CS)
 * --------------------------------------------------------------- */
#define SX1302_SPI_HANDLE     (&hspi3)

/* CS / NSS  –  active low, directly from CubeMX defines */
#define SX1302_NSS_PORT       SX1302_NSS_GPIO_Port   /* GPIOD */
#define SX1302_NSS_PIN        SX1302_NSS_Pin          /* GPIO_PIN_2 (PD2) */

/* ---------------------------------------------------------------
 * SX1302 control GPIO
 * --------------------------------------------------------------- */
#define SX1302_RESET_PORT     SX1302_RESET_GPIO_Port  /* GPIOA */
#define SX1302_RESET_PIN_NUM  SX1302_RESET_Pin        /* GPIO_PIN_8 (PA8) */

/* ---------------------------------------------------------------
 * LEDs  (directly from CubeMX defines)
 * --------------------------------------------------------------- */
/* LED0 = PF9, LED1 = PF10  already defined in main.h */

/* ---------------------------------------------------------------
 * Keys  (already defined in main.h)
 * --------------------------------------------------------------- */
/* KEY0 = PE4 (pull-up), KEY_UP = PA0 (pull-down) */

/* ---------------------------------------------------------------
 * I2C bus  (CubeMX I2C2  PF0-SDA / PF1-SCL  100 kHz)
 * --------------------------------------------------------------- */
#define I2C_HANDLE        (&hi2c2)
#define I2C_TIMEOUT_MS    100
