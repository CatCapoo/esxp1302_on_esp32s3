/*
 * loragw_oled.h  –  SSD1306 128×64 OLED driver (I2C)
 *
 * All drawing operates on a local framebuffer; call oled_refresh()
 * to push the buffer to the display.
 *
 * Character rendering uses a built-in 6×8 font (5 px wide + 1 px gap).
 * - Screen capacity: 21 chars × 8 rows
 * - col: 0 – 127 (pixel column)
 * - row: 0 –   7 (page / character row)
 */

#pragma once

#include <stdint.h>

/* SSD1306 I2C address (SA0 pin: 0x3C if SA0=GND, 0x3D if SA0=VCC) */
#define OLED_I2C_ADDR   0x3C

#define OLED_WIDTH      128
#define OLED_HEIGHT      64
#define OLED_PAGES       (OLED_HEIGHT / 8)   /* 8 */
#define OLED_CHAR_W       6   /* 5 px data + 1 px gap */
#define OLED_COLS_PER_ROW (OLED_WIDTH / OLED_CHAR_W)  /* 21 */

/**
 * @brief Initialise SSD1306 (128×64, charge-pump on, horizontal addressing).
 * @return 0 on success, -1 on I2C error.
 */
int oled_init(void);

/** @brief Fill framebuffer with 0 and refresh display. */
void oled_clear(void);

/** @brief Fill framebuffer with 0xFF and refresh display. */
void oled_fill(void);

/** @brief Push local framebuffer to the display. */
void oled_refresh(void);

/**
 * @brief Set or clear one pixel in the framebuffer (does NOT refresh).
 * @param x   column  0–127
 * @param y   row     0–63
 * @param val 0 = off, non-zero = on
 */
void oled_set_pixel(uint8_t x, uint8_t y, uint8_t val);

/**
 * @brief Draw one ASCII character at a character position (does NOT refresh).
 * @param col  character column 0–20
 * @param row  character row    0–7
 * @param c    ASCII character
 */
void oled_draw_char(uint8_t col, uint8_t row, char c);

/**
 * @brief Draw a null-terminated string; wraps at right edge (does NOT refresh).
 * @param col  starting character column
 * @param row  starting character row
 * @param str  string to draw
 */
void oled_draw_string(uint8_t col, uint8_t row, const char *str);
