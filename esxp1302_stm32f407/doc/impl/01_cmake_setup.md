# 01 — CubeMX + CMake + VS Code 工程搭建

## 概述

本文记录从零创建 STM32F407 工程的完整步骤，包括 CubeMX 配置、CMake 构建系统、VS Code 集成。

---

## 1. 工具链安装

| 工具 | 版本 | 用途 |
|------|------|------|
| STM32CubeCLT | 1.21.0 | 包含 arm-none-eabi-gcc, CMake, Ninja, STM32CubeProgrammer |
| STM32CubeMX | 6.x | 生成初始化代码 |
| VS Code | latest | 编辑器 + 调试 |
| Cortex-Debug 扩展 | latest | VS Code 中通过 ST-Link 调试 |

CubeCLT 安装路径：`C:/ST/STM32CubeCLT_1.21.0/`

工具链关键路径：
```
C:/ST/STM32CubeCLT_1.21.0/GNU-tools-for-STM32/bin/  (gcc, g++, objcopy ...)
C:/ST/STM32CubeCLT_1.21.0/CMake/bin/                 (cmake)
C:/ST/STM32CubeCLT_1.21.0/Ninja/bin/                 (ninja)
```

## 2. CubeMX 工程创建

### 2.1 新建工程

1. 打开 CubeMX，选择 MCU: **STM32F407ZGTx**
2. 配置时钟树：HSE 8 MHz → PLLM/N/P → SYSCLK = 168 MHz

### 2.2 外设配置

#### SPI3 (SX1302 总线)
- Mode: Full-Duplex Master
- Prescaler: 32 (APB1=42 MHz / 32 = 1.3125 MHz)
- CPOL=Low, CPHA=1Edge, MSB First, 8-bit
- NSS: **Software** (不使用硬件 NSS)
- 引脚: SCK=PC10, MISO=PC11, MOSI=PC12

#### I2C2 (OLED + LM75A)
- Mode: I2C, Standard 100 kHz
- 引脚: SDA=PF0, SCL=PF1

#### USART1 (printf)
- Mode: Async, 115200 bps, 8N1
- 引脚: TX=PA9, RX=PA10

#### TIM2 (µs 计时)
- Clock Source: Internal Clock
- Prescaler: 84-1 (84 MHz / 84 = 1 µs)
- Period: 0xFFFFFFFF (32-bit 自由运行)
- 不启用中断

#### GPIO 手动配置
| 引脚 | Label | Mode | 初始值 |
|------|-------|------|--------|
| PB12 | SX1302_NSS | Output Push-Pull | High |
| PD9 | SX1302_RESET | Output Push-Pull | Low |
| PF9 | LED0 | Output Push-Pull | High |
| PF10 | LED1 | Output Push-Pull | High |
| PE4 | KEY0 | Input Pull-Up | — |
| PA0 | KEY_UP | Input Pull-Down | — |

#### FreeRTOS
- 启用 FreeRTOS, CMSIS-RTOS V2
- 默认任务: `defaultTask` (128 words)
- 需要在生成代码后手动添加测试任务

### 2.3 Project Manager

- Project Name: `esxp1302_stm32f407`
- Toolchain/IDE: **CMake**
- 勾选 "Generate Under Root"

### 2.4 生成代码

点击 **Generate Code**。CubeMX 生成以下结构：

```
esxp1302_stm32f407/
  CMakeLists.txt        ← 顶层，用户可编辑
  cmake/
    gcc-arm-none-eabi.cmake
    stm32cubemx/
      CMakeLists.txt    ← CubeMX 自动生成，不要手改
  Core/
    Inc/                ← main.h, gpio.h, spi.h, i2c.h, tim.h ...
    Src/                ← main.c, gpio.c, spi.c, i2c.c, tim.c, freertos.c ...
    Startup/
  Drivers/
    CMSIS/
    STM32F4xx_HAL_Driver/
  Middlewares/
    Third_Party/FreeRTOS/
```

## 3. CMake 配置

### 3.1 CMakePresets.json

在工程根目录创建 `CMakePresets.json`：

