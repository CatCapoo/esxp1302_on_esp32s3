# 03 — 驱动层源文件清单与依赖关系

## 概述

本文列出 STM32F407 版本中 **所有** libloragw 源文件、它们的功能、依赖关系，以及移植过程中的编译修复记录。

---

## 1. 文件层级总览

```
libloragw/
├── 平台抽象层 (直接调用 STM32 HAL)
│   ├── board_config.h      ← 引脚/外设映射
│   ├── config.h             ← 调试开关
│   ├── loragw_spi.c/h       ← SX1302 SPI 总线驱动
│   ├── loragw_i2c.c/h       ← I2C2 总线驱动
│   ├── loragw_gpio.c/h      ← SX1302 复位 GPIO
│   └── loragw_aux.c/h       ← 定时 (wait_ms/us, timestamp_us)
│
├── SX1302 寄存器/通信层
│   ├── loragw_com.c/h       ← COM 抽象 (SPI only on STM32)
│   ├── loragw_reg.c/h       ← SX1302 寄存器读写
│   └── loragw_debug.c/h     ← 调试工具 (dump regs)
│
├── SX1250 射频驱动
│   ├── loragw_sx1250.c/h    ← SX1250 高层 API
│   ├── sx1250_com.c/h       ← SX1250 通信抽象
│   └── sx1250_spi.c/h       ← SX1250 SPI 命令
│
├── SX1261 (LBT 伴侣射频)
│   ├── loragw_sx1261.c/h    ← SX1261 高层 API
│   ├── sx1261_com.c/h       ← SX1261 通信抽象
│   ├── sx1261_spi.c/h       ← SX1261 SPI 命令
│   ├── sx1261_usb.c/h       ← USB 通道桩 (空实现)
│   └── sx1261_defs.h        ← SX1261 常量定义
│
├── SX1302 核心功能
│   ├── loragw_sx1302.c/h    ← SX1302 配置/控制/通道管理
│   ├── loragw_sx1302_rx.c/h ← SX1302 RX 路径 (包解析/时间戳)
│   ├── loragw_sx1302_timestamp.c/h ← 硬件时间戳管理
│   ├── loragw_cal.c/h       ← SX1302 校准流程
│   ├── loragw_lbt.c/h       ← Listen Before Talk
│   └── loragw_agc_params.h  ← AGC 参数表
│
├── I2C 外设驱动
│   ├── loragw_stts751.c/h   ← 温度传感器抽象 (实际用 LM75A)
│   ├── loragw_ad5338r.c/h   ← AD5338R 双 DAC
│   ├── loragw_lm75a.c/h     ← LM75A 温度传感器 (新增)
│   └── loragw_oled.c/h      ← SSD1306 OLED (新增)
│
├── 顶层 HAL
│   └── loragw_hal.c/h       ← 主 HAL API (lgw_start/stop/receive/send)
│
├── 固件二进制
│   ├── agc_fw_sx1250.var    ← AGC 固件 (SX1250)
│   ├── agc_fw_sx1257.var    ← AGC 固件 (SX1257, 未使用)
│   ├── arb_fw.var           ← 仲裁器固件
│   ├── cal_fw.var           ← 校准固件
│   └── sx1261_pram.var      ← SX1261 PRAM 固件
│
└── 工具库
    libtools/
    └── tinymt32.c/h         ← 伪随机数生成器
```

---

## 2. 依赖关系图

```
         ┌──────────┐
         │ loragw_  │
         │  hal.c   │  ← 顶层入口
         └─────┬────┘
               │
    ┌──────────┼──────────────────────┐
    │          │                      │
    ▼          ▼                      ▼
loragw_    loragw_     loragw_    loragw_    loragw_
sx1302.c   cal.c       lbt.c     stts751.c  sx1302_rx.c
    │          │          │          │           │
    ├──────────┤          │          │           │
    ▼          ▼          ▼          │           ▼
loragw_    loragw_     loragw_      │    loragw_sx1302_
sx1250.c   sx1261.c    reg.c       │    timestamp.c
    │          │          │          │
    ▼          ▼          ▼          ▼
sx1250_    sx1261_     loragw_   loragw_
com/spi    com/spi      com.c     i2c.c
    │                     │          │
    └─────────┬───────────┘          │
              ▼                      ▼
         loragw_spi.c          STM32 HAL I2C
              │
              ▼
         STM32 HAL SPI
```

---

## 3. CMakeLists.txt 源文件配置

### 3.1 基础驱动 (LORAGW_SOURCES)

```cmake
set(LORAGW_SOURCES
    libloragw/loragw_spi.c
    libloragw/loragw_com.c
    libloragw/loragw_aux.c
    libloragw/loragw_reg.c
    libloragw/loragw_gpio.c
    libloragw/loragw_sx1250.c
    libloragw/loragw_sx1302.c
    libloragw/loragw_debug.c
    libloragw/sx1250_spi.c
    libloragw/sx1250_com.c
    libtools/tinymt32.c
)
```

### 3.2 I2C 外设 (I2C_SOURCES)

```cmake
set(I2C_SOURCES
    libloragw/loragw_i2c.c
    libloragw/loragw_lm75a.c
    libloragw/loragw_oled.c
)
```

### 3.3 HAL 层 (HAL_SOURCES) — 第三阶段添加

