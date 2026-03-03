/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2019 Semtech

Description:
    Configure Lora concentrator and forward packets to a server
    Use GPS for packet timestamping.
    Send a becon at a regular interval without server intervention

License: Revised BSD License, see LICENSE.TXT file include in the project

    Ported to STM32F407 + W5500 Ethernet
    - BSD sockets → net_transport (W5500 UDP)
    - ESP-IDF / lwip → FreeRTOS (CMSIS-RTOS v2 wrappers)
    - NVS config → gateway_config (Flash Sector 11)
    - WiFi/MQTT/HTTP/NTP → removed
    - GPS → disabled (#define GPS_ENABLE 0)
    - OLED → STM32 I2C SSD1306 driver (loragw_oled)
*/


/* -------------------------------------------------------------------------- */
/*  Standard includes                                                         */
/* -------------------------------------------------------------------------- */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

/* -------------------------------------------------------------------------- */
/*  FreeRTOS                                                                  */
/* -------------------------------------------------------------------------- */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* -------------------------------------------------------------------------- */
/*  Project / HAL includes                                                    */
/* -------------------------------------------------------------------------- */
#include "trace.h"
#include "jitqueue.h"
#include "parson.h"
#include "base64.h"

#include "loragw_hal.h"
#include "loragw_aux.h"
#include "loragw_reg.h"
#include "loragw_gpio.h"
#include "loragw_oled.h"

#include "cmsis_os.h"  /* osPriority definitions */

#include "global_json.h"
#include "board_config.h"
#include "loragw_version.h"

/* Network transport (W5500 abstraction) */
#include "net_transport.h"

/* Flash-based config (replaces ESP32 NVS) */
#include "gateway_config.h"
#include "uart_cli.h"

/* -------------------------------------------------------------------------- */
/*  Helpers                                                                   */
/* -------------------------------------------------------------------------- */
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#define STRINGIFY(x)    #x
#define STR(x)          STRINGIFY(x)

/* GPS coordinate structure — normally in loragw_gps.h which doesn't exist on STM32 */
struct coord_s {
    double  lat;    /* latitude [-90,90] */
    double  lon;    /* longitude [-180,180] */
    short   alt;    /* altitude in meters */
};

/* Simple byte-swap for 32-bit (STM32 is little-endian, network is big-endian) */
static inline uint32_t pkt_htonl(uint32_t h) {
    return __builtin_bswap32(h);
}

/* -------------------------------------------------------------------------- */
/*  Configuration                                                             */
/* -------------------------------------------------------------------------- */

/* GPS disabled for STM32 port */
#define GPS_ENABLE  0

#ifndef VERSION_STRING
    #define VERSION_STRING "undefined"
#endif

#define DEFAULT_SERVER      "127.0.0.1"
#define DEFAULT_PORT_UP     1700
#define DEFAULT_PORT_DW     1700
#define DEFAULT_KEEPALIVE   5
#define DEFAULT_STAT        30
#define PUSH_TIMEOUT_MS     500
#define PULL_TIMEOUT_MS     400
#define FETCH_SLEEP_MS      10
#define BEACON_POLL_MS      50

#define PROTOCOL_VERSION    2
#define PROTOCOL_JSON_RXPK_FRAME_FORMAT 1

#define XERR_INIT_AVG       16
#define XERR_FILT_COEF      256

#define PKT_PUSH_DATA   0
#define PKT_PUSH_ACK    1
#define PKT_PULL_DATA   2
#define PKT_PULL_RESP   3
#define PKT_PULL_ACK    4
#define PKT_TX_ACK      5

/* Reduced from 24 to 8 for STM32 RAM savings (buff_up: 13KB → ~4.5KB) */
#define NB_PKT_MAX      8

#define MIN_LORA_PREAMB 6
#define STD_LORA_PREAMB 8
#define MIN_FSK_PREAMB  3
#define STD_FSK_PREAMB  5

#define STATUS_SIZE     200
#define TX_BUFF_SIZE    ((540 * NB_PKT_MAX) + 30 + STATUS_SIZE)
#define ACK_BUFF_SIZE   64

#define UNIX_GPS_EPOCH_OFFSET 315964800

#define DEFAULT_BEACON_FREQ_HZ      869525000
#define DEFAULT_BEACON_FREQ_NB      1
#define DEFAULT_BEACON_FREQ_STEP    0
#define DEFAULT_BEACON_DATARATE     9
#define DEFAULT_BEACON_BW_HZ        125000
#define DEFAULT_BEACON_POWER        14
#define DEFAULT_BEACON_INFODESC     0

/* For OLED display */
#define TIME_REFRESH    5
#define N_CHAR_A_ROW    21

/* -------------------------------------------------------------------------- */
/*  Global variables                                                          */
/* -------------------------------------------------------------------------- */

/* signal handling variables */
volatile bool exit_sig = false;
volatile bool quit_sig = false;

/* packets filtering configuration variables */
static bool fwd_valid_pkt = true;
static bool fwd_error_pkt = false;
static bool fwd_nocrc_pkt = false;

/* network configuration variables */
static uint64_t lgwm = 0;
static char serv_addr[64] = DEFAULT_SERVER;
static char serv_port_up[8] = STR(DEFAULT_PORT_UP);
static char serv_port_down[8] = STR(DEFAULT_PORT_DW);
static int keepalive_time = DEFAULT_KEEPALIVE;

/* statistics collection configuration variables */
static unsigned stat_interval = DEFAULT_STAT;

/* gateway <-> MAC protocol variables */
static unsigned int net_mac_h;
static unsigned int net_mac_l;

/* W5500 server destination (replaces BSD sockaddr_in) */
static uint8_t  ns_ip[4] = {0};  /* NS server IP from gateway_config */
static uint16_t ns_port_up = DEFAULT_PORT_UP;
static uint16_t ns_port_down = DEFAULT_PORT_DW;

/* hardware access control and correction */
SemaphoreHandle_t mx_concent;
static SemaphoreHandle_t mx_xcorr;
static bool xtal_correct_ok = false;
static double xtal_correct = 1.0;

/* GPS configuration and synchronization — disabled */
static bool gps_enabled = false;
static bool gps_ref_valid = false;

/* Reference coordinates, for broadcasting (beacon) */
static struct coord_s reference_coord;
static bool gps_fake_enable = false;

/* measurements to establish statistics */
static SemaphoreHandle_t mx_meas_up;
static unsigned int meas_nb_rx_rcv = 0;
static unsigned int meas_nb_rx_ok = 0;
static unsigned int meas_nb_rx_bad = 0;
static unsigned int meas_nb_rx_nocrc = 0;
static unsigned int meas_up_pkt_fwd = 0;
static unsigned int meas_up_network_byte = 0;
static unsigned int meas_up_payload_byte = 0;
static unsigned int meas_up_dgram_sent = 0;
static unsigned int meas_up_ack_rcv = 0;

static SemaphoreHandle_t mx_meas_dw;
static unsigned int meas_dw_pull_sent = 0;
static unsigned int meas_dw_ack_rcv = 0;
static unsigned int meas_dw_dgram_rcv = 0;
static unsigned int meas_dw_network_byte = 0;
static unsigned int meas_dw_payload_byte = 0;
static unsigned int meas_nb_tx_ok = 0;
static unsigned int meas_nb_tx_fail = 0;
static unsigned int meas_nb_tx_requested = 0;
static unsigned int meas_nb_tx_rejected_collision_packet = 0;
static unsigned int meas_nb_tx_rejected_collision_beacon = 0;
static unsigned int meas_nb_tx_rejected_too_late = 0;
static unsigned int meas_nb_tx_rejected_too_early = 0;
static unsigned int meas_nb_beacon_queued = 0;
static unsigned int meas_nb_beacon_sent = 0;
static unsigned int meas_nb_beacon_rejected = 0;

static SemaphoreHandle_t mx_stat_rep;
static bool report_ready = false;
static char status_report[STATUS_SIZE];

/* beacon parameters */
static unsigned int beacon_period = 0;
static unsigned int beacon_freq_hz = DEFAULT_BEACON_FREQ_HZ;
static uint8_t beacon_freq_nb = DEFAULT_BEACON_FREQ_NB;
static unsigned int beacon_freq_step = DEFAULT_BEACON_FREQ_STEP;
static uint8_t beacon_datarate = DEFAULT_BEACON_DATARATE;
static unsigned int beacon_bw_hz = DEFAULT_BEACON_BW_HZ;
static int8_t beacon_power = DEFAULT_BEACON_POWER;
static uint8_t beacon_infodesc = DEFAULT_BEACON_INFODESC;

/* auto-quit function */
static unsigned int autoquit_threshold = 0;
static unsigned int autoquit_cnt = 0;

/* Just In Time TX scheduling */
static struct jit_queue_s jit_queue[LGW_RF_CHAIN_NB];

/* Gateway specificities */
static int8_t antenna_gain = 0;

/* TX capabilities */
static struct lgw_tx_gain_lut_s txlut[LGW_RF_CHAIN_NB];
static unsigned int tx_freq_min[LGW_RF_CHAIN_NB];
static unsigned int tx_freq_max[LGW_RF_CHAIN_NB];
static bool tx_enable[LGW_RF_CHAIN_NB] = {false};

static unsigned int nb_pkt_log[LGW_IF_CHAIN_NB][8];
static unsigned int nb_pkt_received_lora = 0;
static unsigned int nb_pkt_received_fsk = 0;

static struct lgw_conf_debug_s debugconf;
static unsigned int nb_pkt_received_ref[16];

/* Interface type */
static lgw_com_type_t com_type = LGW_COM_SPI;

/* Spectral Scan */
typedef struct spectral_scan_s {
    bool enable;
    unsigned int freq_hz_start;
    uint8_t nb_chan;
    uint16_t nb_scan;
    unsigned int pace_s;
} spectral_scan_t;

static spectral_scan_t spectral_scan_params = {
    .enable = false,
    .freq_hz_start = 0,
    .nb_chan = 0,
    .nb_scan = 0,
    .pace_s = 10
};

/* Task handles */
static TaskHandle_t pThreadUp = NULL;
static TaskHandle_t pJit = NULL;

/* Uptime counter (seconds since boot, replaces time(NULL)) */
static volatile uint32_t uptime_sec = 0;

/* -------------------------------------------------------------------------- */
/*  OLED helper — show one line (pad to 21 chars)                             */
/* -------------------------------------------------------------------------- */
static void oled_show_one_line(uint8_t col, uint8_t row, const char *str)
{
    char buf[N_CHAR_A_ROW + 1];
    memset(buf, ' ', N_CHAR_A_ROW);
    buf[N_CHAR_A_ROW] = '\0';

    int len = strlen(str);
    if (len > N_CHAR_A_ROW) len = N_CHAR_A_ROW;
    memcpy(buf, str, len);

    oled_draw_string(col, row, buf);
    oled_refresh();
}

/* -------------------------------------------------------------------------- */
/*  Forward declarations                                                      */
/* -------------------------------------------------------------------------- */
static int parse_SX130x_configuration(const char *conf_array);
static int parse_gateway_configuration(const char *conf_array);
static int parse_debug_configuration(const char *conf_array);
static uint16_t crc16(const uint8_t *data, unsigned size);
static int get_tx_gain_lut_index(uint8_t rf_chain, int8_t rf_power, uint8_t *lut_index);

/* threads */
void thread_up(void *arg);
void thread_down(void *arg);
void thread_jit(void *arg);

static void thread_cli(void *arg)
{
    (void)arg;
    for (;;) {
        uart_cli_task();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* -------------------------------------------------------------------------- */
/*  Time helpers: replace clock_gettime(CLOCK_MONOTONIC, ...) and time(NULL)  */
/*                                                                            */
/*  Uses FreeRTOS tick count for monotonic timing.                            */
/* -------------------------------------------------------------------------- */

/* Monotonic timestamp in milliseconds (wraps at ~49 days) */
static inline uint32_t monotonic_ms(void)
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/* "difftimespec" replacement — returns elapsed time in seconds (as double)
 * between two monotonic_ms() snapshots. */
static double difftime_ms(uint32_t end_ms, uint32_t begin_ms)
{
    int32_t diff = (int32_t)(end_ms - begin_ms);
    return (double)diff / 1000.0;
}

/* uptime in seconds (for stat timestamps) — incremented by the stats loop */
static uint32_t get_uptime_sec(void)
{
    /* Use tick count for a more accurate uptime */
    return (uint32_t)(xTaskGetTickCount() / (configTICK_RATE_HZ));
}


/* ========================================================================== */
/* ===                  PACKET FORWARDER CORE FUNCTIONS                   === */
/* ========================================================================== */

/* -------------------------------------------------------------------------- */
/* --- CONFIGURATION PARSING (identical to ESP32 version) ------------------- */
/* -------------------------------------------------------------------------- */

static int parse_SX130x_configuration(const char * conf_array) {
    int i, j, number;
    char param_name[40];
    const char *str;
    const char conf_obj_name[] = "SX130x_conf";
    JSON_Value *root_val = NULL;
    JSON_Value *val = NULL;
    JSON_Object *conf_obj = NULL;
    JSON_Object *conf_txgain_obj;
    JSON_Object *conf_ts_obj;
    JSON_Object *conf_sx1261_obj = NULL;
    JSON_Object *conf_scan_obj = NULL;
    JSON_Object *conf_lbt_obj = NULL;
    JSON_Object *conf_lbtchan_obj = NULL;
    JSON_Array *conf_txlut_array = NULL;
    JSON_Array *conf_lbtchan_array = NULL;
    JSON_Array *conf_demod_array = NULL;

    struct lgw_conf_board_s boardconf;
    struct lgw_conf_rxrf_s rfconf;
    struct lgw_conf_rxif_s ifconf;
    struct lgw_conf_demod_s demodconf;
    struct lgw_conf_ftime_s tsconf;
    struct lgw_conf_sx1261_s sx1261conf;
    unsigned int sf, bw, fdev;
    bool sx1250_tx_lut;
    size_t size;

    /* try to parse JSON */
    root_val = json_parse_array_with_comments(conf_array);
    if (root_val == NULL) {
        MSG("ERROR: conf array is not a valid JSON string\n");
        return -1;
    }

    /* point to the gateway configuration object */
    conf_obj = json_object_get_object(json_value_get_object(root_val), conf_obj_name);
    if (conf_obj == NULL) {
        MSG("INFO: conf array does not contain a JSON object named %s\n", conf_obj_name);
        return -1;
    } else {
        MSG("INFO: conf array does contain a JSON object named %s, parsing SX1302 parameters\n", conf_obj_name);
    }

    /* set board configuration */
    memset(&boardconf, 0, sizeof boardconf);
    str = json_object_get_string(conf_obj, "com_type");
    if (str == NULL) {
        MSG("ERROR: com_type must be configured in conf array\n");
        return -1;
    } else if (!strncmp(str, "SPI", 3) || !strncmp(str, "spi", 3)) {
        boardconf.com_type = LGW_COM_SPI;
    } else if (!strncmp(str, "USB", 3) || !strncmp(str, "usb", 3)) {
        boardconf.com_type = LGW_COM_USB;
    } else {
        MSG("ERROR: invalid com type: %s (should be SPI or USB)\n", str);
        return -1;
    }
    com_type = boardconf.com_type;
    str = json_object_get_string(conf_obj, "com_path");
    if (str != NULL) {
        strncpy(boardconf.com_path, str, sizeof boardconf.com_path);
        boardconf.com_path[sizeof boardconf.com_path - 1] = '\0';
    } else {
        MSG("ERROR: com_path must be configured in conf array\n");
        return -1;
    }
    val = json_object_get_value(conf_obj, "lorawan_public");
    if (json_value_get_type(val) == JSONBoolean) {
        boardconf.lorawan_public = (bool)json_value_get_boolean(val);
    } else {
        MSG("WARNING: Data type for lorawan_public seems wrong, please check\n");
        boardconf.lorawan_public = false;
    }
    val = json_object_get_value(conf_obj, "clksrc");
    if (json_value_get_type(val) == JSONNumber) {
        boardconf.clksrc = (uint8_t)json_value_get_number(val);
    } else {
        MSG("WARNING: Data type for clksrc seems wrong, please check\n");
        boardconf.clksrc = 0;
    }
    val = json_object_get_value(conf_obj, "full_duplex");
    if (json_value_get_type(val) == JSONBoolean) {
        boardconf.full_duplex = (bool)json_value_get_boolean(val);
    } else {
        MSG("WARNING: Data type for full_duplex seems wrong, please check\n");
        boardconf.full_duplex = false;
    }
    MSG("INFO: com_type %s, com_path %s, lorawan_public %d, clksrc %d, full_duplex %d\n",
            (boardconf.com_type == LGW_COM_SPI) ? "SPI" : "USB",
            boardconf.com_path, boardconf.lorawan_public, boardconf.clksrc,
            boardconf.full_duplex);
    if (lgw_board_setconf(&boardconf) != LGW_HAL_SUCCESS) {
        MSG("ERROR: Failed to configure board\n");
        return -1;
    }

    /* set antenna gain configuration */
    val = json_object_get_value(conf_obj, "antenna_gain");
    if (val != NULL) {
        if (json_value_get_type(val) == JSONNumber) {
            antenna_gain = (int8_t)json_value_get_number(val);
        } else {
            MSG("WARNING: Data type for antenna_gain seems wrong, please check\n");
            antenna_gain = 0;
        }
    }
    MSG("INFO: antenna_gain %d dBi\n", antenna_gain);

    /* set timestamp configuration */
    conf_ts_obj = json_object_get_object(conf_obj, "fine_timestamp");
    if (conf_ts_obj == NULL) {
        MSG("INFO: conf array does not contain a JSON object for fine timestamp\n");
    } else {
        val = json_object_get_value(conf_ts_obj, "enable");
        if (json_value_get_type(val) == JSONBoolean) {
            tsconf.enable = (bool)json_value_get_boolean(val);
        } else {
            MSG("WARNING: Data type for fine_timestamp.enable seems wrong, please check\n");
            tsconf.enable = false;
        }
        if (tsconf.enable == true) {
            str = json_object_get_string(conf_ts_obj, "mode");
            if (str == NULL) {
                MSG("ERROR: fine_timestamp.mode must be configured\n");
                return -1;
            } else if (!strncmp(str, "high_capacity", 13) || !strncmp(str, "HIGH_CAPACITY", 13)) {
                tsconf.mode = LGW_FTIME_MODE_HIGH_CAPACITY;
            } else if (!strncmp(str, "all_sf", 6) || !strncmp(str, "ALL_SF", 6)) {
                tsconf.mode = LGW_FTIME_MODE_ALL_SF;
            } else {
                MSG("ERROR: invalid fine_timestamp.mode: %s\n", str);
                return -1;
            }
            MSG("INFO: fine_timestamp enabled, mode %s\n", str);
        } else {
            MSG("INFO: fine_timestamp disabled\n");
        }
        if (lgw_ftime_setconf(&tsconf) != LGW_HAL_SUCCESS) {
            MSG("ERROR: Failed to configure fine timestamp\n");
            return -1;
        }
    }

    /* set SX1261 configuration */
    conf_sx1261_obj = json_object_get_object(conf_obj, "sx1261_conf");
    if (conf_sx1261_obj == NULL) {
        MSG("INFO: no sx1261 configuration\n");
    } else {
        memset(&sx1261conf, 0, sizeof sx1261conf);
        val = json_object_get_value(conf_sx1261_obj, "spi_path");
        if (json_value_get_type(val) == JSONString) {
            str = json_value_get_string(val);
            strncpy(sx1261conf.spi_path, str, sizeof sx1261conf.spi_path);
            sx1261conf.spi_path[sizeof sx1261conf.spi_path - 1] = '\0';
        } else {
            MSG("WARNING: Data type for sx1261_conf.spi_path seems wrong, please check\n");
        }
        val = json_object_get_value(conf_sx1261_obj, "rssi_offset");
        if (json_value_get_type(val) == JSONNumber) {
            sx1261conf.rssi_offset = (int8_t)json_value_get_number(val);
        } else {
            MSG("WARNING: Data type for sx1261_conf.rssi_offset seems wrong, please check\n");
        }
        conf_lbt_obj = json_object_get_object(conf_sx1261_obj, "lbt");
        if (conf_lbt_obj == NULL) {
            MSG("INFO: no LBT configuration\n");
        } else {
            val = json_object_get_value(conf_lbt_obj, "enable");
            if (json_value_get_type(val) == JSONBoolean) {
                sx1261conf.lbt_conf.enable = (bool)json_value_get_boolean(val);
            } else {
                MSG("WARNING: Data type for lbt.enable seems wrong, please check\n");
            }
            if (sx1261conf.lbt_conf.enable == true) {
                val = json_object_get_value(conf_lbt_obj, "rssi_target");
                if (json_value_get_type(val) == JSONNumber) {
                    sx1261conf.lbt_conf.rssi_target = (int8_t)json_value_get_number(val);
                } else {
                    MSG("WARNING: Data type for lbt.rssi_target seems wrong, please check\n");
                }
                conf_lbtchan_array = json_object_get_array(conf_lbt_obj, "channels");
                if (conf_lbtchan_array != NULL) {
                    sx1261conf.lbt_conf.nb_channel = json_array_get_count(conf_lbtchan_array);
                    MSG("INFO: %d LBT channels configured\n", sx1261conf.lbt_conf.nb_channel);
                    for (i = 0; i < (int)sx1261conf.lbt_conf.nb_channel; i++) {
                        conf_lbtchan_obj = json_array_get_object(conf_lbtchan_array, i);
                        sx1261conf.lbt_conf.channels[i].freq_hz = (unsigned int)json_object_get_number(conf_lbtchan_obj, "freq_hz");
                        sx1261conf.lbt_conf.channels[i].bandwidth = (uint8_t)json_object_get_number(conf_lbtchan_obj, "bandwidth");
                        sx1261conf.lbt_conf.channels[i].scan_time_us = (uint16_t)json_object_get_number(conf_lbtchan_obj, "scan_time_us");
                        sx1261conf.lbt_conf.channels[i].transmit_time_ms = (uint16_t)json_object_get_number(conf_lbtchan_obj, "transmit_time_ms");
                    }
                }
            }
        }
        if (lgw_sx1261_setconf(&sx1261conf) != LGW_HAL_SUCCESS) {
            MSG("ERROR: Failed to configure sx1261\n");
            return -1;
        }
    }

    /* set the RF channels configuration */
    for (i = 0; i < LGW_RF_CHAIN_NB; i++) {
        memset(&rfconf, 0, sizeof rfconf);
        snprintf(param_name, sizeof param_name, "radio_%i", i);
        conf_obj = json_object_get_object(json_value_get_object(root_val), conf_obj_name);
        conf_obj = json_object_get_object(conf_obj, param_name);
        if (conf_obj == NULL) {
            MSG("INFO: no configuration for radio %i\n", i);
            continue;
        }

        val = json_object_get_value(conf_obj, "enable");
        if (json_value_get_type(val) == JSONBoolean) {
            rfconf.enable = (bool)json_value_get_boolean(val);
        } else {
            rfconf.enable = false;
        }
        if (rfconf.enable == false) {
            MSG("INFO: radio %i disabled\n", i);
        } else {
            rfconf.freq_hz = (unsigned int)json_object_get_number(conf_obj, "freq");
            MSG("INFO: radio %i enabled, center frequency %u\n", i, rfconf.freq_hz);
        }

        val = json_object_get_value(conf_obj, "rssi_offset");
        if (val != NULL) {
            rfconf.rssi_offset = (float)json_value_get_number(val);
            MSG("INFO: radio %i rssi_offset %f\n", i, rfconf.rssi_offset);
        }
        val = json_object_get_value(conf_obj, "rssi_tcomp");
        if (val != NULL) {
            JSON_Object *tcomp = json_value_get_object(val);
            if (tcomp != NULL) {
                rfconf.rssi_tcomp.coeff_a = (float)json_object_get_number(tcomp, "coeff_a");
                rfconf.rssi_tcomp.coeff_b = (float)json_object_get_number(tcomp, "coeff_b");
                rfconf.rssi_tcomp.coeff_c = (float)json_object_get_number(tcomp, "coeff_c");
                rfconf.rssi_tcomp.coeff_d = (float)json_object_get_number(tcomp, "coeff_d");
                rfconf.rssi_tcomp.coeff_e = (float)json_object_get_number(tcomp, "coeff_e");
            }
        }

        str = json_object_get_string(conf_obj, "type");
        if (str == NULL) {
            MSG("WARNING: no type for radio %i, set to SX1250 by default\n", i);
            rfconf.type = LGW_RADIO_TYPE_SX1250;
        } else if (!strncmp(str, "SX1250", 6) || !strncmp(str, "sx1250", 6)) {
            rfconf.type = LGW_RADIO_TYPE_SX1250;
        } else if (!strncmp(str, "SX1255", 6) || !strncmp(str, "sx1255", 6)) {
            rfconf.type = LGW_RADIO_TYPE_SX1255;
        } else if (!strncmp(str, "SX1257", 6) || !strncmp(str, "sx1257", 6)) {
            rfconf.type = LGW_RADIO_TYPE_SX1257;
        } else {
            MSG("WARNING: invalid radio type: %s (should be SX1250, SX1255 or SX1257)\n", str);
        }

        val = json_object_get_value(conf_obj, "single_input_mode");
        if (json_value_get_type(val) == JSONBoolean) {
            rfconf.single_input_mode = (bool)json_value_get_boolean(val);
        } else {
            rfconf.single_input_mode = false;
        }

        /* TX gain LUT */
        sx1250_tx_lut = false;
        conf_txlut_array = json_object_get_array(conf_obj, "tx_gain_lut");
        if (conf_txlut_array != NULL) {
            txlut[i].size = json_array_get_count(conf_txlut_array);
            for (j = 0; j < (int)txlut[i].size; j++) {
                conf_txgain_obj = json_array_get_object(conf_txlut_array, j);
                txlut[i].lut[j].rf_power = (int8_t)json_object_get_number(conf_txgain_obj, "rf_power");
                /* Detect SX1250 LUT by presence of pwr_idx field */
                val = json_object_get_value(conf_txgain_obj, "pwr_idx");
                if (val != NULL) {
                    sx1250_tx_lut = true;
                    txlut[i].lut[j].pwr_idx = (uint8_t)json_value_get_number(val);
                    /* pa_gain may also be present for SX1250 */
                    val = json_object_get_value(conf_txgain_obj, "pa_gain");
                    if (val != NULL) {
                        txlut[i].lut[j].pa_gain = (uint8_t)json_value_get_number(val);
                    }
                    /* SX1250: fill mix_gain with a valid value to pass HAL range check */
                    txlut[i].lut[j].dac_gain = 3;
                    txlut[i].lut[j].mix_gain = 10;
                } else {
                    sx1250_tx_lut = false;
                    val = json_object_get_value(conf_txgain_obj, "pa_gain");
                    if (val != NULL) {
                        txlut[i].lut[j].pa_gain = (uint8_t)json_value_get_number(val);
                    }
                    txlut[i].lut[j].dac_gain = (uint8_t)json_object_dotget_number(conf_txgain_obj, "dac_gain");
                    txlut[i].lut[j].mix_gain = (uint8_t)json_object_dotget_number(conf_txgain_obj, "mix_gain");
                }
            }
            if (lgw_txgain_setconf(i, &txlut[i]) != LGW_HAL_SUCCESS) {
                MSG("ERROR: Failed to configure TX gain LUT for radio %i\n", i);
                return -1;
            }
        } else {
            MSG("WARNING: no TX gain LUT for radio %i\n", i);
        }

        /* TX range */
        val = json_object_get_value(conf_obj, "tx_enable");
        if (json_value_get_type(val) == JSONBoolean) {
            tx_enable[i] = (bool)json_value_get_boolean(val);
        }
        if (tx_enable[i] == true) {
            tx_freq_min[i] = (unsigned int)json_object_get_number(conf_obj, "tx_freq_min");
            tx_freq_max[i] = (unsigned int)json_object_get_number(conf_obj, "tx_freq_max");
            if ((tx_freq_min[i] == 0) || (tx_freq_max[i] == 0)) {
                MSG("WARNING: no tx_freq_min/max for radio %i\n", i);
            }
            MSG("INFO: radio %i TX enabled, freq_min %u, freq_max %u\n", i, tx_freq_min[i], tx_freq_max[i]);
        }

        if (lgw_rxrf_setconf(i, &rfconf) != LGW_HAL_SUCCESS) {
            MSG("ERROR: invalid configuration for radio %i\n", i);
            return -1;
        }
    }

    /* set the IF channels configuration  */
    conf_obj = json_object_get_object(json_value_get_object(root_val), conf_obj_name);
    for (i = 0; i < LGW_MULTI_NB; i++) {
        memset(&ifconf, 0, sizeof ifconf);
        snprintf(param_name, sizeof param_name, "chan_multiSF_%i", i);
        val = json_object_get_value(conf_obj, param_name);
        if (json_value_get_type(val) != JSONObject) {
            MSG("INFO: no configuration for channel %i\n", i);
            continue;
        }
        conf_obj = json_object_get_object(json_value_get_object(root_val), conf_obj_name);
        JSON_Object *chan_obj = json_object_get_object(conf_obj, param_name);
        val = json_object_get_value(chan_obj, "enable");
        if (json_value_get_type(val) == JSONBoolean) {
            ifconf.enable = (bool)json_value_get_boolean(val);
        } else {
            ifconf.enable = false;
        }
        if (ifconf.enable == false) {
            MSG("INFO: channel %i disabled\n", i);
        } else {
            ifconf.rf_chain = (uint8_t)json_object_get_number(chan_obj, "radio");
            ifconf.freq_hz = (int32_t)json_object_get_number(chan_obj, "if");
            MSG("INFO: channel %i enabled, radio %i, IF %i Hz\n", i, ifconf.rf_chain, ifconf.freq_hz);
        }
        if (lgw_rxif_setconf(i, &ifconf) != LGW_HAL_SUCCESS) {
            MSG("ERROR: invalid configuration for channel %i\n", i);
            return -1;
        }
    }

    /* set LoRa Service channel configuration */
    conf_obj = json_object_get_object(json_value_get_object(root_val), conf_obj_name);
    memset(&ifconf, 0, sizeof ifconf);
    val = json_object_get_value(conf_obj, "chan_Lora_std");
    if (json_value_get_type(val) == JSONObject) {
        JSON_Object *chan_obj = json_object_get_object(conf_obj, "chan_Lora_std");
        val = json_object_get_value(chan_obj, "enable");
        if (json_value_get_type(val) == JSONBoolean) {
            ifconf.enable = (bool)json_value_get_boolean(val);
        }
        if (ifconf.enable == true) {
            ifconf.rf_chain = (uint8_t)json_object_get_number(chan_obj, "radio");
            ifconf.freq_hz = (int32_t)json_object_get_number(chan_obj, "if");
            bw = (unsigned int)json_object_get_number(chan_obj, "bandwidth");
            switch(bw) {
                case 500000: ifconf.bandwidth = BW_500KHZ; break;
                case 250000: ifconf.bandwidth = BW_250KHZ; break;
                case 125000: ifconf.bandwidth = BW_125KHZ; break;
                default: ifconf.bandwidth = BW_UNDEFINED;
            }
            sf = (unsigned int)json_object_get_number(chan_obj, "spread_factor");
            switch(sf) {
                case  5: ifconf.datarate = DR_LORA_SF5;  break;
                case  6: ifconf.datarate = DR_LORA_SF6;  break;
                case  7: ifconf.datarate = DR_LORA_SF7;  break;
                case  8: ifconf.datarate = DR_LORA_SF8;  break;
                case  9: ifconf.datarate = DR_LORA_SF9;  break;
                case 10: ifconf.datarate = DR_LORA_SF10; break;
                case 11: ifconf.datarate = DR_LORA_SF11; break;
                case 12: ifconf.datarate = DR_LORA_SF12; break;
                default: ifconf.datarate = DR_UNDEFINED;
            }
            val = json_object_get_value(chan_obj, "implicit_hdr");
            if (json_value_get_type(val) == JSONBoolean) {
                ifconf.implicit_hdr = (bool)json_value_get_boolean(val);
                if (ifconf.implicit_hdr == true) {
                    ifconf.implicit_payload_length = (uint8_t)json_object_get_number(chan_obj, "implicit_payload_length");
                    val = json_object_get_value(chan_obj, "implicit_crc_en");
                    if (json_value_get_type(val) == JSONBoolean) {
                        ifconf.implicit_crc_en = (bool)json_value_get_boolean(val);
                    }
                    ifconf.implicit_coderate = (uint8_t)json_object_get_number(chan_obj, "implicit_coderate");
                }
            }
            MSG("INFO: LoRa Service channel> radio %i, IF %i Hz, BW %u, SF %u\n", ifconf.rf_chain, ifconf.freq_hz, bw, sf);
        }
        if (lgw_rxif_setconf(8, &ifconf) != LGW_HAL_SUCCESS) {
            MSG("ERROR: invalid configuration for LoRa Service channel\n");
            return -1;
        }
    }

    /* set FSK channel configuration */
    conf_obj = json_object_get_object(json_value_get_object(root_val), conf_obj_name);
    memset(&ifconf, 0, sizeof ifconf);
    val = json_object_get_value(conf_obj, "chan_FSK");
    if (json_value_get_type(val) == JSONObject) {
        JSON_Object *chan_obj = json_object_get_object(conf_obj, "chan_FSK");
        val = json_object_get_value(chan_obj, "enable");
        if (json_value_get_type(val) == JSONBoolean) {
            ifconf.enable = (bool)json_value_get_boolean(val);
        }
        if (ifconf.enable == true) {
            ifconf.rf_chain = (uint8_t)json_object_get_number(chan_obj, "radio");
            ifconf.freq_hz = (int32_t)json_object_get_number(chan_obj, "if");
            bw = (unsigned int)json_object_get_number(chan_obj, "bandwidth");
            fdev = (unsigned int)json_object_get_number(chan_obj, "freq_deviation");
            ifconf.datarate = (unsigned int)json_object_get_number(chan_obj, "datarate");
            if ((bw == 0) && (fdev != 0)) {
                bw = 2 * fdev + ifconf.datarate;
            }
            if      (bw == 0)      ifconf.bandwidth = BW_UNDEFINED;
            else if (bw <= 7800)   ifconf.bandwidth = BW_7K8HZ;
            else if (bw <= 15600)  ifconf.bandwidth = BW_15K6HZ;
            else if (bw <= 31200)  ifconf.bandwidth = BW_31K2HZ;
            else if (bw <= 62500)  ifconf.bandwidth = BW_62K5HZ;
            else if (bw <= 125000) ifconf.bandwidth = BW_125KHZ;
            else if (bw <= 250000) ifconf.bandwidth = BW_250KHZ;
            else if (bw <= 500000) ifconf.bandwidth = BW_500KHZ;
            else ifconf.bandwidth = BW_UNDEFINED;
            MSG("INFO: FSK channel> radio %i, IF %i Hz, %u bps, BW %u Hz, FDEV %u Hz\n", ifconf.rf_chain, ifconf.freq_hz, ifconf.datarate, bw, fdev);
        }
        if (lgw_rxif_setconf(9, &ifconf) != LGW_HAL_SUCCESS) {
            MSG("ERROR: invalid configuration for FSK channel\n");
            return -1;
        }
    }

    /* set demodulation parameters */
    conf_obj = json_object_get_object(json_value_get_object(root_val), conf_obj_name);
    memset(&demodconf, 0, sizeof demodconf);
    conf_demod_array = json_object_get_array(conf_obj, "chan_multiSF_All");
    if (conf_demod_array != NULL) {
        size = json_array_get_count(conf_demod_array);
        for (i = 0; i < (int)size; i++) {
            number = json_array_get_number(conf_demod_array, i);
            if (number < 5 || number > 12) {
                MSG("WARNING: failed to parse chan_multiSF_All (wrong value at idx %d)\n", i);
                demodconf.multisf_datarate = 0xFF; /* enable all SFs */
                break;
            } else {
                /* set corresponding bit in the bitmask SF5 is LSB -> SF12 is MSB */
                demodconf.multisf_datarate |= (1 << ((int)number - 5));
            }
        }
    } else {
        /* Enable all multi-SF by default */
        demodconf.multisf_datarate = 0xFF;
    }
    if (lgw_demod_setconf(&demodconf) != LGW_HAL_SUCCESS) {
        MSG("ERROR: Failed to configure demodulation\n");
        return -1;
    }

    json_value_free(root_val);
    return 0;
}


static int parse_gateway_configuration(const char * conf_array) {
    const char conf_obj_name[] = "gateway_conf";
    JSON_Value *root_val = NULL;
    JSON_Object *conf_obj = NULL;
    JSON_Value *val = NULL;
    const char *str;

    /* try to parse JSON */
    root_val = json_parse_array_with_comments(conf_array);
    if (root_val == NULL) {
        MSG("ERROR: conf array is not a valid JSON string\n");
        return -1;
    }

    /* point to the gateway configuration object */
    conf_obj = json_object_get_object(json_value_get_object(root_val), conf_obj_name);
    if (conf_obj == NULL) {
        MSG("INFO: no gateway configuration in JSON\n");
        json_value_free(root_val);
        return -1;
    }
    MSG("INFO: parsing gateway configuration\n");

    /* server hostname or IP address (will be overridden by Flash config) */
    str = json_object_get_string(conf_obj, "server_address");
    if (str != NULL) {
        strncpy(serv_addr, str, sizeof serv_addr);
        serv_addr[sizeof serv_addr - 1] = '\0';
    }
    MSG("INFO: server address from JSON: %s\n", serv_addr);

    /* server port */
    val = json_object_get_value(conf_obj, "serv_port_up");
    if (val != NULL) {
        snprintf(serv_port_up, sizeof serv_port_up, "%u", (unsigned int)json_value_get_number(val));
        ns_port_up = (uint16_t)json_value_get_number(val);
    }
    val = json_object_get_value(conf_obj, "serv_port_down");
    if (val != NULL) {
        snprintf(serv_port_down, sizeof serv_port_down, "%u", (unsigned int)json_value_get_number(val));
        ns_port_down = (uint16_t)json_value_get_number(val);
    }
    MSG("INFO: server ports from JSON: up=%s, down=%s\n", serv_port_up, serv_port_down);

    /* keepalive interval */
    val = json_object_get_value(conf_obj, "keepalive_interval");
    if (val != NULL) {
        keepalive_time = (int)json_value_get_number(val);
        MSG("INFO: keepalive interval is configured to %i sec\n", keepalive_time);
    }

    /* stat interval */
    val = json_object_get_value(conf_obj, "stat_interval");
    if (val != NULL) {
        stat_interval = (unsigned)json_value_get_number(val);
        MSG("INFO: statistics display interval is configured to %u sec\n", stat_interval);
    }

    /* push data timeout */
    val = json_object_get_value(conf_obj, "push_timeout_ms");
    if (val != NULL) {
        MSG("INFO: upstream PUSH_DATA time-out is configured to %u ms\n", (unsigned)json_value_get_number(val));
    }

    /* packet filtering */
    val = json_object_get_value(conf_obj, "forward_crc_valid");
    if (json_value_get_type(val) == JSONBoolean) {
        fwd_valid_pkt = (bool)json_value_get_boolean(val);
    }
    val = json_object_get_value(conf_obj, "forward_crc_error");
    if (json_value_get_type(val) == JSONBoolean) {
        fwd_error_pkt = (bool)json_value_get_boolean(val);
    }
    val = json_object_get_value(conf_obj, "forward_crc_disabled");
    if (json_value_get_type(val) == JSONBoolean) {
        fwd_nocrc_pkt = (bool)json_value_get_boolean(val);
    }

    /* get reference coordinates */
    val = json_object_get_value(conf_obj, "ref_latitude");
    if (val != NULL) {
        reference_coord.lat = (double)json_value_get_number(val);
    }
    val = json_object_get_value(conf_obj, "ref_longitude");
    if (val != NULL) {
        reference_coord.lon = (double)json_value_get_number(val);
    }
    val = json_object_get_value(conf_obj, "ref_altitude");
    if (val != NULL) {
        reference_coord.alt = (short)json_value_get_number(val);
    }

    val = json_object_get_value(conf_obj, "fake_gps");
    if (json_value_get_type(val) == JSONBoolean) {
        gps_fake_enable = (bool)json_value_get_boolean(val);
    }

    /* beacon parameters */
    val = json_object_get_value(conf_obj, "beacon_period");
    if (val != NULL) {
        beacon_period = (unsigned int)json_value_get_number(val);
    }
    val = json_object_get_value(conf_obj, "beacon_freq_hz");
    if (val != NULL) {
        beacon_freq_hz = (unsigned int)json_value_get_number(val);
    }
    val = json_object_get_value(conf_obj, "beacon_freq_nb");
    if (val != NULL) {
        beacon_freq_nb = (uint8_t)json_value_get_number(val);
    }
    val = json_object_get_value(conf_obj, "beacon_freq_step");
    if (val != NULL) {
        beacon_freq_step = (unsigned int)json_value_get_number(val);
    }
    val = json_object_get_value(conf_obj, "beacon_datarate");
    if (val != NULL) {
        beacon_datarate = (uint8_t)json_value_get_number(val);
    }
    val = json_object_get_value(conf_obj, "beacon_bw_hz");
    if (val != NULL) {
        beacon_bw_hz = (unsigned int)json_value_get_number(val);
    }
    val = json_object_get_value(conf_obj, "beacon_power");
    if (val != NULL) {
        beacon_power = (int8_t)json_value_get_number(val);
    }
    val = json_object_get_value(conf_obj, "beacon_infodesc");
    if (val != NULL) {
        beacon_infodesc = (uint8_t)json_value_get_number(val);
    }

    /* auto-quit threshold */
    val = json_object_get_value(conf_obj, "autoquit_threshold");
    if (val != NULL) {
        autoquit_threshold = (unsigned int)json_value_get_number(val);
    }

    /* gateway ID (MAC address) */
    str = json_object_get_string(conf_obj, "gateway_ID");
    if (str != NULL) {
        unsigned long long ull = 0;
        /* newlib-nano does not support %llx; parse as two 32-bit halves */
        unsigned int mac_hi = 0, mac_lo = 0;
        sscanf(str, "%8x%8x", &mac_hi, &mac_lo);
        ull = ((unsigned long long)mac_hi << 32) | mac_lo;
        lgwm = ull;
        MSG("INFO: gateway MAC address from JSON: %08X%08X\n",
            (unsigned int)(lgwm >> 32), (unsigned int)(lgwm & 0xFFFFFFFF));
    }

    json_value_free(root_val);
    return 0;
}


static int parse_debug_configuration(const char * conf_array) {
    int i;
    const char conf_obj_name[] = "debug_conf";
    JSON_Value *root_val;
    JSON_Object *conf_obj = NULL;
    JSON_Array *conf_arr = NULL;
    JSON_Object *conf_obj_array = NULL;
    const char *str;

    memset(&debugconf, 0, sizeof debugconf);

    root_val = json_parse_array_with_comments(conf_array);
    if (root_val == NULL) {
        MSG("ERROR: conf array is not a valid JSON string\n");
        return -1;
    }

    conf_obj = json_object_get_object(json_value_get_object(root_val), conf_obj_name);
    if (conf_obj == NULL) {
        MSG("INFO: no debug configuration in JSON\n");
        json_value_free(root_val);
        return -1;
    }

    conf_arr = json_object_get_array(conf_obj, "ref_payload");
    if (conf_arr != NULL) {
        debugconf.nb_ref_payload = json_array_get_count(conf_arr);
        for (i = 0; i < (int)debugconf.nb_ref_payload; i++) {
            conf_obj_array = json_array_get_object(conf_arr, i);
            str = json_object_get_string(conf_obj_array, "id");
            if (str != NULL) {
                sscanf(str, "0x%08X", &(debugconf.ref_payload[i].id));
            }
            nb_pkt_received_ref[i] = 0;
        }
    }

    if (lgw_debug_setconf(&debugconf) != LGW_HAL_SUCCESS) {
        MSG("ERROR: Failed to configure debug\n");
        json_value_free(root_val);
        return -1;
    }

    json_value_free(root_val);
    return 0;
}


static uint16_t crc16(const uint8_t * data, unsigned size) {
    const uint16_t crc_poly = 0x1021;
    const uint16_t init_val = 0x0000;
    uint16_t x = init_val;
    unsigned i, j;

    if (data == NULL) return 0;

    for (i = 0; i < size; ++i) {
        x ^= (uint16_t)data[i] << 8;
        for (j = 0; j < 8; ++j) {
            x = (x & 0x8000) ? (x << 1) ^ crc_poly : (x << 1);
        }
    }
    return x;
}


/* -------------------------------------------------------------------------- */
/* --- TX ACK ---------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static int send_tx_ack(uint8_t token_h, uint8_t token_l, enum jit_error_e error, signed int error_value) {
    uint8_t buff_ack[ACK_BUFF_SIZE];
    int buff_index;
    int j;

    memset(&buff_ack, 0, sizeof buff_ack);

    buff_ack[0] = PROTOCOL_VERSION;
    buff_ack[1] = token_h;
    buff_ack[2] = token_l;
    buff_ack[3] = PKT_TX_ACK;
    *(unsigned int *)(buff_ack + 4) = net_mac_h;
    *(unsigned int *)(buff_ack + 8) = net_mac_l;
    buff_index = 12;

    if (error != JIT_ERROR_OK) {
        memcpy((void *)(buff_ack + buff_index), (void *)"{\"txpk_ack\":{", 13);
        buff_index += 13;
        switch (error) {
            case JIT_ERROR_TX_POWER:
                memcpy((void *)(buff_ack + buff_index), (void *)"\"warn\":", 7);
                buff_index += 7;
                break;
            default:
                memcpy((void *)(buff_ack + buff_index), (void *)"\"error\":", 8);
                buff_index += 8;
                break;
        }
        switch (error) {
            case JIT_ERROR_FULL:
            case JIT_ERROR_COLLISION_PACKET:
                memcpy((void *)(buff_ack + buff_index), (void *)"\"COLLISION_PACKET\"", 18);
                buff_index += 18;
                xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
                meas_nb_tx_rejected_collision_packet += 1;
                xSemaphoreGive(mx_meas_dw);
                break;
            case JIT_ERROR_TOO_LATE:
                memcpy((void *)(buff_ack + buff_index), (void *)"\"TOO_LATE\"", 10);
                buff_index += 10;
                xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
                meas_nb_tx_rejected_too_late += 1;
                xSemaphoreGive(mx_meas_dw);
                break;
            case JIT_ERROR_TOO_EARLY:
                memcpy((void *)(buff_ack + buff_index), (void *)"\"TOO_EARLY\"", 11);
                buff_index += 11;
                xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
                meas_nb_tx_rejected_too_early += 1;
                xSemaphoreGive(mx_meas_dw);
                break;
            case JIT_ERROR_COLLISION_BEACON:
                memcpy((void *)(buff_ack + buff_index), (void *)"\"COLLISION_BEACON\"", 18);
                buff_index += 18;
                xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
                meas_nb_tx_rejected_collision_beacon += 1;
                xSemaphoreGive(mx_meas_dw);
                break;
            case JIT_ERROR_TX_FREQ:
                memcpy((void *)(buff_ack + buff_index), (void *)"\"TX_FREQ\"", 9);
                buff_index += 9;
                break;
            case JIT_ERROR_TX_POWER:
                memcpy((void *)(buff_ack + buff_index), (void *)"\"TX_POWER\"", 10);
                buff_index += 10;
                break;
            case JIT_ERROR_GPS_UNLOCKED:
                memcpy((void *)(buff_ack + buff_index), (void *)"\"GPS_UNLOCKED\"", 14);
                buff_index += 14;
                break;
            default:
                memcpy((void *)(buff_ack + buff_index), (void *)"\"UNKNOWN\"", 9);
                buff_index += 9;
                break;
        }
        switch (error) {
            case JIT_ERROR_TX_POWER:
                j = snprintf((char *)(buff_ack + buff_index), ACK_BUFF_SIZE-buff_index, ",\"value\":%d", error_value);
                if (j > 0) buff_index += j;
                break;
            default:
                break;
        }
        memcpy((void *)(buff_ack + buff_index), (void *)"}}", 2);
        buff_index += 2;
    }

    buff_ack[buff_index] = 0;

    return net_udp_send_default(NET_SOCK_DOWN, buff_ack, buff_index);
}


/* -------------------------------------------------------------------------- */
/* --- TX GAIN LUT INDEX ----------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static int get_tx_gain_lut_index(uint8_t rf_chain, int8_t rf_power, uint8_t *lut_index) {
    uint8_t pow_index;
    int current_best_index = -1;
    uint8_t current_best_match = 0xFF;
    int diff;

    if (lut_index == NULL) {
        MSG("ERROR: %s - wrong parameter\n", __FUNCTION__);
        return -1;
    }

    for (pow_index = 0; pow_index < txlut[rf_chain].size; pow_index++) {
        diff = rf_power - txlut[rf_chain].lut[pow_index].rf_power;
        if (diff < 0) {
            continue;
        }
        if ((unsigned int)diff < current_best_match) {
            current_best_match = diff;
            current_best_index = pow_index;
        }
    }

    if (current_best_index == -1) {
        *lut_index = 0;
        MSG("ERROR: %s - unable to find suitable TX gain LUT index\n", __FUNCTION__);
        return -1;
    }

    *lut_index = (uint8_t)current_best_index;
    return 0;
}


/* -------------------------------------------------------------------------- */
/* --- THREAD 1: RECEIVING PACKETS AND FORWARDING THEM ---------------------- */
/* -------------------------------------------------------------------------- */

static uint8_t buff_up[TX_BUFF_SIZE];
static struct lgw_pkt_rx_s rxpkt[NB_PKT_MAX];

void thread_up(void *arg)
{
    (void)arg;
    int i, j, k;
    unsigned pkt_in_dgram;

    struct lgw_pkt_rx_s *p;
    int nb_pkt;

    /* data buffers */
    int buff_index;
    uint8_t buff_ack[32];

    /* protocol variables */
    uint8_t token_h;
    uint8_t token_l;

    /* ping measurement */
    uint32_t send_time_ms;
    uint32_t recv_time_ms;

    /* mote info */
    unsigned int mote_addr = 0;
    uint16_t mote_fcnt = 0;

    /* report management */
    bool send_report = false;

    /* pre-fill the data buffer with fixed fields */
    buff_up[0] = PROTOCOL_VERSION;
    buff_up[3] = PKT_PUSH_DATA;
    *(unsigned int *)(buff_up + 4) = net_mac_h;
    *(unsigned int *)(buff_up + 8) = net_mac_l;

    while (!exit_sig && !quit_sig) {

        /* fetch packets */
        xSemaphoreTake(mx_concent, portMAX_DELAY);
        nb_pkt = lgw_receive(NB_PKT_MAX, rxpkt);
        xSemaphoreGive(mx_concent);
        if (nb_pkt == LGW_HAL_ERROR) {
            MSG("ERROR: [up] failed packet fetch, exiting\n");
            exit_sig = true;
            break;
        }

        /* check if there are status report to send */
        send_report = report_ready;

        /* wait a short time if no packets, nor status report */
        if ((nb_pkt == 0) && (send_report == false)) {
            vTaskDelay(pdMS_TO_TICKS(FETCH_SLEEP_MS));
            continue;
        }

        /* start composing datagram with the header */
        token_h = (uint8_t)rand();
        token_l = (uint8_t)rand();
        buff_up[1] = token_h;
        buff_up[2] = token_l;
        buff_index = 12;

        /* start of JSON structure */
        memcpy((void *)(buff_up + buff_index), (void *)"{\"rxpk\":[", 9);
        buff_index += 9;

        /* serialize Lora packets metadata and payload */
        pkt_in_dgram = 0;
        for (i = 0; i < nb_pkt; ++i) {
            p = &rxpkt[i];

            /* Get mote information */
            if (p->size >= 8) {
                mote_addr  = p->payload[1];
                mote_addr |= p->payload[2] << 8;
                mote_addr |= p->payload[3] << 16;
                mote_addr |= p->payload[4] << 24;
                mote_fcnt  = p->payload[6];
                mote_fcnt |= p->payload[7] << 8;
            } else {
                mote_addr = 0;
                mote_fcnt = 0;
            }

            /* basic packet filtering */
            xSemaphoreTake(mx_meas_up, portMAX_DELAY);
            meas_nb_rx_rcv += 1;
            switch(p->status) {
                case STAT_CRC_OK:
                    meas_nb_rx_ok += 1;
                    if (!fwd_valid_pkt) {
                        xSemaphoreGive(mx_meas_up);
                        continue;
                    }
                    break;
                case STAT_CRC_BAD:
                    meas_nb_rx_bad += 1;
                    if (!fwd_error_pkt) {
                        xSemaphoreGive(mx_meas_up);
                        continue;
                    }
                    break;
                case STAT_NO_CRC:
                    meas_nb_rx_nocrc += 1;
                    if (!fwd_nocrc_pkt) {
                        xSemaphoreGive(mx_meas_up);
                        continue;
                    }
                    break;
                default:
                    MSG("WARNING: [up] received packet with unknown status %u\n", p->status);
                    xSemaphoreGive(mx_meas_up);
                    continue;
            }
            meas_up_pkt_fwd += 1;
            meas_up_payload_byte += p->size;
            xSemaphoreGive(mx_meas_up);
            printf("\nINFO: Received pkt from mote: %08X (fcnt=%u)\n", mote_addr, mote_fcnt);

            /* Start of packet */
            if (pkt_in_dgram == 0) {
                buff_up[buff_index] = '{';
                ++buff_index;
            } else {
                buff_up[buff_index] = ',';
                buff_up[buff_index+1] = '{';
                buff_index += 2;
            }

            /* JSON rxpk frame format version */
            j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, "\"jver\":%d", PROTOCOL_JSON_RXPK_FRAME_FORMAT);
            if (j > 0) buff_index += j;

            /* RAW timestamp */
            j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"tmst\":%u", p->count_us);
            if (j > 0) buff_index += j;

            /* Fine timestamp */
            if (p->ftime_received == true) {
                j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"ftime\":%u", p->ftime);
                if (j > 0) buff_index += j;
            }

            /* Channel, RF chain, frequency, modem_id */
            j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"chan\":%1u,\"rfch\":%1u,\"freq\":%.6lf,\"mid\":%2u", p->if_chain, p->rf_chain, ((double)p->freq_hz / 1e6), p->modem_id);
            if (j > 0) buff_index += j;

            /* Packet status */
            switch (p->status) {
                case STAT_CRC_OK:
                    memcpy((void *)(buff_up + buff_index), (void *)",\"stat\":1", 9);
                    buff_index += 9;
                    break;
                case STAT_CRC_BAD:
                    memcpy((void *)(buff_up + buff_index), (void *)",\"stat\":-1", 10);
                    buff_index += 10;
                    break;
                case STAT_NO_CRC:
                    memcpy((void *)(buff_up + buff_index), (void *)",\"stat\":0", 9);
                    buff_index += 9;
                    break;
                default:
                    memcpy((void *)(buff_up + buff_index), (void *)",\"stat\":?", 9);
                    buff_index += 9;
                    break;
            }

            /* Packet modulation */
            if (p->modulation == MOD_LORA) {
                memcpy((void *)(buff_up + buff_index), (void *)",\"modu\":\"LORA\"", 14);
                buff_index += 14;

                /* Lora datarate */
                switch (p->datarate) {
                    case DR_LORA_SF5:  memcpy((void*)(buff_up+buff_index), ",\"datr\":\"SF5", 12); buff_index+=12; break;
                    case DR_LORA_SF6:  memcpy((void*)(buff_up+buff_index), ",\"datr\":\"SF6", 12); buff_index+=12; break;
                    case DR_LORA_SF7:  memcpy((void*)(buff_up+buff_index), ",\"datr\":\"SF7", 12); buff_index+=12; break;
                    case DR_LORA_SF8:  memcpy((void*)(buff_up+buff_index), ",\"datr\":\"SF8", 12); buff_index+=12; break;
                    case DR_LORA_SF9:  memcpy((void*)(buff_up+buff_index), ",\"datr\":\"SF9", 12); buff_index+=12; break;
                    case DR_LORA_SF10: memcpy((void*)(buff_up+buff_index), ",\"datr\":\"SF10",13); buff_index+=13; break;
                    case DR_LORA_SF11: memcpy((void*)(buff_up+buff_index), ",\"datr\":\"SF11",13); buff_index+=13; break;
                    case DR_LORA_SF12: memcpy((void*)(buff_up+buff_index), ",\"datr\":\"SF12",13); buff_index+=13; break;
                    default:           memcpy((void*)(buff_up+buff_index), ",\"datr\":\"SF?", 12); buff_index+=12; break;
                }
                /* Bandwidth */
                switch (p->bandwidth) {
                    case BW_125KHZ: memcpy((void*)(buff_up+buff_index), "BW125\"", 6); buff_index+=6; break;
                    case BW_250KHZ: memcpy((void*)(buff_up+buff_index), "BW250\"", 6); buff_index+=6; break;
                    case BW_500KHZ: memcpy((void*)(buff_up+buff_index), "BW500\"", 6); buff_index+=6; break;
                    default:        memcpy((void*)(buff_up+buff_index), "BW?\"",   4); buff_index+=4; break;
                }

                /* Coding rate */
                switch (p->coderate) {
                    case CR_LORA_4_5: memcpy((void*)(buff_up+buff_index), ",\"codr\":\"4/5\"", 13); buff_index+=13; break;
                    case CR_LORA_4_6: memcpy((void*)(buff_up+buff_index), ",\"codr\":\"4/6\"", 13); buff_index+=13; break;
                    case CR_LORA_4_7: memcpy((void*)(buff_up+buff_index), ",\"codr\":\"4/7\"", 13); buff_index+=13; break;
                    case CR_LORA_4_8: memcpy((void*)(buff_up+buff_index), ",\"codr\":\"4/8\"", 13); buff_index+=13; break;
                    case 0:           memcpy((void*)(buff_up+buff_index), ",\"codr\":\"OFF\"", 13); buff_index+=13; break;
                    default:          memcpy((void*)(buff_up+buff_index), ",\"codr\":\"?\"",   11); buff_index+=11; break;
                }

                /* Signal RSSI */
                j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"rssis\":%.0f", roundf(p->rssis));
                if (j > 0) buff_index += j;

                /* Lora SNR */
                j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"lsnr\":%.1f", p->snr);
                if (j > 0) buff_index += j;

                /* Frequency offset */
                j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"foff\":%d", p->freq_offset);
                if (j > 0) buff_index += j;

            } else if (p->modulation == MOD_FSK) {
                memcpy((void *)(buff_up + buff_index), (void *)",\"modu\":\"FSK\"", 13);
                buff_index += 13;
                j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"datr\":%u", p->datarate);
                if (j > 0) buff_index += j;
            } else {
                MSG("ERROR: [up] received packet with unknown modulation 0x%02X\n", p->modulation);
                continue;
            }

            /* Channel RSSI, payload size */
            j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, ",\"rssi\":%.0f,\"size\":%u", roundf(p->rssic), p->size);
            if (j > 0) buff_index += j;

            /* Packet base64-encoded payload */
            memcpy((void *)(buff_up + buff_index), (void *)",\"data\":\"", 9);
            buff_index += 9;
            j = bin_to_b64(p->payload, p->size, (char *)(buff_up + buff_index), 341);
            if (j >= 0) {
                buff_index += j;
            } else {
                MSG("ERROR: [up] bin_to_b64 failed\n");
                continue;
            }
            buff_up[buff_index] = '"';
            ++buff_index;

            /* End of packet */
            buff_up[buff_index] = '}';
            ++buff_index;
            ++pkt_in_dgram;

            /* log */
            if (p->modulation == MOD_LORA) {
                nb_pkt_log[p->if_chain][p->datarate - 5] += 1;
                nb_pkt_received_lora += 1;
            } else if (p->modulation == MOD_FSK) {
                nb_pkt_log[p->if_chain][0] += 1;
                nb_pkt_received_fsk += 1;
            }

            for (k = 0; k < (int)debugconf.nb_ref_payload; k++) {
                if ((p->payload[0] == (uint8_t)(debugconf.ref_payload[k].id >> 24)) &&
                    (p->payload[1] == (uint8_t)(debugconf.ref_payload[k].id >> 16)) &&
                    (p->payload[2] == (uint8_t)(debugconf.ref_payload[k].id >> 8))  &&
                    (p->payload[3] == (uint8_t)(debugconf.ref_payload[k].id >> 0))) {
                        nb_pkt_received_ref[k] += 1;
                    }
            }
        }

        /* restart fetch sequence without sending empty JSON if all filtered out */
        if (pkt_in_dgram == 0) {
            if (send_report == true) {
                buff_index -= 8;
            } else {
                continue;
            }
        } else {
            buff_up[buff_index] = ']';
            ++buff_index;
            if (send_report == true) {
                buff_up[buff_index] = ',';
                ++buff_index;
            }
        }

        /* add status report if available */
        if (send_report == true) {
            xSemaphoreTake(mx_stat_rep, portMAX_DELAY);
            report_ready = false;
            j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE-buff_index, "%s", status_report);
            xSemaphoreGive(mx_stat_rep);
            if (j > 0) {
                buff_index += j;
            } else {
                MSG("ERROR: [up] snprintf failed line %u\n", (__LINE__ - 5));
            }
        }

        /* end of JSON datagram payload */
        buff_up[buff_index] = '}';
        ++buff_index;
        buff_up[buff_index] = 0;

        printf("\nJSON up: %s\n", (char *)(buff_up + 12));

        /* drain stale ACKs */
        net_udp_drain(NET_SOCK_UP);

        /* send datagram to server */
        net_udp_send_default(NET_SOCK_UP, buff_up, buff_index);

        send_time_ms = monotonic_ms();
        xSemaphoreTake(mx_meas_up, portMAX_DELAY);
        meas_up_dgram_sent += 1;
        meas_up_network_byte += buff_index;

        /* wait for acknowledge (in 2 times, to catch extra packets) */
        for (i = 0; i < 2; ++i) {
            j = net_udp_recv(NET_SOCK_UP, buff_ack, sizeof buff_ack, PUSH_TIMEOUT_MS / 2);
            recv_time_ms = monotonic_ms();
            if (j <= 0) {
                continue; /* timeout */
            } else if ((j < 4) || (buff_ack[0] != PROTOCOL_VERSION) || (buff_ack[3] != PKT_PUSH_ACK)) {
                MSG("WARNING: [up] ignored invalid non-ACK packet\n");
                continue;
            } else if ((buff_ack[1] != token_h) || (buff_ack[2] != token_l)) {
                MSG("WARNING: [up] ignored out-of-sync ACK packet\n");
                continue;
            } else {
                MSG("INFO: [up] PUSH_ACK received in %u ms\n", (unsigned)(recv_time_ms - send_time_ms));
                meas_up_ack_rcv += 1;
                break;
            }
        }
        xSemaphoreGive(mx_meas_up);
    }
    MSG("\nINFO: End of upstream thread\n");
    vTaskDelete(NULL);
}


