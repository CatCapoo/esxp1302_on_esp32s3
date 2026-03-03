/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2019 Semtech

Description:
    Packet forwarder main entry point and control interface.

License: Revised BSD License
*/

#ifndef _LORA_PKT_FWD_H
#define _LORA_PKT_FWD_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "semphr.h"

/* -------------------------------------------------------------------------- */
/*  Exported variables                                                        */
/* -------------------------------------------------------------------------- */

/* Concentrator access mutex — shared with other modules */
extern SemaphoreHandle_t mx_concent;

/* Signal flags */
extern volatile bool exit_sig;
extern volatile bool quit_sig;

/* -------------------------------------------------------------------------- */
/*  Public functions                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief  Packet forwarder main entry point.
 *         Initializes config, concentrator, network, and spawns worker threads.
 *         Runs the statistics loop. Returns only on exit_sig/quit_sig.
 * @return 0 on graceful exit, -1 on error.
 */
int pkt_fwd_main(void);

#endif /* _LORA_PKT_FWD_H */