```json
{
    "version": 3,
    "configurePresets": [
        {
            "name": "default",
            "hidden": true,
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/${presetName}",
            "toolchainFile": "${sourceDir}/cmake/gcc-arm-none-eabi.cmake",
            "environment": {
                "PATH": "C:/ST/STM32CubeCLT_1.21.0/GNU-tools-for-STM32/bin;C:/ST/STM32CubeCLT_1.21.0/CMake/bin;$penv{PATH}"
            }
        },
        {
            "name": "Debug",
            "inherits": "default",
            "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" }
        }
    ],
    "buildPresets": [
        {
            "name": "Debug",
            "configurePreset": "Debug"
        }
    ]
}
```

### 3.2 构建命令

```powershell
cd esxp1302_stm32f407
cmake --preset Debug          # Configure
cmake --build --preset Debug -j8   # Build
```

### 3.3 VS Code tasks.json

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "CMake Configure",
            "type": "shell",
            "command": "cmake",
            "args": ["--preset", "Debug"],
            "options": { "cwd": "${workspaceFolder}/esxp1302_stm32f407" },
            "group": "build"
        },
        {
            "label": "Build",
            "type": "shell",
            "command": "cmake",
            "args": ["--build", "--preset", "Debug", "-j8"],
            "options": { "cwd": "${workspaceFolder}/esxp1302_stm32f407" },
            "group": { "kind": "build", "isDefault": true }
        }
    ]
}
```

## 4. VS Code launch.json (ST-Link 调试)

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug (ST-Link)",
            "type": "cortex-debug",
            "request": "launch",
            "servertype": "stlink",
            "stlinkPath": "C:/ST/STM32CubeCLT_1.21.0/STLink-gdb-server/bin/ST-LINK_gdbserver.exe",
            "device": "STM32F407ZG",
            "svdFile": "C:/ST/STM32CubeCLT_1.21.0/STMicroelectronics_CMSIS_SVD/STM32F407.svd",
            "executable": "${workspaceFolder}/esxp1302_stm32f407/build/Debug/esxp1302_stm32f407.elf",
            "runToEntryPoint": "main",
            "stm32cubeprogrammer": "C:/ST/STM32CubeCLT_1.21.0/STM32CubeProgrammer/bin",
            "preLaunchTask": "Build",
            "showDevDebugOutput": "raw",
            "swoConfig": { "enabled": false }
        }
    ]
}
```

## 5. main.c 关键修改

### 5.1 printf 重定向

在 `Core/Src/main.c` 的 USER CODE 区域添加：

```c
/* USER CODE BEGIN 0 */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
/* USER CODE END 0 */
```

### 5.2 TIM2 启动

在 `main()` 中 `MX_TIM2_Init()` 之后添加：

```c
/* USER CODE BEGIN 2 */
HAL_TIM_Base_Start(&htim2);
/* USER CODE END 2 */
```

## 6. freertos.c 关键修改

### 6.1 添加测试任务

在 `Core/Src/freertos.c` 中：

```c
/* USER CODE BEGIN Includes */
#include "test_loragw.h"
/* USER CODE END Includes */

/* USER CODE BEGIN Variables */
osThreadId_t testTaskHandle;
const osThreadAttr_t testTask_attributes = {
    .name       = "testTask",
    .stack_size = 8192 * 4,    /* 32 KB (8192 words) */
    .priority   = (osPriority_t) osPriorityNormal,
};
/* USER CODE END Variables */

void StartTestTask(void *argument)
{
    test_loragw_run();
    for(;;) { osDelay(1000); }
}

/* 在 MX_FREERTOS_Init() 中创建 */
/* USER CODE BEGIN RTOS_THREADS */
testTaskHandle = osThreadNew(StartTestTask, NULL, &testTask_attributes);
/* USER CODE END RTOS_THREADS */
```

> **关键点**：栈大小 8192 words (32 KB)。HAL 层在 `lgw_start()` 期间加载固件 blob、校准等操作需要较大栈空间。

## 7. 链接脚本注意事项

使用 CubeMX 生成的链接脚本 `STM32F407ZGTx_FLASH_cmake.ld`。

需要确保 `.heap` 和 `.stack` 足够大。默认值通常够用（FreeRTOS 使用自己的堆管理）。

## 8. 验证

完成以上步骤后：

```powershell
cmake --preset Debug
cmake --build --preset Debug -j8
```

应能无错误编译，生成 `.elf` 文件。通过 ST-Link 烧录后，USART1 应输出 printf 信息。
