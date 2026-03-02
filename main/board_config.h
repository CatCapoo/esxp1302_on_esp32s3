/*
 * board_config.h
 *
 * Single place for all hardware GPIO / peripheral pin assignments.
 * Modify this file to adapt to your specific board wiring.
 *
 * All values here are picked up by the #ifndef guards in each
 * subsystem header (loragw_gpio.h, loragw_spi.h, loragw_i2c.h,
 * loragw_gps.h, led_indication.h) and in lora_pkt_fwd.c.
 */

#pragma once

#include "driver/gpio.h"   /* for GPIO_NUM_NC and gpio_num_t */

/* ---------------------------------------------------------------
 * SX1302 SPI bus
 * --------------------------------------------------------------- */
#define SX1302_SPI_HOST     SPI2_HOST
#define PIN_NUM_MISO        13
#define PIN_NUM_MOSI        11
#define PIN_NUM_CLK         12
#define PIN_NUM_CS          14

/* ---------------------------------------------------------------
 * SX1302 control GPIO
 * --------------------------------------------------------------- */
#define SX1302_RESET_PIN        10
#define SX1302_POWER_EN_PIN    -1   /* not connected; integer literal needed for #if */

/* ---------------------------------------------------------------
 * I2C (OLED / sensors)
 * --------------------------------------------------------------- */
#define I2C_MASTER_SDA_IO   4
#define I2C_MASTER_SCL_IO   5
#define I2C_MASTER_NUM      0

/* ---------------------------------------------------------------
 * GPS
 * GPS_ENABLE      1 = GPS hardware present and enabled
 *                 0 = disable all GPS code (no threads, no UART)
 * GPS_LOG_VERBOSE 0 = silent
 *                 1 = key events only (sync acquired, fix status, warnings)
 *                 2 = full NMEA dump every cycle (wiring / baud-rate debug)
 * --------------------------------------------------------------- */
#define GPS_ENABLE       1
#define GPS_LOG_VERBOSE  0

#define GPS_UART_TXD  (GPIO_NUM_19)   /* ESP TX  -> GPS module RX */
#define GPS_UART_RXD  (GPIO_NUM_20)   /* ESP RX  <- GPS module TX */

/* ---------------------------------------------------------------
 * LEDs
 * --------------------------------------------------------------- */
#define BLINK_GPIO        1   /* heartbeat LED */
#define LED_BLUE_GPIO     33  /* backhaul indicator  (NC if not fitted) */
#define LED_GREEN_GPIO    7   /* uplink indicator    (NC if not fitted) */
#define LED_RED_GPIO      27  /* downlink indicator  (NC if not fitted) */

/* ---------------------------------------------------------------
 * User buttons
 * --------------------------------------------------------------- */
#define USER_BUTTON_1     0   /* IO0: boot/config button */
#define USER_BUTTON_2     6   /* IO6: reserved, no hardware connected */
