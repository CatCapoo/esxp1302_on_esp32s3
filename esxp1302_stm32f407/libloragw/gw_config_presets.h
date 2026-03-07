/*
 * gw_config_presets.h – LoRaWAN frequency plan presets for ESXP1302 gateway
 *
 * Defines the freq_region_t enum and gw_freq_preset_t struct used by
 * gateway_config.h and uart_cli.c.
 *
 * Supported regions:
 *   CN470 SubBand 0-11 (LoRaWAN Alliance CN470 plan, 12×8 = 96 channels)
 *     SB0-SB7:   CH0-63,  470.3-482.9 MHz
 *     SB8-SB11:  CH64-95, 483.1-489.3 MHz
 *     ChirpStack naming: cn470_N  ↔  CN470_N  (N = 0..11)
 *   EU868              (European 863-870 MHz band)
 *   US915 SubBand 0    (US 902-928 MHz, first 8 upstream channels)
 *   AS923              (Asia 915-928 MHz)
 *   AU915 SubBand 0    (Australia 915-928 MHz, first 8 channels)
 *   CUSTOM             (manual radio0_freq / radio1_freq override)
 */

#ifndef GW_CONFIG_PRESETS_H
#define GW_CONFIG_PRESETS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Frequency region enum                                             */
/* ------------------------------------------------------------------ */

typedef enum {
    /* CN470 LoRaWAN Alliance plan – 12 sub-bands, each 8 uplink channels
     * f_up(k) = 470.3 + k × 0.2 MHz,  k = 0..95
     * SB_N covers CH (N*8) .. (N*8+7)
     * ChirpStack region name: cn470_N  */
    FREQ_REGION_CN470_SB0  =  0,  /* CH0-7:   470.3-471.7 MHz */
    FREQ_REGION_CN470_SB1  =  1,  /* CH8-15:  471.9-473.3 MHz */
    FREQ_REGION_CN470_SB2  =  2,  /* CH16-23: 473.5-474.9 MHz */
    FREQ_REGION_CN470_SB3  =  3,  /* CH24-31: 475.1-476.5 MHz */
    FREQ_REGION_CN470_SB4  =  4,  /* CH32-39: 476.7-478.1 MHz */
    FREQ_REGION_CN470_SB5  =  5,  /* CH40-47: 478.3-479.7 MHz */
    FREQ_REGION_CN470_SB6  =  6,  /* CH48-55: 479.9-481.3 MHz */
    FREQ_REGION_CN470_SB7  =  7,  /* CH56-63: 481.5-482.9 MHz */
    FREQ_REGION_CN470_SB8  =  8,  /* CH64-71: 483.1-484.5 MHz */
    FREQ_REGION_CN470_SB9  =  9,  /* CH72-79: 484.7-486.1 MHz */
    FREQ_REGION_CN470_SB10 = 10,  /* CH80-87: 486.3-487.7 MHz  ← ChirpStack cn470_10 */
    FREQ_REGION_CN470_SB11 = 11,  /* CH88-95: 487.9-489.3 MHz */

    FREQ_REGION_EU868      = 12,  /* EU 863-870 MHz           */
    FREQ_REGION_US915_SB0  = 13,  /* US 902-928 MHz, SB0      */
    FREQ_REGION_AU915_SB0  = 14,  /* AU 915-928 MHz, SB0      */
    FREQ_REGION_AS923      = 15,  /* AS 915-928 MHz           */

    FREQ_REGION_CUSTOM     = 255  /* Manual radio0/radio1 Hz  */
} freq_region_t;

/* Number of named (non-custom) presets */
#define GW_FREQ_PRESET_COUNT  16

/* ------------------------------------------------------------------ */
/*  Preset struct                                                     */
/* ------------------------------------------------------------------ */

typedef struct {
    freq_region_t  region;       /* Region enum value         */
    const char    *name;         /* Short name for CLI        */
    uint32_t       radio0_freq;  /* SX1302 radio_0 center Hz  */
    uint32_t       radio1_freq;  /* SX1302 radio_1 center Hz  */
    const char    *description;  /* Human-readable description*/
} gw_freq_preset_t;

/* ------------------------------------------------------------------ */
/*  Public preset table (defined in gw_config_presets.c)             */
/* ------------------------------------------------------------------ */

extern const gw_freq_preset_t gw_freq_presets[GW_FREQ_PRESET_COUNT];

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief Look up preset by region enum.
 * @return Pointer to preset, or NULL for FREQ_REGION_CUSTOM.
 */
const gw_freq_preset_t *gw_preset_get(freq_region_t region);

/**
 * @brief Find preset by short name string (case-insensitive).
 * @return Region enum, or FREQ_REGION_CUSTOM if not found.
 */
freq_region_t gw_preset_find_by_name(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* GW_CONFIG_PRESETS_H */
