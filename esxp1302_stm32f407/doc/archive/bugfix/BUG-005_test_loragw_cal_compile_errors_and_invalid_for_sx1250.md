# BUG-005：test_loragw_cal.c 多处编译错误及测试无效声明（SX1250 不适用）

- **日期**：2026-02-20  
- **文件**：`main/test/test_loragw_cal.c`  
- **严重级别**：编译失败 + 功能性无效

---

## 问题描述

编译 `test_loragw_cal.c` 时报出 11 处错误（函数未声明、格式符不匹配、参数不足、FreeRTOS 符号未声明）。修复编译错误后在 SX1250 硬件上运行，测试全部失败，SX125x 寄存器访问报错。

**结论：该测试文件仅适用于 SX1255/SX1257 硬件，不支持 SX1250，在 SX1250 平台上测试结果无效。**

---

## 编译错误详情与修复

### 错误 1–2：`lgw_sx125x_reg_w` / `lgw_sx125x_reg_r` 未声明

```
error: implicit declaration of function 'lgw_sx125x_reg_w';
       did you mean 'sx125x_reg_w'?
error: implicit declaration of function 'lgw_sx125x_reg_r';
       did you mean 'sx125x_reg_r'?
```

**原因**：原始 Semtech 代码中使用了旧版函数名 `lgw_sx125x_reg_w/r`，当前 HAL 库中对应接口为 `sx125x_reg_w/r`（无 `lgw_` 前缀）。

**修复**：将 `setup_tx_dc_offset()` 中所有 `lgw_sx125x_reg_w` / `lgw_sx125x_reg_r` 替换为 `sx125x_reg_w` / `sx125x_reg_r`。

---

### 错误 3–8：`printf` 格式符与 `int32_t` 类型不匹配

```
error: format '%d' expects argument of type 'int',
       but argument has type 'int32_t' {aka 'long int'}
error: format '%u' expects argument of type 'unsigned int',
       but argument has type 'int32_t' {aka 'long int'}
```

**原因**：`i_offset`、`q_offset`、`f_offset`、`val_min`、`val_max`、`val_mean` 均为 `int32_t`，在 Xtensa 平台等同于 `long int`，`%d`/`%u` 对应 `int`/`unsigned int`，`-Werror=format=` 将警告升为错误。

**修复**：

```c
// 修复前
printf("i_offset:%d q_offset:%d f_offset:%d ...", i_offset, q_offset, f_offset, ...);
printf(" min:%u max:%u mean:%u std:%f\n", val_min, val_max, val_mean, val_std);

// 修复后
printf("i_offset:%ld q_offset:%ld f_offset:%ld ...", (long)i_offset, (long)q_offset, (long)f_offset, ...);
printf(" min:%ld max:%ld mean:%ld std:%f\n", (long)val_min, (long)val_max, (long)val_mean, val_std);
```

---

### 错误 9：`lgw_connect()` 缺少必要参数

```
error: too few arguments to function 'lgw_connect'
note: declared here: int lgw_connect(const lgw_com_type_t com_type, const char * com_path);
```

**原因**：原始代码调用 `lgw_connect()` 无参数，当前接口要求传入通信类型和路径字符串。同时传入 `NULL` 会被 `lgw_connect` 内部的空指针检查直接返回错误（`if (com_path == NULL) return LGW_REG_ERROR`）。

**修复**：

```c
// 修复前
x = lgw_connect();

// 修复后
x = lgw_connect(LGW_COM_SPI, "spi");
```

> **说明**：ESP32 SPI 实现（`lgw_spi_open`）不使用 `com_path` 字符串内容，但该参数必须非 NULL。

同时在 `#include` 中添加 `"loragw_com.h"` 以获得 `LGW_COM_SPI` 定义。

---

### 错误 10–11：`vTaskDelay` / `portTICK_PERIOD_MS` 未声明

```
error: implicit declaration of function 'vTaskDelay'
error: 'portTICK_PERIOD_MS' undeclared (first use in this function)
```

**原因**：`info_and_delay()` 和 `app_main()` 中使用了 FreeRTOS API，但文件未包含对应头文件。

**修复**：在文件顶部添加：

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
```

---

### 警告（提升为错误）：未使用的局部变量

```
warning: unused variable 'arg_u', 'arg_d', 'i'
```

**修复**：从 `main_test()` 中删除未使用的变量声明 `int i`、`double arg_d`、`unsigned int arg_u`。

---

## 运行时失败与根本原因分析（SX1250 不适用声明）

编译修复后在搭载 **SX1250** 射频芯片的硬件平台上运行，输出如下：

```
INFO: Configuring SX1250_0 in single input mode
Loading CAL fw for sx125x
-------------------------------------
ERROR: sx125x register 5 write failed (w:216 r:210)!!
...
ERROR: PLL failed to lock
```

### 原因分析

`test_loragw_cal.c` 是 Semtech 专为 **SX1255/SX1257** 设计的 TX DC 偏置校准测试，其核心函数 `setup_tx_dc_offset()` 通过 SX1302 的 SX125x 子 SPI 总线直接操作 SX125x PLL/DAC/MIX 寄存器。SX1250 与 SX125x 是完全不同的射频架构：

| 对比项 | SX1255/SX1257 | SX1250 |
|--------|--------------|--------|
| 接口 | SX1302 通过专用子 SPI 访问 | SX1302 通过内部 SPIB 访问，协议不同 |
| TX DC 偏置校准 | `cal_firmware_sx125x` + `sx1302_cal_start()` | SX1250 内部自动完成，由 `sx1250_calibrate()` 触发 |
| 寄存器操作 | `sx125x_reg_w/r()` | `sx1250_reg_w/r()`，寄存器地址映射完全不同 |

读回值 `0xD2 (210)` 是 SX125x 子 SPI 总线上无设备响应时的浮空噪声值，确认 SX1250 硬件不存在 SX125x 寄存器空间。

### 结论

**在 SX1250 平台上，`test_loragw_cal.c` 的测试结果完全无效，不可用于验证任何校准功能。**

SX1250 平台的射频校准应使用：
- `sx1250_calibrate()`（已集成于 `lgw_start()` 流程中，自动执行）
- `main/test/test_loragw_spi_sx1250.c`（验证 SX1250 SPI 通信）

---

## 变更摘要

| 修改位置 | 修改内容 |
|----------|----------|
| 头文件区 | 添加 `freertos/FreeRTOS.h`、`freertos/task.h`、`loragw_com.h` |
| `setup_tx_dc_offset()` | `lgw_sx125x_reg_w/r` → `sx125x_reg_w/r` |
| `cal_tx_dc_offset()` printf | `%d`/`%u` → `%ld`，`int32_t` 参数加 `(long)` 转换 |
| `main_test()` | `lgw_connect()` → `lgw_connect(LGW_COM_SPI, "spi")` |
| `main_test()` | 删除未使用变量 `i`、`arg_d`、`arg_u` |