/* -------------------------------------------------------------------------- */
/* --- THREAD 2: POLLING SERVER AND ENQUEUING PACKETS IN JIT QUEUE ---------- */
/* -------------------------------------------------------------------------- */

void thread_down(void *arg)
{
    (void)arg;
    int i;
    int msg_len;

    /* data buffers */
    uint8_t buff_down[1000];
    uint8_t buff_req[12];

    /* protocol variables */
    uint8_t token_h;
    uint8_t token_l;

    /* ping measurement */
    uint32_t send_time_ms;
    uint32_t recv_time_ms;

    /* auto-quit variables */
    bool req_ack = false;

    /* JSON parsing */
    JSON_Value *root_val = NULL;
    JSON_Object *txpk_obj = NULL;
    JSON_Value *val;
    const char *str;

    /* TX packet */
    struct lgw_pkt_tx_s txpkt;
    bool sent_immediate = false;
    short x0, x1;

    unsigned int current_concentrator_time;
    enum jit_error_e jit_result = JIT_ERROR_OK;
    enum jit_pkt_type_e downlink_type;
    enum jit_error_e warning_result = JIT_ERROR_OK;
    signed int warning_value = 0;
    uint8_t tx_lut_idx = 0;

    /* pre-fill the pull request buffer */
    buff_req[0] = PROTOCOL_VERSION;
    buff_req[3] = PKT_PULL_DATA;
    *(unsigned int *)(buff_req + 4) = net_mac_h;
    *(unsigned int *)(buff_req + 8) = net_mac_l;

    while (!exit_sig && !quit_sig) {

        /* auto-quit if the threshold is crossed */
        if ((autoquit_threshold > 0) && (autoquit_cnt >= autoquit_threshold)) {
            exit_sig = true;
            MSG("INFO: [down] the last %u PULL_DATA were not ACKed, exiting\n", autoquit_threshold);
            break;
        }

        /* generate random token for request */
        token_h = (uint8_t)rand();
        token_l = (uint8_t)rand();
        buff_req[1] = token_h;
        buff_req[2] = token_l;

        /* send PULL request and record time */
        net_udp_send_default(NET_SOCK_DOWN, buff_req, sizeof buff_req);
        send_time_ms = monotonic_ms();
        xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
        meas_dw_pull_sent += 1;
        xSemaphoreGive(mx_meas_dw);
        req_ack = false;
        autoquit_cnt++;

        /* listen to packets until a new PULL request must be sent */
        recv_time_ms = send_time_ms;
        while ((difftime_ms(recv_time_ms, send_time_ms) < (double)keepalive_time) && !exit_sig && !quit_sig) {

            /* try to receive a datagram */
            msg_len = net_udp_recv(NET_SOCK_DOWN, buff_down, (sizeof buff_down) - 1, PULL_TIMEOUT_MS);
            recv_time_ms = monotonic_ms();

            /* if no network message was received, loop back */
            if (msg_len <= 0) {
                continue;
            }

            /* if the datagram does not respect protocol, just ignore it */
            if ((msg_len < 4) || (buff_down[0] != PROTOCOL_VERSION) || ((buff_down[3] != PKT_PULL_RESP) && (buff_down[3] != PKT_PULL_ACK))) {
                MSG("WARNING: [down] ignoring invalid packet len=%d, protocol_version=%d, id=%d\n",
                        msg_len, buff_down[0], buff_down[3]);
                continue;
            }

            /* if the datagram is an ACK, check token */
            if (buff_down[3] == PKT_PULL_ACK) {
                if ((buff_down[1] == token_h) && (buff_down[2] == token_l)) {
                    if (req_ack) {
                        MSG("INFO: [down] duplicate ACK received :)\n");
                    } else {
                        req_ack = true;
                        autoquit_cnt = 0;
                        xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
                        meas_dw_ack_rcv += 1;
                        xSemaphoreGive(mx_meas_dw);
                        MSG("INFO: [down] PULL_ACK received in %u ms\n", (unsigned)(recv_time_ms - send_time_ms));
                    }
                } else {
                    MSG("INFO: [down] received out-of-sync ACK\n");
                }
                continue;
            }

            /* the datagram is a PULL_RESP */
            buff_down[msg_len] = 0;
            MSG("INFO: [down] PULL_RESP received  - token[%d:%d] :)\n", buff_down[1], buff_down[2]);
            printf("\nJSON down: %s\n", (char *)(buff_down + 4));

            /* initialize TX struct and try to parse JSON */
            memset(&txpkt, 0, sizeof txpkt);
            root_val = json_parse_string_with_comments((const char *)(buff_down + 4));
            if (root_val == NULL) {
                MSG("WARNING: [down] invalid JSON, TX aborted\n");
                continue;
            }

            /* look for JSON sub-object 'txpk' */
            txpk_obj = json_object_get_object(json_value_get_object(root_val), "txpk");
            if (txpk_obj == NULL) {
                MSG("WARNING: [down] no \"txpk\" object in JSON, TX aborted\n");
                json_value_free(root_val);
                continue;
            }

            /* Parse "immediate" tag */
            i = json_object_get_boolean(txpk_obj, "imme");
            if (i == 1) {
                sent_immediate = true;
                downlink_type = JIT_PKT_TYPE_DOWNLINK_CLASS_C;
                MSG("INFO: [down] a packet will be sent in \"immediate\" mode\n");
            } else {
                sent_immediate = false;
                val = json_object_get_value(txpk_obj, "tmst");
                if (val != NULL) {
                    txpkt.count_us = (unsigned int)json_value_get_number(val);
                    downlink_type = JIT_PKT_TYPE_DOWNLINK_CLASS_A;
                } else {
                    /* GPS time not supported on STM32 port */
                    MSG("WARNING: [down] no \"txpk.tmst\" in JSON. GPS timing not supported, TX aborted\n");
                    json_value_free(root_val);
                    send_tx_ack(buff_down[1], buff_down[2], JIT_ERROR_GPS_UNLOCKED, 0);
                    continue;
                }
            }

            /* Parse "No CRC" flag */
            val = json_object_get_value(txpk_obj, "ncrc");
            if (val != NULL) {
                txpkt.no_crc = (bool)json_value_get_boolean(val);
            }

            /* Parse "No header" flag */
            val = json_object_get_value(txpk_obj, "nhdr");
            if (val != NULL) {
                txpkt.no_header = (bool)json_value_get_boolean(val);
            }

            /* parse target frequency */
            val = json_object_get_value(txpk_obj, "freq");
            if (val == NULL) {
                MSG("WARNING: [down] no mandatory \"txpk.freq\" in JSON, TX aborted\n");
                json_value_free(root_val);
                continue;
            }
            txpkt.freq_hz = (unsigned int)((double)(1.0e6) * json_value_get_number(val));

            /* parse RF chain */
            val = json_object_get_value(txpk_obj, "rfch");
            if (val == NULL) {
                MSG("WARNING: [down] no mandatory \"txpk.rfch\" in JSON, TX aborted\n");
                json_value_free(root_val);
                continue;
            }
            txpkt.rf_chain = (uint8_t)json_value_get_number(val);
            if (tx_enable[txpkt.rf_chain] == false) {
                MSG("WARNING: [down] TX not enabled on RF chain %u, TX aborted\n", txpkt.rf_chain);
                json_value_free(root_val);
                continue;
            }

            /* parse TX power */
            val = json_object_get_value(txpk_obj, "powe");
            if (val != NULL) {
                txpkt.rf_power = (int8_t)json_value_get_number(val) - antenna_gain;
            }

            /* Parse modulation */
            str = json_object_get_string(txpk_obj, "modu");
            if (str == NULL) {
                MSG("WARNING: [down] no \"txpk.modu\" in JSON, TX aborted\n");
                json_value_free(root_val);
                continue;
            }
            if (strcmp(str, "LORA") == 0) {
                txpkt.modulation = MOD_LORA;
                str = json_object_get_string(txpk_obj, "datr");
                if (str == NULL) {
                    MSG("WARNING: [down] no \"txpk.datr\" in JSON, TX aborted\n");
                    json_value_free(root_val);
                    continue;
                }
                i = sscanf(str, "SF%2hdBW%3hd", &x0, &x1);
                if (i != 2) {
                    MSG("WARNING: [down] format error in \"txpk.datr\", TX aborted\n");
                    json_value_free(root_val);
                    continue;
                }
                switch (x0) {
                    case  5: txpkt.datarate = DR_LORA_SF5;  break;
                    case  6: txpkt.datarate = DR_LORA_SF6;  break;
                    case  7: txpkt.datarate = DR_LORA_SF7;  break;
                    case  8: txpkt.datarate = DR_LORA_SF8;  break;
                    case  9: txpkt.datarate = DR_LORA_SF9;  break;
                    case 10: txpkt.datarate = DR_LORA_SF10; break;
                    case 11: txpkt.datarate = DR_LORA_SF11; break;
                    case 12: txpkt.datarate = DR_LORA_SF12; break;
                    default:
                        MSG("WARNING: [down] invalid SF in \"txpk.datr\", TX aborted\n");
                        json_value_free(root_val);
                        continue;
                }
                switch (x1) {
                    case 125: txpkt.bandwidth = BW_125KHZ; break;
                    case 250: txpkt.bandwidth = BW_250KHZ; break;
                    case 500: txpkt.bandwidth = BW_500KHZ; break;
                    default:
                        MSG("WARNING: [down] invalid BW in \"txpk.datr\", TX aborted\n");
                        json_value_free(root_val);
                        continue;
                }

                str = json_object_get_string(txpk_obj, "codr");
                if (str == NULL) {
                    MSG("WARNING: [down] no \"txpk.codr\" in JSON, TX aborted\n");
                    json_value_free(root_val);
                    continue;
                }
                if      (strcmp(str, "4/5") == 0) txpkt.coderate = CR_LORA_4_5;
                else if (strcmp(str, "4/6") == 0) txpkt.coderate = CR_LORA_4_6;
                else if (strcmp(str, "2/3") == 0) txpkt.coderate = CR_LORA_4_6;
                else if (strcmp(str, "4/7") == 0) txpkt.coderate = CR_LORA_4_7;
                else if (strcmp(str, "4/8") == 0) txpkt.coderate = CR_LORA_4_8;
                else if (strcmp(str, "1/2") == 0) txpkt.coderate = CR_LORA_4_8;
                else {
                    MSG("WARNING: [down] invalid \"txpk.codr\", TX aborted\n");
                    json_value_free(root_val);
                    continue;
                }

                val = json_object_get_value(txpk_obj, "ipol");
                if (val != NULL) txpkt.invert_pol = (bool)json_value_get_boolean(val);

                val = json_object_get_value(txpk_obj, "prea");
                if (val != NULL) {
                    i = (int)json_value_get_number(val);
                    txpkt.preamble = (i >= MIN_LORA_PREAMB) ? (uint16_t)i : MIN_LORA_PREAMB;
                } else {
                    txpkt.preamble = STD_LORA_PREAMB;
                }

            } else if (strcmp(str, "FSK") == 0) {
                txpkt.modulation = MOD_FSK;
                val = json_object_get_value(txpk_obj, "datr");
                if (val == NULL) {
                    MSG("WARNING: [down] no \"txpk.datr\" in JSON, TX aborted\n");
                    json_value_free(root_val);
                    continue;
                }
                txpkt.datarate = (unsigned int)(json_value_get_number(val));
                val = json_object_get_value(txpk_obj, "fdev");
                if (val == NULL) {
                    MSG("WARNING: [down] no \"txpk.fdev\" in JSON, TX aborted\n");
                    json_value_free(root_val);
                    continue;
                }
                txpkt.f_dev = (uint8_t)(json_value_get_number(val) / 1000.0);
                val = json_object_get_value(txpk_obj, "prea");
                if (val != NULL) {
                    i = (int)json_value_get_number(val);
                    txpkt.preamble = (i >= MIN_FSK_PREAMB) ? (uint16_t)i : MIN_FSK_PREAMB;
                } else {
                    txpkt.preamble = STD_FSK_PREAMB;
                }
            } else {
                MSG("WARNING: [down] invalid modulation in \"txpk.modu\", TX aborted\n");
                json_value_free(root_val);
                continue;
            }

            /* Parse payload length */
            val = json_object_get_value(txpk_obj, "size");
            if (val == NULL) {
                MSG("WARNING: [down] no \"txpk.size\" in JSON, TX aborted\n");
                json_value_free(root_val);
                continue;
            }
            txpkt.size = (uint16_t)json_value_get_number(val);

            /* Parse payload data */
            str = json_object_get_string(txpk_obj, "data");
            if (str == NULL) {
                MSG("WARNING: [down] no \"txpk.data\" in JSON, TX aborted\n");
                json_value_free(root_val);
                continue;
            }
            i = b64_to_bin(str, strlen(str), txpkt.payload, sizeof txpkt.payload);
            if (i != txpkt.size) {
                MSG("WARNING: [down] mismatch between .size and .data size\n");
            }

            json_value_free(root_val);

            /* select TX mode */
            if (sent_immediate) {
                txpkt.tx_mode = IMMEDIATE;
            } else {
                txpkt.tx_mode = TIMESTAMPED;
            }

            /* record measurement data */
            xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
            meas_dw_dgram_rcv += 1;
            meas_dw_network_byte += msg_len;
            meas_dw_payload_byte += txpkt.size;
            xSemaphoreGive(mx_meas_dw);

            /* reset error/warning */
            jit_result = warning_result = JIT_ERROR_OK;
            warning_value = 0;

            /* check TX frequency */
            if ((txpkt.freq_hz < tx_freq_min[txpkt.rf_chain]) || (txpkt.freq_hz > tx_freq_max[txpkt.rf_chain])) {
                jit_result = JIT_ERROR_TX_FREQ;
                MSG("ERROR: Packet REJECTED, unsupported frequency - %u (min:%u,max:%u)\n", txpkt.freq_hz, tx_freq_min[txpkt.rf_chain], tx_freq_max[txpkt.rf_chain]);
            }

            /* check TX power */
            if (jit_result == JIT_ERROR_OK) {
                i = get_tx_gain_lut_index(txpkt.rf_chain, txpkt.rf_power, &tx_lut_idx);
                if ((i < 0) || (txlut[txpkt.rf_chain].lut[tx_lut_idx].rf_power != txpkt.rf_power)) {
                    warning_result = JIT_ERROR_TX_POWER;
                    warning_value = (signed int)txlut[txpkt.rf_chain].lut[tx_lut_idx].rf_power;
                    printf("WARNING: Requested TX power not supported (%ddBm), using: %ddBm\n", txpkt.rf_power, warning_value);
                    txpkt.rf_power = txlut[txpkt.rf_chain].lut[tx_lut_idx].rf_power;
                }
            }

            /* insert packet into JIT queue */
            if (jit_result == JIT_ERROR_OK) {
                xSemaphoreTake(mx_concent, portMAX_DELAY);
                lgw_get_instcnt(&current_concentrator_time);
                xSemaphoreGive(mx_concent);
                jit_result = jit_enqueue(&jit_queue[txpkt.rf_chain], current_concentrator_time, &txpkt, downlink_type);
                if (jit_result != JIT_ERROR_OK) {
                    printf("ERROR: Packet REJECTED (jit error=%d)\n", jit_result);
                } else {
                    jit_result = warning_result;
                }
                xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
                meas_nb_tx_requested += 1;
                xSemaphoreGive(mx_meas_dw);
            }

            /* Send TX acknowledge */
            send_tx_ack(buff_down[1], buff_down[2], jit_result, warning_value);
        }
    }
    MSG("\nINFO: End of downstream thread\n");
    vTaskDelete(NULL);
}


