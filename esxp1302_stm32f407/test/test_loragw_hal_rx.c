/*
 * test_loragw_hal_rx.c  –  HAL RX test for STM32F407 + SX1302
 *
 * Ported from ESP32S3 bringup/test branch.
 * Frequency plan: CN470_10 (Sub-band 10, CH80–CH87, 486.3–487.7 MHz)
 *
 * Radio A (radio_0) = 486.6 MHz  (covers CH80–CH83)
 * Radio B (radio_1) = 487.4 MHz  (covers CH84–CH87)
 * IF offsets: -300, -100, +100, +300 kHz per radio
 *
 * Pair with: e77_node_tx.py (E77 node sends packets on CN470_10)
 */

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "loragw_hal.h"
#include "loragw_reg.h"
#include "loragw_gpio.h"
#include "loragw_aux.h"

/* -------------------------------------------------------------------------- */
/* --- MACROS --------------------------------------------------------------- */

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))

/* -------------------------------------------------------------------------- */
/* --- CN470_10 FREQUENCY PLAN --------------------------------------------- */
/*
 * Sub-band 10:  CH80 = 486.3 MHz  ...  CH87 = 487.7 MHz
 * Radio A center = 486.6 MHz,  Radio B center = 487.4 MHz
 */

#define FREQ_RADIO_A    486600000U   /* Radio 0 center */
#define FREQ_RADIO_B    487400000U   /* Radio 1 center */

/* IF offsets (Hz) – 8 multi-SF channels */
static const int32_t channel_if[8] = {
    -300000,    /* ch0 -> radio 0 -> 486.3 MHz = CH80 */
    -100000,    /* ch1 -> radio 0 -> 486.5 MHz = CH81 */
     100000,    /* ch2 -> radio 0 -> 486.7 MHz = CH82 */
     300000,    /* ch3 -> radio 0 -> 486.9 MHz = CH83 */
    -300000,    /* ch4 -> radio 1 -> 487.1 MHz = CH84 */
    -100000,    /* ch5 -> radio 1 -> 487.3 MHz = CH85 */
     100000,    /* ch6 -> radio 1 -> 487.5 MHz = CH86 */
     300000,    /* ch7 -> radio 1 -> 487.7 MHz = CH87 */
};

/* RF chain assignment per IF channel */
static const uint8_t channel_rfchain[8] = {
    0, 0, 0, 0,    /* ch0–ch3 on radio 0 */
    1, 1, 1, 1,    /* ch4–ch7 on radio 1 */
};

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTION ------------------------------------------------------ */

