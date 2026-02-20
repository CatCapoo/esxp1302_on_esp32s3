# Bug 修改记录索引

| 编号 | 日期 | 文件 | 简要描述 | 严重级别 |
|------|------|------|----------|----------|
| [BUG-001](BUG-001_test_network_connection_compile_errors.md) | 2026-02-19 | `main/test/test_network_connection.c` | 格式符类型不匹配、xTaskCreate参数过多、esp_console API废弃、事件句柄注销传反 | 编译失败 + 运行崩溃 |
| [BUG-002](BUG-002_radio_spi_rb_wrong_frame_format.md) | 2026-02-19 | `main/libloragw/loragw_spi.c` | `radio_spi_rb` SPI 帧 address_bits 多1字节且 tx_buffer 全零，导致 SX1250 寄存器回读全零 | 功能性错误 |
| [BUG-003](BUG-003_test_loragw_hal_rx_compile_errors.md) | 2026-02-19 | `main/test/test_loragw_hal_rx.c` | 格式符类型不匹配（`%d` vs `uint32_t`）、esp_console REPL API废弃 | 编译失败 |
| [BUG-004](BUG-004_gpio_pin_mask_error_enum_in_preprocessor.md) | 2026-02-19 | `main/libloragw/loragw_gpio.h` | `SX1302_POWER_EN_PIN` 默认值为枚举 `GPIO_NUM_NC`，预处理器 `#if` 无法识别枚举值将其当 `0` 处理，导致 `pin_bit_mask` 含无效位，`gpio_config()` 报 GPIO_PIN mask error | 功能性错误 |
