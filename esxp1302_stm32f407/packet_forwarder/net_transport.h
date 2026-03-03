/*
 * net_transport.h - W5500 UDP network abstraction for packet forwarder
 *
 * Replaces BSD socket API (used on ESP32 with lwip) with W5500 hardware
 * socket operations via WIZnet ioLibrary.
 *
 * W5500 has 8 hardware sockets (0-7). This layer allocates two:
 *   - Socket 0: upstream traffic   (PUSH_DATA / PUSH_ACK)
 *   - Socket 1: downstream traffic (PULL_DATA / PULL_RESP / PULL_ACK)
 */

#ifndef _NET_TRANSPORT_H
#define _NET_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>

/* Hardware socket assignments */
#define NET_SOCK_UP     0   /* upstream socket number   */
#define NET_SOCK_DOWN   1   /* downstream socket number */

/* Return codes aligned with WIZnet conventions */
#define NET_OK           0
#define NET_ERR         -1
#define NET_TIMEOUT     -2

/**
 * @brief  Initialise the W5500 chip and configure network parameters
 *         from gateway_config (IP/GW/SN).
 *
 * Must be called once before any other net_* function.
 * Internally calls W5500_RESET(), wizchip register callbacks, and
 * NetworkParameterConfiguration().
 *
 * @return NET_OK on success, NET_ERR on failure
 */
int net_init(void);

/**
 * @brief  Open a UDP socket on the given hardware socket number
 *         and bind it to a local port.
 *
 * @param  sn         Hardware socket number (0-7, use NET_SOCK_UP/DOWN)
 * @param  local_port Local UDP port to bind
 * @return NET_OK on success, NET_ERR on failure
 */
int net_udp_open(uint8_t sn, uint16_t local_port);

/**
 * @brief  Close a previously opened socket.
 *
 * @param  sn  Hardware socket number
 */
void net_udp_close(uint8_t sn);

/**
 * @brief  Send a UDP datagram to the specified destination.
 *
 * @param  sn        Hardware socket number
 * @param  buf       Pointer to data buffer
 * @param  len       Number of bytes to send
 * @param  dest_ip   Destination IP address (4 bytes, network order)
 * @param  dest_port Destination port number (host order)
 * @return Number of bytes sent on success, NET_ERR on failure
 */
int net_udp_send(uint8_t sn, uint8_t *buf, uint16_t len,
                 uint8_t *dest_ip, uint16_t dest_port);

/**
 * @brief  Receive a UDP datagram with timeout.
 *
 * Polls getSn_RX_RSR() with vTaskDelay() until data arrives or timeout
 * expires.
 *
 * @param  sn         Hardware socket number
 * @param  buf        Receive buffer
 * @param  max_len    Maximum bytes to read
 * @param  timeout_ms Timeout in milliseconds (0 = non-blocking poll)
 * @return Number of bytes received (>0), 0 if timeout, NET_ERR on error
 */
int net_udp_recv(uint8_t sn, uint8_t *buf, uint16_t max_len,
                 uint32_t timeout_ms);

/**
 * @brief  Non-blocking drain: read and discard any pending data.
 *
 * Used to flush stale ACKs before sending a new datagram.
 *
 * @param  sn  Hardware socket number
 * @return Number of bytes drained
 */
int net_udp_drain(uint8_t sn);

/**
 * @brief  Set the default destination address for a socket.
 *
 * Since W5500 UDP does not have BSD-style connect(), this stores
 * the server address so net_udp_send() can use it without the caller
 * passing dest_ip/dest_port every time.
 *
 * @param  sn        Hardware socket number
 * @param  dest_ip   4-byte destination IP
 * @param  dest_port Destination port
 */
void net_set_dest(uint8_t sn, const uint8_t *dest_ip, uint16_t dest_port);

/**
 * @brief  Send a UDP datagram to the pre-configured destination.
 *
 * Uses the address set by net_set_dest().
 *
 * @param  sn   Hardware socket number
 * @param  buf  Pointer to data
 * @param  len  Number of bytes
 * @return Number of bytes sent, or NET_ERR
 */
int net_udp_send_default(uint8_t sn, uint8_t *buf, uint16_t len);

#endif /* _NET_TRANSPORT_H */
