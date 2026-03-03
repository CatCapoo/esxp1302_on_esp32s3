/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2019 Semtech

Description:
    LoRa concentrator : Packet Forwarder trace helpers
    STM32F407 port — no stderr, everything goes to printf (USART1).
*/

#ifndef _LORA_PKTFWD_TRACE_H
#define _LORA_PKTFWD_TRACE_H

#include <stdio.h>

#define DEBUG_PKT_FWD   0
#define DEBUG_JIT       0
#define DEBUG_JIT_ERROR 1
#define DEBUG_TIMERSYNC 0
#define DEBUG_BEACON    0
#define DEBUG_LOG       1

#define MSG(args...) printf(args)

#define MSG_DEBUG(FLAG, fmt, ...)                                                                           \
            do  {                                                                                           \
                if (FLAG)                                                                                   \
                    printf("%s:%d:%s(): " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);            \
            } while (0)

#define MSG_PRINTF(FLAG, fmt, ...)                                                                          \
            do  {                                                                                           \
                if (FLAG)                                                                                   \
                    printf(fmt, ##__VA_ARGS__);                                                             \
            } while (0)

#endif
/* --- EOF ------------------------------------------------------------------ */
