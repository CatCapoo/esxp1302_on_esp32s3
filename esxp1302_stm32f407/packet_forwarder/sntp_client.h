/*
 * sntp_client.h — Lightweight SNTP client for W5500 Ethernet
 *
 * Replaces ESP-IDF's esp_sntp on the STM32F407 + W5500 platform.
 * Mirrors the original design: SNTP runs asynchronously in a background
 * FreeRTOS task so it never blocks the packet forwarder main loop.
 *
 * After a successful sync, time(NULL) / gettimeofday() return real UTC.
 *
 * Usage:
 *   1. Call sntp_task_start(gw_ip) once after net_init() succeeds
 *   2. time(NULL) / gmtime() / strftime() work as usual from any task
 *
 * NTP server selection (tried in order):
 *   1. gw_ip     (eth_gw router — if correctly configured)
 *   2. eth_ip[0..2].1 (derived local gateway from eth_ip, handles wrong eth_gw)
 *   3. NTP_SERVER_IP_DEFAULT (Alibaba Cloud, requires working gateway)
 */

#ifndef SNTP_CLIENT_H
#define SNTP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* Default public NTP server IP (Alibaba Cloud NTP, China mainland) */
#define NTP_SERVER_IP_DEFAULT   { 120, 25, 115, 20 }

/* NTP standard UDP port */
#define NTP_PORT                123

/* W5500 hardware socket for NTP (0=upstream, 1=downstream, 2=NTP) */
#define NET_SOCK_NTP            2

/* Initial retry interval (seconds) before first successful sync */
#define SNTP_RETRY_INTERVAL     60

/* Re-sync interval after successful sync (1 hour) */
#define SNTP_RESYNC_INTERVAL    3600

/**
 * @brief  Start the background SNTP task.
 *
 * Creates a low-priority FreeRTOS task that syncs time on startup,
 * then re-syncs every SNTP_RESYNC_INTERVAL seconds. Non-blocking.
 *
 * NTP servers tried in order:
 *   1. gw_ip      (configured gateway)
 *   2. eth_ip[0..2].1  (derived local gateway)
 *   3. ns_ip      (Network Server — on same LAN, ARP guaranteed)
 *   4. public NTP (120.25.115.20)
 *
 * @param  gw_ip   Ethernet default gateway IP (4 bytes). NULL to skip.
 * @param  eth_ip  Local Ethernet IP (4 bytes). NULL to skip.
 * @param  ns_ip   Network Server IP (4 bytes). NULL to skip.
 */
void sntp_task_start(const uint8_t *gw_ip, const uint8_t *eth_ip,
                    const uint8_t *ns_ip);

/**
 * @brief  Perform one SNTP time synchronisation (blocking, used by task).
 *
 * @param  ntp_ip  NTP server IPv4 (4 bytes). NULL → NTP_SERVER_IP_DEFAULT.
 * @return 0 on success, -1 on failure.
 */
int sntp_sync(const uint8_t *ntp_ip);

/**
 * @brief  Get current UTC time from the software clock.
 * @return Seconds since 1970-01-01 00:00:00 UTC, or 0 if never synced.
 */
time_t sntp_get_utc(void);

/**
 * @brief  Check whether time has been synchronised at least once.
 */
bool sntp_is_synced(void);

/**
 * @brief  Seconds elapsed since the last successful sync.
 * @return Elapsed seconds, or UINT32_MAX if never synced.
 */
uint32_t sntp_age(void);

#endif /* SNTP_CLIENT_H */
