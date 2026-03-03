/*
 * sx1261_usb.h  –  Stub for STM32 (USB not supported)
 *
 * The SX1261 is only accessed via SPI on this platform.
 * This header provides empty declarations to satisfy sx1261_com.c includes.
 */

#ifndef _SX1261_USB_H
#define _SX1261_USB_H

#include <stdint.h>

/* Stub functions – never called on SPI-only platform */
int sx1261_usb_w(void *com_target, uint8_t op_code,
                 uint8_t *data, uint16_t size);
int sx1261_usb_r(void *com_target, uint8_t op_code,
                 uint8_t *data, uint16_t size);
int sx1261_usb_set_write_mode(uint8_t write_mode);
int sx1261_usb_rmw(void *com_target);
int sx1261_usb_flush(void *com_target);

#endif /* _SX1261_USB_H */
