/*
 * uart_cli.c  –  UART command-line interface for gateway configuration
 *
 * Receive flow (ring-buffer architecture):
 *   HAL_UART_Receive_IT() → USART1_IRQHandler (stm32f4xx_it.c)
 *     → HAL_UART_IRQHandler() → HAL_UART_RxCpltCallback()
 *       → store ONE byte in ring buffer, re-arm immediately.
 *
 *   uart_cli_task() (called from FreeRTOS task @ 10ms):
 *     → drain ring buffer → echo each char → build line buffer
 *     → when CR/LF received → parse + execute command
 *
 *   No blocking calls inside the ISR – prevents character loss.
 */

#include "uart_cli.h"
#include "gateway_config.h"

#include "stm32f4xx_hal.h"
#include "usart.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/*  Ring buffer (ISR → task)                                           */
/* ------------------------------------------------------------------ */

#define RING_SIZE  256                     /* must be power of 2 */
#define RING_MASK  (RING_SIZE - 1)

static volatile uint8_t  s_ring[RING_SIZE];
static volatile uint8_t  s_ring_head;      /* written by ISR only */
static volatile uint8_t  s_ring_tail;      /* read by task only  */

static volatile uint8_t  s_rx_byte;        /* HAL single-byte target */

/* ------------------------------------------------------------------ */
/*  Line buffer (task only – not volatile)                             */
/* ------------------------------------------------------------------ */

#define CLI_LINE_MAX  256
#define CLI_ARGS_MAX    8

static char     s_line[CLI_LINE_MAX];
static uint16_t s_line_pos;

/* ------------------------------------------------------------------ */
/*  ISR callback – absolute minimum work                               */
/* ------------------------------------------------------------------ */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1) {
        return;
    }

    /* 1. Snapshot the byte */
    uint8_t c = s_rx_byte;

    /* 2. Re-arm FIRST – ready for the very next byte */
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&s_rx_byte, 1);

    /* 3. Push into ring buffer (no blocking, no echo!) */
    uint8_t next = (s_ring_head + 1) & RING_MASK;
    if (next != s_ring_tail) {          /* not full */
        s_ring[s_ring_head] = c;
        s_ring_head = next;
    }
    /* That's it – task handles echo + line building */
}

/* ------------------------------------------------------------------ */
/*  Command helpers                                                    */
/* ------------------------------------------------------------------ */

static void cli_print_help(void)
{
    printf("\r\n=== Gateway CLI ===\r\n");
    printf("  help\r\n");
    printf("  config show\r\n");
    printf("  config set ns_host  <ip_or_hostname>\r\n");
    printf("  config set ns_port_up   <port>\r\n");
    printf("  config set ns_port_down <port>\r\n");
    printf("  config set gw_eui   <16-hex-chars>   e.g. AA555A0000000001\r\n");
    printf("  config set eth_ip   <a.b.c.d>\r\n");
    printf("  config set eth_gw   <a.b.c.d>\r\n");
    printf("  config set eth_sn   <a.b.c.d>\r\n");
    printf("  config save\r\n");
    printf("  config load\r\n");
    printf("  config reset\r\n");
    printf("  reboot\r\n");
    printf("===================\r\n");
}