void test_loragw_hal_rx(void)
{
    int i, j, x;
    struct lgw_conf_board_s boardconf;
    struct lgw_conf_rxrf_s  rfconf;
    struct lgw_conf_rxif_s  ifconf;

    unsigned long nb_pkt_total = 0;
    int nb_pkt;
    int cnt_loop;

    printf("\n");
    printf("===== sx1302 HAL RX test (CN470_10) =====\n");
    printf("Radio A: %u Hz   Radio B: %u Hz\n", FREQ_RADIO_A, FREQ_RADIO_B);
    printf("Channels CH80–CH87 (486.3–487.7 MHz)\n\n");

    /* ---- Board configuration ---- */
    memset(&boardconf, 0, sizeof(boardconf));
    boardconf.lorawan_public = true;
    boardconf.clksrc          = 0;
    boardconf.full_duplex     = false;
    if (lgw_board_setconf(&boardconf) != LGW_HAL_SUCCESS) {
        printf("ERROR: failed to configure board\n");
        return;
    }

    /* ---- RF chain 0 (Radio A) ---- */
    memset(&rfconf, 0, sizeof(rfconf));
    rfconf.enable           = true;
    rfconf.freq_hz          = FREQ_RADIO_A;
    rfconf.type             = LGW_RADIO_TYPE_SX1250;
    rfconf.rssi_offset      = 0.0f;
    rfconf.tx_enable        = false;
    rfconf.single_input_mode = false;
    if (lgw_rxrf_setconf(0, &rfconf) != LGW_HAL_SUCCESS) {
        printf("ERROR: failed to configure rxrf 0\n");
        return;
    }

    /* ---- RF chain 1 (Radio B) ---- */
    memset(&rfconf, 0, sizeof(rfconf));
    rfconf.enable           = true;
    rfconf.freq_hz          = FREQ_RADIO_B;
    rfconf.type             = LGW_RADIO_TYPE_SX1250;
    rfconf.rssi_offset      = 0.0f;
    rfconf.tx_enable        = false;
    rfconf.single_input_mode = false;
    if (lgw_rxrf_setconf(1, &rfconf) != LGW_HAL_SUCCESS) {
        printf("ERROR: failed to configure rxrf 1\n");
        return;
    }

    /* ---- 8 multi-SF channels (BW=125 kHz, SF7–SF12) ---- */
    for (i = 0; i < 8; i++) {
        memset(&ifconf, 0, sizeof(ifconf));
        ifconf.enable   = true;
        ifconf.rf_chain = channel_rfchain[i];
        ifconf.freq_hz  = channel_if[i];
        ifconf.datarate = DR_LORA_SF7;          /* multi-SF: SF7 is min */
        if (lgw_rxif_setconf(i, &ifconf) != LGW_HAL_SUCCESS) {
            printf("ERROR: failed to configure rxif %d\n", i);
            return;
        }
    }

    /* ---- RX buffer ---- */
    #define MAX_RX_PKT  16
    struct lgw_pkt_rx_s rxpkt[MAX_RX_PKT];
    printf("INFO: rxpkt buffer size = %d\n", MAX_RX_PKT);

    /* ---- Main loop : 10 start/stop cycles ---- */
    for (cnt_loop = 1; cnt_loop <= 10; cnt_loop++) {
        printf("\n--- Loop %d/10 ---\n", cnt_loop);

        /* Board reset */
        lgw_reset();

        /* Start concentrator */
        x = lgw_start();
        if (x != 0) {
            printf("ERROR: lgw_start() failed\n");
            return;
        }
        printf("INFO: concentrator started, waiting for packets ...\n");

        /* Receive packets for ~30 seconds */
        uint32_t t_start = timestamp_us() / 1000;  /* ms */
        while ((timestamp_us() / 1000 - t_start) < 30000) {
            nb_pkt = lgw_receive(MAX_RX_PKT, rxpkt);
            if (nb_pkt == 0) {
                wait_ms(10);
                continue;
            }
            for (i = 0; i < nb_pkt; i++) {
                nb_pkt_total++;
                printf("\n----- %s packet #%lu -----\n",
                       (rxpkt[i].modulation == MOD_LORA) ? "LoRa" : "FSK",
                       nb_pkt_total);
                printf("  count_us: %u\n",   rxpkt[i].count_us);
                printf("  size:     %u\n",   rxpkt[i].size);
                printf("  chan:     %u\n",   rxpkt[i].if_chain);
                printf("  status:   0x%02X\n", rxpkt[i].status);
                printf("  datr:     %u\n",   rxpkt[i].datarate);
                printf("  codr:     %u\n",   rxpkt[i].coderate);
                printf("  rf_chain  %u\n",   rxpkt[i].rf_chain);
                printf("  freq_hz   %u\n",   rxpkt[i].freq_hz);
                printf("  snr_avg:  %.1f\n", rxpkt[i].snr);
                printf("  rssi_chan:%.1f\n", rxpkt[i].rssic);
                printf("  rssi_sig :%.1f\n", rxpkt[i].rssis);
                printf("  crc:      0x%04X\n", rxpkt[i].crc);
                printf("  payload:  ");
                for (j = 0; j < rxpkt[i].size; j++) {
                    printf("%02X ", rxpkt[i].payload[j]);
                }
                printf("\n");
            }
            printf("Received %d packets (total: %lu)\n", nb_pkt, nb_pkt_total);
        }

        printf("\nLoop %d: %lu packets received so far\n", cnt_loop, nb_pkt_total);

        /* Stop concentrator */
        x = lgw_stop();
        if (x != 0) {
            printf("ERROR: lgw_stop() failed\n");
            return;
        }
    }

    /* Final reset */
    lgw_reset();

    printf("\n===== HAL RX Test End =====\n");
    printf("Total packets received: %lu\n", nb_pkt_total);
}
