/*
 * test_loragw.h  –  Test selection header for STM32F407
 *
 * Define exactly ONE of these in CMakeLists.txt compile definitions:
 *   TEST_LORAGW_SPI        – SPI read/write stress test
 *   TEST_LORAGW_SPI_SX1250 – SX1250 register R/W via SX1302
 *   TEST_LORAGW_REG        – SX1302 register default-value & R/W test
 */

#ifndef _TEST_LORAGW_H
#define _TEST_LORAGW_H

/* Declarations of all available test entry points */
void test_loragw_spi(void);
void test_loragw_spi_sx1250(void);
void test_loragw_reg(void);

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
#else
    #error "No test selected. Define TEST_LORAGW_SPI, TEST_LORAGW_SPI_SX1250, or TEST_LORAGW_REG."
#endif
}

#endif /* _TEST_LORAGW_H */
