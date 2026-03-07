/*
 * gateway_defaults.h  –  Compile-time default configuration for ESXP1302 gateway
 *
 * ============================================================================
 * HOW TO USE
 * ============================================================================
 * Edit the values in this file to set your default gateway configuration.
 * These values are compiled in and used as defaults when the Flash config
 * sector is empty (first boot) or after a "config reset" CLI command.
 *
 * To permanently change a setting at runtime, use the UART CLI:
 *   config set ns_host     192.168.1.100
 *   config set gw_eui      AA555A0000000001
 *   config set freq_region CN470_0
 *   config save
 *   reboot
 *
 * ============================================================================
 * NETWORK SERVER
 * ============================================================================
 */

#ifndef GATEWAY_DEFAULTS_H
#define GATEWAY_DEFAULTS_H

/* LoRaWAN Network Server IP address (IPv4 dotted notation) */
#define GW_DEFAULT_NS_HOST          "192.168.71.100"

/* NS UDP uplink port (standard: 1700, Semtech legacy: 1680) */
#define GW_DEFAULT_NS_PORT_UP       1700

/* NS UDP downlink port */
#define GW_DEFAULT_NS_PORT_DOWN     1700

/*
 * ============================================================================
 * GATEWAY EUI
 * ============================================================================
 * 64-bit EUI written as 0xAABBCCDDEEFF0011 (big-endian hex literal).
 * This EUI is sent in PULL_DATA and PUSH_DATA packets.
 * Change to match your gateway registration in the NS.
 */
#define GW_DEFAULT_EUI              0xAA555A00000021FBULL

/*
 * ============================================================================
 * ETHERNET (W5500 static IP)
 * ============================================================================
 * Set DHCP_ENABLE to 1 to use DHCP (not yet implemented; reserved for future).
 * Currently only static IP is supported.
 */
#define GW_DEFAULT_ETH_IP           {192, 168, 71,  110}
#define GW_DEFAULT_ETH_GW           {192, 168, 10,   1}
#define GW_DEFAULT_ETH_SN           {255, 255, 255,  0}
#define GW_DEFAULT_ETH_DNS          {  8,   8,   8,  8}

/* W5500 MAC address (must be unique on your LAN).
 * First byte must be even (unicast) and not end in 01 (avoid conflicts). */
#define GW_DEFAULT_ETH_MAC          {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}

/*
 * ============================================================================
 * FREQUENCY PLAN
 * ============================================================================
 * Set GW_DEFAULT_FREQ_REGION to one of the freq_region_t enum values defined
 * in gw_config_presets.h:
 *
 *   FREQ_REGION_CN470_SB0    CLI "CN470_0"   CH0-7   470.3-471.7 MHz  ← default
 *   FREQ_REGION_CN470_SB1    CLI "CN470_1"   CH8-15  471.9-473.3 MHz
 *   FREQ_REGION_CN470_SB2    CLI "CN470_2"   CH16-23 473.5-474.9 MHz
 *   FREQ_REGION_CN470_SB3    CLI "CN470_3"   CH24-31 475.1-476.5 MHz
 *   FREQ_REGION_CN470_SB4    CLI "CN470_4"   CH32-39 476.7-478.1 MHz
 *   FREQ_REGION_CN470_SB5    CLI "CN470_5"   CH40-47 478.3-479.7 MHz
 *   FREQ_REGION_CN470_SB6    CLI "CN470_6"   CH48-55 479.9-481.3 MHz
 *   FREQ_REGION_CN470_SB7    CLI "CN470_7"   CH56-63 481.5-482.9 MHz
 *   FREQ_REGION_CN470_SB8    CLI "CN470_8"   CH64-71 483.1-484.5 MHz
 *   FREQ_REGION_CN470_SB9    CLI "CN470_9"   CH72-79 484.7-486.1 MHz
 *   FREQ_REGION_CN470_SB10   CLI "CN470_10"  CH80-87 486.3-487.7 MHz  ← ChirpStack cn470_10
 *   FREQ_REGION_CN470_SB11   CLI "CN470_11"  CH88-95 487.9-489.3 MHz
 *   FREQ_REGION_EU868         CLI "EU868"     867.1-868.5 MHz
 *   FREQ_REGION_US915_SB0    CLI "US915"     903.9-905.3 MHz
 *   FREQ_REGION_AU915_SB0    CLI "AU915"     916.4-917.9 MHz
 *   FREQ_REGION_AS923         CLI "AS923"     923.2-924.6 MHz
 *   FREQ_REGION_CUSTOM        Custom (set radio freqs manually below)
 *
 * When a named region is selected, GW_DEFAULT_RADIO0_FREQ and
 * GW_DEFAULT_RADIO1_FREQ are overridden by the preset values at runtime.
 * Setting FREQ_REGION_CUSTOM uses the values below as-is.
 */
