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

## SPI3 — SX1302

SX1302 通过 SPI3 总线连接。CubeMX 中配置为 Full-Duplex Master, MSB First, CPOL=0 CPHA=0, 8bit。

| 信号 | 引脚 | 说明 |
|------|------|------|
| SCK | PC10 | SPI3_SCK |
| MISO | PC11 | SPI3_MISO |
| MOSI | PC12 | SPI3_MOSI |
| NSS | PB12 | **软件控制 GPIO**（非硬件 NSS） |
| RESET | PD9 | SX1302 RESET（高有效） |

> **关键点**：NSS 使用软件 GPIO 手动控制（`cs_select()` / `cs_deselect()`），不使用 SPI 硬件 NSS，因为 SX1302 的 SPI 协议需要在每个事务期间保持 CS 低电平。

### SPI 时钟分频

CubeMX 中 SPI3 挂在 APB1 (42 MHz) 上，分频系数设为 32，实际 SPI 时钟 ≈ 1.3125 MHz。SX1302 最高支持 10 MHz，实测 1.3 MHz 稳定可靠。

## I2C2 — OLED / LM75A

| 信号 | 引脚 | 说明 |
|------|------|------|
| SDA | PF0 | I2C2_SDA（开漏 + 外部上拉） |
| SCL | PF1 | I2C2_SCL（开漏 + 外部上拉） |

CubeMX 配置：Standard Mode (100 kHz)。

### I2C 设备地址

| 设备 | 地址 (7-bit) | 说明 |
|------|-------------|------|
| SSD1306 OLED | 0x3C | 128×64, Page Mode |
| LM75A | 0x48 | A0=A1=A2=GND |

## GPIO

| 引脚 | 功能 | 模式 | 说明 |
|------|------|------|------|
| PB12 | SX1302_NSS | Output, Push-Pull, High | SPI CS 软件控制 |
| PD9 | SX1302_RESET | Output, Push-Pull, Low | 高电平复位 |
| PF9 | LED0 | Output | 调试指示 |
| PF10 | LED1 | Output | 调试指示 |
| PE4 | KEY0 | Input, Pull-Up | 用户按键 |
| PA0 | KEY_UP | Input, Pull-Down | 用户按键 |

## USART1 — printf 输出

| 引脚 | 说明 |
|------|------|
| PA9 | USART1_TX |
| PA10 | USART1_RX |

波特率 115200, 8N1。通过 `__io_putchar()` 重定向 printf 到 USART1：

```c
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
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
#define SX1302_SPI_HANDLE   (&hspi3)
#define SX1302_NSS_PORT     SX1302_NSS_GPIO_Port
#define SX1302_NSS_PIN      SX1302_NSS_Pin

/* RESET */
#define SX1302_RESET_PORT   SX1302_RESET_GPIO_Port
#define SX1302_RESET_PIN_NUM SX1302_RESET_Pin

/* I2C */
#define I2C_HANDLE          (&hi2c2)
#define I2C_TIMEOUT_MS      100
```

## 参考资料

- SX1302 Datasheet (Semtech)
- SX1250 Datasheet (Semtech)
- STM32F407 Reference Manual (RM0090)
- 正点原子 STM32F407 探索者开发板原理图
- E77-400M22S 用户手册 (`doc/E106-470G27P2_UserManual_CN_v1.5.pdf`)
