/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2023 Semtech

Description:
    Tests for SPI function checking

License: Revised BSD License, see LICENSE.TXT file include in the project
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "loragw_spi.h"
#include "loragw_com.h"

#ifndef SX1302_REG_COMMON
#define SX1302_REG_COMMON   0x5600
#endif


// get sx1302 version
uint8_t spi_get_version(spi_device_handle_t *spi)
{
    esp_err_t ret;
    spi_transaction_t t;
    uint8_t buf[] = {0x00, 0x56, 0x06, 0x00, 0x00};
    uint8_t rbuf[16];

    memset(&t, 0, sizeof(t));
    t.length = 8 * 5;
    t.tx_buffer = buf;
    t.rx_buffer = rbuf;

    ret = spi_device_polling_transmit(*spi, &t);
    if (ret != ESP_OK) {
        printf("spi tx failed: %d\n", ret);
        return 0xFF;
    }

    printf("spi rbuf:");
    for (int i = 0; i < 5; ++i) {
        printf(" %02X", rbuf[i]);
    }
    printf("\n");

    int all_ff = 1, all_00 = 1;
    for (int i = 0; i < 5; ++i) {
        if (rbuf[i] != 0xFF) all_ff = 0;
        if (rbuf[i] != 0x00) all_00 = 0;
    }
    if (all_ff || all_00) {
        printf("read looks invalid (all %s)\n", all_ff ? "FF" : "00");
        return 0xFF;
    }

    return rbuf[4];
}


void app_main(void)
{
    spi_device_handle_t *spi = NULL;
    uint8_t version;

    printf("Testing ESXP1302 SPI...\n");

    for(int i = 0; i < 2; i++){
        printf("waiting %d...\n", i+1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // Get SX1302 Versioin
    lgw_spi_open(&spi);
    version = spi_get_version(spi);
    printf("version = 0x%x\n", version);
    {
        uint8_t vreg = 0;
        lgw_spi_r(spi, LGW_SPI_MUX_TARGET_SX1302, SX1302_REG_COMMON + 6, &vreg);
        printf("REG_COMMON+6 = 0x%02X\n", vreg);
    }
    lgw_spi_close(spi);

    // test it again to check open()/close()
    lgw_spi_open(&spi);
    version = spi_get_version(spi);
    printf("version = 0x%x\n", version);
    lgw_spi_close(spi);

    while(true){
        printf("hello\n");
        vTaskDelay(8000 / portTICK_PERIOD_MS);
    }
}