#define GW_DEFAULT_FREQ_REGION      FREQ_REGION_CN470_SB0

/*
 * Custom / override radio center frequencies (Hz).
 * These are only used when GW_DEFAULT_FREQ_REGION == FREQ_REGION_CUSTOM.
 * For named regions, the preset values from gw_config_presets.c apply.
 *
 * Example: CN470 SubBand 0
 *   radio_0 = 470.6 MHz  → 8 channels: 470.3, 470.5, 470.7, 470.9, 471.1, ...
 *   radio_1 = 471.4 MHz  → 8 channels: 471.1, 471.3, 471.5, 471.7 (shared overlap)
 */
#define GW_DEFAULT_RADIO0_FREQ      470600000UL   /* Hz */
#define GW_DEFAULT_RADIO1_FREQ      471400000UL   /* Hz */

/*
 * ============================================================================
 * PACKET FORWARDING BEHAVIOUR
 * ============================================================================
 */

/* Forward packets with valid CRC */
#define GW_DEFAULT_FWD_CRC_VALID    1

/* Forward packets with invalid CRC (0 = discard bad CRC packets) */
#define GW_DEFAULT_FWD_CRC_ERROR    0

/* Forward packets with CRC disabled */
#define GW_DEFAULT_FWD_CRC_DISABLED 0

/* Keepalive interval for PULL_DATA in seconds */
#define GW_DEFAULT_KEEPALIVE_SEC    10

/* Statistics reporting interval in seconds */
#define GW_DEFAULT_STAT_INTERVAL    30

/* PUSH_DATA acknowledgement timeout in ms */
#define GW_DEFAULT_PUSH_TIMEOUT_MS  500

/*
 * ============================================================================
 * GPS (currently disabled on STM32 port)
 * ============================================================================
 * GPS is not yet ported to STM32. The following defines reserve configuration
 * space for a future GPS port. Set GPS_ENABLE to 1 to enable.
 */
#define GW_GPS_ENABLE               0

/* Reference coordinates (used as fallback when GPS is absent) */
#define GW_DEFAULT_REF_LAT          0.0
#define GW_DEFAULT_REF_LON          0.0
#define GW_DEFAULT_REF_ALT          0

/*
 * ============================================================================
 * BEACON (Class B — currently not implemented)
 * ============================================================================
 */
#define GW_DEFAULT_BEACON_PERIOD    0         /* 0 = disabled */
#define GW_DEFAULT_BEACON_FREQ_HZ   508300000 /* Hz (CN470 beacon channel) */
#define GW_DEFAULT_BEACON_DR        9
#define GW_DEFAULT_BEACON_BW_HZ     125000
#define GW_DEFAULT_BEACON_POWER     14
#define GW_DEFAULT_BEACON_INFODESC  0

/*
 * ============================================================================
 * FLASH STORAGE
 * ============================================================================
 * Config is stored in STM32F407ZGTx Flash Sector 11 (0x080E0000, 128 KB).
 * The magic number in gateway_config.h (CONFIG_MAGIC) must be bumped when
 * the gateway_config_t struct layout changes; otherwise old Flash data will
 * be rejected and defaults re-applied automatically.
 */

#endif /* GATEWAY_DEFAULTS_H */
