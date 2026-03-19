# 11 — 引脚重映射：SX1302 切换至 SPI1，OLED/LM75A 切换至 I2C1

> **所属项目**：ESXP1302 STM32F407 移植  
> **实施时间**：2026-03-19  
> **关联文档**：[hardware/01_hardware_overview.md](../hardware/01_hardware_overview.md)、[impl/02_platform_adapt.md](02_platform_adapt.md)  
> **状态**：✅ 已实施，编译通过

---

## 目录

1. [变更原因](#1-变更原因)
2. [引脚变更明细](#2-引脚变更明细)
3. [CubeMX 重新生成文件](#3-cubemx-重新生成文件)
4. [手工适配文件](#4-手工适配文件)
5. [_sccmram extern 声明修复](#5-_sccmram-extern-声明修复)
6. [验证结果](#6-验证结果)

---

## 1. 变更原因

PCB 走线调整，将 SX1302 的 SPI 总线从 SPI3 改走 SPI1，同时将 OLED/LM75A 的 I2C 总线从 I2C2 改走 I2C1；NSS 和 RESET 引脚也随之迁移至与 SPI1 同组的 GPIOA/GPIOC。

---

## 2. 引脚变更明细

### SX1302 SPI 总线

| 信号 | 旧引脚（SPI3） | 新引脚（SPI1） |
|------|--------------|--------------|
| SCK  | PC10 | PA5 |
| MISO | PC11 | PA6 |
| MOSI | PC12 | PA7 |

### SX1302 控制 GPIO

| 信号 | 旧引脚 | 新引脚 |
|------|--------|--------|
| NSS（软件 CS） | PB12 | PA4 |
| RESET（高有效） | PD9 | PC4 |

### I2C 总线（OLED SSD1306 + LM75A）

| 信号 | 旧引脚（I2C2） | 新引脚（I2C1） |
|------|--------------|--------------|
| SCL  | PF1 | PB6 |
| SDA  | PF0 | PB7 |

W5500（SPI2）及其他引脚**不变**。

---

## 3. CubeMX 重新生成文件

以下文件由 CubeMX 自动重新生成，无需手动修改：

| 文件 | 主要变化 |
|------|----------|
| `Core/Src/spi.c` | 删除 SPI3 初始化，新增 SPI1（PA5/PA6/PA7, APB2 84MHz, 分频16 ≈ 5.25 MHz）；保留 SPI2（W5500） |
| `Core/Inc/spi.h` | `hspi3` 声明改为 `hspi1` |
| `Core/Src/i2c.c` | 删除 I2C2 初始化，新增 I2C1（PB6/PB7, 100 kHz） |
| `Core/Inc/i2c.h` | `hi2c2` 声明改为 `hi2c1` |
| `Core/Src/gpio.c` | `SX1302_NSS_Pin` 所在端口由 GPIOB 改为 GPIOA；`SX1302_RESET_Pin` 所在端口由 GPIOD 改为 GPIOC |
| `Core/Inc/main.h` | 更新宏定义：`SX1302_NSS_GPIO_Port = GPIOA / GPIO_PIN_4`，`SX1302_RESET_GPIO_Port = GPIOC / GPIO_PIN_4` |
| `Core/Src/stm32f4xx_it.c` | 移除 SPI3 / I2C2 中断处理桩 |
| `Core/Inc/stm32f4xx_it.h` | 同上 |
| `esxp1302_stm32f407.ioc` | 记录完整 CubeMX 配置快照 |

> **SPI1 时钟说明**：SPI1 挂在 APB2（84 MHz），分频系数 16，实际 SPI 时钟 = 84 / 16 = **5.25 MHz**，与之前 SPI3 @ APB1/8 保持相同速率，无需调整驱动层超时参数。

---

## 4. 手工适配文件

CubeMX 重新生成后，以下平台适配文件需手工同步：

### `libloragw/board_config.h`

```c
/* 旧 */
#include "spi.h"           /* hspi3 handle */
#include "i2c.h"           /* hi2c2 handle */
#define SX1302_SPI_HANDLE     (&hspi3)
#define SX1302_NSS_PORT       SX1302_NSS_GPIO_Port   /* GPIOB */
#define SX1302_NSS_PIN        SX1302_NSS_Pin          /* GPIO_PIN_12 (PB12) */
#define SX1302_RESET_PORT     SX1302_RESET_GPIO_Port  /* GPIOD */
#define SX1302_RESET_PIN_NUM  SX1302_RESET_Pin        /* GPIO_PIN_9 (PD9) */
#define I2C_HANDLE        (&hi2c2)

/* 新 */
#include "spi.h"           /* hspi1 handle */
#include "i2c.h"           /* hi2c1 handle */
#define SX1302_SPI_HANDLE     (&hspi1)
#define SX1302_NSS_PORT       SX1302_NSS_GPIO_Port   /* GPIOA */
#define SX1302_NSS_PIN        SX1302_NSS_Pin          /* GPIO_PIN_4 (PA4) */
#define SX1302_RESET_PORT     SX1302_RESET_GPIO_Port  /* GPIOC */
#define SX1302_RESET_PIN_NUM  SX1302_RESET_Pin        /* GPIO_PIN_4 (PC4) */
#define I2C_HANDLE        (&hi2c1)
```

`SX1302_NSS_PORT/PIN` 和 `SX1302_RESET_PORT/PIN_NUM` 均引用 `main.h` 中 CubeMX 生成的宏，不含硬编码引脚号，下次 CubeMX 重新生成时自动跟随。

### `libloragw/loragw_i2c.c`

头部注释及 `/* hi2c2 extern */` 注释更新为 `hi2c1`，逻辑代码无变化（均通过 `I2C_HANDLE` 宏间接引用句柄）。

### `CMakeLists.txt`

注释行 `TEST_LORAGW_I2C_OLED` 和 `TEST_LORAGW_I2C_LM75A` 中的 `I2C2` 改为 `I2C1`，不影响编译。

---

## 5. _sccmram extern 声明修复

CubeMX 重新生成 `main.c` 时会清空 `USER CODE BEGIN PV` 保护区以外的全局变量声明。本次重新生成后，`_sccmram` / `_eccmram` 的链接脚本符号 `extern` 声明被删除，导致编译错误：

```
error: '_sccmram' undeclared (first use in this function)
error: '_eccmram' undeclared (first use in this function)
```

**修复**：将两条 `extern` 声明移入 `USER CODE BEGIN PV` 保护区，确保 CubeMX 不会再次清除：

```c
/* USER CODE BEGIN PV */
/* Linker-script symbols for CCMRAM section (defined in STM32F407ZGTx_FLASH.ld) */
extern uint32_t _sccmram;
extern uint32_t _eccmram;
/* USER CODE END PV */
```

> 链接脚本 `STM32F407ZGTx_FLASH.ld` 中的 `_sccmram` / `_eccmram` 符号定义不受 CubeMX 影响，无需处理。

---

## 6. 验证结果

```
cmake --build --preset Debug -j8
...
[100%] Linking CXX executable esxp1302_stm32f407.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:      446820 B       1 MB     42.62%
            RAM:      154724 B     192 KB     78.71%
         CCMRAM:       68796 B      64 KB    105.00%  ← 超额，需关注
  BUILD SUCCESSFUL
```

编译通过，无 warning 新增。CCMRAM 使用率仍超 100%（链接器使用 SRAM 溢出填充，已知问题，见 [impl/09_ccmram_optimization.md](09_ccmram_optimization.md)）。
