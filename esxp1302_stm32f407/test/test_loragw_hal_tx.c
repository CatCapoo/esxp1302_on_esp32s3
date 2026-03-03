/*
 * test_loragw_hal_tx.c  –  HAL TX test for STM32F407 + SX1302
 *
 * Ported from ESP32S3 bringup/test branch.
 * Frequency plan: CN470_10 (Sub-band 10, CH80–CH87, 486.3–487.7 MHz)
 *
 * Default TX on CH80 = 486.3 MHz, LoRa SF7 BW125, 14 dBm
 *
 * Pair with: e77_node_rx.py (E77 node listens for LoRa packets)
 */

#include <stdint.h>
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
/* --- CN470_10 TX PARAMETERS ---------------------------------------------- */

#define TX_FREQ_HZ      486300000U   /* CH80 = 486.3 MHz */
#define TX_RF_CHAIN     0            /* Use Radio A for TX */
#define TX_RF_POWER     14           /* dBm */
#define TX_SF           7            /* SF7 */
#define TX_BW_KHZ       125          /* BW 125 kHz */
#define TX_PKT_COUNT    10           /* Number of packets per loop */
#define TX_PKT_SIZE     32           /* Payload size in bytes */
#define TX_LOOP_COUNT   3            /* Number of start/stop cycles */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTION ------------------------------------------------------ */

