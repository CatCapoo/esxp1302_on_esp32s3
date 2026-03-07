# Bug 汇总与修复记录

本文档汇总移植过程中遇到的所有 Bug，按发现阶段排列。

---

## 阶段一: 编译与链接

### B01 — `_write` / `_read` 等 syscall 缺失

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | 链接报错 `undefined reference to '_write'`, `_read`, `_close` 等 |
| 原因 | newlib 需要底层 I/O 桩函数，ESP32 对应函数不存在于 STM32 环境 |
| 修复 | 启用 `syscalls.c` 或在 CMakeLists.txt 中添加 `--specs=nosys.specs` |

### B02 — 浮点 printf 不输出

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 / 运行期 |
| 现象 | `printf("%.1f", value)` 不输出任何字符，整数 `%d` 正常 |
| 原因 | newlib-nano 默认不包含浮点格式化 |
| 修复 | CMakeLists.txt 添加 `target_link_options(... PRIVATE -u_printf_float)` |

### B03 — `struct timespec` 未定义

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | `error: unknown type name 'struct timespec'` |
| 原因 | ESP32 代码使用 POSIX `clock_gettime()`，STM32 无此支持 |
| 修复 | 用 `HAL_GetTick()` / TIM2 替代，删除 `<time.h>` 依赖 |

### B04 — `pthread.h` 未找到

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | `fatal error: pthread.h: No such file or directory` |
| 原因 | ESP32 使用 POSIX 线程 API，STM32 bare-metal 无此支持 |
| 修复 | 移除 `pthread_mutex_*` 调用，如需互斥使用 FreeRTOS `osMutex*` |

### B05 — `esp_log.h` / `esp_err.h` 等 ESP-IDF 头文件

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | 大量 ESP-IDF 特定头文件找不到 |
| 修复 | 创建 `config.h` 中定义替代宏 (`ESP_OK` → `0`, `ESP_LOGI` → `printf`) |

### B06 — `jit_queue` / `jit_result` 未定义

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | HAL TX 路径引用 JIT queue 类型但未包含 |
| 修复 | 添加头文件包含或在 config.h 中声明前置类型 |

---

## 阶段二: SPI / SX1250 Bringup

### B07 — SX1250 GET_STATUS 返回错误值

| 项目 | 内容 |
|------|------|
| 阶段 | TEST_LORAGW_SPI_SX1250 |
| 现象 | `GET_STATUS` 返回 `0x00` 或 `0xA2` 而非期望的 `0x32` |
| 原因 | SPI bridge 的 opcode 高位被 `& 0x7F` 掩码截断，`0xC0` → `0x40` |
| 修复 | 删除 `sx1302_reg_read/write` 中对 ≥0x80 opcode 的掩码操作 |
| 影响 | **关键 Bug**，不修复则所有 SX1250 通信失败 |

### B08 — SPI 事务中 CS 时序不正确

| 项目 | 内容 |
|------|------|
| 阶段 | TEST_LORAGW_SPI |
| 现象 | 偶发的数据错误 |
| 原因 | ESP32 SPI 驱动在事务前后自动管理 CS，STM32 HAL SPI 不会 |
| 修复 | 每次 SPI 事务手动拉低/拉高 NSS (PB12) |

---

## 阶段三: I2C / 传感器

### B09 — LM75A 温度传感器地址错误

| 项目 | 内容 |
|------|------|
| 阶段 | TEST_LORAGW_I2C_LM75A / HAL RX |
| 现象 | I2C 扫描找不到温度传感器，HAL 启动报温度传感器错误 |
| 原因 | 原始代码适配 STTS751 (地址 0x39/0x3B/0x38/0x3A)，板载实际是 LM75A (0x48) |
| 修复 | `loragw_stts751.c` 中将地址列表改为 `{0x48, 0x49, 0x4A, 0x4B}` |

### B10 — I2C 地址未左移

| 项目 | 内容 |
|------|------|
| 阶段 | TEST_LORAGW_I2C_OLED |
| 现象 | I2C 通信失败，NACK |
| 原因 | STM32 HAL I2C 函数要求 7-bit 地址左移 1 位 |
| 修复 | `loragw_i2c.c` 中传地址时 `dev_addr << 1` |

---

## 阶段四: HAL 层编译

### B11 — 固件数组 `const` 限定符不匹配

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | Warning: assignment discards 'const' qualifier |
| 原因 | 固件 `.var` 文件生成的数组是 `const`，但指针声明为非 `const` |
| 修复 | 将指针改为 `const uint8_t *` |

### B12 — `LGW_REG_SUCCESS` 等宏重定义

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | Warning: macro redefined |
| 原因 | 多个头文件中重复定义了返回码宏 |
| 修复 | 使用 `#ifndef` 包裹或统一在一个头文件定义 |

