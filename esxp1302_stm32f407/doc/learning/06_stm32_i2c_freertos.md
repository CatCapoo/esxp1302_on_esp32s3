# STM32 I2C 与 FreeRTOS 互动：陷阱与解法

> **所属项目**：ESXP1302 STM32F407 移植  
> **触发场景**：OLED 第 6 行时间显示永久空白  
> **关联实现**：[impl/08_oled_display_bug.md](../impl/08_oled_display_bug.md)

本文档记录在 FreeRTOS 环境下使用 STM32 HAL I2C 时的三个经典陷阱，以及本项目遭遇和解决这些陷阱的过程。

---

## 目录

1. [I2C 总线时序与超时计算](#1-i2c-总线时序与超时计算)
2. [STM32F4 I2C BUSY 标志锁死（Errata ES0182）](#2-stm32f4-i2c-busy-标志锁死errata-es0182)
3. [HAL_LOCK 在 FreeRTOS 下的不安全性](#3-hal_lock-在-freertos-下的不安全性)
4. [FreeRTOS Mutex 与 HAL_LOCK 的区别](#4-freertos-mutex-与-hal_lock-的区别)
5. [void 返回值的"沉默失败"](#5-void-返回值的沉默失败)

---

## 1. I2C 总线时序与超时计算

### 1.1 I2C 标准模式（Standard Mode）时序

I2C 100kHz 标准模式（SM）中，每个 bit 的传输时间：

$$T_{\text{bit}} = \frac{1}{100\,000} = 10\,\mu\text{s}$$

每个字节 = 8 个数据位 + 1 个 ACK 位 = **9 bits**，加上 START/STOP 条件，每字节约：

$$T_{\text{byte}} \approx 9 \times 10\,\mu\text{s} = 90\,\mu\text{s}$$

### 1.2 SSD1306 OLED 全屏刷新的传输时间

SSD1306 采用**帧缓冲模型**：更新显示内容需要将整个 8×128 像素帧缓冲通过 I2C 写入屏幕：

```
帧缓冲大小 = 8 pages × 128 bytes/page = 1024 bytes
I2C 传输包 = 1 字节控制头(0x40) + 1024 字节数据 = 1025 字节
```

实际传输时间：

$$T_{\text{refresh}} = 1025 \times 90\,\mu\text{s} \approx 92.3\,\text{ms}$$

### 1.3 超时配置与风险

本项目在 `board_config.h` 中：

```c
#define I2C_TIMEOUT_MS    100   // 原始配置
```

裕量只有：$100 - 92.3 = 7.7\,\text{ms}$

FreeRTOS 抢占可能在 `HAL_I2C_Master_Transmit()` 内部的任意时刻挂起当前任务（例如 `thread_up` 以 `AboveNormal` 优先级抢占主任务）。一次典型的 SPI 操作约 1-5ms，极端情况下（SX1302 大批量收包）可能积累到 8ms 以上，触发 `HAL_TIMEOUT`。

### 1.4 修复：动态超时

```c
// loragw_i2c.c — lgw_i2c_write_buf()
uint32_t timeout = I2C_TIMEOUT_MS + (uint32_t)(size / 5);
// 1025 字节 → timeout = 100 + 205 = 305ms，裕量充足
```

---

## 2. STM32F4 I2C BUSY 标志锁死（Errata ES0182）

### 2.1 现象描述

STM32F407 的 I2C 硬件存在一个已知硅片勘误（Silicon Errata）：

> **ES0182 Rev 14, 2.14.7**:  
> *"I2C analog filter may provide wrong value, locking BUSY flag and preventing master mode entry"*  
> 中文：I2C 模拟滤波器可能输出错误值，使 BUSY 标志永久锁定，阻止主机模式进入。

**触发条件**：某些情况下 I2C 总线上的 NACK 或噪声信号会导致 I2C 模拟滤波器进入异常状态。此后 `I2C_SR2` 的 `BUSY` 位被置 1，**永远不会被硬件清除**。

### 2.2 为什么 LM75A 不存在会触发这个问题

本板没有焊接 LM75A 温度传感器。但 `lgw_start()` 在初始化时会依次向 4 个地址（0x48, 0x49, 0x4A, 0x4B）发起 I2C 读操作来探测传感器：

```c
// loragw_hal.c
static const uint8_t lm75a_probe_addrs[] = {0x48, 0x49, 0x4A, 0x4B};
for (i = 0; i < (int)sizeof(lm75a_probe_addrs); i++) {
    err = lm75a_configure(ts_addr);   // 内部调用 lgw_i2c_read()
    // → HAL_I2C_Mem_Read() → 无设备应答 → NACK
}
```

4 次 NACK 都对 SDA/SCL 产生扰动。依照 Errata，这可能导致 `BUSY` 标志被锁定。

### 2.3 HAL 如何响应 BUSY 标志

`HAL_I2C_Master_Transmit()` 的第一步是检查总线是否忙碌：

```c
// stm32f4xx_hal_i2c.c (STM32Cube HAL 源码，简化)
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, ...) {
    // 检查 HAL 级别的锁
    if (hi2c->State != HAL_I2C_STATE_READY) return HAL_BUSY;
    // 检查硬件 BUSY 标志
    if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_BUSY) == SET) return HAL_BUSY;
    // ...（以下代码永远不会执行）
}
```

**结果**：BUSY 标志被锁定后，每次 `HAL_I2C_Master_Transmit()` 调用立即返回 `HAL_BUSY`，从不发出任何 I2C 信号。OLED 刷新完全失败，但调用方看不到任何错误（原因见第 5 节）。

### 2.4 间歇性表现的原因

| 因素 | 说明 |
|------|------|
| Errata 非必然触发 | NACK 不是 100% 触发 BUSY 锁死，取决于精确的 SDA/SCL 时序 |
| I2C 总线噪声 | 取决于 PCB 布线、上拉电阻值、温度等物理因素 |
| 每次上电不同 | 电源上升时序略有差异，影响 I2C 上电复位序列 |

**实际观察**：在某些复位后正常（BUSY 未锁死），某些复位后第 6 行永久空白（BUSY 锁死）。这与 Errata 描述的间歇性现象完全一致。

### 2.5 官方推荐的修复方案

ST 官方勘误的软件 workaround（参见 ES0182 和 AN2838）：

1. 检测到 HAL_BUSY 或 HAL_TIMEOUT 时
2. 调用 `HAL_I2C_DeInit()` 复位 I2C 外设寄存器（清除 BUSY 标志）
3. 调用 `HAL_I2C_Init()` 重新初始化
4. 重试一次操作

```c
// loragw_i2c.c 实现
if (ret == HAL_BUSY || ret == HAL_TIMEOUT) {
    HAL_I2C_DeInit(I2C_HANDLE);
    HAL_I2C_Init(I2C_HANDLE);
    ret = HAL_I2C_Master_Transmit(...);  // 重试
}
```

> **注意**：`HAL_I2C_DeInit()` 会清除 I2C_CR1 的 PE 位，触发 I2C 外设软复位，从而清除 BUSY 标志。这是针对此 Errata 的标准 workaround。

---

## 3. HAL_LOCK 在 FreeRTOS 下的不安全性

### 3.1 HAL_LOCK 机制

STM32 HAL 为每个外设句柄维护一个 `Lock` 字段（`HAL_LockTypeDef`，实际是 `uint8_t`），通过宏实现简单的"重入保护"：

```c
// STM32Cube HAL/Inc/stm32f4xx_hal_def.h
#define __HAL_LOCK(__HANDLE__)                      \
    do {                                             \
        if ((__HANDLE__)->Lock == HAL_LOCKED) {      \
            return HAL_BUSY;                         \
        } else {                                     \
            (__HANDLE__)->Lock = HAL_LOCKED;         \
        }                                            \
    } while (0U)

#define __HAL_UNLOCK(__HANDLE__)                    \
    do {                                             \
        (__HANDLE__)->Lock = HAL_UNLOCKED;           \
    } while (0U)
```

这是一个**非原子的 test-and-set**：

```
读取 Lock → 判断是否为 LOCKED → 写入 LOCKED
```

三步操作之间没有任何原子性保证。

### 3.2 FreeRTOS 抢占破坏 HAL_LOCK

```
时间轴:
  Task A (主任务, Normal):     [1] Lock=UNLOCKED, [2] 判断=OK, [3] 写LOCKED...
  ────────────────────────────────────────────────────────────────────
  Task B (thread_up, AboveNormal):    ...抢占...  [2.5] 读Lock=UNLOCKED, 写LOCKED
  ────────────────────────────────────────────────────────────────────
  Task A 恢复:               [3] 写LOCKED（重复写）→ 两个任务都认为自己持有锁
```

**结果**：两个任务同时进行 I2C 操作，`HAL_I2C_HandleTypeDef` 内部状态（`State`、`XferCount`、`pBuffPtr` 等）被两个任务同时修改，导致不可预测的行为。

### 3.3 本项目的具体竞争路径

```
thread_up (AboveNormal, 每 ~100ms 一次):
  lgw_receive()
    → lgw_get_temperature()
      → stts751_get_temperature()
        → lgw_i2c_read(0x48, ...)     ← HAL_I2C_Mem_Read (hi2c2)

主任务 (Normal, 每 5 秒一次):
  oled_show_one_line()
    → oled_refresh()
      → lgw_i2c_write_buf(0x3C, ...) ← HAL_I2C_Master_Transmit (hi2c2, 1025字节)
```

两条路径共用同一个 `hi2c2` 句柄，`lgw_i2c_write_buf` 传输持续约 92ms，期间极有可能被 `thread_up` 中断。

---

## 4. FreeRTOS Mutex 与 HAL_LOCK 的区别

| 特性 | HAL_LOCK | FreeRTOS Mutex |
|------|----------|---------------|
| 实现 | 简单标志位（uint8_t） | 二值信号量，内核管理 |
| 原子性 | 无（读-改-写三步） | 有（关中断/SVC 保证原子） |
| 阻塞语义 | 不阻塞，立即返回 `HAL_BUSY` | `xSemaphoreTake()` 挂起任务直到可用 |
| 优先级继承 | 无 | 有（防止优先级反转） |
| FreeRTOS 安全 | **否** | **是** |
| 适用场景 | 单任务裸机，防止 ISR 重入 | 多任务 FreeRTOS 环境 |

### 本项目解决方案

```c
// loragw_i2c.c
static SemaphoreHandle_t s_i2c_mtx = NULL;

static void i2c_mtx_ensure(void) {
    if (s_i2c_mtx == NULL)
        s_i2c_mtx = xSemaphoreCreateMutex();
}

// 每个 I2C 操作的模式
xSemaphoreTake(s_i2c_mtx, portMAX_DELAY);   // 阻塞等待，无优先级问题
// ... HAL I2C 操作 ...
xSemaphoreGive(s_i2c_mtx);
```

`portMAX_DELAY` 意味着任务会一直等到 mutex 可用，而非立即返回失败。这才是多任务环境下正确的共享资源保护方式。

> **懒初始化（lazy init）的合理性**：`i2c_mtx_ensure()` 在首次 I2C 操作时创建 mutex，无需担心 FreeRTOS 调度器未启动的问题——因为 I2C 操作只在 FreeRTOS 任务中调用，调度器此时已运行。

---

## 5. void 返回值的"沉默失败"

### 5.1 原始代码的问题

```c
// 修复前的 loragw_oled.c
void oled_refresh(void) {
    oled_cmd(0x21); oled_cmd(0x00); oled_cmd(0x7F);
    oled_cmd(0x22); oled_cmd(0x00); oled_cmd(0x07);
    // ... 构造 tx_data ...
    lgw_i2c_write_buf(OLED_I2C_ADDR, tx_data, sizeof tx_data);
    // 返回值被直接丢弃！
}
```

`oled_refresh()` 是 `void` 函数，调用方完全看不到 I2C 是否成功。当 I2C 因为任何原因失败时（HAL_BUSY、HAL_TIMEOUT、NACK……），OLED 内容保持不变，但代码继续执行，没有任何日志、没有任何报错、没有任何重试。

### 5.2 `err |= expr` 累积模式

```c
// 修复后
void oled_refresh(void) {
    int err = 0;
    err |= oled_cmd(0x21); err |= oled_cmd(0x00); err |= oled_cmd(0x7F);
    err |= oled_cmd(0x22); err |= oled_cmd(0x00); err |= oled_cmd(0x07);
    err |= lgw_i2c_write_buf(OLED_I2C_ADDR, tx_data, sizeof tx_data);
    if (err != 0) {
        static uint32_t cnt = 0;
        if (++cnt <= 10)
            printf("[OLED] ERR: refresh failed (#%lu)\r\n", (unsigned long)cnt);
    }
}
```

`err |= f()` 的含义：

- `f()` 返回 0（成功）：`err |= 0` → err 不变
- `f()` 返回非 0（失败）：`err |= nonzero` → err 被置为非 0，之后永远不会变回 0

**只要任何一步失败，最终 `err != 0`**，无需每行写 `if` 检查。这是嵌入式代码中常用的错误累积模式（error accumulation pattern）。

### 5.3 为什么限制为 10 次打印

```c
if (++cnt <= 10)  // 只打印前 10 次
```

若 I2C 永久故障，`oled_refresh()` 每 5 秒调用一次，不限制会无限刷屏。限制 10 次后，即使故障持续，串口也只会看到 10 条错误，不影响正常日志阅读。