void test_loragw_hal_tx(void)
{
    int i, x;
    struct lgw_conf_board_s  boardconf;
    struct lgw_conf_rxrf_s   rfconf;
    struct lgw_tx_gain_lut_s txlut;
    struct lgw_pkt_tx_s      pkt;
    uint8_t tx_status;
    unsigned int cnt_loop;

    printf("\n");
    printf("===== sx1302 HAL TX test (CN470_10) =====\n");
    printf("TX freq: %u Hz  SF%d  BW%d  Power: %d dBm\n",
           TX_FREQ_HZ, TX_SF, TX_BW_KHZ, TX_RF_POWER);
    printf("Packets: %d per loop x %d loops = %d total\n\n",
           TX_PKT_COUNT, TX_LOOP_COUNT, TX_PKT_COUNT * TX_LOOP_COUNT);

    /* ---- Board configuration ---- */
    memset(&boardconf, 0, sizeof(boardconf));
    boardconf.lorawan_public = true;
    boardconf.clksrc          = 0;
    boardconf.full_duplex     = false;
    if (lgw_board_setconf(&boardconf) != LGW_HAL_SUCCESS) {
        printf("ERROR: failed to configure board\n");
        return;
    }

    /* ---- RF chain 0 (Radio A) – TX enabled ---- */
    memset(&rfconf, 0, sizeof(rfconf));
    rfconf.enable           = true;
    rfconf.freq_hz          = TX_FREQ_HZ;
    rfconf.type             = LGW_RADIO_TYPE_SX1250;
    rfconf.tx_enable        = true;
    rfconf.single_input_mode = false;
    if (lgw_rxrf_setconf(0, &rfconf) != LGW_HAL_SUCCESS) {
        printf("ERROR: failed to configure rxrf 0\n");
        return;
    }

    /* ---- RF chain 1 (Radio B) – disabled for TX test ---- */
    memset(&rfconf, 0, sizeof(rfconf));
    rfconf.enable  = false;
    rfconf.freq_hz = TX_FREQ_HZ;
    rfconf.type    = LGW_RADIO_TYPE_SX1250;
    if (lgw_rxrf_setconf(1, &rfconf) != LGW_HAL_SUCCESS) {
        printf("ERROR: failed to configure rxrf 1\n");
        return;
    }

    /* ---- TX gain LUT (SX1250 power index) ---- */
    memset(&txlut, 0, sizeof(txlut));
    txlut.size = 1;
    txlut.lut[0].rf_power = TX_RF_POWER;
    txlut.lut[0].pa_gain  = 1;       /* SX1250: 0 or 1 */
    txlut.lut[0].pwr_idx  = 14;      /* SX1250 power index for ~14 dBm */
    txlut.lut[0].mix_gain = 5;
    txlut.lut[0].dig_gain = 0;
    if (lgw_txgain_setconf(TX_RF_CHAIN, &txlut) != LGW_HAL_SUCCESS) {
        printf("ERROR: failed to configure TX gain LUT\n");
        return;
    }

    /* ---- Main TX loop ---- */
    for (cnt_loop = 0; cnt_loop < TX_LOOP_COUNT; cnt_loop++) {
        printf("\n--- TX Loop %u/%d ---\n", cnt_loop + 1, TX_LOOP_COUNT);

        /* Board reset */
        lgw_reset();

        /* Start concentrator */
        x = lgw_start();
        if (x != 0) {
            printf("ERROR: lgw_start() failed\n");
            return;
        }
        printf("INFO: concentrator started\n");

        /* Prepare packet */
        memset(&pkt, 0, sizeof(pkt));
        pkt.rf_chain   = TX_RF_CHAIN;
        pkt.freq_hz    = TX_FREQ_HZ;
        pkt.rf_power   = TX_RF_POWER;
        pkt.tx_mode    = IMMEDIATE;
        pkt.modulation = MOD_LORA;
        pkt.coderate   = CR_LORA_4_5;
        pkt.no_crc     = true;
        pkt.invert_pol = false;
        pkt.preamble   = 8;
        pkt.no_header  = false;

        /* Bandwidth */
        switch (TX_BW_KHZ) {
            case 125: pkt.bandwidth = BW_125KHZ; break;
            case 250: pkt.bandwidth = BW_250KHZ; break;
            case 500: pkt.bandwidth = BW_500KHZ; break;
            default:  pkt.bandwidth = BW_125KHZ; break;
        }
        pkt.datarate = TX_SF;
        pkt.size     = TX_PKT_SIZE;

        /* Fill payload: header + sequential bytes */
        pkt.payload[0] = 0x40;  /* Confirmed Data Up */
        pkt.payload[1] = 0xAB;
        pkt.payload[2] = 0xAB;
        pkt.payload[3] = 0xAB;
        pkt.payload[4] = 0xAB;
        pkt.payload[5] = 0x00;  /* FCtrl */
        pkt.payload[6] = 0x00;  /* FCnt LSB */
        pkt.payload[7] = 0x00;  /* FCnt MSB */
        pkt.payload[8] = 0x02;  /* FPort */
        for (i = 9; i < 255; i++) {
            pkt.payload[i] = (uint8_t)i;
        }

        /* Send packets */
        for (i = 0; i < TX_PKT_COUNT; i++) {
            pkt.payload[6] = (uint8_t)(i >> 0);  /* FCnt LSB */
            pkt.payload[7] = (uint8_t)(i >> 8);  /* FCnt MSB */

            printf("Sending packet %d/%d (size=%d) ...\n",
                   i + 1, TX_PKT_COUNT, pkt.size);
            x = lgw_send(&pkt);
            if (x != 0) {
                printf("ERROR: lgw_send() failed\n");
                return;
            }

            /* Wait for TX done */
            do {
                wait_ms(5);
                lgw_status(pkt.rf_chain, TX_STATUS, &tx_status);
            } while (tx_status != TX_FREE);

            printf("TX done.\n");
            wait_ms(500);  /* small delay between packets */
        }

        printf("\nSent %d packets (loop %u)\n", TX_PKT_COUNT, cnt_loop + 1);

        /* Stop concentrator */
        x = lgw_stop();
        if (x != 0) {
            printf("ERROR: lgw_stop() failed\n");
            return;
        }

        /* Board reset */
        lgw_reset();
    }

    printf("\n===== HAL TX Test End =====\n");
    printf("Total packets sent: %d\n", TX_PKT_COUNT * TX_LOOP_COUNT);
}
