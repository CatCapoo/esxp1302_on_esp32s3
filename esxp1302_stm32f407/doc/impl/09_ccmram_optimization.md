# 09 — CCMRAM 优化：将大型静态全局变量迁移至核心耦合内存

> **所属项目**：ESXP1302 STM32F407 移植  
> **实施时间**：2026-03-12  
> **关联知识**：[learning/07_stm32_memory_and_startup.md](../learning/07_stm32_memory_and_startup.md)  
> **状态**：✅ 已实施，已验证（网关正常上线，LoRa 收发正常）

---

## 目录

1. [问题背景：链接期 RAM 占用 95%](#1-问题背景链接期-ram-占用-95)
2. [大型静态变量分析](#2-大型静态变量分析)
3. [迁移可行性评估](#3-迁移可行性评估)
4. [具体代码改动](#4-具体代码改动)
5. [踩坑记录](#5-踩坑记录)
6. [构建结果对比](#6-构建结果对比)

---

## 1. 问题背景：链接期 RAM 占用 95%

链接完成后，构建系统输出：

```
Memory region    Used Size  Region Size  %age Used
         RAM:   125196 B    128 KB       95.5%
      CCMRAM:       0 B     64 KB        0.0%
       FLASH:   314744 B    896 KB       34.30%
```

128 KB SRAM 几乎全部消耗在**静态分配**（全局变量 + 静态局部变量 + FreeRTOS 堆）阶段，
任务栈等动态分配完全没有余量。这在实际运行中会触发 FreeRTOS 堆耗尽（`vTaskCreate` 失败）
或栈溢出。

CCMRAM（64 KB）完全空闲，存在明显优化空间。

---

## 2. 大型静态变量分析

通过分析 `.map` 文件（`build/Debug/esxp1302_stm32f407.map`）找出 `RAM` 中占用最大的符号：

| 变量名 | 文件 | 段 | 大小 | 说明 |
|--------|------|----|------|------|
| `ucHeap` | `heap_4.c` | `.bss` | 65,536 B | FreeRTOS 内部堆（`configTOTAL_HEAP_SIZE = 65536`） |
| `jit_queue[2]` | `lora_pkt_fwd.c` | `.bss` | 19,464 B | JIT 发包队列，每条链路 32 个时间槽 × ~304 B |
| `lgw_context` | `loragw_hal.c` | `.data` | 5,196 B | SX1302 HAL 上下文，**含初始值** |
| `debugconf` | `lora_pkt_fwd.c` | `.bss` | 4,228 B | 调试帧过滤配置 |
| `rx_buffer` | `loragw_sx1302.c` | `.bss` | 4,096 B | SX1302 RX DMA 读取缓冲 |

`ucHeap` 必须留在 SRAM（FreeRTOS 内部的堆管理操作需要 DMA 能访问的内存，且移动它需要修改第三方库代码）。

---

## 3. 迁移可行性评估

CCMRAM 有两个关键限制（详见 [learning/07](../learning/07_stm32_memory_and_startup.md)）：

| 限制 | 影响 |
|------|------|
| DMA 控制器无法访问 CCMRAM | 若变量被 DMA 读写，迁移后数据损坏 |
| 启动代码不自动初始化 CCMRAM | `.data` 类型（含非零初始值）变量迁移后初始值丢失 |

### 3.1 DMA 排查

项目 SPI2 驱动（`loragw_spi.c`）调用：

```c
HAL_SPI_Transmit(&hspi2, ...);           // 轮询传输
HAL_SPI_TransmitReceive(&hspi2, ...);    // 轮询收发
```

未调用任何 `HAL_SPI_Transmit_DMA` / `HAL_SPI_Receive_DMA` 系列函数。虽然 `hdma_spi3_rx` / `hdma_spi3_tx`
句柄存在于 BSS，但实际上**从未触发过 DMA 传输**。W5500 以太网也通过 SPI2 轮询模式驱动。

→ 所有候选变量均不经过 DMA 路径，**DMA 限制不阻碍迁移**。

### 3.2 段类型排查（.data vs .bss）

| 变量 | 声明方式 | 段 | 可迁移？ |
|------|----------|----|----------|
| `jit_queue[2]` | `static struct jit_queue_s jit_queue[LGW_RF_CHAIN_NB];` | `.bss` | ✅ |
| `debugconf` | `static struct lgw_conf_debug_s debugconf;` | `.bss` | ✅ |
| `rx_buffer` | `rx_buffer_t rx_buffer;` | `.bss` | ✅ |
| `lgw_context` | `static lgw_context_t lgw_context = { .is_started = false, .board_cfg.com_type = LGW_COM_SPI, ... };` | **`.data`** | ❌（见[踩坑记录](#5-踩坑记录)） |

---

## 4. 具体代码改动

### 4.1 `packet_forwarder/lora_pkt_fwd.c`

迁移 `jit_queue` 和 `debugconf`：

```c
/* 原始代码 */
static struct jit_queue_s jit_queue[LGW_RF_CHAIN_NB];
static struct lgw_conf_debug_s debugconf;

/* 修改后 */
/* Placed in CCMRAM (CPU-only, no DMA): saves ~19 KB in SRAM */
static struct jit_queue_s jit_queue[LGW_RF_CHAIN_NB] __attribute__((section(".ccmram")));
/* Placed in CCMRAM (CPU-only, no DMA): saves ~4 KB in SRAM */
static struct lgw_conf_debug_s debugconf __attribute__((section(".ccmram")));
```

### 4.2 `libloragw/loragw_sx1302.c`

迁移 `rx_buffer`：

```c
/* 原始代码 */
rx_buffer_t rx_buffer;

/* 修改后 */
/* Placed in CCMRAM (SPI is polling mode, no DMA): saves ~4 KB in SRAM */
rx_buffer_t rx_buffer __attribute__((section(".ccmram")));
```

### 4.3 `Core/Src/main.c` — CCMRAM 零填充

`.ccmram` 段变量属于 `.bss` 类型，但启动代码（`startup_stm32f407xx.s`）只清零 SRAM 的
`.bss`，不会初始化 CCMRAM。因此需要在 `main()` 的最早时机（任何访问这些变量之前）手动清零。

将初始化代码放入 `USER CODE BEGIN 1` 块，可以**在 CubeMX 重新生成代码时被保留**：

```c
/* USER CODE BEGIN ETM */
extern uint32_t _sccmram;
extern uint32_t _eccmram;
/* USER CODE END ETM */

int main(void)
{
  /* USER CODE BEGIN 1 */
  /* Zero-fill CCMRAM (.ccmram section) before any C code runs.
   * The startup file only clears .bss (SRAM); CCMRAM must be cleared
   * explicitly. Placed here so it survives CubeMX code regeneration. */
  {
    uint32_t *p = &_sccmram;
    while (p < &_eccmram) { *p++ = 0; }
  }
  /* USER CODE END 1 */
  HAL_Init();
  ...
```

`_sccmram` 和 `_eccmram` 是链接脚本 `STM32F407ZGTx_FLASH_cmake.ld` 中已有的符号：

```ld
/* CCMRAM (Core Coupled Memory) */
.ccmram (NOLOAD) :
{
  . = ALIGN(4);
  _sccmram = .;
  *(.ccmram)
  *(.ccmram*)
  . = ALIGN(4);
  _eccmram = .;
} >CCMRAM
```

> **`(NOLOAD)` 属性**：告知链接器该段不需要从 Flash 加载初始值（与 `.bss` 同理）。
> 这意味着 Flash 中不为该段预留任何 LMA 空间，节省 Flash。

---

## 5. 踩坑记录

### 5.1 `lgw_context` 迁移后运行时报 `ERROR: Failed to configure board`

**现象**：首次将 `lgw_context` 也加上 `__attribute__((section(".ccmram")))` 后，
烧录运行立即报：

```
ERROR: Failed to configure board
ERROR: failed to parse SX130x configuration
```

**根因**：`lgw_context` 是 `.data` 段变量（含非零初始值）：

```c
static lgw_context_t lgw_context = {
    .is_started = false,
    .board_cfg.com_type = LGW_COM_SPI,   // 值为 1，非零
    .board_cfg.com_path = "/dev/spidev0.0",
    ...
};
```

标准启动流程（`startup_stm32f407xx.s`）会将 Flash 中的初始值镜像复制到 SRAM：

```
LMA (Flash) ──copy──► VMA (SRAM)   // 只针对 SRAM 的 .data 段
```

将其放入 `.ccmram` 后，启动代码不知道还需要拷贝到 CCMRAM，导致变量内容全为 0。
`board_cfg.com_type == 0`（COM_UNKNOWN）不通过校验，`lgw_board_setconf()` 失败。

**修复**：将 `lgw_context` 回迁至 SRAM（保留默认放置，不加任何 attribute）。

### 5.2 首次在 `startup_stm32f407xx.s` 中添加 CCMRAM 清零，存在 CubeMX 覆盖风险

**现象**：在汇编启动文件中直接添加了 CCMRAM 清零循环，后来意识到 CubeMX 重新生成
工程代码时会重写 `startup_stm32f407xx.s`，清零代码会丢失。

**修复**：将清零代码迁移到 `main.c` 的 `USER CODE BEGIN 1`（见 §4.3），
把启动文件完全恢复为 CubeMX 生成的原始状态。

---

## 6. 构建结果对比

| 内存区 | 优化前 | 优化后 | 变化 |
|--------|--------|--------|------|
| RAM | 125,196 B (95.5%) | 97,432 B (74.33%) | **−27,764 B** |
| CCMRAM | 0 B (0%) | 27,800 B (42.42%) | +27,800 B |
| Flash | 314,744 B (34.30%) | 314,744 B (34.30%) | 无变化 |

优化效果：SRAM 使用率从 95.5% 降至 74.33%，释放约 27 KB，为任务栈和运行时动态分配保留了充足余量。
