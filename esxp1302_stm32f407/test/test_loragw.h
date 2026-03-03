/*
 * test_loragw.h  –  Test selection header for STM32F407
 *
 * Define exactly ONE of these in CMakeLists.txt compile definitions:
 *   TEST_LORAGW_SPI           – SPI read/write stress test
 *   TEST_LORAGW_SPI_SX1250    – SX1250 register R/W via SX1302
 *   TEST_LORAGW_REG           – SX1302 register default-value & R/W test
 *   TEST_LORAGW_I2C_OLED      – SSD1306 OLED on I2C2 (PF0/PF1)
 *   TEST_LORAGW_I2C_LM75A     – LM75A temperature sensor on I2C2
 *   TEST_W5500_UDP             – W5500 Ethernet + UDP echo test
 *   TEST_CONFIG_CLI            – Flash config R/W + UART CLI validation
 */

#ifndef _TEST_LORAGW_H
#define _TEST_LORAGW_H

/* Declarations of all available test entry points */
void test_loragw_spi(void);
void test_loragw_spi_sx1250(void);
void test_loragw_reg(void);
void test_loragw_i2c_oled(void);
void test_loragw_i2c_lm75a(void);
void test_w5500_udp(void);
void test_config_cli(void);

/**
 * @brief Run the selected test. Call from a FreeRTOS task.
 */
static inline void test_loragw_run(void) {
#if defined(TEST_LORAGW_SPI)
    test_loragw_spi();
#elif defined(TEST_LORAGW_SPI_SX1250)
    test_loragw_spi_sx1250();
#elif defined(TEST_LORAGW_REG)
    test_loragw_reg();
#elif defined(TEST_LORAGW_I2C_OLED)
    test_loragw_i2c_oled();
#elif defined(TEST_LORAGW_I2C_LM75A)
    test_loragw_i2c_lm75a();
#elif defined(TEST_W5500_UDP)
    test_w5500_udp();
#elif defined(TEST_CONFIG_CLI)
    test_config_cli();
#else
    #error "No test selected. See test_loragw.h for available TEST_LORAGW_* macros."
#endif
}

#endif /* _TEST_LORAGW_H */
