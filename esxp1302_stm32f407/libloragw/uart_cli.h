/*
 * uart_cli.h  –  UART command-line interface for gateway configuration
 *
 * Interrupt-driven receive on USART1 (PA9/PA10, 115200 8N1).
 * Commands are processed by uart_cli_task() called from a FreeRTOS task.
 *
 * Supported commands:
 *   help
 *   config show
 *   config set ns_host  <ip_or_hostname>
 *   config set ns_port_up   <port>
 *   config set ns_port_down <port>
 *   config set gw_eui   <16-hex-chars>
 *   config set eth_ip   <a.b.c.d>
 *   config set eth_gw   <a.b.c.d>
 *   config set eth_sn   <a.b.c.d>
 *   config save
 *   config load
 *   config reset
 *   reboot
 */

#ifndef UART_CLI_H
#define UART_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise UART CLI.
 *        Enables the NVIC for USART1 and arms the first HAL_UART_Receive_IT.
 *        Must be called after MX_USART1_UART_Init().
 */
void uart_cli_init(void);

/**
 * @brief Process one received line if available.
 *        Call from a FreeRTOS task:
 *          while (1) { uart_cli_task(); vTaskDelay(10); }
 */
void uart_cli_task(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_CLI_H */
