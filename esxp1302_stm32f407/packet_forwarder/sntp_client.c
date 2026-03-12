/*
 * sntp_client.c — Lightweight SNTP client for W5500 Ethernet
 *
 * Implements SNTPv4 (RFC 4330) over a W5500 hardware UDP socket.
 * After a successful query, a software clock is maintained using
 * HAL_GetTick() as the monotonic time base.
 *
 * Mirrors the ESP-IDF esp_sntp design from bringup/test:
 *   esp_sntp_init() (async)  →  sntp_task_start() + background FreeRTOS task
 *   LWIP settimeofday()      →  s_epoch_at_sync updated in sntp_sync()
 *   time(NULL)               →  sntp_get_utc() via _gettimeofday
 *
 * NTP server selection:
 *   1. gw_ip (eth_gw router) — always on the same LAN, ARP guaranteed;
 *      most routers run an NTP daemon (stratum 2-3)
 *   2. NTP_SERVER_IP_DEFAULT (public Alibaba Cloud) — needs working
 *      gateway route to internet
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sntp_client.h"

/* STM32 HAL (for HAL_GetTick) */
#include "stm32f4xx_hal.h"

/* WIZnet W5500 driver */
#include "wizchip_conf.h"
#include "socket.h"

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"

/* For STR() macro used in printf format */
#define STRINGIFY(x) #x
#define STR(x)       STRINGIFY(x)

/* ------------------------------------------------------------------ */
/*  Constants                                                          */
/* ------------------------------------------------------------------ */

/* Seconds between NTP epoch (1900) and Unix epoch (1970) */
#define NTP_UNIX_OFFSET     2208988800UL

/* NTP packet size */
#define NTP_PACKET_SIZE     48

/* Timeout for NTP UDP response (ms). Keep short so main loop isn't
 * blocked for too long if the NTP server is unreachable. */
#define NTP_TIMEOUT_MS      1500

/* Ephemeral local UDP port for NTP query */
#define NTP_LOCAL_PORT      10123

/* WIZnet sendto() SOCKERR_TIMEOUT = (SOCK_ERROR - 13) = -13
 * This means the W5500 ARP for the gateway timed out at the Ethernet
 * level (gateway IP unreachable / wrong subnet). Log it specifically. */
#define WIZNET_SOCKERR_TIMEOUT  (-13)

/* ------------------------------------------------------------------ */
/*  Software clock state                                               */
/* ------------------------------------------------------------------ */

static volatile time_t   s_epoch_at_sync = 0;
static volatile uint32_t s_tick_at_sync  = 0;
static volatile bool     s_synced        = false;

/* IPs passed to sntp_task_start() */
static uint8_t s_gw_ip[4]  = {0, 0, 0, 0};
static uint8_t s_eth_ip[4] = {0, 0, 0, 0};
static uint8_t s_ns_ip[4]  = {0, 0, 0, 0};
static bool    s_gw_valid  = false;
static bool    s_eth_valid = false;
static bool    s_ns_valid  = false;

/* ------------------------------------------------------------------ */
/*  Public query functions                                             */
/* ------------------------------------------------------------------ */

time_t sntp_get_utc(void)
{
    if (!s_synced)
        return 0;
    uint32_t elapsed_ms = HAL_GetTick() - s_tick_at_sync;
    return s_epoch_at_sync + (time_t)(elapsed_ms / 1000);
}

bool sntp_is_synced(void)
{
    return s_synced;
}

uint32_t sntp_age(void)
{
    if (!s_synced)
        return UINT32_MAX;
    return (HAL_GetTick() - s_tick_at_sync) / 1000;
}

/* ------------------------------------------------------------------ */
/*  Single-shot SNTP query                                             */
/* ------------------------------------------------------------------ */

