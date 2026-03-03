/*
 * net_transport.c - W5500 UDP network abstraction for packet forwarder
 *
 * Wraps WIZnet ioLibrary socket API to provide a simpler interface for
 * the packet forwarder's UDP send/recv operations.
 */

#include <stdio.h>
#include <string.h>

#include "net_transport.h"
#include "gateway_config.h"

/* W5500 driver */
#include "wizchip_conf.h"
#include "socket.h"
#include "w5500.h"

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"

/* External W5500 network info struct (defined in wizchip_conf.c) */
extern wiz_NetInfo gWIZNETINFO;

/* -------------------------------------------------------------------------- */
/*  Per-socket destination cache (replaces BSD connect() for UDP)             */
/* -------------------------------------------------------------------------- */
typedef struct {
    uint8_t  dest_ip[4];
    uint16_t dest_port;
    bool     valid;
} net_dest_t;

static net_dest_t sock_dest[8]; /* W5500 supports sockets 0-7 */

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

int net_init(void)
{
    gateway_config_t *cfg = config_get();

    /* Update gWIZNETINFO from gateway_config before calling W5500_ChipInit() */
    memcpy(gWIZNETINFO.ip, cfg->eth_ip, 4);
    memcpy(gWIZNETINFO.gw, cfg->eth_gw, 4);
    memcpy(gWIZNETINFO.sn, cfg->eth_sn, 4);

    /* Call the platform init (reset, SPI callbacks, set network params) */
    W5500_ChipInit();

    printf("[NET] W5500 initialized: %d.%d.%d.%d\r\n",
           gWIZNETINFO.ip[0], gWIZNETINFO.ip[1],
           gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);

    memset(sock_dest, 0, sizeof(sock_dest));

    return NET_OK;
}

int net_udp_open(uint8_t sn, uint16_t local_port)
{
    int8_t ret = socket(sn, Sn_MR_UDP, local_port, 0);
    if (ret != (int8_t)sn) {
        printf("[NET] ERROR: socket(%d) returned %d\r\n", sn, ret);
        return NET_ERR;
    }
    printf("[NET] UDP socket %d opened on port %d\r\n", sn, local_port);
    return NET_OK;
}

void net_udp_close(uint8_t sn)
{
    close(sn);
    sock_dest[sn].valid = false;
}

void net_set_dest(uint8_t sn, const uint8_t *dest_ip, uint16_t dest_port)
{
    memcpy(sock_dest[sn].dest_ip, dest_ip, 4);
    sock_dest[sn].dest_port = dest_port;
    sock_dest[sn].valid = true;
}

int net_udp_send(uint8_t sn, uint8_t *buf, uint16_t len,
                 uint8_t *dest_ip, uint16_t dest_port)
{
    int32_t ret = sendto(sn, buf, len, dest_ip, dest_port);
    if (ret < 0) {
        printf("[NET] ERROR: sendto(%d) returned %ld\r\n", sn, (long)ret);
        return NET_ERR;
    }
    return (int)ret;
}

int net_udp_send_default(uint8_t sn, uint8_t *buf, uint16_t len)
{
    if (!sock_dest[sn].valid) {
        printf("[NET] ERROR: no destination configured for socket %d\r\n", sn);
        return NET_ERR;
    }
    return net_udp_send(sn, buf, len,
                        sock_dest[sn].dest_ip, sock_dest[sn].dest_port);
}

int net_udp_recv(uint8_t sn, uint8_t *buf, uint16_t max_len,
                 uint32_t timeout_ms)
{
    uint8_t  peer_ip[4];
    uint16_t peer_port;
    int32_t  ret;
    uint32_t elapsed = 0;
    const uint32_t poll_interval = 5; /* ms between polls */

    while (1) {
        uint16_t rsr = getSn_RX_RSR(sn);
        if (rsr > 0) {
            ret = recvfrom(sn, buf, max_len, peer_ip, &peer_port);
            if (ret > 0) {
                return (int)ret;
            }
            /* recvfrom error - treat as no data */
        }

        /* Non-blocking mode: return immediately */
        if (timeout_ms == 0) {
            return 0;
        }

        /* Check timeout */
        if (elapsed >= timeout_ms) {
            return 0; /* timeout, no data */
        }

        /* Wait and try again */
        vTaskDelay(pdMS_TO_TICKS(poll_interval));
        elapsed += poll_interval;
    }
}

int net_udp_drain(uint8_t sn)
{
    uint8_t  tmp[64];
    uint8_t  peer_ip[4];
    uint16_t peer_port;
    int      total = 0;

    while (getSn_RX_RSR(sn) > 0) {
        int32_t n = recvfrom(sn, tmp, sizeof(tmp), peer_ip, &peer_port);
        if (n > 0) {
            total += n;
        } else {
            break;
        }
    }
    return total;
}