---

## 阶段五: HAL RX/TX 测试

### B13 — E77 串口识别错误

| 项目 | 内容 |
|------|------|
| 阶段 | HAL RX 测试准备 |
| 现象 | 向 COM3 发 AT 命令无反应 |
| 原因 | COM3 是蓝牙虚拟串口，E77 实际在 COM11 |
| 修复 | 用 `e77_probe.py` 探测或在设备管理器中确认 |

### B14 — RadioLib SyncWord 不匹配

| 项目 | 内容 |
|------|------|
| 阶段 | HAL TX 测试 |
| 现象 | SX1302 发射成功但 SX1278 收不到 |
| 原因 | SX1278 默认 SyncWord 0x12 (private)，SX1302 使用 0x34 (public) |
| 修复 | RadioLib 端 `radio.begin()` 参数设置 SyncWord = 0x34 |

### B15 — RadioLib CRC/IQ 设置不匹配

| 项目 | 内容 |
|------|------|
| 阶段 | HAL TX 测试 |
| 现象 | 偶尔收到包但 CRC 校验失败 |
| 原因 | SX1302 TX 设置 `no_crc=true, invert_pol=false`，接收端未对齐 |
| 修复 | RadioLib 端 `setCRC(false)`, `setInvertIQ(false)` |

---

## 快速参考表

| ID | 简述 | 阶段 | 严重度 |
|----|------|------|--------|
| B01 | syscall 缺失 | 编译 | 高 |
| B02 | 浮点 printf | 编译/运行 | 中 |
| B03 | timespec 未定义 | 编译 | 高 |
| B04 | pthread.h 缺失 | 编译 | 高 |
| B05 | ESP-IDF 头文件 | 编译 | 高 |
| B06 | JIT 类型缺失 | 编译 | 中 |
| B07 | SX1250 opcode 掩码 | 运行 | **致命** |
| B08 | SPI CS 时序 | 运行 | 高 |
| B09 | LM75A 地址错误 | 运行 | 中 |
| B10 | I2C 地址未左移 | 运行 | 高 |
| B11 | const 限定符 | 编译 | 低 |
| B12 | 宏重定义 | 编译 | 低 |
| B13 | 串口识别 | 测试 | 低 |
| B14 | SyncWord 不匹配 | 测试 | 高 |
| B15 | CRC/IQ 不匹配 | 测试 | 高 |
| B16 | stat 时间戳格式错误 | 集成 | **致命** |
| B17 | 链接脚本未保护配置扇区 | 工程 | 高 |

---

## 阶段五：ChirpStack 集成

### B16 — stat 时间戳格式导致网关永不上线

| 项目 | 内容 |
|------|------|
| 阶段 | 集成测试 |
| 现象 | 网关 UDP PULL_ACK 100% 正常，ChirpStack 中网关始终离线（never seen） |
| 定位 | `docker logs chirpstack-gateway-bridge` 中每 30 秒出现一条 error：`parsing time "\"00:00:36\"" as "\"2006-01-02 15:04:05 MST\""` |
| 根因 | `lora_pkt_fwd.c` 的 stat JSON `time` 字段格式为 `"00:00:36"`（设备运行时长），ChirpStack Gateway Bridge 要求 `"YYYY-MM-DD HH:MM:SS TZD"` 格式，解析失败后直接丢弃整个 PUSH_DATA 报文 |
| 修复 | `snprintf(stat_timestamp, ..., "2000-01-01 %02lu:%02lu:%02lu GMT", h, m, s)`；缓冲区 24→32 字节 |
| 影响 | 修复后 Gateway Bridge 立即发布 `event/stats` 到 MQTT，网关上线 |
| 文件 | `packet_forwarder/lora_pkt_fwd.c` |
| 详见 | [testing/08_chirpstack_gateway_online_test.md](../testing/08_chirpstack_gateway_online_test.md) |

### B17 — 链接脚本未保护配置扇区

| 项目 | 内容 |
|------|------|
| 阶段 | 工程设计 |
| 现象 | 当前无影响；潜在风险：固件增长超过 896KB 时链接器会把代码放入 Sector 11（`0x080E0000`），覆盖 Flash 配置数据 |
| 根因 | `STM32F407ZGTx_FLASH_cmake.ld` 中 `FLASH LENGTH = 1024K`，覆盖了配置存储区 |
| 修复 | `FLASH LENGTH = 896K`（Sectors 0-10），Sector 11 不再属于可链接区域；固件超限时链接器报错而非静默覆盖 |
| 文件 | `STM32F407ZGTx_FLASH_cmake.ld` |
| 详见 | [impl/06_freq_plan_flash_config_v4.md](../impl/06_freq_plan_flash_config_v4.md#part-4链接脚本-flash-区域保护) |
