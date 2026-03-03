/*
 * gateway_config.c  –  Flash-based gateway configuration storage
 *
 * Flash Sector 11:  0x080E0000 – 0x080FFFFF (128KB)
 * Firmware is only ~43KB so this sector is free for config use.
 *
 * Erase: VOLTAGE_RANGE_3 (2.7–3.6V), 32-bit parallelism
 * Write: FLASH_TYPEPROGRAM_WORD (4 bytes at a time)
 * Read:  memory-mapped – plain pointer cast
 */

#include "gateway_config.h"

#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  Flash placement                                                    */
/* ------------------------------------------------------------------ */

#define CONFIG_FLASH_SECTOR   FLASH_SECTOR_11
#define CONFIG_FLASH_ADDR     0x080E0000UL
#define CONFIG_FLASH_VOLTAGE  FLASH_VOLTAGE_RANGE_3  /* 2.7–3.6 V, 32-bit */

/* ------------------------------------------------------------------ */
/*  Global RAM copy                                                    */
/* ------------------------------------------------------------------ */

static gateway_config_t s_config;

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                   */
/* ------------------------------------------------------------------ */

static uint32_t checksum_compute(const gateway_config_t *cfg)
{
    const uint8_t *p = (const uint8_t *)cfg;
    uint32_t sum = 0;
    /* Sum every byte EXCEPT the trailing checksum field itself */
    for (size_t i = 0; i < offsetof(gateway_config_t, checksum); i++) {
        sum += p[i];
    }
    return sum;
}

static int config_is_valid(const gateway_config_t *cfg)
{
    if (cfg->magic != CONFIG_MAGIC) {
        return 0;
    }
    return (cfg->checksum == checksum_compute(cfg));
}

static void apply_defaults(gateway_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->magic = CONFIG_MAGIC;

    strncpy(cfg->ns_host, CONFIG_DEFAULT_NS_HOST, sizeof(cfg->ns_host) - 1);
    cfg->ns_host[sizeof(cfg->ns_host) - 1] = '\0';
    cfg->ns_port_up   = CONFIG_DEFAULT_NS_PORT_UP;
    cfg->ns_port_down = CONFIG_DEFAULT_NS_PORT_DOWN;
    cfg->gateway_eui  = CONFIG_DEFAULT_GW_EUI;

    const uint8_t ip[] = CONFIG_DEFAULT_ETH_IP;
    const uint8_t gw[] = CONFIG_DEFAULT_ETH_GW;
    const uint8_t sn[] = CONFIG_DEFAULT_ETH_SN;
    memcpy(cfg->eth_ip, ip, 4);
    memcpy(cfg->eth_gw, gw, 4);
    memcpy(cfg->eth_sn, sn, 4);

    cfg->checksum = checksum_compute(cfg);
}

/* Write the RAM config struct to Flash.
   Assumes cfg->checksum has already been updated. */
static HAL_StatusTypeDef flash_write(const gateway_config_t *cfg)
{
    HAL_StatusTypeDef st;
    uint32_t addr = CONFIG_FLASH_ADDR;

    /* --- Erase sector --- */
    st = HAL_FLASH_Unlock();
    if (st != HAL_OK) {
        printf("[CFG] Flash unlock error %d\r\n", (int)st);
        return st;
    }

    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .VoltageRange = CONFIG_FLASH_VOLTAGE,
        .Sector       = CONFIG_FLASH_SECTOR,
        .NbSectors    = 1,
    };
    uint32_t sector_error = 0;
    st = HAL_FLASHEx_Erase(&erase, &sector_error);
    if (st != HAL_OK) {
        printf("[CFG] Flash erase error %d (sector_error=%lu)\r\n",
               (int)st, (unsigned long)sector_error);
        HAL_FLASH_Lock();
        return st;
    }

    /* --- Program word-by-word --- */
    const uint8_t *src = (const uint8_t *)cfg;
    size_t size = sizeof(gateway_config_t);

    for (size_t offset = 0; offset < size; offset += 4) {
        uint32_t word;
        memcpy(&word, src + offset, 4);
        st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + offset, (uint64_t)word);
        if (st != HAL_OK) {
            printf("[CFG] Flash program error at offset %u\r\n", (unsigned)offset);
            HAL_FLASH_Lock();
            return st;
        }
    }

    HAL_FLASH_Lock();
    return HAL_OK;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

void config_load(void)
{
    const gateway_config_t *flash = (const gateway_config_t *)CONFIG_FLASH_ADDR;

    if (config_is_valid(flash)) {
        memcpy(&s_config, flash, sizeof(s_config));
        printf("[CFG] Loaded from Flash (valid).\r\n");
    } else {
        printf("[CFG] Flash empty/invalid – using defaults.\r\n");
        apply_defaults(&s_config);
    }
}

void config_save(void)
{
    s_config.checksum = checksum_compute(&s_config);

    HAL_StatusTypeDef st = flash_write(&s_config);
    if (st == HAL_OK) {
        printf("[CFG] Saved to Flash OK.\r\n");
    } else {
        printf("[CFG] Save FAILED (HAL error %d).\r\n", (int)st);
    }
}

void config_reset_defaults(void)
{
    apply_defaults(&s_config);
    printf("[CFG] Defaults applied.\r\n");
    config_save();
}

void config_print(void)
{
    printf("--- Gateway Configuration ---\r\n");
    printf("  ns_host       : %s\r\n", s_config.ns_host);
    printf("  ns_port_up    : %u\r\n", (unsigned)s_config.ns_port_up);
    printf("  ns_port_down  : %u\r\n", (unsigned)s_config.ns_port_down);
    printf("  gateway_eui   : %016llX\r\n", (unsigned long long)s_config.gateway_eui);
    printf("  eth_ip        : %u.%u.%u.%u\r\n",
           s_config.eth_ip[0], s_config.eth_ip[1],
           s_config.eth_ip[2], s_config.eth_ip[3]);
    printf("  eth_gw        : %u.%u.%u.%u\r\n",
           s_config.eth_gw[0], s_config.eth_gw[1],
           s_config.eth_gw[2], s_config.eth_gw[3]);
    printf("  eth_sn        : %u.%u.%u.%u\r\n",
           s_config.eth_sn[0], s_config.eth_sn[1],
           s_config.eth_sn[2], s_config.eth_sn[3]);
    printf("  checksum      : 0x%08X\r\n", (unsigned)s_config.checksum);
    printf("-----------------------------\r\n");
}

gateway_config_t *config_get(void)
{
    return &s_config;
}
