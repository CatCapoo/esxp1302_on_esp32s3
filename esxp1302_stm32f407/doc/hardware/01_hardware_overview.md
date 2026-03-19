# 01 — 硬件平台概述

## 目标

将 SX1302 LoRa 网关 HAL 驱动从 ESP32-S3 (ESP-IDF) 移植到 STM32F407ZGT6 (STM32 HAL + FreeRTOS CMSIS-RTOS2)。

## 硬件平台

| 项目 | 规格 |
|------|------|
| MCU | STM32F407ZGT6 (Cortex-M4, 168 MHz, 1 MB Flash, 192 KB SRAM) |
| 开发板 | 正点原子探索者 v3 (或兼容板) |
| LoRa 前端 | SX1302 网关基带 + 双 SX1250 射频前端 |
| OLED | SSD1306 128×64 I2C (0x3C) |
| 温度传感器 | LM75A I2C (0x48) |
| 测试节点 | E77-400M22S (EBYTE, LoRaWAN 模块, AT 命令) |
| 接收验证 | SX1278 模块 + Arduino + RadioLib |
| 下载/调试 | ST-Link V2 |
| 串口输出 | USART1 → USB CH340 (printf 重定向) |
| CubeMX 版本 | STM32CubeCLT 1.21.0 |

## SPI1 — SX1302

SX1302 通过 SPI1 总线连接。CubeMX 中配置为 Full-Duplex Master, MSB First, CPOL=0 CPHA=0, 8bit。

| 信号 | 引脚 | 说明 |
|------|------|------|
| SCK | PA5 | SPI1_SCK |
| MISO | PA6 | SPI1_MISO |
| MOSI | PA7 | SPI1_MOSI |
| NSS | PA4 | **软件控制 GPIO**（非硬件 NSS） |
| RESET | PC4 | SX1302 RESET（高有效） |

> **关键点**：NSS 使用软件 GPIO 手动控制（`cs_select()` / `cs_deselect()`），不使用 SPI 硬件 NSS，因为 SX1302 的 SPI 协议需要在每个事务期间保持 CS 低电平。

### SPI 时钟分频

CubeMX 中 SPI1 挂在 APB2 (84 MHz) 上，分频系数设为 16，实际 SPI 时钟 ≈ 5.25 MHz。SX1302 最高支持 10 MHz，5.25 MHz 在规格范围内，实测验证通过。

> **变更记录**：2026-03-19 由 SPI3 (PC10/PC11/PC12, NSS=PB12, RESET=PD9) 切换至 SPI1 (PA5/PA6/PA7, NSS=PA4, RESET=PC4)，见 [impl/11_pin_remap_spi1_i2c1.md](../impl/11_pin_remap_spi1_i2c1.md)。

## SPI2 — W5500

W5500 以太网芯片通过 SPI2 总线连接。CubeMX 中配置为 Full-Duplex Master, MSB First, CPOL=0 CPHA=0, 8bit。

| 信号 | 引脚 | 说明 |
|------|------|------|
| SCK | PB10 | SPI2_SCK |
| MISO | PC2 | SPI2_MISO |
| MOSI | PC3 | SPI2_MOSI |
| NSS | PA3 | **软件控制 GPIO**（W5500_NSS） |
| RESET | PA2 | W5500 硬件复位（低有效） |

### SPI 时钟分频

SPI2 同样挂在 APB1 (42 MHz) 上，当前分频系数为 8，实际 SPI 时钟 ≈ 5.25 MHz。
W5500 最高支持 80 MHz，5.25 MHz 为保守配置，适合初版 PCB（无阻抗控制）。
如需提升网络吞吐，可在 CubeMX 中将分频调整至 4（≈ 10.5 MHz）。
详细速率选型分析见 [impl/04_w5500_ethernet.md](../impl/04_w5500_ethernet.md)。

## I2C1 — OLED / LM75A

| 信号 | 引脚 | 说明 |
|------|------|------|
| SCL | PB6 | I2C1_SCL（开漏 + 外部上拉） |
| SDA | PB7 | I2C1_SDA（开漏 + 外部上拉） |

CubeMX 配置：Standard Mode (100 kHz)。

> **变更记录**：2026-03-19 由 I2C2 (PF0-SDA/PF1-SCL) 切换至 I2C1 (PB6-SCL/PB7-SDA)，见 [impl/11_pin_remap_spi1_i2c1.md](../impl/11_pin_remap_spi1_i2c1.md)。

### I2C 设备地址

| 设备 | 地址 (7-bit) | 说明 |
|------|-------------|------|
| SSD1306 OLED | 0x3C | 128×64, Page Mode |
| LM75A | 0x48 | A0=A1=A2=GND |

## GPIO

