/*
 * gateway_config.h  –  Flash-based gateway configuration storage
 *
 * Replaces ESP32 NVS. Configuration is stored in STM32F407ZGTx Flash
 * Sector 11 (0x080E0000, 128KB).
 *
 * Config parameters (pkt_fwd relevant):
 *   - ns_host        LoRaWAN Network Server IP/hostname
 *   - ns_port_up     Uplink UDP port (default 1680)
 *   - ns_port_down   Downlink UDP port (default 1680)
 *   - gateway_eui    64-bit Gateway EUI
 *   - eth_ip/gw/sn   Static Ethernet configuration
 */

#ifndef GATEWAY_CONFIG_H
#define GATEWAY_CONFIG_H

#include <stdint.h>
#include <stddef.h>

#include "gw_config_presets.h"
#include "gateway_defaults.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Magic                                                              */
/* ------------------------------------------------------------------ */

/* Bump CONFIG_MAGIC when gateway_config_t layout changes.
 * v3: added freq_region, radio0_freq, radio1_freq fields.
 * v4: CN470 expanded from 8 to 12 sub-bands; EU868/US915/AU915/AS923
 *     enum values shifted (+4).  Old v3 Flash data will be rejected. */
#define CONFIG_MAGIC            0xC0FFEE04U

/* ------------------------------------------------------------------ */
/*  Compile-time defaults  (see gateway_defaults.h to change them)    */
/* ------------------------------------------------------------------ */

#define CONFIG_DEFAULT_NS_HOST      GW_DEFAULT_NS_HOST
#define CONFIG_DEFAULT_NS_PORT_UP   GW_DEFAULT_NS_PORT_UP
#define CONFIG_DEFAULT_NS_PORT_DOWN GW_DEFAULT_NS_PORT_DOWN
#define CONFIG_DEFAULT_GW_EUI       GW_DEFAULT_EUI

#define CONFIG_DEFAULT_ETH_IP       GW_DEFAULT_ETH_IP
#define CONFIG_DEFAULT_ETH_GW       GW_DEFAULT_ETH_GW
#define CONFIG_DEFAULT_ETH_SN       GW_DEFAULT_ETH_SN

#define CONFIG_DEFAULT_FREQ_REGION  GW_DEFAULT_FREQ_REGION
#define CONFIG_DEFAULT_RADIO0_FREQ  GW_DEFAULT_RADIO0_FREQ
#define CONFIG_DEFAULT_RADIO1_FREQ  GW_DEFAULT_RADIO1_FREQ

/* ------------------------------------------------------------------ */
/*  Config struct                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    uint32_t magic;              /* CONFIG_MAGIC when valid                */
    char     ns_host[64];        /* NS hostname or IP string               */
    uint16_t ns_port_up;         /* Uplink UDP port                        */
    uint16_t ns_port_down;       /* Downlink UDP port                      */
    uint64_t gateway_eui;        /* 64-bit EUI (big-endian in network use) */
    uint8_t  eth_ip[4];          /* Static Ethernet IP                     */
    uint8_t  eth_gw[4];          /* Ethernet default gateway               */
    uint8_t  eth_sn[4];          /* Ethernet subnet mask                   */
    /* Frequency plan (added in v3) ----------------------------------- */
    uint32_t radio0_freq;        /* SX1302 radio_0 center freq, Hz         */
    uint32_t radio1_freq;        /* SX1302 radio_1 center freq, Hz         */
    uint8_t  freq_region;        /* freq_region_t enum value               */
    uint8_t  _pad[3];            /* reserved, must be 0                    */
    /* ---------------------------------------------------------------- */
    uint32_t checksum;           /* 32-bit sum of all preceding bytes      */
} gateway_config_t;

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

/**
 * @brief Load config from Flash into RAM.
 *        Falls back to defaults if Flash is empty or checksum fails.
 */
void config_load(void);

/**
 * @brief Save current RAM config to Flash Sector 11.
 *        Erases sector first, then writes word-by-word.
 */
void config_save(void);

/**
 * @brief Reset RAM config to compile-time defaults and save to Flash.
 */
void config_reset_defaults(void);

/**
 * @brief Print current RAM config to USART1 (printf).
 */
void config_print(void);

/**
 * @brief Get pointer to current RAM config (for read/modify).
 */
gateway_config_t *config_get(void);

#ifdef __cplusplus
}
#endif

#endif /* GATEWAY_CONFIG_H */
