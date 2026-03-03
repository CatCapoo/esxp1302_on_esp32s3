/*
 * sx1261_usb.c  –  Stub for STM32 (USB not supported)
 *
 * Provides empty implementations for USB SX1261 functions
 * that are referenced by sx1261_com.c but never actually called
 * on the SPI-only STM32 platform.
 */

#include <stdint.h>
#include <stdio.h>

int sx1261_usb_w(void *com_target, uint8_t op_code,
                 uint8_t *data, uint16_t size)
{
    (void)com_target; (void)op_code; (void)data; (void)size;
    printf("ERROR: sx1261_usb_w not supported on STM32\n");
    return -1;
}

int sx1261_usb_r(void *com_target, uint8_t op_code,
                 uint8_t *data, uint16_t size)
{
    (void)com_target; (void)op_code; (void)data; (void)size;
    printf("ERROR: sx1261_usb_r not supported on STM32\n");
    return -1;
}

int sx1261_usb_set_write_mode(uint8_t write_mode)
{
    (void)write_mode;
    return -1;
}

int sx1261_usb_rmw(void *com_target)
{
    (void)com_target;
    return -1;
}

int sx1261_usb_flush(void *com_target)
{
    (void)com_target;
    return -1;
}