/* -------------------------------------------------------------------------- */
/* --- THREAD 3: JIT TX SCHEDULING ------------------------------------------ */
/* -------------------------------------------------------------------------- */

static void print_tx_status(uint8_t tx_status)
{
    switch (tx_status) {
        case TX_OFF:       MSG("INFO: [jit] lgw_status returned TX_OFF\n"); break;
        case TX_FREE:      MSG("INFO: [jit] lgw_status returned TX_FREE\n"); break;
        case TX_EMITTING:  MSG("INFO: [jit] lgw_status returned TX_EMITTING\n"); break;
        case TX_SCHEDULED: MSG("INFO: [jit] lgw_status returned TX_SCHEDULED\n"); break;
        default:           MSG("INFO: [jit] lgw_status returned UNKNOWN (%d)\n", tx_status); break;
    }
}

void thread_jit(void *arg)
{
    (void)arg;
    int result = LGW_HAL_SUCCESS;
    struct lgw_pkt_tx_s pkt;
    int pkt_index = -1;
    unsigned int current_concentrator_time;
    enum jit_error_e jit_result;
    enum jit_pkt_type_e pkt_type;
    uint8_t tx_status;
    int i;

    while (!exit_sig && !quit_sig) {
        vTaskDelay(pdMS_TO_TICKS(10));

        for (i = 0; i < LGW_RF_CHAIN_NB; i++) {
            xSemaphoreTake(mx_concent, portMAX_DELAY);
            lgw_get_instcnt(&current_concentrator_time);
            xSemaphoreGive(mx_concent);
            jit_result = jit_peek(&jit_queue[i], current_concentrator_time, &pkt_index);
            if (jit_result == JIT_ERROR_OK) {
                if (pkt_index > -1) {
                    jit_result = jit_dequeue(&jit_queue[i], pkt_index, &pkt, &pkt_type);
                    if (jit_result == JIT_ERROR_OK) {
                        if (pkt_type == JIT_PKT_TYPE_BEACON) {
                            xSemaphoreTake(mx_xcorr, portMAX_DELAY);
                            pkt.freq_hz = (unsigned int)(xtal_correct * (double)pkt.freq_hz);
                            xSemaphoreGive(mx_xcorr);
                            xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
                            meas_nb_beacon_sent += 1;
                            xSemaphoreGive(mx_meas_dw);
                            MSG("INFO: Beacon dequeued (count_us=%u)\n", pkt.count_us);
                        }

                        xSemaphoreTake(mx_concent, portMAX_DELAY);
                        result = lgw_status(pkt.rf_chain, TX_STATUS, &tx_status);
                        xSemaphoreGive(mx_concent);
                        if (result == LGW_HAL_ERROR) {
                            MSG("WARNING: [jit%d] lgw_status failed\n", i);
                        } else {
                            if (tx_status == TX_EMITTING) {
                                MSG("ERROR: concentrator is currently emitting on rf_chain %d\n", i);
                                print_tx_status(tx_status);
                                continue;
                            } else if (tx_status == TX_SCHEDULED) {
                                MSG("WARNING: a downlink was already scheduled on rf_chain %d\n", i);
                                print_tx_status(tx_status);
                            }
                        }

                        xSemaphoreTake(mx_concent, portMAX_DELAY);
                        if (spectral_scan_params.enable == true) {
                            result = lgw_spectral_scan_abort();
                            if (result != LGW_HAL_SUCCESS) {
                                MSG("WARNING: [jit%d] lgw_spectral_scan_abort failed\n", i);
                            }
                        }
                        result = lgw_send(&pkt);
                        xSemaphoreGive(mx_concent);
                        if (result != LGW_HAL_SUCCESS) {
                            xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
                            meas_nb_tx_fail += 1;
                            xSemaphoreGive(mx_meas_dw);
                            MSG("WARNING: [jit] lgw_send failed on rf_chain %d\n", i);
                            continue;
                        } else {
                            xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
                            meas_nb_tx_ok += 1;
                            xSemaphoreGive(mx_meas_dw);
                            MSG_DEBUG(DEBUG_PKT_FWD, "lgw_send done on rf_chain %d: count_us=%u\n", i, pkt.count_us);
                        }
                    } else {
                        MSG("ERROR: jit_dequeue failed on rf_chain %d with %d\n", i, jit_result);
                    }
                }
            } else if (jit_result == JIT_ERROR_EMPTY) {
                /* Do nothing */
            } else {
                MSG("ERROR: jit_peek failed on rf_chain %d with %d\n", i, jit_result);
            }
        }
    }

    MSG("\nINFO: End of JIT thread\n");
    vTaskDelete(NULL);
}