```cmake
set(HAL_SOURCES
    libloragw/loragw_hal.c
    libloragw/loragw_cal.c
    libloragw/loragw_lbt.c
    libloragw/loragw_stts751.c
    libloragw/loragw_sx125x.c
    libloragw/loragw_sx1261.c
    libloragw/loragw_sx1302_rx.c
    libloragw/loragw_sx1302_timestamp.c
    libloragw/loragw_ad5338r.c
    libloragw/sx125x_com.c
    libloragw/sx125x_spi.c
    libloragw/sx1261_com.c
    libloragw/sx1261_spi.c
    libloragw/sx1261_usb.c
)
```

### 3.4 测试选择

```cmake
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # TEST_LORAGW_SPI
    # TEST_LORAGW_SPI_SX1250
    # TEST_LORAGW_REG
    # TEST_LORAGW_I2C_OLED
    # TEST_LORAGW_I2C_LM75A
    # TEST_LORAGW_HAL_RX
    TEST_LORAGW_HAL_TX
)
```

在 `test_loragw.h` 中通过 `#if defined(TEST_xxx)` 选择入口函数。

---

## 4. 编译错误修复记录

按照添加 HAL 层源文件后出现的编译错误顺序记录：

### 4.1 `_meas_time_start` / `_meas_time_stop` 未定义

- **现象**: `undefined reference to '_meas_time_start'`
- **原因**: ESP32 版本在 `loragw_aux.h` 中定义了这些宏/函数，STM32 版本缺失
- **修复**: 在 `loragw_aux.c/h` 中添加实现（见 [02_platform_adapt.md §12](02_platform_adapt.md#12-性能测量宏-_meas_time_startstop)）

### 4.2 `timeout_start` / `timeout_check` 未定义

- **现象**: `undefined reference to 'timeout_start'`
- **原因**: HAL 层使用这些超时函数，STM32 版本缺失
- **修复**: 在 `loragw_aux.c/h` 中添加基于 `HAL_GetTick()` 的实现

### 4.3 `lora_packet_time_on_air` 未定义

- **现象**: `undefined reference to 'lora_packet_time_on_air'`
- **原因**: ToA 计算函数缺失
- **修复**: 完整移植到 `loragw_aux.c`，需要 `#include <math.h>`

### 4.4 `i2c_esp32_open` / `i2c_esp32_close` 未定义

- **现象**: `undefined reference to 'i2c_esp32_open'`
- **原因**: `loragw_hal.c` 中使用 ESP32 I2C API
- **修复**: 替换为 `lgw_i2c_open()` / `lgw_i2c_close()`

### 4.5 `I2C_PORT_TEMP_SENSOR` 未声明

- **现象**: `'I2C_PORT_TEMP_SENSOR' undeclared`
- **原因**: 此常量定义在 `loragw_stts751.h` 中
- **修复**: 在 `loragw_hal.c` 顶部添加 `#include "loragw_stts751.h"`

### 4.6 `lgw_com_get_temperature` USB 路径

- **现象**: USB 通信函数缺失
- **原因**: `loragw_hal.c` 中有 USB 通信分支
- **修复**: 替换为 `printf("ERROR: USB not supported on STM32\n")`

### 4.7 `sx1261_usb.h` 不存在

- **现象**: `fatal error: sx1261_usb.h: No such file`
- **原因**: ESP32 版有此文件，STM32 未移植
- **修复**: 创建桩文件 `sx1261_usb.c/h`

### 4.8 `sx1261_usb_flush` 未定义

- **现象**: `undefined reference to 'sx1261_usb_flush'`
- **原因**: 桩文件初始未声明此函数
- **修复**: 在 `sx1261_usb.h` 中添加声明，`sx1261_usb.c` 中添加空实现

### 4.9 `timestamp_get` 未定义 (测试文件)

- **现象**: `undefined reference to 'timestamp_get'`
- **原因**: ESP32 测试代码使用 `timestamp_get()`，STM32 版本函数名为 `timestamp_us()`
- **修复**: `test_loragw_hal_rx.c` 中改为 `timestamp_us() / 1000`

### 4.10 `-u_scanf_float` 链接参数格式

- **现象**: `unrecognized option 'u_scanf_float'`
- **原因**: CMake 中格式错误，被当作两个参数
- **修复**: 确保 `-u_scanf_float` 作为一个完整字符串

---

## 5. 固件文件 (.var)

这些是 SX1302 运行时需要的固件二进制：

| 文件 | 大小 | 用途 |
|------|------|------|
| `agc_fw_sx1250.var` | ~8 KB | SX1250 AGC 固件 |
| `agc_fw_sx1257.var` | ~8 KB | SX1257 AGC 固件（未使用） |
| `arb_fw.var` | ~2 KB | 仲裁器固件 |
| `cal_fw.var` | ~2 KB | 校准固件 |
| `sx1261_pram.var` | ~16 KB | SX1261 PRAM 固件 |

在 `lgw_start()` 期间由 `sx1302_agc_load_firmware()` / `sx1302_arb_load_firmware()` 通过 SPI burst write 加载。

**注意**：这些文件以 C 数组形式（`const uint8_t agc_firmware_sx1250[] = { ... }`）嵌入源码中（`.var` 扩展名的 C 源文件），由编译器直接编译进 Flash。

---

## 6. 最终构建结果

```
Memory region         Used Size  Region Size  %age Used
           CCMRAM:          0 B        64 KB      0.00%
              RAM:       49580 B       128 KB     37.84%
            FLASH:      179532 B         1 MB     17.12%
```
