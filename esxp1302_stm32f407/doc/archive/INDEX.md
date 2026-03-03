# Archive — ESP32 bringup/test 分支文档

本目录保存了 ESP32S3 平台 `bringup/test` 分支的原始文档，供日后参考。
这些文档记录了 ESP32 平台上的移植工作、Bug 修复和测试记录。

> 文件从 `git show bringup/test:doc/<path>` 导出，未做修改。

---

## 目录结构

### 根目录文档

| 文件 | 说明 |
|------|------|
| [CHANGELOG.md](CHANGELOG.md) | 变更日志 |
| [esxp1302_upgrade_notes.md](esxp1302_upgrade_notes.md) | ESP32 升级说明 |
| [notes_tips.md](notes_tips.md) | 开发笔记与提示 |
| [todo_issue_list.md](todo_issue_list.md) | 待办与问题列表 |
| [user_guide.md](user_guide.md) | 用户指南 |
| [windows_build.md](windows_build.md) | Windows 构建说明 |

### bugfix/ — Bug 修复记录

| 文件 | 说明 |
|------|------|
| [INDEX.md](bugfix/INDEX.md) | 索引 |
| [BUG-001](bugfix/BUG-001_test_network_connection_compile_errors.md) | 网络连接编译错误 |
| [BUG-002](bugfix/BUG-002_radio_spi_rb_wrong_frame_format.md) | Radio SPI 帧格式错误 |
| [BUG-003](bugfix/BUG-003_test_loragw_hal_rx_compile_errors.md) | HAL RX 编译错误 |
| [BUG-004](bugfix/BUG-004_gpio_pin_mask_error_enum_in_preprocessor.md) | GPIO 枚举预处理器错误 |
| [BUG-005](bugfix/BUG-005_test_loragw_cal_compile_errors_and_invalid_for_sx1250.md) | 校准编译错误 |
| [BUG-006](bugfix/BUG-006_cmake_include_dirs_multiple_declaration.md) | CMake 头文件重复声明 |
| [BUG-007](bugfix/BUG-007_webpage_h_not_generated_by_idf_build.md) | webpage.h 未生成 |
| [BUG-008](bugfix/BUG-008_esp32s3_invalid_gpio_pin_defaults.md) | ESP32S3 GPIO 默认值无效 |
| [BUG-009](bugfix/BUG-009_http_431_httpd_max_req_hdr_len_too_small.md) | HTTP 431 请求头过小 |
| [BUG-010](bugfix/BUG-010_push_data_ack_low_out_of_sync.md) | push_data ACK 不同步 |
| [BUG-011](bugfix/BUG-011_tmms_wrong_gps_epoch_nmea_only.md) | GPS 时间 epoch 错误 |

### change_notes/ — 改动记录

| 文件 | 说明 |
|------|------|
| [INDEX.md](change_notes/INDEX.md) | 索引 |
| [CHANGE-001](change_notes/CHANGE-001_lm75a_driver_port.md) | LM75A 驱动移植 |
| [CHANGE-002](change_notes/CHANGE-002_gps_thread_esp32_port.md) | GPS 线程移植 |

### learning/ — 学习笔记

| 文件 | 说明 |
|------|------|
| [README.md](learning/README.md) | 索引 |
| [esxp1302_code_walkthrough.md](learning/esxp1302_code_walkthrough.md) | 代码走读 |
| [esxp1302_timestamp_system.md](learning/esxp1302_timestamp_system.md) | 时间戳系统 |
| [lora_cn470_frequency_plan.md](learning/lora_cn470_frequency_plan.md) | CN470 频率计划 |
| [lorawan_gateway_ns_protocol.md](learning/lorawan_gateway_ns_protocol.md) | 网关-NS 协议 |
| [lorawan_node_e77_otaa_debug.md](learning/lorawan_node_e77_otaa_debug.md) | E77 OTAA 调试 |
| [lm75a_i2c_driver_porting.md](learning/lm75a_i2c_driver_porting.md) | LM75A 驱动移植笔记 |
| [gps_nmea_and_lorawan_sync.md](learning/gps_nmea_and_lorawan_sync.md) | GPS/NMEA 同步 |
| [chirpstack_v4_usage.md](learning/chirpstack_v4_usage.md) | ChirpStack V4 使用 |
| [BUG-010_notes.md](learning/BUG-010_notes.md) | BUG-010 分析笔记 |

### test_notes/ — 测试日志

| 文件 | 说明 |
|------|------|
| [test_loragw_hal_rx_memo.md](test_notes/test_loragw_hal_rx_memo.md) | HAL RX 测试日志 |
| [test_loragw_hal_tx_memo.md](test_notes/test_loragw_hal_tx_memo.md) | HAL TX 测试日志 |
| [test_pkt_fwd_memo.md](test_notes/test_pkt_fwd_memo.md) | Packet Forwarder 测试 |
| [test_pkt_fwd_ns_uplink_memo.md](test_notes/test_pkt_fwd_ns_uplink_memo.md) | 上行数据测试 |
| [test_e77_lorawan_node_validation_memo.md](test_notes/test_e77_lorawan_node_validation_memo.md) | E77 节点验证 |
| [test_chirpstack_cn470_10_setup_memo.md](test_notes/test_chirpstack_cn470_10_setup_memo.md) | ChirpStack CN470 配置 |
| [test_gps_pps_tmms_chirpstack_memo.md](test_notes/test_gps_pps_tmms_chirpstack_memo.md) | GPS PPS 测试 |
| [windows_build_memo.md](test_notes/windows_build_memo.md) | Windows 构建日志 |