int sntp_sync(const uint8_t *ntp_ip)
{
    static const uint8_t default_ip[] = NTP_SERVER_IP_DEFAULT;
    uint8_t  ntp_pkt[NTP_PACKET_SIZE];
    uint8_t  peer_ip[4];
    uint16_t peer_port;
    int32_t  ret;

    if (ntp_ip == NULL)
        ntp_ip = default_ip;

    printf("[SNTP] Querying %d.%d.%d.%d ...\r\n",
           ntp_ip[0], ntp_ip[1], ntp_ip[2], ntp_ip[3]);

    /* Open temporary UDP socket on W5500 socket 2 */
    ret = socket(NET_SOCK_NTP, Sn_MR_UDP, NTP_LOCAL_PORT, 0);
    if (ret != (int8_t)NET_SOCK_NTP) {
        printf("[SNTP] ERROR: socket open failed (%ld)\r\n", (long)ret);
        return -1;
    }

    /* --- TEMPORARY DIAGNOSTIC: dump W5500 registers --- */
    {
        uint8_t gar[4], sipr[4], subr[4];
        getGAR(gar); getSIPR(sipr); getSUBR(subr);
        printf("[SNTP] DBG W5500 regs: SIPR=%d.%d.%d.%d  "
               "SUBR=%d.%d.%d.%d  GAR=%d.%d.%d.%d  "
               "Sn_SR[%d]=0x%02X\r\n",
               sipr[0], sipr[1], sipr[2], sipr[3],
               subr[0], subr[1], subr[2], subr[3],
               gar[0],  gar[1],  gar[2],  gar[3],
               NET_SOCK_NTP, getSn_SR(NET_SOCK_NTP));
    }
    /* --- END DIAGNOSTIC --- */

    /* Build NTP request (Mode 3 = client, Version 4) */
    memset(ntp_pkt, 0, NTP_PACKET_SIZE);
    ntp_pkt[0] = 0x23;   /* LI=00, VN=100(4), Mode=011(client) */

    /* Send request */
    ret = sendto(NET_SOCK_NTP, ntp_pkt, NTP_PACKET_SIZE,
                 (uint8_t *)ntp_ip, NTP_PORT);
    if (ret < 0) {
        if (ret == WIZNET_SOCKERR_TIMEOUT) {
            printf("[SNTP] WARN: ARP timeout reaching %d.%d.%d.%d "
                   "(check eth_gw config)\r\n",
                   ntp_ip[0], ntp_ip[1], ntp_ip[2], ntp_ip[3]);
        } else {
            printf("[SNTP] ERROR: sendto failed (%ld)\r\n", (long)ret);
        }
        close(NET_SOCK_NTP);
        return -1;
    }

    /* Wait for response */
    uint32_t t_start = HAL_GetTick();
    while ((HAL_GetTick() - t_start) < NTP_TIMEOUT_MS) {
        if (getSn_RX_RSR(NET_SOCK_NTP) >= NTP_PACKET_SIZE) {
            ret = recvfrom(NET_SOCK_NTP, ntp_pkt, NTP_PACKET_SIZE,
                           peer_ip, &peer_port);
            if (ret >= NTP_PACKET_SIZE)
                goto parse_ok;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    printf("[SNTP] WARN: no response from %d.%d.%d.%d within %d ms\r\n",
           ntp_ip[0], ntp_ip[1], ntp_ip[2], ntp_ip[3], NTP_TIMEOUT_MS);
    close(NET_SOCK_NTP);
    return -1;

parse_ok:
    close(NET_SOCK_NTP);

    /* Extract Transmit Timestamp (bytes 40-43, seconds since 1900) */
    uint32_t ntp_sec = ((uint32_t)ntp_pkt[40] << 24) |
                       ((uint32_t)ntp_pkt[41] << 16) |
                       ((uint32_t)ntp_pkt[42] <<  8) |
                       ((uint32_t)ntp_pkt[43]);

    if (ntp_sec < NTP_UNIX_OFFSET) {
        printf("[SNTP] ERROR: invalid timestamp (%lu)\r\n",
               (unsigned long)ntp_sec);
        return -1;
    }

    time_t unix_time  = (time_t)(ntp_sec - NTP_UNIX_OFFSET);
    uint32_t tick_now = HAL_GetTick();

    s_epoch_at_sync = unix_time;
    s_tick_at_sync  = tick_now;
    s_synced        = true;

    struct tm tm_buf;
    struct tm *tm_info = gmtime_r(&unix_time, &tm_buf);
    char buf[32];
    if (tm_info != NULL &&
        strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S UTC", tm_info) > 0) {
        printf("[SNTP] Time synced: %s\r\n", buf);
    } else {
        printf("[SNTP] Time synced: epoch=%lu\r\n", (unsigned long)unix_time);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Background task (mirrors ESP-IDF esp_sntp async design)            */
/* ------------------------------------------------------------------ */

static void sntp_background_task(void *arg)
{
    (void)arg;
    static const uint8_t pub_ip[] = NTP_SERVER_IP_DEFAULT;

    /* Wait for W5500 packet forwarder to send the first UDP frames
     * so the ARP cache for the local gateway is populated first. */
    vTaskDelay(pdMS_TO_TICKS(5000));

    for (;;) {
        int ok = -1;

        /* Candidate 1: configured eth_gw (may be wrong if misconfigured) */
        if (s_gw_valid) {
            ok = sntp_sync(s_gw_ip);
        }

        /* Candidate 2: derived .1 gateway (eth_ip[0..2].1) */
        if (ok != 0 && s_eth_valid) {
            uint8_t d1[4] = { s_eth_ip[0], s_eth_ip[1], s_eth_ip[2], 1 };
            if (d1[0] != s_gw_ip[0] || d1[1] != s_gw_ip[1] ||
                d1[2] != s_gw_ip[2] || d1[3] != s_gw_ip[3]) {
                ok = sntp_sync(d1);
            }
        }

        /* Candidate 3: derived .254 gateway (another common router addr) */
        if (ok != 0 && s_eth_valid) {
            uint8_t d254[4] = { s_eth_ip[0], s_eth_ip[1], s_eth_ip[2], 254 };
            if (d254[0] != s_gw_ip[0] || d254[1] != s_gw_ip[1] ||
                d254[2] != s_gw_ip[2] || d254[3] != s_gw_ip[3]) {
                ok = sntp_sync(d254);
            }
        }

        /* Candidate 4: NS server (same LAN, ARP works even if no NTP) */
        if (ok != 0 && s_ns_valid) {
            ok = sntp_sync(s_ns_ip);
        }

        /* Candidate 5: public NTP (requires working gateway route) */
        if (ok != 0) {
            ok = sntp_sync(pub_ip);
        }

        if (ok == 0) {
            /* Success: sleep 1 hour then re-sync */
            vTaskDelay(pdMS_TO_TICKS((uint32_t)SNTP_RESYNC_INTERVAL * 1000));
        } else {
            /* Both failed: retry after SNTP_RETRY_INTERVAL seconds */
            printf("[SNTP] Both NTP sources failed, retry in %ds\r\n",
                   SNTP_RETRY_INTERVAL);
            vTaskDelay(pdMS_TO_TICKS((uint32_t)SNTP_RETRY_INTERVAL * 1000));
        }
    }
}

void sntp_task_start(const uint8_t *gw_ip, const uint8_t *eth_ip,
                    const uint8_t *ns_ip)
{
    if (gw_ip != NULL &&
        !(gw_ip[0] == 0 && gw_ip[1] == 0 && gw_ip[2] == 0 && gw_ip[3] == 0)) {
        memcpy(s_gw_ip, gw_ip, 4);
        s_gw_valid = true;
    } else {
        s_gw_valid = false;
    }

    if (eth_ip != NULL &&
        !(eth_ip[0] == 0 && eth_ip[1] == 0 && eth_ip[2] == 0 && eth_ip[3] == 0)) {
        memcpy(s_eth_ip, eth_ip, 4);
        s_eth_valid = true;
    } else {
        s_eth_valid = false;
    }

    if (ns_ip != NULL &&
        !(ns_ip[0] == 0 && ns_ip[1] == 0 && ns_ip[2] == 0 && ns_ip[3] == 0)) {
        memcpy(s_ns_ip, ns_ip, 4);
        s_ns_valid = true;
    } else {
        s_ns_valid = false;
    }

    printf("[SNTP] Task start. candidates: "
           "gw=%d.%d.%d.%d, derived=%d.%d.%d.1, "
           "ns=%d.%d.%d.%d, pub=120.25.115.20\r\n",
           s_gw_valid  ? s_gw_ip[0]  : 0, s_gw_valid  ? s_gw_ip[1]  : 0,
           s_gw_valid  ? s_gw_ip[2]  : 0, s_gw_valid  ? s_gw_ip[3]  : 0,
           s_eth_valid ? s_eth_ip[0] : 0, s_eth_valid ? s_eth_ip[1] : 0,
           s_eth_valid ? s_eth_ip[2] : 0,
           s_ns_valid  ? s_ns_ip[0]  : 0, s_ns_valid  ? s_ns_ip[1]  : 0,
           s_ns_valid  ? s_ns_ip[2]  : 0, s_ns_valid  ? s_ns_ip[3]  : 0);

    /* Low priority — never starves the LoRa packet forwarder */
    xTaskCreate(sntp_background_task, "sntp", 512, NULL,
                tskIDLE_PRIORITY + 1, NULL);
}
