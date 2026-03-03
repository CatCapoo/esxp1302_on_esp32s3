# 02 — ESP32 → STM32 平台适配改动速查表

## 概述

本文列出从 ESP-IDF 移植到 STM32 HAL 时的 **所有** 平台适配改动。每项说明原始代码、改动方式和原因。

---

## 1. SPI 句柄类型替换

### 问题
ESP-IDF 使用 `spi_device_handle_t`（`void*` 类型），STM32 HAL 使用 `SPI_HandleTypeDef*`。

### 改动

| 文件 | 原始 (ESP32) | 改为 (STM32) |
|------|-------------|-------------|
| `loragw_spi.h` | `spi_device_handle_t` | `SPI_HandleTypeDef*` |
| `loragw_spi.c` | `spi_bus_*`, `spi_device_*` API | `HAL_SPI_Transmit/TransmitReceive` |
| `loragw_com.h/c` | `spi_device_handle_t` | `SPI_HandleTypeDef*` |
| `loragw_spi.h` | 添加 typedef 兼容层 | `typedef SPI_HandleTypeDef* spi_device_handle_t;` |

### 关键代码 (`loragw_spi.h`)

```c
#include "stm32f4xx_hal.h"
#include "board_config.h"
typedef SPI_HandleTypeDef* spi_device_handle_t;  /* 兼容上层代码 */
```

---

## 2. SPI CS (NSS) 控制

### 问题
ESP-IDF 在 `spi_device_transmit()` 中自动管理 CS，STM32 HAL 需要手动控制。

### 改动 (`loragw_spi.c`)

```c
static inline void cs_select(void) {
    HAL_GPIO_WritePin(SX1302_NSS_PORT, SX1302_NSS_PIN, GPIO_PIN_RESET);
}
static inline void cs_deselect(void) {
    HAL_GPIO_WritePin(SX1302_NSS_PORT, SX1302_NSS_PIN, GPIO_PIN_SET);
}
```

每个 SPI 事务前调用 `cs_select()`，后调用 `cs_deselect()`。

---

## 3. SPI 打开/关闭

### 问题
ESP-IDF 需要 `spi_bus_initialize()` + `spi_bus_add_device()` + `spi_bus_remove_device()` + `spi_bus_free()`。

### 改动
STM32 中 CubeMX 已初始化 SPI3，`lgw_spi_open()` 只需返回 handle：

```c
int lgw_spi_open(SPI_HandleTypeDef **spi_target) {
    *spi_target = SX1302_SPI_HANDLE;  /* &hspi3 */
    cs_deselect();
    return LGW_SPI_SUCCESS;
}
```

---

## 4. radio_spi_rb/wb opcode 掩码 BUG 修复

### 问题
原始 ESP32 代码中 `radio_spi_rb()` 和 `radio_spi_wb()` 对 op_code 做了 `& 0x7F` 掩码：

```c
tx[1] = (uint8_t)(READ_ACCESS | (op_code & 0x7F));  /* 错误! */
```

这会把 SX1250 的 `GET_STATUS` (0xC0)、`SET_STANDBY` (0x80) 等 ≥ 0x80 的命令截断。

### 修复
直接发送原始 opcode：

```c
tx[1] = op_code;  /* 不做掩码 */
```

---

## 5. GPIO 复位

### 问题
ESP-IDF 使用 `gpio_set_level()` API，STM32 使用 `HAL_GPIO_WritePin()`。

### 改动 (`loragw_gpio.c`)

```c
void lgw_reset(void) {
    /* SX1302 RESET 高有效 */
    HAL_GPIO_WritePin(SX1302_RESET_PORT, SX1302_RESET_PIN_NUM, GPIO_PIN_SET);
    wait_ms(100);
    HAL_GPIO_WritePin(SX1302_RESET_PORT, SX1302_RESET_PIN_NUM, GPIO_PIN_RESET);
    wait_ms(500);  /* 500ms post-reset wait */
}
```

---

## 6. 定时函数替换

### 问题
ESP-IDF 使用 `usleep()` / `gettimeofday()` / `vTaskDelay()`。

### 改动 (`loragw_aux.c`)