/* ========================================================================== */
/* ===                     PACKET FORWARDER MAIN                          === */
/* ========================================================================== */

int pkt_fwd_main(void)
{
    int i, x;
    int l, m;

    /* variables for statistics */
    unsigned int cp_nb_rx_rcv, cp_nb_rx_ok, cp_nb_rx_bad, cp_nb_rx_nocrc;
    unsigned int cp_up_pkt_fwd, cp_up_network_byte, cp_up_payload_byte;
    unsigned int cp_up_dgram_sent, cp_up_ack_rcv;
    unsigned int cp_dw_pull_sent, cp_dw_ack_rcv, cp_dw_dgram_rcv;
    unsigned int cp_dw_network_byte, cp_dw_payload_byte;
    unsigned int cp_nb_tx_ok, cp_nb_tx_fail;
    unsigned int cp_nb_tx_requested = 0;
    unsigned int cp_nb_tx_rejected_collision_packet = 0;
    unsigned int cp_nb_tx_rejected_collision_beacon = 0;
    unsigned int cp_nb_tx_rejected_too_late = 0;
    unsigned int cp_nb_tx_rejected_too_early = 0;
    unsigned int cp_nb_beacon_queued = 0;
    unsigned int cp_nb_beacon_sent = 0;
    unsigned int cp_nb_beacon_rejected = 0;

    /* SX1302 data */
    unsigned int trig_tstamp;
    unsigned int inst_tstamp;
    float temperature = 0.0f;  /* 0 when sensor unavailable */

    /* statistics */
    float rx_ok_ratio, rx_bad_ratio, rx_nocrc_ratio;
    float up_ack_ratio, dw_ack_ratio;
    char stat_timestamp[24];
    unsigned int time_count = 0;

    /* GPS placeholder */
    struct coord_s cp_gps_coord = {0.0, 0.0, 0};

    /* OLED display buffer */
    char out_info[96];

    /* ============== Create Mutexes ============== */
    mx_concent = xSemaphoreCreateMutex();     assert(mx_concent);
    mx_xcorr = xSemaphoreCreateMutex();       assert(mx_xcorr);
    mx_meas_up = xSemaphoreCreateMutex();     assert(mx_meas_up);
    mx_meas_dw = xSemaphoreCreateMutex();     assert(mx_meas_dw);
    mx_stat_rep = xSemaphoreCreateMutex();    assert(mx_stat_rep);

    /* Load gateway config from Flash (or apply defaults if Flash is blank).
     * Must be before net_init() so gWIZNETINFO gets the correct eth_ip. */
    uart_cli_init();
    config_load();

    /* Initialize OLED display */
    if (oled_init() != 0) {
        MSG("WARNING: OLED init failed\n");
    }

    /* Redirect Parson JSON allocations to FreeRTOS heap.
     * Must be done before any json_parse_* call.
     * We do NOT globally override malloc() because newlib calls malloc
     * before the FreeRTOS scheduler starts (stdio buffer init, etc.). */
    json_set_allocation_functions(pvPortMalloc, vPortFree);

    /* ============== Banner ============== */
    MSG("*** Packet Forwarder ***\nVersion: " VERSION_STRING "\n");
    MSG("*** SX1302 HAL library version info ***\n%s\n***\n", lgw_version_info());

    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        MSG("INFO: Little endian host\n");
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        MSG("INFO: Big endian host\n");
    #else
        MSG("INFO: Host endianness unknown\n");
    #endif

    /* ============== Load JSON Configuration ============== */

    /* Default to CN470 config (hardcoded in global_json.h).
     * global_cn_conf is a Semtech binary-with-header format:
     *   bytes 0-1: big-endian uint16 JSON length
     *   bytes 2..(2+len-1): JSON text
     * Parson's json_parse_array_with_comments / copy_array() handles this
     * format natively, so pass the raw array directly — no malloc needed. */
    MSG("INFO: Loading CN470 default config (free heap %u)...\n",
        (unsigned)xPortGetFreeHeapSize());
    const char *conf_array = (const char *)global_cn_conf;

    /* parse concentrator, gateway, debug configuration */
    x = parse_SX130x_configuration(conf_array);
    if (x != 0) {
        MSG("ERROR: failed to parse SX130x configuration\n");
        return -1;
    }
    x = parse_gateway_configuration(conf_array);
    if (x != 0) {
        MSG("INFO: no gateway configuration in JSON\n");
    }
    x = parse_debug_configuration(conf_array);
    if (x != 0) {
        MSG("INFO: no debug configuration in JSON\n");
    }

    /* ============== Apply Flash Config Overrides ============== */
    gateway_config_t *cfg = config_get();

    /* Override gateway EUI */
    if (cfg->gateway_eui != 0) {
        lgwm = cfg->gateway_eui;
        MSG("INFO: gateway EUI from Flash: %08X%08X\n",
            (unsigned int)(lgwm >> 32), (unsigned int)(lgwm & 0xFFFFFFFF));
    }

    /* Override NS server IP and port */
    if (cfg->ns_host[0] != '\0') {
        MSG("INFO: NS server from Flash: %s\n", cfg->ns_host);
        /* Parse IP address from ns_host string (e.g. "192.168.10.1") */
        unsigned int a, b, c, d;
        if (sscanf(cfg->ns_host, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
            ns_ip[0] = (uint8_t)a;
            ns_ip[1] = (uint8_t)b;
            ns_ip[2] = (uint8_t)c;
            ns_ip[3] = (uint8_t)d;
        } else {
            MSG("WARNING: invalid ns_host IP format \"%s\", using JSON default\n", cfg->ns_host);
            /* Try to parse from JSON serv_addr */
            if (sscanf(serv_addr, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
                ns_ip[0] = (uint8_t)a;
                ns_ip[1] = (uint8_t)b;
                ns_ip[2] = (uint8_t)c;
                ns_ip[3] = (uint8_t)d;
            }
        }
    }
    if (cfg->ns_port_up != 0) ns_port_up = cfg->ns_port_up;
    if (cfg->ns_port_down != 0) ns_port_down = cfg->ns_port_down;

    /* If ns_ip not set by Flash config, fall back to JSON server_address */
    if (ns_ip[0] == 0 && ns_ip[1] == 0 && ns_ip[2] == 0 && ns_ip[3] == 0) {
        unsigned int a, b, c, d;
        if (sscanf(serv_addr, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
            ns_ip[0] = (uint8_t)a; ns_ip[1] = (uint8_t)b;
            ns_ip[2] = (uint8_t)c; ns_ip[3] = (uint8_t)d;
            MSG("INFO: NS server IP from JSON: %s\n", serv_addr);
        } else {
            MSG("WARNING: cannot parse server address \"%s\", NS will be unreachable\n", serv_addr);
        }
    }

    /* Flash overrides Flash-configured EUI display */
    MSG("INFO: NS server IP: %d.%d.%d.%d, port_up: %u, port_down: %u\n",
        ns_ip[0], ns_ip[1], ns_ip[2], ns_ip[3], ns_port_up, ns_port_down);

    /* ============== Gateway MAC Processing ============== */
    net_mac_h = pkt_htonl((unsigned int)(0xFFFFFFFF & (lgwm >> 32)));
    net_mac_l = pkt_htonl((unsigned int)(0xFFFFFFFF & lgwm));

    /* ============== Initialize W5500 Network ============== */
    if (net_init() != NET_OK) {
        MSG("ERROR: failed to initialize W5500 network\n");
        return -1;
    }

    /* Open UDP sockets */
    if (net_udp_open(NET_SOCK_UP, ns_port_up) != NET_OK) {
        MSG("ERROR: failed to open upstream socket\n");
        return -1;
    }
    if (net_udp_open(NET_SOCK_DOWN, ns_port_down + 1) != NET_OK) {
        MSG("ERROR: failed to open downstream socket\n");
        return -1;
    }

    /* Set server destinations */
    net_set_dest(NET_SOCK_UP, ns_ip, ns_port_up);
    net_set_dest(NET_SOCK_DOWN, ns_ip, ns_port_down);

    MSG("INFO: UDP sockets opened, NS=%d.%d.%d.%d up:%u down:%u\n",
        ns_ip[0], ns_ip[1], ns_ip[2], ns_ip[3], ns_port_up, ns_port_down);

    /* ============== Board Reset & Start Concentrator ============== */
    lgw_reset();

    for (l = 0; l < LGW_IF_CHAIN_NB; l++) {
        for (m = 0; m < 8; m++) {
            nb_pkt_log[l][m] = 0;
        }
    }

    i = lgw_start();
    if (i == LGW_HAL_SUCCESS) {
        MSG("INFO: [main] concentrator started, packet can now be received\n");
        /* Seed rand() with SX1302 free-running counter so tokens differ each boot */
        unsigned int seed_ts = 0;
        lgw_get_instcnt(&seed_ts);
        srand(seed_ts ^ HAL_GetTick());
    } else {
        MSG("ERROR: [main] failed to start the concentrator\n");
        return -1;
    }

    /* ============== Initialize JIT Queues ============== */
    jit_queue_init(&jit_queue[0]);
    jit_queue_init(&jit_queue[1]);

    /* ============== Create FreeRTOS Tasks ============== */
    /* Stack sizes reduced for STM32 RAM constraints */
    if (xTaskCreate(thread_up, "thread_up", 2048, NULL, osPriorityAboveNormal, &pThreadUp) != pdPASS) {
        MSG("ERROR: failed to create thread_up\n");
    } else {
        MSG("INFO: thread_up created\n");
    }

    if (xTaskCreate(thread_down, "thread_down", 1536, NULL, osPriorityAboveNormal, NULL) != pdPASS) {
        MSG("ERROR: failed to create thread_down\n");
    } else {
        MSG("INFO: thread_down created\n");
    }

    if (xTaskCreate(thread_jit, "thread_jit", 1024, NULL, osPriorityAboveNormal, &pJit) != pdPASS) {
        MSG("ERROR: failed to create thread_jit\n");
    } else {
        MSG("INFO: thread_jit created\n");
    }

    if (xTaskCreate(thread_cli, "thread_cli", 512, NULL, osPriorityNormal, NULL) != pdPASS) {
        MSG("ERROR: failed to create thread_cli\n");
    } else {
        MSG("INFO: thread_cli created (UART config CLI active)\n");
    }

    /* OLED startup screen rows 0-4 (written into framebuf now, flushed together with row 5) */
    {
        gateway_config_t *cfg2 = config_get();
        oled_draw_string(0, 0, "ESXP1302 STM32  ");
        snprintf(out_info, sizeof out_info, "EUI:%08X", (unsigned int)(lgwm >> 32));
        oled_draw_string(0, 1, out_info);
        snprintf(out_info, sizeof out_info, "    %08X", (unsigned int)(lgwm & 0xFFFFFFFFu));
        oled_draw_string(0, 2, out_info);
        snprintf(out_info, sizeof out_info, "IP:%d.%d.%d.%d",
                 cfg2->eth_ip[0], cfg2->eth_ip[1], cfg2->eth_ip[2], cfg2->eth_ip[3]);
        oled_draw_string(0, 3, out_info);
        oled_draw_string(0, 4, "Concentrator OK ");
    }
    /* Update NS info on OLED — oled_show_one_line flushes the entire display */
    snprintf(out_info, sizeof out_info, "NS=%d.%d.%d.%d:%u",
             ns_ip[0], ns_ip[1], ns_ip[2], ns_ip[3], ns_port_up);
    oled_show_one_line(0, 5, out_info);

    /* ============== Main Loop: Statistics Collection ============== */
    while (!exit_sig && !quit_sig) {
        time_count = 0;
        while (time_count < stat_interval) {
            vTaskDelay(pdMS_TO_TICKS(1000 * TIME_REFRESH));
            time_count += TIME_REFRESH;

            /* Update time display on OLED */
            uint32_t up = get_uptime_sec();
            snprintf(stat_timestamp, sizeof stat_timestamp, "Up %02lu:%02lu:%02lu",
                     (unsigned long)(up / 3600),
                     (unsigned long)((up % 3600) / 60),
                     (unsigned long)(up % 60));
            oled_show_one_line(0, 6, stat_timestamp);
        }

        /* access upstream statistics, copy and reset */
        xSemaphoreTake(mx_meas_up, portMAX_DELAY);
        cp_nb_rx_rcv       = meas_nb_rx_rcv;
        cp_nb_rx_ok        = meas_nb_rx_ok;
        cp_nb_rx_bad       = meas_nb_rx_bad;
        cp_nb_rx_nocrc     = meas_nb_rx_nocrc;
        cp_up_pkt_fwd      = meas_up_pkt_fwd;
        cp_up_network_byte = meas_up_network_byte;
        cp_up_payload_byte = meas_up_payload_byte;
        cp_up_dgram_sent   = meas_up_dgram_sent;
        cp_up_ack_rcv      = meas_up_ack_rcv;
        meas_nb_rx_rcv = 0;
        meas_nb_rx_ok = 0;
        meas_nb_rx_bad = 0;
        meas_nb_rx_nocrc = 0;
        meas_up_pkt_fwd = 0;
        meas_up_network_byte = 0;
        meas_up_payload_byte = 0;
        meas_up_dgram_sent = 0;
        meas_up_ack_rcv = 0;
        xSemaphoreGive(mx_meas_up);

        if (cp_nb_rx_rcv > 0) {
            rx_ok_ratio = (float)cp_nb_rx_ok / (float)cp_nb_rx_rcv;
            rx_bad_ratio = (float)cp_nb_rx_bad / (float)cp_nb_rx_rcv;
            rx_nocrc_ratio = (float)cp_nb_rx_nocrc / (float)cp_nb_rx_rcv;
        } else {
            rx_ok_ratio = rx_bad_ratio = rx_nocrc_ratio = 0.0;
        }
        up_ack_ratio = (cp_up_dgram_sent > 0) ? (float)cp_up_ack_rcv / (float)cp_up_dgram_sent : 0.0;

        /* access downstream statistics, copy and reset */
        xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
        cp_dw_pull_sent    =  meas_dw_pull_sent;
        cp_dw_ack_rcv      =  meas_dw_ack_rcv;
        cp_dw_dgram_rcv    =  meas_dw_dgram_rcv;
        cp_dw_network_byte =  meas_dw_network_byte;
        cp_dw_payload_byte =  meas_dw_payload_byte;
        cp_nb_tx_ok        =  meas_nb_tx_ok;
        cp_nb_tx_fail      =  meas_nb_tx_fail;
        cp_nb_tx_requested                 +=  meas_nb_tx_requested;
        cp_nb_tx_rejected_collision_packet +=  meas_nb_tx_rejected_collision_packet;
        cp_nb_tx_rejected_collision_beacon +=  meas_nb_tx_rejected_collision_beacon;
        cp_nb_tx_rejected_too_late         +=  meas_nb_tx_rejected_too_late;
        cp_nb_tx_rejected_too_early        +=  meas_nb_tx_rejected_too_early;
        cp_nb_beacon_queued   +=  meas_nb_beacon_queued;
        cp_nb_beacon_sent     +=  meas_nb_beacon_sent;
        cp_nb_beacon_rejected +=  meas_nb_beacon_rejected;
        meas_dw_pull_sent = 0;
        meas_dw_ack_rcv = 0;
        meas_dw_dgram_rcv = 0;
        meas_dw_network_byte = 0;
        meas_dw_payload_byte = 0;
        meas_nb_tx_ok = 0;
        meas_nb_tx_fail = 0;
        meas_nb_tx_requested = 0;
        meas_nb_tx_rejected_collision_packet = 0;
        meas_nb_tx_rejected_collision_beacon = 0;
        meas_nb_tx_rejected_too_late = 0;
        meas_nb_tx_rejected_too_early = 0;
        meas_nb_beacon_queued = 0;
        meas_nb_beacon_sent = 0;
        meas_nb_beacon_rejected = 0;
        xSemaphoreGive(mx_meas_dw);
        dw_ack_ratio = (cp_dw_pull_sent > 0) ? (float)cp_dw_ack_rcv / (float)cp_dw_pull_sent : 0.0;

        /* display report */
        uint32_t up = get_uptime_sec();
        printf("\n##### Uptime %02lu:%02lu:%02lu #####\n",
               (unsigned long)(up / 3600), (unsigned long)((up % 3600) / 60), (unsigned long)(up % 60));
        printf("### [UPSTREAM] ###\n");
        printf("# RF packets received by concentrator: %u\n", cp_nb_rx_rcv);
        printf("# CRC_OK: %.2f%%, CRC_FAIL: %.2f%%, NO_CRC: %.2f%%\n", 100.0 * rx_ok_ratio, 100.0 * rx_bad_ratio, 100.0 * rx_nocrc_ratio);
        printf("# RF packets forwarded: %u (%u bytes)\n", cp_up_pkt_fwd, cp_up_payload_byte);
        printf("# PUSH_DATA datagrams sent: %u (%u bytes)\n", cp_up_dgram_sent, cp_up_network_byte);
        printf("# PUSH_DATA acknowledged: %.2f%%\n", 100.0 * up_ack_ratio);
        printf("### [DOWNSTREAM] ###\n");
        printf("# PULL_DATA sent: %u (%.2f%% acknowledged)\n", cp_dw_pull_sent, 100.0 * dw_ack_ratio);
        printf("# PULL_RESP(onse) datagrams received: %u (%u bytes)\n", cp_dw_dgram_rcv, cp_dw_network_byte);
        printf("# RF packets sent to concentrator: %u (%u bytes)\n", (cp_nb_tx_ok + cp_nb_tx_fail), cp_dw_payload_byte);
        printf("# TX errors: %u\n", cp_nb_tx_fail);
        if (cp_nb_tx_requested != 0) {
            printf("# TX rejected (collision packet): %.2f%%\n", 100.0 * cp_nb_tx_rejected_collision_packet / cp_nb_tx_requested);
            printf("# TX rejected (too late): %.2f%%\n", 100.0 * cp_nb_tx_rejected_too_late / cp_nb_tx_requested);
            printf("# TX rejected (too early): %.2f%%\n", 100.0 * cp_nb_tx_rejected_too_early / cp_nb_tx_requested);
        }
        printf("### SX1302 Status ###\n");
        xSemaphoreTake(mx_concent, portMAX_DELAY);
        i  = lgw_get_instcnt(&inst_tstamp);
        i |= lgw_get_trigcnt(&trig_tstamp);
        xSemaphoreGive(mx_concent);
        if (i != LGW_HAL_SUCCESS) {
            printf("# SX1302 counter unknown\n");
        } else {
            printf("# SX1302 counter (INST): %u\n", inst_tstamp);
            printf("# SX1302 counter (PPS):  %u\n", trig_tstamp);
        }
        printf("### [JIT] ###\n");
        jit_print_queue(&jit_queue[0], false, DEBUG_LOG);
        printf("#--------\n");
        jit_print_queue(&jit_queue[1], false, DEBUG_LOG);
        printf("### [GPS] ###\n");
        printf("# GPS sync is disabled\n");
        xSemaphoreTake(mx_concent, portMAX_DELAY);
        i = lgw_get_temperature(&temperature);
        xSemaphoreGive(mx_concent);
        if (i != LGW_HAL_SUCCESS) {
            printf("### Concentrator temperature unknown ###\n");
        } else {
            printf("### Concentrator temperature: %.0f C ###\n", temperature);
            snprintf(out_info, 22, "Temp=%.1fC GPS=(N/A)", temperature);
            oled_show_one_line(0, 7, out_info);
        }
        printf("##### END #####\n");

        /* generate JSON status report */
        xSemaphoreTake(mx_stat_rep, portMAX_DELAY);
        up = get_uptime_sec();
        snprintf(stat_timestamp, sizeof stat_timestamp, "%02lu:%02lu:%02lu",
                 (unsigned long)(up / 3600), (unsigned long)((up % 3600) / 60), (unsigned long)(up % 60));
        if (gps_fake_enable == true) {
            snprintf(status_report, STATUS_SIZE,
                "\"stat\":{\"time\":\"%s\",\"lati\":%.5f,\"long\":%.5f,\"alti\":%i,\"rxnb\":%u,\"rxok\":%u,\"rxfw\":%u,\"ackr\":%.1f,\"dwnb\":%u,\"txnb\":%u,\"temp\":%.1f}",
                stat_timestamp, cp_gps_coord.lat, cp_gps_coord.lon, cp_gps_coord.alt,
                cp_nb_rx_rcv, cp_nb_rx_ok, cp_up_pkt_fwd, 100.0 * up_ack_ratio,
                cp_dw_dgram_rcv, cp_nb_tx_ok, temperature);
        } else {
            snprintf(status_report, STATUS_SIZE,
                "\"stat\":{\"time\":\"%s\",\"rxnb\":%u,\"rxok\":%u,\"rxfw\":%u,\"ackr\":%.1f,\"dwnb\":%u,\"txnb\":%u,\"temp\":%.1f}",
                stat_timestamp, cp_nb_rx_rcv, cp_nb_rx_ok, cp_up_pkt_fwd,
                100.0 * up_ack_ratio, cp_dw_dgram_rcv, cp_nb_tx_ok, temperature);
        }
        report_ready = true;
        xSemaphoreGive(mx_stat_rep);
    }

    /* ============== Cleanup ============== */
    /* stop the hardware */
    i = lgw_stop();
    if (i == LGW_HAL_SUCCESS) {
        MSG("INFO: concentrator stopped successfully\n");
    } else {
        MSG("WARNING: failed to stop concentrator\n");
    }

    lgw_reset();

    net_udp_close(NET_SOCK_UP);
    net_udp_close(NET_SOCK_DOWN);

    MSG("INFO: Exiting packet forwarder\n");
    return 0;
}
