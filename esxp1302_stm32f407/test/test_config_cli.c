/*
 * test_config_cli.c  –  Validate Flash config storage + UART CLI
 *
 * Test sequence
 * ─────────────
 * Phase 1 – Automated Flash R/W verification
 *   1. Load config from Flash (uses defaults if Flash is blank/invalid)
 *   2. Print current config
 *   3. Write a known test value for ns_host and save to Flash
 *   4. Corrupt the RAM struct
 *   5. Reload from Flash
 *   6. Verify the reloaded value matches what was written → PASS / FAIL
 *   7. Restore previous ns_host value and save
 *
 * Phase 2 – Interactive UART CLI
 *   Start the CLI and enter a FreeRTOS task loop.
 *   Open a serial terminal at 115200 8N1 and type 'help'.
 */

#include "test_loragw.h"
#include "gateway_config.h"
#include "uart_cli.h"

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"

/* ------------------------------------------------------------------ */
/*  CLI task wrapper                                                   */
/* ------------------------------------------------------------------ */

static void cli_task_fn(void *arg)
{
    (void)arg;
    uart_cli_init();

    for (;;) {
        uart_cli_task();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ------------------------------------------------------------------ */
/*  Entry point                                                        */
/* ------------------------------------------------------------------ */

void test_config_cli(void)
{
    printf("\r\n");
    printf("==================================================\r\n");
    printf("  test_config_cli – Flash Config + UART CLI test  \r\n");
    printf("==================================================\r\n");

    /* ---- Phase 1: Load current config ---- */
    config_load();
    printf("\r\n[TEST] Current config after load:\r\n");
    config_print();

    /* Remember original ns_host */
    char orig_ns_host[64];
    strncpy(orig_ns_host, config_get()->ns_host, sizeof(orig_ns_host) - 1);
    orig_ns_host[sizeof(orig_ns_host) - 1] = '\0';

    /* ---- Phase 2: Write a known value and save ---- */
    const char *test_host = "10.99.88.77";
    printf("[TEST] Setting ns_host = \"%s\" and saving to Flash...\r\n", test_host);
    strncpy(config_get()->ns_host, test_host, sizeof(config_get()->ns_host) - 1);
    config_save();

    /* ---- Phase 3: Corrupt RAM and reload ---- */
    printf("[TEST] Corrupting RAM copy...\r\n");
    memset(config_get()->ns_host, 0xAA, sizeof(config_get()->ns_host));

    printf("[TEST] Reloading from Flash...\r\n");
    config_load();

    /* ---- Phase 4: Verify ---- */
    if (strcmp(config_get()->ns_host, test_host) == 0) {
        printf("[TEST] PASS  –  ns_host correctly persisted: \"%s\"\r\n",
               config_get()->ns_host);
    } else {
        printf("[TEST] FAIL  –  ns_host mismatch. Got: \"%s\"  Expected: \"%s\"\r\n",
               config_get()->ns_host, test_host);
    }

    /* ---- Phase 5: Restore original ns_host ---- */
    printf("[TEST] Restoring original ns_host = \"%s\"\r\n", orig_ns_host);
    strncpy(config_get()->ns_host, orig_ns_host, sizeof(config_get()->ns_host) - 1);
    config_save();

    printf("\r\n[TEST] Flash R/W verification complete.\r\n");

    /* ---- Phase 6: Start interactive UART CLI ---- */
    printf("[TEST] Starting interactive CLI (UART1, 115200 8N1)...\r\n");
    printf("       Open a serial terminal and type 'help'.\r\n\r\n");

    xTaskCreate(cli_task_fn, "cli", 512, NULL, tskIDLE_PRIORITY + 1, NULL);

    /* This test entry task is done — suspend itself */
    vTaskSuspend(NULL);
}