| 原始 (ESP32) | 改为 (STM32) | 说明 |
|-------------|-------------|------|
| `usleep(us)` | `wait_us(us)` | TIM2 忙等 |
| `vTaskDelay(ms/portTICK)` | `osDelay(ms)` | CMSIS-RTOS2 |
| `gettimeofday()` | `__HAL_TIM_GET_COUNTER(&htim2)` | TIM2 自由运行 |
| `clock()` | `HAL_GetTick()` | ms 级时间 |

```c
void wait_ms(unsigned long delay_ms) {
    if (delay_ms == 0) return;
    osDelay(delay_ms);
}

void wait_us(unsigned long delay_us) {
    uint32_t start = __HAL_TIM_GET_COUNTER(&htim2);
    while ((__HAL_TIM_GET_COUNTER(&htim2) - start) < delay_us) { /* spin */ }
}

uint32_t timestamp_us(void) {
    return __HAL_TIM_GET_COUNTER(&htim2);
}
```

---

## 7. I2C 接口替换

### 问题
ESP-IDF 使用 `i2c_master_*` / `i2c_esp32_open()` API，STM32 使用 `HAL_I2C_*`。

### 改动

| 原始函数 | 替换为 | 文件 |
|----------|--------|------|
| `i2c_esp32_open()` | `lgw_i2c_open()` | `loragw_hal.c` |
| `i2c_esp32_close()` | `lgw_i2c_close()` | `loragw_hal.c` |
| `i2c_linuxdev_read()` | `lgw_i2c_read()` | `loragw_stts751.c` 等 |
| `i2c_linuxdev_write()` | `lgw_i2c_write()` | `loragw_stts751.c` 等 |

`loragw_i2c.c` 封装了 `HAL_I2C_Mem_Read/Write` + `HAL_I2C_Master_Transmit`，提供统一接口。

### 关键注意点

STM32 HAL I2C 地址需要左移 1 位：
```c
HAL_I2C_Mem_Read(I2C_HANDLE, (uint16_t)(dev_addr << 1), ...);
```

---

## 8. 温度传感器地址修改

### 问题
ESP32 版本扫描 STTS751 地址 `{0x39, 0x3B, 0x38}`，本板使用的是 LM75A。

### 改动 (`loragw_hal.c`)

```c
/* 替换 STTS751 地址数组 */
static const uint8_t lm75a_probe_addrs[] = {0x48, 0x49, 0x4A, 0x4B};
/* 扫描逻辑改用此数组 */
```

---

## 9. USB 通信路径禁用

### 问题
`loragw_hal.c` 中有 USB 通信路径代码（`lgw_com_get_temperature` for USB），STM32 不支持。

### 改动

```c
/* USB 路径替换为错误消息 */
printf("ERROR: USB communication not supported on STM32\n");
return LGW_HAL_ERROR;
```

---

## 10. qsort_r → qsort

### 问题
`qsort_r()` 是 GNU 扩展，newlib-nano 不支持。

### 改动
使用标准 `qsort()` + 文件级静态变量传递上下文：

```c
static int sort_context;  /* 代替 qsort_r 的 arg 参数 */

static int compare_func(const void *a, const void *b) {
    /* 使用 sort_context */
}

/* 调用 */
sort_context = xxx;
qsort(array, count, sizeof(elem), compare_func);
```

---

## 11. POSIX 头文件移除

### 问题
ESP-IDF 代码包含 `<sys/time.h>` / `<unistd.h>` 等 POSIX 头。

### 改动
- 移除 `#include <unistd.h>`
- 移除 `#include <sys/time.h>`（或保留但不使用 `gettimeofday`）
- `struct timeval` → `uint32_t timestamp_us()`

---

## 12. 性能测量宏 (`_meas_time_start/stop`)

### 问题
ESP32 HAL 中使用 `_meas_time_start()` / `_meas_time_stop()` 宏进行性能测量，依赖 `clock()`。

### 改动 (`loragw_aux.c/h`)

```c
#if DEBUG_PERF
struct meas_time_s {
    uint32_t start_ms;
    uint32_t stop_ms;
};

#define _meas_time_start(x)  do { (x)->start_ms = HAL_GetTick(); } while(0)
#define _meas_time_stop(n, x, y) do { \
    (x)->stop_ms = HAL_GetTick(); \
    printf("PERF %s: %lu ms\n", (n), (x)->stop_ms - (x)->start_ms); \
} while(0)
#else
#define _meas_time_start(x)
#define _meas_time_stop(n, x, y)
#endif
```

