# ESXP1302 STM32F407 移植文档总索引

> **项目**：SX1302 LoRa 网关驱动 — 从 ESP32S3 (ESP-IDF) 移植到 STM32F407ZGT6 (STM32 HAL + FreeRTOS)  
> **分支**：`F407`  
> **日期**：2026-02-28 ~ 2026-03-03  
> **状态**：SPI/REG/SX1250/OLED/LM75A/HAL-RX/HAL-TX 全部硬件验证通过

---

## 一、时间线还原路径（按顺序阅读可完整复现）

按以下顺序阅读并操作，即可从零完成全部移植工作：

| 阶段 | 文档 | 说明 |
|------|------|------|
| 1 | [hardware/01_hardware_overview.md](hardware/01_hardware_overview.md) | 硬件平台、接线、引脚映射 |
| 2 | [impl/01_cmake_setup.md](impl/01_cmake_setup.md) | CubeMX 工程 + CMake + VS Code 搭建 |
| 2.1 | [impl/05_cross_platform_config.md](impl/05_cross_platform_config.md) | 跨平台构建配置（STM32_CLT_PATH） |
| 3 | [impl/02_platform_adapt.md](impl/02_platform_adapt.md) | 所有 ESP32→STM32 适配改动 |
| 4 | [impl/03_driver_layers.md](impl/03_driver_layers.md) | 驱动层文件清单与依赖关系 |
| 5 | [testing/01_bringup_tests.md](testing/01_bringup_tests.md) | SPI/SX1250/REG/OLED/LM75A 五项测试 |
| 6 | [testing/02_hal_rx_test.md](testing/02_hal_rx_test.md) | HAL RX 测试（含 E77 节点配置） |
| 7 | [testing/03_hal_tx_test.md](testing/03_hal_tx_test.md) | HAL TX 测试（含 SX1278 接收配置） |
| 随用 | [testing/04_scripts_reference.md](testing/04_scripts_reference.md) | Python 测试脚本用法手册 |
| 8 | [impl/06_freq_plan_flash_config_v4.md](impl/06_freq_plan_flash_config_v4.md) | 多区域频率计划、Flash 配置 v4、UART CLI 频率命令 |
| 9 | [testing/08_chirpstack_gateway_online_test.md](testing/08_chirpstack_gateway_online_test.md) | ChirpStack 网关上线调试与 stat 时间戳 Bug 修复 |
| 10 | [learning/05_ntp_sntp_concepts.md](learning/05_ntp_sntp_concepts.md) | NTP/SNTP 概念与原理（协议格式、软件时钟模型、Newlib 桥接） |
| 10.1 | [impl/07_sntp_implementation.md](impl/07_sntp_implementation.md) | SNTP 代码实现剖析（调用链、状态机、5级回退机制） |
| 10.2 | [impl/07_sntp_configuration.md](impl/07_sntp_configuration.md) | SNTP 功能配置清单（网络、Flash 参数、CMake、CubeMX） |
| 11 | [learning/06_stm32_i2c_freertos.md](learning/06_stm32_i2c_freertos.md) | STM32 I2C + FreeRTOS 陷阱（Errata ES0182、HAL_LOCK、Mutex） |
| 11.1 | [impl/08_oled_display_bug.md](impl/08_oled_display_bug.md) | OLED Row 6 永久空白 Bug 全程分析（5秒窗口、超时、BUSY锁死、竞态） |

---

## 二、分类速查

### 硬件 / 接线
- [hardware/01_hardware_overview.md](hardware/01_hardware_overview.md) — 引脚映射、外设配置

### 工程搭建 / 代码适配
- [impl/01_cmake_setup.md](impl/01_cmake_setup.md) — CMake + Ninja + CubeMX
- [impl/05_cross_platform_config.md](impl/05_cross_platform_config.md) — 跨平台构建配置（STM32_CLT_PATH 环境变量方案）
- [impl/02_platform_adapt.md](impl/02_platform_adapt.md) — 平台适配速查表
- [impl/03_driver_layers.md](impl/03_driver_layers.md) — 源文件依赖关系
- [impl/06_freq_plan_flash_config_v4.md](impl/06_freq_plan_flash_config_v4.md) — 多区域频率计划预设、Flash 配置 v4、UART CLI 频率命令
- [impl/07_sntp_implementation.md](impl/07_sntp_implementation.md) — SNTP 代码实现剖析（软件时钟、调用链、5级回退、Newlib桥接）
- [impl/07_sntp_configuration.md](impl/07_sntp_configuration.md) — SNTP 功能配置清单（Flash参数、CMake、CubeMX、网络前提）
- [impl/08_oled_display_bug.md](impl/08_oled_display_bug.md) — OLED Row 6 永久空白 Bug：5秒窗口 + I2C超时 + BUSY Errata + 无Mutex + 沉默失败