static int parse_ipv4(const char *str, uint8_t out[4])
{
    unsigned a, b, c, d;
    if (sscanf(str, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        return -1;
    }
    if (a > 255 || b > 255 || c > 255 || d > 255) {
        return -1;
    }
    out[0] = (uint8_t)a;
    out[1] = (uint8_t)b;
    out[2] = (uint8_t)c;
    out[3] = (uint8_t)d;
    return 0;
}

static void cmd_config_set(int argc, char *argv[])
{
    /* argv[0]="config"  argv[1]="set"  argv[2]=key  argv[3]=value */
    if (argc < 4) {
        printf("[CLI] Usage: config set <key> <value>\r\n");
        return;
    }

    const char *key = argv[2];
    const char *val = argv[3];
    gateway_config_t *cfg = config_get();

    if (strcmp(key, "ns_host") == 0) {
        strncpy(cfg->ns_host, val, sizeof(cfg->ns_host) - 1);
        cfg->ns_host[sizeof(cfg->ns_host) - 1] = '\0';
        printf("[CLI] ns_host = %s\r\n", cfg->ns_host);

    } else if (strcmp(key, "ns_port_up") == 0) {
        int p = atoi(val);
        if (p <= 0 || p > 65535) {
            printf("[CLI] Invalid port: %s\r\n", val);
            return;
        }
        cfg->ns_port_up = (uint16_t)p;
        printf("[CLI] ns_port_up = %u\r\n", (unsigned)cfg->ns_port_up);

    } else if (strcmp(key, "ns_port_down") == 0) {
        int p = atoi(val);
        if (p <= 0 || p > 65535) {
            printf("[CLI] Invalid port: %s\r\n", val);
            return;
        }
        cfg->ns_port_down = (uint16_t)p;
        printf("[CLI] ns_port_down = %u\r\n", (unsigned)cfg->ns_port_down);

    } else if (strcmp(key, "gw_eui") == 0) {
        if (strlen(val) != 16) {
            printf("[CLI] gw_eui must be 16 hex chars, e.g. AA555A0000000001\r\n");
            return;
        }
        uint64_t eui = 0;
        for (int i = 0; i < 16; i++) {
            char h = val[i];
            uint8_t nibble;
            if (h >= '0' && h <= '9')      nibble = (uint8_t)(h - '0');
            else if (h >= 'A' && h <= 'F') nibble = (uint8_t)(h - 'A' + 10);
            else if (h >= 'a' && h <= 'f') nibble = (uint8_t)(h - 'a' + 10);
            else {
                printf("[CLI] Invalid hex char '%c'\r\n", h);
                return;
            }
            eui = (eui << 4) | nibble;
        }
        cfg->gateway_eui = eui;
        printf("[CLI] gw_eui = %016llX\r\n", (unsigned long long)cfg->gateway_eui);

    } else if (strcmp(key, "eth_ip") == 0) {
        if (parse_ipv4(val, cfg->eth_ip) != 0) {
            printf("[CLI] Invalid IP: %s\r\n", val);
            return;
        }
        printf("[CLI] eth_ip = %u.%u.%u.%u\r\n",
               cfg->eth_ip[0], cfg->eth_ip[1],
               cfg->eth_ip[2], cfg->eth_ip[3]);

    } else if (strcmp(key, "eth_gw") == 0) {
        if (parse_ipv4(val, cfg->eth_gw) != 0) {
            printf("[CLI] Invalid IP: %s\r\n", val);
            return;
        }
        printf("[CLI] eth_gw = %u.%u.%u.%u\r\n",
               cfg->eth_gw[0], cfg->eth_gw[1],
               cfg->eth_gw[2], cfg->eth_gw[3]);

    } else if (strcmp(key, "eth_sn") == 0) {
        if (parse_ipv4(val, cfg->eth_sn) != 0) {
            printf("[CLI] Invalid mask: %s\r\n", val);
            return;
        }
        printf("[CLI] eth_sn = %u.%u.%u.%u\r\n",
               cfg->eth_sn[0], cfg->eth_sn[1],
               cfg->eth_sn[2], cfg->eth_sn[3]);

    } else {
        printf("[CLI] Unknown key: %s\r\n", key);
    }
}

/* ------------------------------------------------------------------ */
/*  Line tokeniser                                                     */
/* ------------------------------------------------------------------ */

static void dispatch_line(char *line)
{
    /* Skip leading whitespace */
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '\0') return;

    /* Tokenise in-place */
    char *argv[CLI_ARGS_MAX];
    int   argc = 0;
    char *p = line;

    while (*p && argc < CLI_ARGS_MAX) {
        /* Skip whitespace */
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        argv[argc++] = p;

        /* Advance to end of token */
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = '\0';
    }

    if (argc == 0) return;

    /* Dispatch */
    if (strcmp(argv[0], "help") == 0) {
        cli_print_help();

    } else if (strcmp(argv[0], "config") == 0 && argc >= 2) {

        if (strcmp(argv[1], "show") == 0) {
            config_print();
        } else if (strcmp(argv[1], "save") == 0) {
            config_save();
        } else if (strcmp(argv[1], "load") == 0) {
            config_load();
        } else if (strcmp(argv[1], "reset") == 0) {
            config_reset_defaults();
        } else if (strcmp(argv[1], "set") == 0) {
            cmd_config_set(argc, argv);
        } else {
            printf("[CLI] Unknown config sub-command: %s\r\n", argv[1]);
        }

    } else if (strcmp(argv[0], "reboot") == 0) {
        printf("[CLI] Rebooting...\r\n");
        HAL_Delay(100);
        NVIC_SystemReset();

    } else {
        printf("[CLI] Unknown command: %s  (type 'help')\r\n", argv[0]);
    }
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

void uart_cli_init(void)
{
    s_ring_head = 0;
    s_ring_tail = 0;
    s_line_pos  = 0;

    /* Enable USART1 interrupt in NVIC with a lower priority than SysTick */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    /* Arm receive */
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&s_rx_byte, 1);

    printf("\r\n[CLI] Ready. Type 'help' for commands.\r\n> ");
}

void uart_cli_task(void)
{
    /* --- Drain ring buffer: echo + line accumulation --- */
    while (s_ring_tail != s_ring_head) {
        uint8_t c = s_ring[s_ring_tail];
        s_ring_tail = (s_ring_tail + 1) & RING_MASK;

        if (c == '\r' || c == '\n') {
            /* Echo newline */
            uint8_t crlf[] = {'\r', '\n'};
            HAL_UART_Transmit(&huart1, crlf, 2, 10);

            if (s_line_pos > 0) {
                s_line[s_line_pos] = '\0';
                s_line_pos = 0;

                dispatch_line(s_line);
                printf("> ");
            }
        } else if (c == 0x7F || c == 0x08) {
            /* Backspace */
            if (s_line_pos > 0) {
                s_line_pos--;
                const uint8_t bs[] = {0x08, ' ', 0x08};
                HAL_UART_Transmit(&huart1, (uint8_t *)bs, 3, 10);
            }
        } else if (s_line_pos < CLI_LINE_MAX - 1) {
            s_line[s_line_pos++] = (char)c;
            /* Echo printable char */
            HAL_UART_Transmit(&huart1, &c, 1, 10);
        }
    }
}