---

## 13. timeout_start / timeout_check

### 问题
HAL 中使用 `timeout_start()` / `timeout_check()` 进行超时判断，原始实现用 `gettimeofday()`。

### 改动 (`loragw_aux.c/h`)

```c
uint32_t timeout_start(void) {
    return HAL_GetTick();
}

bool timeout_check(uint32_t start, uint32_t timeout_ms) {
    return (HAL_GetTick() - start) >= timeout_ms;
}
```

---

## 14. lora_packet_time_on_air

### 问题
`lora_packet_time_on_air()` 函数在 ESP32 `loragw_aux.c` 中实现，STM32 版本缺失。

### 改动
完整移植到 `loragw_aux.c`，需要 `#include <math.h>` 以使用 `ceil()` 函数。

---

## 15. sx1261_usb 桩文件

### 问题
编译时链接器找不到 `sx1261_usb_w/r/rmw/flush` 符号。这些函数只在 USB 通道使用。

### 改动
创建桩文件 `sx1261_usb.h` + `sx1261_usb.c`，所有函数返回 `-1`。

---

## 16. newlib-nano float printf

### 问题
默认 newlib-nano 不支持 `%f` 格式化，所有浮点数 printf 输出为空。

### 改动 (`CMakeLists.txt`)

```cmake
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -u_printf_float
    -u_scanf_float
)
```

---

## 17. SX1302 连接轮询

### 问题
SX1302 复位后需要一段时间才能响应 SPI，ESP32 代码直接读取版本寄存器。

### 改动 (`loragw_com.c` / `lgw_connect`)
添加轮询循环，最多等待 200 ms：

```c
uint32_t t0 = HAL_GetTick();
do {
    lgw_reg_r(SX1302_REG_COMMON_VERSION_VERSION, &ver);
    if (ver == expected_version) break;
    wait_ms(10);
} while ((HAL_GetTick() - t0) < 200);
```

---

## 18. SX1250 双脉冲复位

### 问题
SX1250 需要额外的 RST 脉冲以触发内部自动校准。

### 改动 (`loragw_sx1302.c` / `sx1302_radio_reset`)
在标准复位之后追加第二个 RST 脉冲：

```c
/* 第一次复位 */
lgw_reset();

/* 第二次复位脉冲 (SX1250 auto-cal 触发) */
HAL_GPIO_WritePin(SX1302_RESET_PORT, SX1302_RESET_PIN_NUM, GPIO_PIN_SET);
wait_ms(10);
HAL_GPIO_WritePin(SX1302_RESET_PORT, SX1302_RESET_PIN_NUM, GPIO_PIN_RESET);
wait_ms(10);
```

---

## 速查表汇总

| # | 改动类别 | 关键文件 | 复杂度 |
|---|---------|---------|--------|
| 1 | SPI 类型 | `loragw_spi.h/c`, `loragw_com.h/c` | 中 |
| 2 | SPI CS | `loragw_spi.c` | 低 |
| 3 | SPI 开关 | `loragw_spi.c` | 低 |
| 4 | opcode 掩码 | `loragw_spi.c` | 低（关键 bug） |
| 5 | GPIO | `loragw_gpio.c` | 低 |
| 6 | 定时 | `loragw_aux.c/h` | 中 |
| 7 | I2C | `loragw_i2c.c/h`, `loragw_hal.c` | 中 |
| 8 | 温度传感器 | `loragw_hal.c` | 低 |
| 9 | USB 禁用 | `loragw_hal.c` | 低 |
| 10 | qsort_r | `loragw_sx1302.c` | 低 |
| 11 | POSIX | 多个文件 | 低 |
| 12 | 性能测量 | `loragw_aux.c/h` | 低 |
| 13 | 超时 | `loragw_aux.c/h` | 低 |
| 14 | ToA | `loragw_aux.c` | 中 |
| 15 | USB 桩 | `sx1261_usb.c/h` | 低 |
| 16 | float printf | `CMakeLists.txt` | 低（易遗忘） |
| 17 | 连接轮询 | `loragw_com.c` | 低 |
| 18 | SX1250 双脉冲 | `loragw_sx1302.c` | 低 |
