/*
 * gw_config_presets.c – LoRaWAN frequency plan preset table
 *
 * Radio center frequencies follow the same convention as the Semtech
 * global_conf.json files: radio_0 covers the lower 4 channels, radio_1
 * covers the upper 4 channels, each with ±300 kHz / ±100 kHz IF offsets.
 *
 * CN470 sub-band layout (LoRaWAN Alliance plan, 200 kHz channel spacing):
 *   f_up(k) = 470.3 + k × 0.2 MHz,  k = 0..95  (96 uplink channels)
 *   SB_N covers CH (N*8)..(N*8+7),   radio0 = 470.6 + N*1.6 MHz
 *                                     radio1 = radio0 + 0.8 MHz
 *
 *   SB0:  CH0-7   470.3–471.7 MHz  radio0=470.6  radio1=471.4
 *   SB1:  CH8-15  471.9–473.3 MHz  radio0=472.2  radio1=473.0
 *   SB2:  CH16-23 473.5–474.9 MHz  radio0=473.8  radio1=474.6
 *   SB3:  CH24-31 475.1–476.5 MHz  radio0=475.4  radio1=476.2
 *   SB4:  CH32-39 476.7–478.1 MHz  radio0=477.0  radio1=477.8
 *   SB5:  CH40-47 478.3–479.7 MHz  radio0=478.6  radio1=479.4
 *   SB6:  CH48-55 479.9–481.3 MHz  radio0=480.2  radio1=481.0
 *   SB7:  CH56-63 481.5–482.9 MHz  radio0=481.8  radio1=482.6
 *   SB8:  CH64-71 483.1–484.5 MHz  radio0=483.4  radio1=484.2
 *   SB9:  CH72-79 484.7–486.1 MHz  radio0=485.0  radio1=485.8
 *   SB10: CH80-87 486.3–487.7 MHz  radio0=486.6  radio1=487.4  ← ChirpStack cn470_10
 *   SB11: CH88-95 487.9–489.3 MHz  radio0=488.2  radio1=489.0
 *
 * ChirpStack naming: cn470_N  ↔  FREQ_REGION_CN470_SB_N  (N = 0..11)
 */

#include "gw_config_presets.h"
#include <string.h>
#include <ctype.h>

/* ------------------------------------------------------------------ */
/*  Preset table                                                       */
/* ------------------------------------------------------------------ */

const gw_freq_preset_t gw_freq_presets[GW_FREQ_PRESET_COUNT] = {
    /* ---- CN470 sub-bands (cli names match ChirpStack: CN470_0..CN470_11) ---- */
    { FREQ_REGION_CN470_SB0,   "CN470_0",  470600000UL, 471400000UL,
      "CN470 SB0  (CH0-7,   470.3-471.7 MHz)"  },
    { FREQ_REGION_CN470_SB1,   "CN470_1",  472200000UL, 473000000UL,
      "CN470 SB1  (CH8-15,  471.9-473.3 MHz)"  },
    { FREQ_REGION_CN470_SB2,   "CN470_2",  473800000UL, 474600000UL,
      "CN470 SB2  (CH16-23, 473.5-474.9 MHz)"  },
    { FREQ_REGION_CN470_SB3,   "CN470_3",  475400000UL, 476200000UL,
      "CN470 SB3  (CH24-31, 475.1-476.5 MHz)"  },
    { FREQ_REGION_CN470_SB4,   "CN470_4",  477000000UL, 477800000UL,
      "CN470 SB4  (CH32-39, 476.7-478.1 MHz)"  },
    { FREQ_REGION_CN470_SB5,   "CN470_5",  478600000UL, 479400000UL,
      "CN470 SB5  (CH40-47, 478.3-479.7 MHz)"  },
    { FREQ_REGION_CN470_SB6,   "CN470_6",  480200000UL, 481000000UL,
      "CN470 SB6  (CH48-55, 479.9-481.3 MHz)"  },
    { FREQ_REGION_CN470_SB7,   "CN470_7",  481800000UL, 482600000UL,
      "CN470 SB7  (CH56-63, 481.5-482.9 MHz)"  },
    { FREQ_REGION_CN470_SB8,   "CN470_8",  483400000UL, 484200000UL,
      "CN470 SB8  (CH64-71, 483.1-484.5 MHz)"  },
    { FREQ_REGION_CN470_SB9,   "CN470_9",  485000000UL, 485800000UL,
      "CN470 SB9  (CH72-79, 484.7-486.1 MHz)"  },
    { FREQ_REGION_CN470_SB10,  "CN470_10", 486600000UL, 487400000UL,
      "CN470 SB10 (CH80-87, 486.3-487.7 MHz) = ChirpStack cn470_10" },
    { FREQ_REGION_CN470_SB11,  "CN470_11", 488200000UL, 489000000UL,
      "CN470 SB11 (CH88-95, 487.9-489.3 MHz)"  },

    /* ---- Other regions ---- */
    { FREQ_REGION_EU868,       "EU868",    867500000UL, 868500000UL,
      "EU868 (867.1-868.5 MHz)"                },
    { FREQ_REGION_US915_SB0,   "US915",    904300000UL, 905000000UL,
      "US915 SB0 (903.9-905.3 MHz)"            },
    { FREQ_REGION_AU915_SB0,   "AU915",    916800000UL, 917500000UL,
      "AU915 SB0 (916.4-917.9 MHz)"            },
    { FREQ_REGION_AS923,       "AS923",    923200000UL, 923400000UL,
      "AS923 (923.2-924.6 MHz)"                },
};

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

const gw_freq_preset_t *gw_preset_get(freq_region_t region)
{
    for (int i = 0; i < GW_FREQ_PRESET_COUNT; i++) {
        if (gw_freq_presets[i].region == region) {
            return &gw_freq_presets[i];
        }
    }
    return NULL;  /* CUSTOM or unknown */
}

freq_region_t gw_preset_find_by_name(const char *name)
{
    if (!name) return FREQ_REGION_CUSTOM;

    for (int i = 0; i < GW_FREQ_PRESET_COUNT; i++) {
        /* Case-insensitive compare */
        const char *a = gw_freq_presets[i].name;
        const char *b = name;
        int match = 1;
        while (*a || *b) {
            if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
                match = 0;
                break;
            }
            a++; b++;
        }
        if (match) {
            return gw_freq_presets[i].region;
        }
    }
    return FREQ_REGION_CUSTOM;
}
