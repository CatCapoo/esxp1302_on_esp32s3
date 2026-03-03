/*
 * test_loragw_i2c_oled.c  –  SSD1306 OLED test on STM32F407
 *
 * Tests:
 *   1. Init SSD1306 via I2C2
 *   2. Fill screen white, pause, clear screen
 *   3. Draw text on each row
 *   4. Animate a bouncing pixel
 */

#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"
#include "loragw_i2c.h"
#include "loragw_oled.h"
#include "loragw_aux.h"

void test_loragw_i2c_oled(void)
{
    int err;
    int cycle = 0;
    uint8_t px = 0, py = 0;
    int8_t  dx = 1, dy = 1;
    char    buf[32];

    printf("Beginning of test for OLED (SSD1306)\n");

    lgw_i2c_open();

    /* -----------------------------------------------------------------------
     * Init OLED
     * --------------------------------------------------------------------- */
    err = oled_init();
    if (err != 0) {
        printf("ERROR: oled_init failed\n");
        goto idle;
    }

    /* -----------------------------------------------------------------------
     * Test 1: fill white, then clear
     * --------------------------------------------------------------------- */
    printf("TEST#1: fill + clear\n");
    oled_fill();
    wait_ms(800);
    oled_clear();
    wait_ms(400);

    /* -----------------------------------------------------------------------
     * Test 2: draw static text on all 8 rows
     * --------------------------------------------------------------------- */
    printf("TEST#2: static text\n");
    oled_draw_string(0, 0, "SSD1306 OLED TEST");
    oled_draw_string(0, 1, "STM32F407  I2C2");
    oled_draw_string(0, 2, "PF0-SDA  PF1-SCL");
    oled_draw_string(0, 3, "100kHz  0x3C");
    oled_draw_string(0, 4, "libloragw v1.0");
    oled_draw_string(0, 5, "LoRa GW bringup");
    oled_draw_string(0, 6, "0123456789ABCDEF");
    oled_draw_string(0, 7, "abcdefghijklmnop");
    oled_refresh();
    wait_ms(3000);

    /* -----------------------------------------------------------------------
     * Test 3: bouncing pixel animation with cycle counter
     * --------------------------------------------------------------------- */
    printf("TEST#3: bouncing pixel animation (100 cycles)\n");
    oled_clear();

    while (cycle < 100) {
        oled_clear();

        /* Update position */
        px = (uint8_t)(px + dx);
        py = (uint8_t)(py + dy);
        if (px == 0 || px == OLED_WIDTH - 1)  dx = (int8_t)-dx;
        if (py == 0 || py == OLED_HEIGHT - 1) dy = (int8_t)-dy;

        /* Draw 3×3 square around pixel */
        for (int ox = -1; ox <= 1; ox++) {
            for (int oy = -1; oy <= 1; oy++) {
                int nx = (int)px + ox;
                int ny = (int)py + oy;
                if (nx >= 0 && nx < OLED_WIDTH && ny >= 0 && ny < OLED_HEIGHT) {
                    oled_set_pixel((uint8_t)nx, (uint8_t)ny, 1);
                }
            }
        }

        /* Draw cycle counter in bottom-right corner */
        snprintf(buf, sizeof buf, "C:%03d", cycle);
        oled_draw_string(15, 7, buf);

        oled_refresh();
        wait_ms(30);
        cycle++;
    }

    oled_clear();
    oled_draw_string(0, 3, "  TEST COMPLETE ");
    oled_refresh();
    printf("End of test for OLED (SSD1306)\n");

idle:
    lgw_i2c_close();
    for (;;) {
        printf("test_oled: idle\n");
        osDelay(10000);
    }
}
