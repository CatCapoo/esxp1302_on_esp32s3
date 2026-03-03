/*
 * uart_cli.c  –  UART command-line interface for gateway configuration
 *
 * Receive flow:
 *   HAL_UART_Receive_IT() → USART1_IRQHandler (stm32f4xx_it.c)
 *     → HAL_UART_IRQHandler() → HAL_UART_RxCpltCallback()
 *       → accumulate into line buffer, set cli_line_ready flag on newline
 *   uart_cli_task() polls cli_line_ready, copies line, parses, executes.
 *
 * Echo:  Each received character is echoed back so the user can see what
 *        they type. Backspace (0x7F / 0x08) erases the previous character.
 */

#include "uart_cli.h"
#include "gateway_config.h"

#include "stm32f4xx_hal.h"
#include "usart.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/*  Internal buffers                                                   */
/* ------------------------------------------------------------------ */

#define CLI_LINE_MAX  256
#define CLI_ARGS_MAX    8

static volatile uint8_t  s_rx_byte;
static volatile char     s_rx_line[CLI_LINE_MAX];
static volatile uint16_t s_rx_pos;
static volatile uint8_t  s_line_ready;

/* ------------------------------------------------------------------ */
/*  IRQ / HAL callback                                                 */
/* ------------------------------------------------------------------ */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1) {
        return;
    }

    /* Snapshot and re-arm IMMEDIATELY – never miss the next byte */
    uint8_t c = s_rx_byte;
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&s_rx_byte, 1);

    /* Echo back */
    HAL_UART_Transmit(&huart1, &c, 1, 10);

    if (c == '\r' || c == '\n') {
        /* CR or LF ends the line */
        if (s_rx_pos > 0 && !s_line_ready) {
            s_rx_line[s_rx_pos] = '\0';
            s_line_ready = 1;
            s_rx_pos     = 0;
        }
        uint8_t nl = '\n';
        HAL_UART_Transmit(&huart1, &nl, 1, 10);
    } else if (c == 0x7F || c == 0x08) {
        /* Backspace / DEL – erase last char */
        if (s_rx_pos > 0) {
            s_rx_pos--;
            /* Send VT100 erase-char sequence */
            const uint8_t bs[] = {0x08, ' ', 0x08};
            HAL_UART_Transmit(&huart1, (uint8_t *)bs, 3, 10);
        }
    } else if (s_rx_pos < CLI_LINE_MAX - 1) {
        s_rx_line[s_rx_pos++] = (char)c;
    }
    /* No re-arm here – already done at the top */
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
        printf("[CLI] gw_eui = %08X%08X\r\n",
               (unsigned)((eui >> 32) & 0xFFFFFFFFUL),
               (unsigned)(eui & 0xFFFFFFFFUL));

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
    s_rx_pos    = 0;
    s_line_ready = 0;

    /* Enable USART1 interrupt in NVIC with a lower priority than SysTick */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    /* Arm receive */
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&s_rx_byte, 1);

    printf("\r\n[CLI] Ready. Type 'help' for commands.\r\n> ");
}

void uart_cli_task(void)
{
    if (!s_line_ready) {
        return;
    }

    /* Snapshot and clear before processing (allow new input while parsing) */
    char line_copy[CLI_LINE_MAX];
    strncpy(line_copy, (const char *)s_rx_line, CLI_LINE_MAX - 1);
    line_copy[CLI_LINE_MAX - 1] = '\0';
    s_line_ready = 0;

    dispatch_line(line_copy);

    /* Re-prompt */
    printf("> ");
}
