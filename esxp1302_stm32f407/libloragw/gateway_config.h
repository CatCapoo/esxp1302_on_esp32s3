/*
 * gateway_config.h  –  Flash-based gateway configuration storage
 *
 * Replaces ESP32 NVS. Configuration is stored in STM32F407ZGTx Flash
 * Sector 11 (0x080E0000, 128KB).
 *
 * Config parameters (pkt_fwd relevant):
 *   - ns_host        LoRaWAN Network Server IP/hostname
 *   - ns_port_up     Uplink UDP port (default 1700)
 *   - ns_port_down   Downlink UDP port (default 1700)
 *   - gateway_eui    64-bit Gateway EUI
 *   - eth_ip/gw/sn   Static Ethernet configuration
 */

#ifndef GATEWAY_CONFIG_H
#define GATEWAY_CONFIG_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Magic & defaults                                                   */
/* ------------------------------------------------------------------ */

#define CONFIG_MAGIC            0xC0FFEE01U  /* valid config marker */

#define CONFIG_DEFAULT_NS_HOST      "192.168.10.1"
#define CONFIG_DEFAULT_NS_PORT_UP   1700
#define CONFIG_DEFAULT_NS_PORT_DOWN 1700
#define CONFIG_DEFAULT_GW_EUI       0xAA555A0000000000ULL

/* Defaults are applied from W5500 network config */
#define CONFIG_DEFAULT_ETH_IP   {192, 168, 10,  15}
#define CONFIG_DEFAULT_ETH_GW   {192, 168, 10,   1}
#define CONFIG_DEFAULT_ETH_SN   {255, 255, 255,  0}

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