| 引脚 | 功能 | 模式 | 说明 |
|------|------|------|------|
| PA4 | SX1302_NSS | Output, Push-Pull, High | SPI CS 软件控制 |
| PC4 | SX1302_RESET | Output, Push-Pull, Low | 高电平复位 |
| PF9 | LED0 | Output | 调试指示 |
| PF10 | LED1 | Output | 调试指示 |
| PE4 | KEY0 | Input, Pull-Up | 用户按键 |
| PA0 | KEY_UP | Input, Pull-Down | 用户按键 |

## USART1 — printf 输出

| 引脚 | 说明 |
|------|------|
| PA9 | USART1_TX |
| PA10 | USART1_RX |

波特率 115200, 8N1。通过 `__io_putchar()` 重定向 printf 到 USART1（自动 LF→CRLF）。

### ⚠️ 一键下载电路与串口终端注意事项

正点原子 STM32F407 最小系统板板载 **一键下载电路**（Q1 + Q2 晶体管），CH340C 的 DTR/RTS 信号通过该电路分别控制 MCU 的 **NRST（复位）** 和 **BOOT0（启动模式选择）**：

```
CH340C DTR# ──(反相)── DTR_N ──> Q1 (NPN) ──> NRST    (复位)
CH340C RTS# ──(反相)── RTS_N ──> Q2 (PNP) ──> BOOT0   (启动模式)
```

**信号逻辑表：**

| 软件操作 | CH340 引脚 | PCB 信号 | 晶体管 | MCU 效果 |
|---------|-----------|---------|-------|---------|
| DTR raised (assert) | DTR# = LOW | DTR_N = LOW | Q1 OFF | NRST 不被拉低，正常运行 |
| DTR lowered (deassert) | DTR# = HIGH | DTR_N = HIGH | Q1 ON | **NRST = LOW → MCU 复位** |
| RTS raised (assert) | RTS# = LOW | RTS_N = LOW | Q2 ON | **BOOT0 = HIGH → 进入 Bootloader** |
| RTS lowered (deassert) | RTS# = HIGH | RTS_N = HIGH | Q2 OFF | BOOT0 = LOW（被下拉电阻保持），从 Flash 启动 |

**关键问题：** Linux 打开串口时默认 raise DTR 和 RTS。RTS 被 raise 后 Q2 导通使 BOOT0=HIGH。此时 MCU 虽然正常运行（BOOT0 仅在复位时采样），但**一旦发生任何复位事件**（按复位键、CLI `reboot` 命令），MCU 会采样到 BOOT0=HIGH，进入 System Bootloader 而非从 Flash 启动，表现为 LED 停止闪烁、串口无输出。

**正确的串口终端命令：**

```bash
# ✅ 正确：--lower-rts 确保 BOOT0 保持 LOW
picocom -b 115200 --lower-rts /dev/ttyUSB0

# ❌ 错误：默认 raise RTS → 复位后进入 Bootloader
picocom -b 115200 /dev/ttyUSB0

# ❌ 错误：--lower-dtr → 持续拉低 NRST → MCU 被钉在复位状态
picocom -b 115200 --lower-dtr /dev/ttyUSB0
```

**pyserial 正确打开方式：**

```python
import serial
ser = serial.Serial()
ser.port     = '/dev/ttyUSB0'
ser.baudrate = 115200
ser.dtr      = False   # 不触发复位
ser.rts      = False   # 不拉高 BOOT0
ser.open()
```

## TIM2 — 微秒定时器

TIM2 配置为 32-bit 自由运行计数器，时钟 84 MHz，预分频 84-1，计数器 1 µs/tick。
在 `main.c` 中 `HAL_TIM_Base_Start(&htim2)` 启动。

用途：
- `timestamp_us()` — 获取 µs 时间戳
- `wait_us()` — µs 级忙等延时

## FreeRTOS

使用 CMSIS-RTOS2 接口，CubeMX 自动生成。

- **默认任务**：`StartDefaultTask` (128 words stack) — 空闲
- **测试任务**：`StartTestTask` (8192 words stack = 32 KB) — 调用 `test_loragw_run()`

> **注意**：HAL 层运行时栈消耗较大（SX1302 固件加载、校准等），测试任务栈不能低于 8 KB words。

## 板级配置文件 `board_config.h`

所有硬件引脚映射集中在此文件中：

```c
/* SPI */
#define SX1302_SPI_HANDLE   (&hspi1)
#define SX1302_NSS_PORT     SX1302_NSS_GPIO_Port
#define SX1302_NSS_PIN      SX1302_NSS_Pin

/* RESET */
#define SX1302_RESET_PORT   SX1302_RESET_GPIO_Port
#define SX1302_RESET_PIN_NUM SX1302_RESET_Pin

/* I2C */
#define I2C_HANDLE          (&hi2c1)
#define I2C_TIMEOUT_MS      100
```

## 参考资料

- SX1302 Datasheet (Semtech)
- SX1250 Datasheet (Semtech)
- STM32F407 Reference Manual (RM0090)
- 正点原子 STM32F407 探索者开发板原理图
- E77-400M22S 用户手册 (`doc/E106-470G27P2_UserManual_CN_v1.5.pdf`)
