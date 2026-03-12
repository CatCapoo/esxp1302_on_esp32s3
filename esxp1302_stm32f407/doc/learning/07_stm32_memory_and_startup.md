# STM32 内存架构与上电启动行为

> **所属项目**：ESXP1302 STM32F407 移植  
> **触发场景**：CCMRAM 优化（SRAM 95% → 74%）  
> **关联实现**：[impl/09_ccmram_optimization.md](../impl/09_ccmram_optimization.md)

---

## 目录

1. [STM32F407 内存地址空间](#1-stm32f407-内存地址空间)
2. [CCMRAM 的特殊性](#2-ccmram-的特殊性)
3. [C 程序的内存段分类](#3-c-程序的内存段分类)
4. [上电后、`main()` 之前发生了什么](#4-上电后-main-之前发生了什么)
5. [CCMRAM 段的特殊初始化需求](#5-ccmram-段的特殊初始化需求)
6. [链接脚本：LMA 与 VMA](#6-链接脚本lma-与-vma)
7. [GCC `__attribute__((section(...)))`](#7-gcc-__attribute__section)
8. [FreeRTOS 堆与内存区域的关系](#8-freertos-堆与内存区域的关系)
9. [快速决策指南：变量放哪里？](#9-快速决策指南变量放哪里)

---

## 1. STM32F407 内存地址空间

ARM Cortex-M4 采用统一地址空间（32 位，4 GB）。STM32F407ZGTx 的实际内存映射：

```
地址范围                   大小    名称       总线
────────────────────────── ─────── ────────── ──────────────────────────────
0x0800_0000 ~ 0x080F_FFFF  1 MB    Flash      AHB (I-Code / D-Code)
0x1000_0000 ~ 0x1000_FFFF  64 KB   CCMRAM     CPU D-Bus（仅 CPU，DMA 不可达）
0x2000_0000 ~ 0x2001_FFFF  128 KB  SRAM1      AHB（CPU + DMA 均可访问）
0x2002_0000 ~ 0x2002_FFFF  64 KB   SRAM2      AHB（CPU + DMA 均可访问）
```

> **STM32F407ZGTx 具体配置**：SRAM1 + SRAM2 = 192 KB，但实际可用作连续 SRAM 的是 128 KB（另 64 KB 细节参见参考手册 §2.3）。  
> 本项目链接脚本只定义了 128 KB RAM，因此 `%age Used` 以 128 KB 为基准计算。

---

## 2. CCMRAM 的特殊性

### 2.1 连接方式

SRAM 通过 **AHB 矩阵**连接，CPU 和 DMA 控制器都挂在 AHB 上，都能访问：

```
CPU ───┐
       ├──► AHB 矩阵 ──► SRAM1/SRAM2
DMA ───┘
```

CCMRAM 通过专用的 **D-Bus** 直连 CPU，完全绕开 AHB 矩阵：

```
CPU (Cortex-M4 Core)
 ├── I-Code Bus ──► Flash（取指）
 ├── D-Code Bus ──► Flash（常量/字面量）
 └── System Bus / D-Bus ──► CCMRAM（数据读写）

DMA ──── AHB 矩阵 ──► SRAM ← DMA 只能到这里
                     ✗ DMA 无法越过 AHB 矩阵访问 CCMRAM
```

### 2.2 性能优势：零等待周期

普通 SRAM 经过 AHB 矩阵，存在总线仲裁延迟（尤其是 DMA 和 CPU 竞争时）。
CCMRAM 专属 D-Bus，**零等待周期**，适合存放高频访问的数据结构（如队列、状态机）。

### 2.3 限制汇总

| 特性 | SRAM | CCMRAM |
|------|------|--------|
| CPU 可读写 | ✅ | ✅ |
| DMA 可访问 | ✅ | ❌ |
| 访问速度 | AHB 总线速度，可能存在等待 | D-Bus 直连，0 等待 |
| 零散初始化（.bss） | 由启动代码自动完成 | **需手动清零** |
| 非零初始化（.data） | 由启动代码从 Flash 复制 | **需手动复制**（复杂） |

---

## 3. C 程序的内存段分类

GCC 将 C 程序分配到以下段（section）：

```
┌─────────────────────────────────────────────────────────────────────┐
│ Flash（只读）                                                        │
│  .text     — 函数机器码                                              │
│  .rodata   — const 全局变量、字符串字面量                            │
│  .data_lma — .data 段的初始值镜像（启动时被复制到 SRAM）             │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│ SRAM（运行时可读写）                                                 │
│  .data     — 有初始值的全局/静态变量（值从 Flash 镜像复制）          │
│  .bss      — 无初始值或零初始化的全局/静态变量（启动时清零）         │
│  .heap     — malloc/FreeRTOS pvPortMalloc 动态分配区                 │
│  .stack    — 中断/主线程调用栈（向低地址增长）                       │
└─────────────────────────────────────────────────────────────────────┘
```

### 3.1 区分 `.data` 与 `.bss` 的方式

| 声明方式 | 段 | 说明 |
|----------|----|------|
| `int x;` | `.bss` | 无初始化，C 标准保证为 0 |
| `int x = 0;` | `.bss` | 显式 0 初始化，编译器通常优化为 `.bss` |
| `int x = 5;` | `.data` | 非零初始值，必须从 Flash 复制 |
| `static struct S s;` | `.bss` | 复合类型，零初始化 |
| `static struct S s = { .a = 1 };` | `.data` | 含非零字段，成为 `.data` |

---

## 4. 上电后、`main()` 之前发生了什么

MCU 上电或复位后，硬件跳转到**中断向量表**中的 Reset Handler：

```
上电/复位
  │
  ▼
Reset_Handler (startup_stm32f407xx.s)
  │
  ├── 1. 拷贝 .data 段：Flash LMA → SRAM VMA
  │       for (p = &_sdata; p < &_edata; p++, src++)
  │           *p = *src;
  │
  ├── 2. 清零 .bss 段：SRAM VMA → 全 0
  │       for (p = &_sbss; p < &_ebss; p++)
  │           *p = 0;
  │
  ├── 3. 调用 SystemInit()
  │       配置时钟树（HSE → PLL → SYSCLK 168 MHz 等）
  │
  ├── 4. 调用 __libc_init_array()
  │       执行 C++ 静态构造函数（纯 C 项目此步为空）
  │
  └── 5. 调用 main()
```

关键点：**步骤 1 只针对 SRAM 上的 `.data` 段**，**步骤 2 只针对 SRAM 上的 `.bss` 段**。
CCMRAM 完全不在这个流程之内。

### 4.1 `startup_stm32f407xx.s` 关键汇编（简化注释版）

```asm
Reset_Handler:
    /* 拷贝 .data 初始值 Flash → SRAM */
    ldr   r0, =_sdata        /* SRAM 目标起始地址 */
    ldr   r1, =_edata        /* SRAM 目标结束地址 */
    ldr   r2, =_sidata       /* Flash 源地址（LMA） */
LoopCopyDataInit:
    cmp   r0, r1
    ittt  lt
    ldrlt r3, [r2], #4
    strlt r3, [r0], #4
    blt   LoopCopyDataInit

    /* 清零 .bss 段 */
    ldr   r2, =_sbss
    ldr   r4, =_ebss
    mov   r3, #0
LoopFillZerobss:
    cmp   r2, r4
    itt   lt
    strlt r3, [r2], #4
    blt   LoopFillZerobss

    bl    SystemInit
    bl    __libc_init_array
    bl    main
```

---

## 5. CCMRAM 段的特殊初始化需求

### 5.1 为什么 `.ccmram` 放 `.bss` 类型变量需要手动清零

`.ccmram` 变量（零初始化）在链接脚本中以 `(NOLOAD)` 标记，告知加载器不从 Flash 加载数据。
Flash 中不存在该段的镜像，启动代码也没有清零它的逻辑。

MCU 上电后 CCMRAM 内容为**随机值**（掉电后内容不确定），如果不在 `main()` 前清零，程序可能读到垃圾数据。

### 5.2 本项目的解法：在 `main.c` USER CODE BEGIN 1 中手动清零

```c
/* 声明链接脚本中定义的符号 */
/* USER CODE BEGIN ETM */
extern uint32_t _sccmram;   /* .ccmram 段起始地址 */
extern uint32_t _eccmram;   /* .ccmram 段结束地址 */
/* USER CODE END ETM */

int main(void)
{
  /* USER CODE BEGIN 1 */
  /* 在任何 C 代码访问 CCMRAM 变量之前，手动清零整个 .ccmram 段 */
  {
    uint32_t *p = &_sccmram;
    while (p < &_eccmram) { *p++ = 0; }
  }
  /* USER CODE END 1 */
  HAL_Init();
  ...
```

**选择 `USER CODE BEGIN 1` 的理由**：
- 这是 `main()` 内最早可以写代码的位置
- 在 `HAL_Init()` 之前，不依赖任何 HAL 初始化
- CubeMX 重新生成代码时，**`USER CODE BEGIN/END` 之间的内容会被保留**

### 5.3 为什么不在 `startup_stm32f407xx.s` 中加？

`startup_stm32f407xx.s` 是 CubeMX 管理的文件。每次点击 CubeMX 的
"Generate Code"，该文件会被完全覆盖，手动添加的 CCMRAM 清零代码会丢失。
除非项目确定永不再使用 CubeMX，否则应避免修改此文件。

### 5.4 `.data` 类型变量能放 CCMRAM 吗？

理论上可以，但需要在启动代码（或 `main()` 早期）手动添加一段 Flash → CCMRAM 的复制循环：

```c
extern uint32_t _siccmram;  /* Flash 中 CCMRAM 数据的 LMA 起始 */
extern uint32_t _sccmram;   /* CCMRAM VMA 起始 */
extern uint32_t _eccmram;   /* CCMRAM VMA 结束 */

uint32_t *dst = &_sccmram;
uint32_t *src = &_siccmram;
while (dst < &_eccmram) { *dst++ = *src++; }
```

本项目中，唯一的 `.data` 候选变量 `lgw_context` 选择了**不迁移**，
原因是其含有很多字符串指针和枚举值，增加启动复制逻辑的维护成本远超节省的 5 KB。

---

## 6. 链接脚本：LMA 与 VMA

### 6.1 基本概念

| 术语 | 全称 | 含义 |
|------|------|------|
| LMA | Load Memory Address | **存储地址**：数据在 Flash（或其他只读存储）中的物理位置 |
| VMA | Virtual Memory Address | **运行地址**：CPU 在运行时通过此地址访问数据 |

对于 `.text` 和 `.rodata`，LMA == VMA（一直在 Flash 中）。  
对于 `.data`，LMA（Flash 镜像）≠ VMA（SRAM 运行地址）。  
对于 `.bss` 和 `.ccmram (NOLOAD)`，只有 VMA，无 LMA（Flash 中无镜像）。

### 6.2 本项目链接脚本（`STM32F407ZGTx_FLASH_cmake.ld`）中的 `.ccmram` 段

```ld
/* 内存区域定义 */
MEMORY
{
  CCMRAM  (xrw) : ORIGIN = 0x10000000, LENGTH = 64K
  RAM     (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
  FLASH   (rx)  : ORIGIN = 0x08000000, LENGTH = 896K
}

/* .ccmram 输出段 */
.ccmram (NOLOAD) :
{
  . = ALIGN(4);
  _sccmram = .;          /* 段起始符号 */
  *(.ccmram)
  *(.ccmram*)
  . = ALIGN(4);
  _eccmram = .;          /* 段结束符号 */
} >CCMRAM               /* 放在 CCMRAM 区域 */
```

`(NOLOAD)` 指令告知加载器（以及 `objcopy`/烧录工具）：
不要在 .elf 或 .bin 中为该段生成初始化数据，Flash 中不为其预留空间。

---

## 7. GCC `__attribute__((section(...)))`

### 7.1 语法

```c
type variable_name __attribute__((section("section_name")));
```

或在 `static` 修饰符之后：

```c
static type variable_name __attribute__((section("section_name")));
```

GCC 扩展语法，用于将变量或函数放置到指定链接段中。

### 7.2 常见用法

```c
/* 将变量放到 CCMRAM */
static uint8_t fast_buf[1024] __attribute__((section(".ccmram")));

/* 将函数放到 RAM 中执行（RAM 函数，用于擦写 Flash 时不中断执行） */
void __attribute__((section(".RamFunc"))) erase_flash(void) { ... }

/* 将变量放到不初始化段（`.noinit`），复位后保留值 */
uint32_t __attribute__((section(".noinit"))) reset_count;

/* 强制 4 字节对齐 */
uint8_t __attribute__((aligned(4), section(".ccmram"))) aligned_buf[256];
```

### 7.3 对 `.data` / `.bss` 分类的影响

`__attribute__((section(...)))` **只改变变量的放置位置**，不改变其初始化语义。

```c
/* 仍是 .data 语义（有初始值），但放在 ".ccmram" 段 */
/* 危险：启动代码不会把 Flash 中的初始值复制到 CCMRAM */
static int x __attribute__((section(".ccmram"))) = 42;   // ← 运行时 x == 0（非 42）！

/* 安全：零初始化，手动清零后即可正常使用 */
static int y __attribute__((section(".ccmram")));         // ← 运行时 y == 0（正确）
```

### 7.4 多属性组合

```c
static struct BigStruct s
    __attribute__((section(".ccmram"), aligned(8)));
```

---

## 8. FreeRTOS 堆与内存区域的关系

FreeRTOS 使用 `heap_4.c` 算法，堆的实际存储是一个静态数组：

```c
/* FreeRTOS/Source/portable/MemMang/heap_4.c */
static uint8_t ucHeap[configTOTAL_HEAP_SIZE];
```

`ucHeap` 是 `.bss` 段变量，默认在 SRAM 中。

**不能将 `ucHeap` 放入 CCMRAM 的原因**：

FreeRTOS 任务（如网络驱动、以太网中断处理）可能从 `pvPortMalloc` 分配的内存中进行 DMA 操作，
或者将堆上的数据地址直接传给 DMA 控制器。若堆在 CCMRAM，DMA 将无法访问这些地址，产生数据损坏
或 HardFault。

> **原则**：凡是可能被 DMA 间接访问（通过指针传递）的内存区域，不要放 CCMRAM。
> FreeRTOS 堆是一个典型的"动态分配通用池"，无法保证其上的数据不被 DMA 访问。

---

## 9. 快速决策指南：变量放哪里？

```
变量是否有非零初始值？
├─ 是（.data）─── 放 SRAM（默认）
│                 （若必须放 CCMRAM，需手动添加 Flash→CCMRAM 复制代码）
│
└─ 否（.bss）──── 是否被 DMA 直接或间接访问？
                  ├─ 是 ──── 放 SRAM（默认）
                  │          例：DMA 目标缓冲区、通过 DMA 传递指针的结构体
                  │
                  └─ 否 ─── 可以放 CCMRAM ✅
                             加 __attribute__((section(".ccmram")))
                             确保 main() 早期清零了 _sccmram ~ _eccmram
```
