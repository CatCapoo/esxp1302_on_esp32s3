# 08 — OLED 第 6 行时间显示永久空白 Bug 分析与修复

> **所属项目**：ESXP1302 STM32F407 移植  
> **发现时间**：2026-03-08（SNTP 功能集成后首次观察）  
> **关联知识**：[learning/06_stm32_i2c_freertos.md](../learning/06_stm32_i2c_freertos.md)  
> **状态**：✅ 已修复，已验证（多次复位均正常）

---

## 目录

1. [问题现场还原](#1-问题现场还原)
2. [第一轮调查：5 秒空白窗口（根因 1）](#2-第一轮调查5-秒空白窗口根因-1)
3. [第二轮调查：永久空白（根因 2/3/4）](#3-第二轮调查永久空白根因-234)
4. [根因 2 详析：I2C 超时裕量不足](#4-根因-2-详析i2c-超时裕量不足)
5. [根因 3 详析：STM32F4 I2C BUSY 标志锁死](#5-根因-3-详析stm32f4-i2c-busy-标志锁死)
6. [根因 4 详析：I2C 总线无互斥保护](#6-根因-4-详析i2c-总线无互斥保护)
7. [根因 5 详析：OLED 刷新失败沉默](#7-根因-5-详析oled-刷新失败沉默)
8. [修复方案与代码变更](#8-修复方案与代码变更)
9. [验证过程](#9-验证过程)
10. [修复后剩余的临时诊断日志](#10-修复后剩余的临时诊断日志)

---

## 1. 问题现场还原

### 1.1 OLED 布局

网关 OLED（SSD1306，128×64，I2C 地址 0x3C，I2C2 总线）分 8 行：

```
Row 0: ESXP1302 STM32
Row 1: EUI:AABBCCDD
Row 2:     11223344
Row 3: IP:192.168.71.110
Row 4: Concentrator OK
Row 5: NS=192.168.71.100:1700
Row 6: [时间显示]           ← 本 Bug 的目标行
Row 7: Temp=26.1C GPS=(N/A)
```

Row 6 的预期行为：
- SNTP 未同步时：`Up 00:00:35`（运行时长）
- SNTP 同步后：`2026-03-08 10:23:45 Z`（UTC 时间）

### 1.2 Bug 现象

**复现步骤**：上电或按下 RESET 按钮。  
**期望**：Row 6 从 `Up 00:00:XX` 开始，同步后切换为 UTC 时间。  
**实际**：Row 6 **完全空白**，无论等多久（即使 SNTP 日志显示同步成功）。

**出现频率**：间歇性。约有 30-60% 的复位会出现此现象，其余复位正常。

### 1.3 串口日志（异常时）

```
INFO: UDP sockets opened, NS=192.168.71.100 up:1700 down:1700
...（5 秒后）...
[SNTP] Querying 192.168.71.1 ...
[SNTP] Time synced: 2026-03-08 10:23:45 UTC       ← SNTP 成功
...（继续等待）...
##### 2026-03-08 10:23:45 UTC #####               ← 统计报告时间正确
### [UPSTREAM] ###
...
```

串口一切正常，但 OLED Row 6 依然空白。

---

## 2. 第一轮调查：5 秒空白窗口（根因 1）

### 2.1 分析：启动屏只写了 Row 0–5

检查启动代码（`lora_pkt_fwd.c`，`pkt_fwd_main()` 末尾）：

```c
/* OLED 启动屏写 Row 0-4（draw 到帧缓冲）*/
oled_draw_string(0, 0, "ESXP1302 STM32  ");
oled_draw_string(0, 1, out_info);
oled_draw_string(0, 2, out_info);
oled_draw_string(0, 3, out_info);
oled_draw_string(0, 4, "Concentrator OK ");

/* Row 5：NS 信息（含 oled_refresh，推送全部 8 行到屏幕）*/
oled_show_one_line(0, 5, out_info);   // ← 调用 oled_refresh()

/* 进入主循环 */
while (!exit_sig && !quit_sig) {
    time_count = 0;
    while (time_count < stat_interval) {
        vTaskDelay(pdMS_TO_TICKS(1000 * TIME_REFRESH));   // ← 先睡 5 秒！
        time_count += TIME_REFRESH;
        // ... 然后才更新 Row 6 ...
        oled_show_one_line(0, 6, stat_timestamp);
    }
}
```

**问题**：`oled_show_one_line(0, 5, ...)` 调用 `oled_refresh()`，将整个帧缓冲（包括全零的 Row 6）推送到屏幕。主循环第一次更新 Row 6 要等 `TIME_REFRESH = 5` 秒。

**这就是最初 5 秒内 Row 6 空白的原因。**

但这不能解释"无论等多久都空白"的现象。

### 2.2 修复一：启动后立即写 Row 6

```c
/* 在 oled_show_one_line(0, 5, ...) 之后、主循环之前，立即写 Row 6 */
{
    uint32_t up = get_uptime_sec();
    snprintf(stat_timestamp, sizeof stat_timestamp, "Up %02lu:%02lu:%02lu",
             (unsigned long)(up / 3600),
             (unsigned long)((up % 3600) / 60),
             (unsigned long)(up % 60));
    oled_show_one_line(0, 6, stat_timestamp);
}
```

**验证**：上电后 Row 6 立即显示 `Up 00:00:01`。

**但是**："永久空白"现象依然存在于约半数复位中。

---

## 3. 第二轮调查：永久空白（根因 2/3/4）

### 3.1 加诊断日志

在主循环的 Row 6 更新处加入诊断：

```c
// lora_pkt_fwd.c — 主循环内
oled_show_one_line(0, 6, stat_timestamp);

/* --- TEMPORARY DIAGNOSTIC --- */
{
    static uint32_t r6_cnt = 0;
    r6_cnt++;
    if (r6_cnt <= 3 || (r6_cnt % 6) == 0) {
        printf("[OLED-DBG] Row6 #%lu: \"%s\"\r\n",
               (unsigned long)r6_cnt, stat_timestamp);
    }
}
```

**观察到的日志（异常时）**：

```
[OLED-DBG] Row6 #1: "Up 00:00:06"    ← 字符串正确
[I2C] ERR: write 0x3C len=1025 HAL=2, resetting...  ← HAL=2 即 HAL_BUSY
[I2C] OK: recovered after reset
[OLED-DBG] Row6 #2: "Up 00:00:11"    ← 字符串正确，但屏幕依然空白
[I2C] ERR: write 0x3C len=1025 HAL=2, resetting...
[I2C] OK: recovered after reset
...
```

**关键发现**：
1. `stat_timestamp` 字符串是正确的（打印内容没问题）
2. `oled_show_one_line()` 调用了 `oled_refresh()`
3. `oled_refresh()` 中的 `lgw_i2c_write_buf()` **始终返回 HAL_BUSY**
4. 即使 DeInit/Init 之后重试成功了，屏幕也没有刷新

---

## 4. 根因 2 详析：I2C 超时裕量不足

### 4.1 计算

```
I2C 100kHz，1025 字节传输时间 ≈ 92ms
原始超时: I2C_TIMEOUT_MS = 100ms
裕量: 100 - 92 = 8ms
```

FreeRTOS `thread_up` 以 `AboveNormal` 优先级运行，每次 `lgw_receive()` 调用会持续数毫秒（SX1302 SPI 批量读取）。在 1025 字节 I2C 传输的 92ms 期间，高优先级任务至少被抢占几次，每次抢占消耗几毫秒，很容易超过 8ms 的裕量。

### 4.2 触发条件

```
主任务: HAL_I2C_Master_Transmit(hi2c2, 0x3C<<1, tx_data, 1025, 100ms)
                ↑ 传输进行到 ~92ms...
thread_up 抢占: lgw_receive() 持续 ~5-10ms
                ↑ 抢占期间 I2C 时钟继续走（硬件 DMA/轮询在 HAL 内部）
主任务恢复: 经过时间 = 92ms + 抢占时间 > 100ms → HAL_TIMEOUT
```

### 4.3 修复

```c
// 动态超时：100ms 基础 + 每 5 字节 1ms 额外裕量
uint32_t timeout = I2C_TIMEOUT_MS + (uint32_t)(size / 5);
// 1025 字节 → timeout = 100 + 205 = 305ms
```

---

## 5. 根因 3 详析：STM32F4 I2C BUSY 标志锁死

### 5.1 I2C 初始化阶段的 NACK 序列

`lgw_start()` 中探测 LM75A 温度传感器：

```c
// loragw_hal.c
static const uint8_t lm75a_probe_addrs[] = {0x48, 0x49, 0x4A, 0x4B};
for (i = 0; i < 4; i++) {
    err = lm75a_configure(lm75a_probe_addrs[i]);
    // → lgw_i2c_read() → HAL_I2C_Mem_Read(0x48<<1, ...)
    // → I2C 总线发起寻址，无设备应答 → NACK
}
// 4 次 NACK 全部失败（板上无 LM75A）
```

### 5.2 NACK 如何锁死 BUSY 标志

```
时序图：

SCL: ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
SDA: ‾\_START_/‾addr‾\_W\_NACK_/‾STOP‾‾‾‾‾‾
                                  ↑
                       从设备拉高 SDA = NACK

I2C 硬件模拟滤波器（依照 Errata ES0182）：
  在特定 NACK 时序下，滤波器输出错误值
  → SR2.BUSY 位被置 1
  → 无法被软件清除（只有外设复位才能清除）
```

### 5.3 后续 OLED 调用的结果

```c
// 每次 lgw_i2c_write_buf() 内部
HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c2, ...);

// HAL 源码内部（stm32f4xx_hal_i2c.c）：
// if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_BUSY) == SET)
//     return HAL_BUSY;   ← 立即返回，没有任何 I2C 信号发出！
```

### 5.4 间歇性的物理原因

Errata ES0182 不是每次都触发，触发概率依赖于：
- 4 次 NACK 期间 SDA/SCL 的精确电压波形
- PCB 上 I2C 上拉电阻和寄生电容（影响边沿速率）
- 上电时 I2C 总线的初始状态

实测：约 30-60% 的复位触发此问题，与 Errata 描述的"随机性"一致。

### 5.5 修复

```c
// loragw_i2c.c — 所有 HAL I2C 调用后
if (ret == HAL_BUSY || ret == HAL_TIMEOUT) {
    HAL_I2C_DeInit(I2C_HANDLE);   // 复位外设，清除 BUSY 标志
    HAL_I2C_Init(I2C_HANDLE);    // 重新初始化
    ret = HAL_I2C_Master_Transmit(...);  // 重试一次
}
```

`HAL_I2C_DeInit()` 将 I2C 外设的 `PE`（Peripheral Enable）位清零，触发 STM32F4 I2C 状态机复位，从而清除 `BUSY` 标志。这是 ST 官方推荐的 Errata workaround。

---

## 6. 根因 4 详析：I2C 总线无互斥保护

### 6.1 并发竞争路径

```
thread_up（AboveNormal 优先级，每 ~100ms）：
  lgw_receive() → lgw_get_temperature()
    → stts751_get_temperature(ts_addr)
      → lgw_i2c_read(ts_addr, reg, &val)    ← 使用 hi2c2
        → HAL_I2C_Mem_Read(hi2c2, ...)

主任务（Normal 优先级，每 5s）：
  oled_show_one_line(0, 6, ...)
    → oled_refresh()
      → lgw_i2c_write_buf(0x3C, tx, 1025)  ← 使用同一 hi2c2
        → HAL_I2C_Master_Transmit(hi2c2, ...)
```

`hi2c2` 是同一个硬件外设句柄，无任何同步保护。

### 6.2 HAL_LOCK 的不足

STM32 HAL 使用 `__HAL_LOCK()` / `__HAL_UNLOCK()` 宏，但它们是非原子的 test-and-set，在 FreeRTOS 抢占调度下**不能保证互斥**（见 [learning/06 第 3 节](../learning/06_stm32_i2c_freertos.md#3-hal_lock-在-freertos-下的不安全性)）。

### 6.3 修复：FreeRTOS Mutex

```c
// loragw_i2c.c
static SemaphoreHandle_t s_i2c_mtx = NULL;

static void i2c_mtx_ensure(void) {
    if (s_i2c_mtx == NULL)
        s_i2c_mtx = xSemaphoreCreateMutex();  // 懒初始化，调度器已运行时安全
}

// lgw_i2c_write_buf() 示例
int lgw_i2c_write_buf(uint8_t dev_addr, const uint8_t *data, size_t size) {
    i2c_mtx_ensure();
    xSemaphoreTake(s_i2c_mtx, portMAX_DELAY);
    // ... HAL 操作 ...
    xSemaphoreGive(s_i2c_mtx);
    return ...;
}
```

`xSemaphoreTake(portMAX_DELAY)` 会阻塞调用任务直到 mutex 可用，保证同一时刻只有一个任务访问 I2C2 总线。

---

## 7. 根因 5 详析：OLED 刷新失败沉默

### 7.1 原始代码

```c
// 修复前
void oled_refresh(void) {
    oled_cmd(0x21); oled_cmd(0x00); oled_cmd(0x7F);
    oled_cmd(0x22); oled_cmd(0x00); oled_cmd(0x07);
    // ...
    lgw_i2c_write_buf(OLED_I2C_ADDR, tx_data, sizeof tx_data);
    // 返回值被丢弃！
}
```

`oled_refresh()` 是 `void`，所有 I2C 操作的返回值全部被丢弃。这意味着：
- I2C 返回 `HAL_BUSY` → 不知道
- I2C 返回 `HAL_TIMEOUT` → 不知道
- 调用方 `oled_show_one_line()` → 不知道
- 结果：屏幕不更新，代码认为一切正常

**这是 "沉默失败（silent failure）"** 的典型案例。

### 7.2 修复：err |= 累积模式

```c
// 修复后
void oled_refresh(void) {
    int err = 0;
    err |= oled_cmd(0x21); err |= oled_cmd(0x00); err |= oled_cmd(0x7F);
    err |= oled_cmd(0x22); err |= oled_cmd(0x00); err |= oled_cmd(0x07);
    // ...
    err |= lgw_i2c_write_buf(OLED_I2C_ADDR, tx_data, sizeof tx_data);
    if (err != 0) {
        static uint32_t cnt = 0;
        if (++cnt <= 10)
            printf("[OLED] ERR: refresh failed (#%lu)\r\n", (unsigned long)cnt);
    }
}
```

这使得 I2C 失败立即在串口可见，是后续诊断的关键。

---

## 8. 修复方案与代码变更

### 8.1 修改文件汇总

| 文件 | 修改内容 | 解决根因 |
|------|---------|---------|
| `libloragw/loragw_i2c.c` | FreeRTOS mutex + 动态超时 + HAL_BUSY/TIMEOUT 恢复 + 诊断 printf | 根因 2, 3, 4 |
| `libloragw/loragw_oled.c` | `oled_refresh()` 改为 `err |=` 累积模式 + 错误 printf | 根因 5 |
| `packet_forwarder/lora_pkt_fwd.c` | 主循环前立即写 Row 6 + OLED-DBG 诊断 | 根因 1 |

### 8.2 loragw_i2c.c 核心变更

**新增依赖：**
```c
#include "FreeRTOS.h"
#include "semphr.h"
```

**新增状态变量：**
```c
static SemaphoreHandle_t s_i2c_mtx = NULL;

static void i2c_mtx_ensure(void) {
    if (s_i2c_mtx == NULL)
        s_i2c_mtx = xSemaphoreCreateMutex();
}
```

**lgw_i2c_write_buf() 完整变更（关键）：**
```c
// 修复前
int lgw_i2c_write_buf(uint8_t dev_addr, const uint8_t *data, size_t size) {
    if (!data || size == 0) return LGW_I2C_ERROR;
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(
        I2C_HANDLE, (uint16_t)(dev_addr << 1),
        (uint8_t *)data, (uint16_t)size, I2C_TIMEOUT_MS);
    return (ret == HAL_OK) ? LGW_I2C_SUCCESS : LGW_I2C_ERROR;
}

// 修复后
int lgw_i2c_write_buf(uint8_t dev_addr, const uint8_t *data, size_t size) {
    if (!data || size == 0) return LGW_I2C_ERROR;
    i2c_mtx_ensure();

    uint32_t timeout = I2C_TIMEOUT_MS + (uint32_t)(size / 5);  // ← 动态超时

    xSemaphoreTake(s_i2c_mtx, portMAX_DELAY);                  // ← 互斥锁
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(
        I2C_HANDLE, (uint16_t)(dev_addr << 1),
        (uint8_t *)data, (uint16_t)size, timeout);
    if (ret != HAL_OK) {
        printf("[I2C] ERR: write 0x%02X len=%u HAL=%d, resetting...\r\n",
               dev_addr, (unsigned)size, (int)ret);
        HAL_I2C_DeInit(I2C_HANDLE);                             // ← BUSY 恢复
        HAL_I2C_Init(I2C_HANDLE);
        ret = HAL_I2C_Master_Transmit(
            I2C_HANDLE, (uint16_t)(dev_addr << 1),
            (uint8_t *)data, (uint16_t)size, timeout);
        if (ret != HAL_OK)
            printf("[I2C] ERR: retry also failed (HAL=%d)\r\n", (int)ret);
        else
            printf("[I2C] OK: recovered after reset\r\n");
    }
    xSemaphoreGive(s_i2c_mtx);                                  // ← 释放互斥锁
    return (ret == HAL_OK) ? LGW_I2C_SUCCESS : LGW_I2C_ERROR;
}
```

`lgw_i2c_read()` 和 `lgw_i2c_read_word()` 也同样加入了 mutex 和 HAL_BUSY 恢复逻辑（但超时保持原始 `I2C_TIMEOUT_MS`，因为读操作数据量小）。

### 8.3 lora_pkt_fwd.c Row 6 立即初始化

```c
// 在 oled_show_one_line(0, 5, out_info) 之后，主循环 while() 之前：
{
    uint32_t up = get_uptime_sec();
    snprintf(stat_timestamp, sizeof stat_timestamp, "Up %02lu:%02lu:%02lu",
             (unsigned long)(up / 3600),
             (unsigned long)((up % 3600) / 60),
             (unsigned long)(up % 60));
    oled_show_one_line(0, 6, stat_timestamp);
}
```

---

## 9. 验证过程

### 9.1 修复后串口输出（正常复位）

```
[SNTP] Task start. candidates: gw=192.168.71.1, ...
INFO: thread_up created
...
[OLED-DBG] Row6 #1: "Up 00:00:01"        ← 立即有内容
...（5秒后）...
[SNTP] Querying 192.168.71.1 ...
[SNTP] Time synced: 2026-03-08 10:23:45 UTC
[OLED-DBG] Row6 #2: "2026-03-08 10:23:50 Z"  ← 同步后显示真实时间
```

### 9.2 修复后串口输出（触发 BUSY 但自恢复）

```
[I2C] ERR: write 0x3C len=1025 HAL=2, resetting...   ← BUSY，触发 Errata workaround
[I2C] OK: recovered after reset                       ← DeInit/Init 后重试成功
[OLED-DBG] Row6 #1: "Up 00:00:01"                    ← Row 6 正常显示
```

### 9.3 多次复位测试结果

| 测试次数 | Row 6 正常 | Row 6 空白 |
|---------|-----------|-----------|
| 修复前（10次） | ~5次 | ~5次 |
| 修复后（10次） | 10次 | 0次 |

---

## 10. 修复后剩余的临时诊断日志

以下 `printf` 在功能稳定后可以移除（标记为 `TEMPORARY DIAGNOSTIC`）：

| 位置 | 日志前缀 | 移除条件 |
|------|---------|---------|
| `loragw_i2c.c lgw_i2c_write_buf()` | `[I2C] ERR: write ...` | 确认 BUSY 不再出现后 |
| `loragw_i2c.c lgw_i2c_write_buf()` | `[I2C] OK: recovered` | 同上 |
| `loragw_oled.c oled_refresh()` | `[OLED] ERR: refresh failed` | 同上 |
| `lora_pkt_fwd.c` 主循环 | `[OLED-DBG] Row6 #N: "..."` | 确认显示稳定后 |

移除这些日志不影响功能，只减少串口输出量。
