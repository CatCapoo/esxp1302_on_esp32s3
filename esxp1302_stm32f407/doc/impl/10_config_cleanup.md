# 10 — 配置整理：global_conf.json 构建管道、`gateway_defaults.h` 清理与版本号修复

> **所属项目**：ESXP1302 STM32F407 移植  
> **实施时间**：2026-03-13  
> **关联文档**：[impl/06_freq_plan_flash_config_v4.md](06_freq_plan_flash_config_v4.md)  
> **状态**：✅ 已实施，已验证（串口确认三层配置链路全部正常）

---

## 目录

1. [问题背景](#1-问题背景)
2. [global_conf.json 构建管道](#2-global_confjson-构建管道)
3. [gateway_defaults.h 清理](#3-gateway_defaultsh-清理)
4. [版本号拼写错误修复](#4-版本号拼写错误修复)
5. [VERSION_STRING 宏补充定义](#5-version_string-宏补充定义)
6. [串口验证结果](#6-串口验证结果)

---

## 1. 问题背景

ChirpStack 端到端测试通过后，对配置层进行了审计，发现以下四个问题：

| 问题 | 文件 | 详情 |
|------|------|------|
| `global_conf.json` 缺少构建集成 | `CMakeLists.txt` | JSON 只能手动替换为 C 数组，无法版本管控；且三个频率计划无法快速切换 |
| `gateway_defaults.h` 含 16 个死代码宏 | `libloragw/gateway_defaults.h` | 均未被任何 `.c` 文件引用；其中一个与 `lora_pkt_fwd.c` 中同名宏**值冲突** |
| 版本宏拼写错误 | `packet_forwarder/loragw_version.h` | `EXSP1302_VERSION`（字母倒置）而非 `ESXP1302_VERSION` |
| `VERSION_STRING` 未定义 | `CMakeLists.txt` | `lora_pkt_fwd.c` 有 `#ifndef` 守卫但 CMake 从未传入该宏，串口持续打印 `Version: undefined` |

> **三层配置架构回顾**（详见 [impl/06](06_freq_plan_flash_config_v4.md)）：
> ```
> #define 编译期默认值   ← gateway_defaults.h（最低优先级）
>       ↓
> JSON 嵌入字节数组     ← global_json.h（CMake 自动生成）
>       ↓
> Flash Sector 11 覆盖  ← UART CLI 写入（最高优先级）
> ```

---

## 2. global_conf.json 构建管道

### 背景

`lora_pkt_fwd.c` 通过 `parse_SX130x_configuration()` 和 `parse_gateway_configuration()` 读取信道频率、滤波参数等中间层配置。原始 ESP-IDF 版本从 LittleFS 文件系统读取；移植到 STM32 后改为将 JSON 内容嵌入 Flash，以 C 字节数组形式编译进固件（`global_json.h`）。

**原有问题**：`global_json.h` 需手工生成，切换频率计划时需重新手工操作，且文件不在版本库中导致无从追溯。

### 解决方案

添加三个频率计划 JSON 源文件和一个 Python 转换脚本，通过 CMake 在编译前自动生成 `global_json.h`。

#### 新增文件

```
packet_forwarder/
└── global_conf.json/
    ├── global_conf.cn470.json   ← CN470 Sub-Band 10（486.3~487.7 MHz，当前默认）
    ├── global_conf.eu868.json   ← EU868
    └── global_conf.us915.json   ← US915 Sub-Band 0

scripts/
└── gen_global_json.py           ← JSON → global_json.h 转换器
```

#### `gen_global_json.py` 格式

脚本将指定 JSON 文件转换为以下头文件格式：

```c
/* AUTO-GENERATED — DO NOT EDIT. Source: global_conf.cn470.json */
#pragma once
#include <stdint.h>
static const uint8_t global_json_data[] = {
    /* 2-byte little-endian length prefix */
    0xXX, 0xXX,
    /* JSON content */
    0x7B, 0x0A, ...
};
#define GLOBAL_JSON_LEN (sizeof(global_json_data) - 2)
```

JSON 字节流以 **2 字节小端长度前缀** 打包，与 Flash 配置读取接口一致。

```bash
# 手动调用示例（正常构建中由 CMake 自动触发）
python3 scripts/gen_global_json.py \
    packet_forwarder/global_conf.json/global_conf.cn470.json \
    packet_forwarder/global_json.h
```

#### CMakeLists.txt 集成

```cmake
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(GLOBAL_JSON_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/packet_forwarder/global_conf.json/global_conf.cn470.json")
set(GLOBAL_JSON_HEADER
    "${CMAKE_CURRENT_SOURCE_DIR}/packet_forwarder/global_json.h")

add_custom_command(
    OUTPUT  "${GLOBAL_JSON_HEADER}"
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_global_json.py"
            "${GLOBAL_JSON_SRC}"
            "${GLOBAL_JSON_HEADER}"
    DEPENDS "${GLOBAL_JSON_SRC}"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_global_json.py"
    COMMENT "Generating global_json.h from global_conf.cn470.json"
)
add_custom_target(gen_global_json DEPENDS "${GLOBAL_JSON_HEADER}")
add_dependencies(${PROJECT_NAME}.elf gen_global_json)
```

`global_json.h` 已加入 `.gitignore`（构建产物，不纳入版本库）。

#### 切换频率计划

修改 `CMakeLists.txt` 中 `GLOBAL_JSON_SRC` 变量指向目标 JSON 文件后重新构建即可：

```cmake
# 切换至 EU868
set(GLOBAL_JSON_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/packet_forwarder/global_conf.json/global_conf.eu868.json")
```

---

## 3. `gateway_defaults.h` 清理

### 文件位置

`libloragw/gateway_defaults.h`

### 方法

通过 `grep -r <宏名> --include="*.c" --include="*.h"` 逐一确认每个宏在所有 `.c` 文件中的引用情况，确认为 0 引用后列为死代码。

### 清理的 16 个死代码宏

| 宏名 | 原始值 | 死代码原因 |
|------|--------|-----------|
| `GW_DEFAULT_FWD_CRC_VALID` | `true` | 转发决策由 `lora_pkt_fwd.c` 内 `DEFAULT_CRC_EN` 控制，此宏从未引用 |
| `GW_DEFAULT_FWD_CRC_ERROR` | `false` | 同上 |
| `GW_DEFAULT_FWD_CRC_DISABLED` | `false` | 同上 |
| `KEEPALIVE_SEC` | `10` | `lora_pkt_fwd.c` 使用 `DEFAULT_KEEPALIVE=5`；**两者值不一致**，且此宏无引用 |
| `STAT_INTERVAL` | `30` | `lora_pkt_fwd.c` 直接定义 `DEFAULT_STAT=30`，从不引用此宏 |
| `PUSH_TIMEOUT_MS` | `100` | `lora_pkt_fwd.c` 直接定义 `PUSH_TIMEOUT_MS=500`（STM32 适配值），从不引用此宏 |
| `GW_GPS_ENABLE` | `false` | GPS 功能代码整体已 `#ifdef` 注释掉，不编译 |
| `GW_DEFAULT_REF_LAT` | `0.0` | 同上 |
| `GW_DEFAULT_REF_LON` | `0.0` | 同上 |
| `GW_DEFAULT_REF_ALT` | `0` | 同上 |
| `BEACON_PERIOD` | `0` | Beacon 功能代码整体已注释 |
| `BEACON_FREQ_HZ` | `869525000` | 同上 |
| `BEACON_DR` | `9` | 同上 |
| `BEACON_BW_HZ` | `125000` | 同上 |
| `BEACON_POWER` | `14` | 同上 |
| `BEACON_INFODESC` | `0` | 同上 |

> **关键冲突**：`KEEPALIVE_SEC=10` 与 `lora_pkt_fwd.c` 中实际运行的 `DEFAULT_KEEPALIVE=5`
> **值不一致**。若将来误引用该宏，会使 keepalive 行为静默出错。及时清除消除了潜在歧义。

### 清理后保留的 12 个宏

下列宏均被 `gateway_config.h` / `gateway_config.c` 直接引用，予以保留：

```c
#define GW_DEFAULT_NS_HOST       "127.0.0.1"
#define GW_DEFAULT_NS_PORT_UP    1680
#define GW_DEFAULT_NS_PORT_DOWN  1680
#define GW_DEFAULT_EUI           0xAA555A0000000000ULL
#define GW_DEFAULT_ETH_IP        "192.168.1.100"
#define GW_DEFAULT_ETH_GW        "192.168.1.1"
#define GW_DEFAULT_ETH_SN        "255.255.255.0"
#define GW_DEFAULT_ETH_DNS       "8.8.8.8"
#define GW_DEFAULT_ETH_MAC       {0xAA, 0x55, 0x5A, 0x00, 0x00, 0x00}
#define GW_DEFAULT_FREQ_REGION   GW_FREQ_REGION_CN470_SB10
#define GW_DEFAULT_RADIO0_FREQ   486600000
#define GW_DEFAULT_RADIO1_FREQ   487400000
```

---

## 4. 版本号拼写错误修复

### 问题

`packet_forwarder/loragw_version.h` 宏名字母顺序颠倒：

```c
// 修复前（错误：E-X-S-P）
#define EXSP1302_VERSION "0.9.0"

// 修复后（正确：E-S-X-P）
#define ESXP1302_VERSION "0.9.0"
```

### 影响范围

`packet_forwarder/_pkt_fwd_ref.c` 两处引用同步修改：

```c
// 版本横幅打印
printf("*** Packet Forwarder for ESXP1302 ***\nVersion: " ESXP1302_VERSION "\n");

// stat JSON 构建
sprintf(stat_str, ..., ESXP1302_VERSION, ...);
```

---

## 5. `VERSION_STRING` 宏补充定义

### 问题

`lora_pkt_fwd.c` 中存在如下守卫：

```c
#ifndef VERSION_STRING
#define VERSION_STRING "undefined"
#endif
```

`CMakeLists.txt` 从未通过 `-D` 传入 `VERSION_STRING`，导致串口持续打印：

```
Version: undefined
```

### 修复

在 `CMakeLists.txt` 的 `target_compile_definitions` 中添加：

```cmake
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    ...
    VERSION_STRING="ESXP1302-v0.9.0"
)
```

修复后串口输出：

```
*** Packet Forwarder for ESXP1302 ***
Version: ESXP1302-v0.9.0
```

---

## 6. 串口验证结果

通过在 `pkt_fwd_main()` 早期添加临时诊断日志（验证完毕后删除），逐层确认三层配置链路：

```
[CFG] === CONFIG CHAIN DEBUG ===
[CFG] gateway_defaults.h defaults:
[CFG]   ns_host   = 127.0.0.1 : 1680
[CFG]   eui       = AA:55:5A:00:00:00:00:00
[CFG]   region    = 10  radio0 = 486600000 Hz
[CFG] After JSON parse:
[CFG]   keepalive = 10 s  (default was 5)
[CFG] After Flash override:
[CFG]   eui       = AA:55:5A:00:00:21:FB   (Flash 写入值)
[CFG]   region    = 10  (CN470 SB10)
[CFG]   radio0    = 486.6 MHz  radio1 = 487.4 MHz
[CFG]   ns_host   = 192.168.71.100 : 1700  (Flash 覆盖了 JSON 的 192.168.1.202:1680)
[CFG] === END DEBUG ===
```

| 验证项 | 结果 |
|--------|------|
| `#define` 默认值正确传递 | ✅ |
| JSON 层覆盖 keepalive（10 s，默认为 5 s）| ✅ |
| Flash 层覆盖 EUI（`AA:55:5A:00:00:21:FB`）| ✅ |
| Flash 层覆盖 NS 地址（`192.168.71.100:1700`）| ✅ |
| 集中器启动成功，所有线程创建 | ✅ |
| 串口打印 `Version: ESXP1302-v0.9.0` | ✅ |

完整启动串口输出（关键行）：

```
*** Packet Forwarder for ESXP1302 ***
Version: ESXP1302-v0.9.0
INFO: concentrator started, packet can now be received
INFO: [up] Thread activated
INFO: [down] Thread activated
INFO: [jit] Thread activated
```

### 构建内存占用（验证时固件版本）

| 内存区 | 使用量 | 总量 | 占用率 |
|--------|--------|------|--------|
| FLASH | 327,928 B | 896 KB | 35.74% |
| RAM | 97,128 B | 128 KB | 74.10% |
| CCMRAM | 27,800 B | 64 KB | 42.42% |