### 测试流程 / 脚本
- [testing/01_bringup_tests.md](testing/01_bringup_tests.md) — 底层外设测试
- [testing/02_hal_rx_test.md](testing/02_hal_rx_test.md) — 网关收包测试
- [testing/03_hal_tx_test.md](testing/03_hal_tx_test.md) — 网关发包测试
- [testing/04_scripts_reference.md](testing/04_scripts_reference.md) — E77 脚本
- [testing/08_chirpstack_gateway_online_test.md](testing/08_chirpstack_gateway_online_test.md) — ChirpStack 网关上线调试（stat 时间戳 Bug）

### 背景知识
- [learning/01_sx1302_architecture.md](learning/01_sx1302_architecture.md) — SX1302 架构
- [learning/02_lorawan_basics.md](learning/02_lorawan_basics.md) — LoRaWAN / CN470
- [learning/03_stm32_hal_notes.md](learning/03_stm32_hal_notes.md) — STM32 HAL 注意事项
- [learning/04_lora_rf_notes.md](learning/04_lora_rf_notes.md) — LoRa RF 参数
- [learning/05_ntp_sntp_concepts.md](learning/05_ntp_sntp_concepts.md) — NTP/SNTP 概念（协议报文、Unix/NTP纪元、软件时钟模型、Newlib time()）
- [learning/06_stm32_i2c_freertos.md](learning/06_stm32_i2c_freertos.md) — STM32 I2C + FreeRTOS 陷阱（100kHz时序、Errata ES0182 BUSY锁死、HAL_LOCK非原子、Mutex对比）

### Bug 速查
- [troubleshooting/bugs_and_fixes.md](troubleshooting/bugs_and_fixes.md) — 全部问题与修复

### 参考资料
- [references/INDEX.md](references/INDEX.md) — 数据手册 / AT 命令 / 标准文档

### ESP32 原始文档（待整理）
- [archive/INDEX.md](archive/INDEX.md) — bringup/test 分支原始 doc

---

## 三、关键源文件索引

| 源文件 | 说明 | 详见文档 |
|--------|------|----------|
| `board_config.h` | 引脚映射 / SPI / I2C / GPIO 配置 | [hardware/01](hardware/01_hardware_overview.md) |
| `config.h` | 调试开关 | [impl/03](impl/03_driver_layers.md) |
| `loragw_spi.c/h` | SX1302 SPI 驱动 | [impl/02](impl/02_platform_adapt.md), [impl/03](impl/03_driver_layers.md) |
| `loragw_aux.c/h` | 定时、超时、ToA 计算 | [impl/02](impl/02_platform_adapt.md) |
| `loragw_gpio.c/h` | SX1302 复位 GPIO | [impl/02](impl/02_platform_adapt.md) |
| `loragw_i2c.c/h` | I2C 总线驱动 | [impl/02](impl/02_platform_adapt.md), [impl/03](impl/03_driver_layers.md) |
| `loragw_hal.c` | 核心 HAL（start/stop/receive/send） | [impl/03](impl/03_driver_layers.md) |
| `loragw_sx1302.c/h` | SX1302 寄存器+通道配置 | [impl/03](impl/03_driver_layers.md) |
| `loragw_cal.c/h` | 校准流程 | [impl/03](impl/03_driver_layers.md) |
| `test_loragw.h` | 测试选择器 | [testing/01](testing/01_bringup_tests.md) |
| `test_loragw_hal_rx.c` | HAL RX 测试 | [testing/02](testing/02_hal_rx_test.md) |
| `test_loragw_hal_tx.c` | HAL TX 测试 | [testing/03](testing/03_hal_tx_test.md) |
| `scripts/e77_node_tx.py` | E77 节点发包脚本 | [testing/04](testing/04_scripts_reference.md) |
| `scripts/e77_probe.py` | E77 串口探测脚本 | [testing/04](testing/04_scripts_reference.md) |
| `sntp_client.c/h` | SNTP 客户端（软件时钟、后台任务、5级回退） | [impl/07](impl/07_sntp_implementation.md) |
| `sntp_gettimeofday.c` | Newlib `_gettimeofday()` 强符号桥接 | [impl/07](impl/07_sntp_implementation.md) |
| `loragw_i2c.c/h` | I2C2 驱动（FreeRTOS Mutex、动态超时、BUSY恢复） | [impl/08](impl/08_oled_display_bug.md) |
| `loragw_oled.c/h` | SSD1306 OLED 驱动（帧缓冲、err\|=累积） | [impl/08](impl/08_oled_display_bug.md) |
