# Bug 修改记录索引

| 编号 | 日期 | 文件 | 简要描述 | 严重级别 |
|------|------|------|----------|----------|
| [BUG-001](BUG-001_test_network_connection_compile_errors.md) | 2026-02-19 | `main/test/test_network_connection.c` | 格式符类型不匹配、xTaskCreate参数过多、esp_console API废弃、事件句柄注销传反 | 编译失败 + 运行崩溃 |
| [BUG-002](BUG-002_radio_spi_rb_wrong_frame_format.md) | 2026-02-19 | `main/libloragw/loragw_spi.c` | `radio_spi_rb` SPI 帧 address_bits 多1字节且 tx_buffer 全零，导致 SX1250 寄存器回读全零 | 功能性错误 |
| [BUG-003](BUG-003_test_loragw_hal_rx_compile_errors.md) | 2026-02-19 | `main/test/test_loragw_hal_rx.c` | 格式符类型不匹配（`%d` vs `uint32_t`）、esp_console REPL API废弃 | 编译失败 |
| [BUG-004](BUG-004_gpio_pin_mask_error_enum_in_preprocessor.md) | 2026-02-19 | `main/libloragw/loragw_gpio.h` | `SX1302_POWER_EN_PIN` 默认值为枚举 `GPIO_NUM_NC`，预处理器 `#if` 无法识别枚举值将其当 `0` 处理，导致 `pin_bit_mask` 含无效位，`gpio_config()` 报 GPIO_PIN mask error | 功能性错误 |
| [BUG-005](BUG-005_test_loragw_cal_compile_errors_and_invalid_for_sx1250.md) | 2026-02-20 | `main/test/test_loragw_cal.c` | 函数名变更（`lgw_sx125x_reg_w/r`→`sx125x_reg_w/r`）、格式符类型不匹配、`lgw_connect` 参数不足、FreeRTOS 头文件缺失；另：该测试仅适用于 SX1255/SX1257，在 SX1250 平台上测试结果完全无效 | 编译失败 + 功能性无效 |
| [BUG-006](BUG-006_cmake_include_dirs_multiple_declaration.md) | 2026-02-20 | `main/CMakeLists.txt` | `idf_component_register` 中多次声明 `INCLUDE_DIRS`，CMake 只保留最后一条，`main/` 根目录及其余子目录头文件均不可见，导致 `global_json.h` 编译失败 | 编译失败 |
| [BUG-007](BUG-007_webpage_h_not_generated_by_idf_build.md) | 2026-02-20 | `main/packet_forwarder/webpage.h`（生成文件） | `webpage.h` 由 PlatformIO 预构建钩子生成，ESP-IDF CMake 构建不自动执行，首次构建或 clean 后需手动运行 `scripts/dump_html.py` | 编译失败 |
| [BUG-008](BUG-008_esp32s3_invalid_gpio_pin_defaults.md) | 2026-02-20 | `main/libloragw/loragw_i2c.h`、`main/packet_forwarder/lora_pkt_fwd.c`、`main/board_config.h` | I2C 引脚默认 GPIO 22（ESP32-S3 不存在），按键引脚默认 GPIO 23/25（ESP32-S3 不存在），导致运行时 `i2c_set_pin error` 和 `gpio_set_direction error`；新建 `board_config.h` 统一管理 | 运行时崩溃 |
| [BUG-009](BUG-009_http_431_httpd_max_req_hdr_len_too_small.md) | 2026-02-20 | `sdkconfig`、`sdkconfig.defaults` | `CONFIG_HTTPD_MAX_REQ_HDR_LEN` 默认值 512 字节不够现代浏览器请求头，Web 配置页面返回 HTTP 431；`httpd_config_t` 无运行时字段，只能通过 Kconfig 编译时调整为 2048 | 功能性错误 |
| [BUG-011](BUG-011_tmms_wrong_gps_epoch_nmea_only.md) | 2026-03-02 | `main/libloragw/loragw_gps.c` | `gps_gps_time` 仅由 UBX NAV-TIMEGPS 消息赋值，ATGM336H（NMEA-only）永远不发此帧，导致 `ref.gps={0,0}`，`lgw_cnt2gps()` 输出约等于设备运行时间，`tmms` 显示 1980 GPS 纪元附近（如 `523537 ms`）而非当前时间（约 `1456477xxxxx ms`）。修复：以 `gps_week` 为标志区分 UBX/NMEA 模式，NMEA 模式下从 UTC 派生 GPS 纪元时间 | 功能性错误 |
| [BUG-010](BUG-010_push_data_ack_low_out_of_sync.md) | 2026-02-21 | `main/packet_forwarder/lora_pkt_fwd.c`、`global_conf.cn490.json` | 四个独立问题导致 ackr 低至 22%：① MQTT TLS 阻塞 10s；② `tv_usec` 溢出（`PUSH_TIMEOUT_MS=2000` → `tv_usec=1,000,000`，lwIP 处理为 0）；③ drain 循环双 `setsockopt` 失败风险；④ WiFi Modem Sleep 导致 AP 缓冲 ACK 100–300ms 超出 recv() 窗口。修复后 ackr 达 86–100% | 功能性错误 |
