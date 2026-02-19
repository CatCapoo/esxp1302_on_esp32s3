# Bug 修改记录索引

| 编号 | 日期 | 文件 | 简要描述 | 严重级别 |
|------|------|------|----------|----------|
| [BUG-001](BUG-001_test_network_connection_compile_errors.md) | 2026-02-19 | `main/test/test_network_connection.c` | 格式符类型不匹配、xTaskCreate参数过多、esp_console API废弃、事件句柄注销传反 | 编译失败 + 运行崩溃 |
| [BUG-002](BUG-002_radio_spi_rb_wrong_frame_format.md) | 2026-02-19 | `main/libloragw/loragw_spi.c` | `radio_spi_rb` SPI 帧 address_bits 多1字节且 tx_buffer 全零，导致 SX1250 寄存器回读全零 | 功能性错误 |
