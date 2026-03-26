# 12 — OLED 启动画面：上电即显示加载进度，消除黑屏

> **所属项目**：ESXP1302 STM32F407 移植  
> **实施时间**：2026-03-26  
> **关联文档**：[impl/08_oled_display_bug.md](08_oled_display_bug.md)  
> **状态**：✅ 已实施，编译通过（RAM 74.10%，FLASH 35.80%）

---

## 目录

1. [问题描述](#1-问题描述)
2. [解决思路](#2-解决思路)
3. [新增辅助函数](#3-新增辅助函数)
4. [启动阶段 OLED 完整流程](#4-启动阶段-oled-完整流程)
5. [运行时 OLED 完整布局](#5-运行时-oled-完整布局)
6. [运行时刷新逻辑](#6-运行时刷新逻辑)
7. [代码变更摘要](#7-代码变更摘要)

---

## 1. 问题描述

网关上电后，`pkt_fwd_main()` 执行了多个耗时的初始化步骤（Flash 读取、JSON 解析、W5500 网络初始化、SX1302 启动），整个过程大约需要数秒。  
原实现中 `oled_init()` 被放置在互斥锁创建之后、所有初始化完成之后才绘制启动画面，导致这段时间内 OLED **保持黑屏**，用户无法判断设备是否正常启动。

---

## 2. 解决思路

将 `oled_init()` **提前**到所有耗时初始化步骤之前执行（仅在互斥锁创建之后），并在每个关键初始化节点调用 `oled_boot_progress()` 更新进度，共分 8 步：

```
Step 1/8  Power-on init          ← oled_init() 成功后立即显示
Step 2/8  Load flash config      ← uart_cli_init() / config_load() 前
Step 3/8  Load region profile    ← 区域 JSON 选择前
Step 4/8  Parse JSON config      ← parse_SX130x_configuration() 前
Step 5/8  Init W5500 network     ← net_init() 前
Step 6/8  Start concentrator     ← lgw_reset() / lgw_start() 前
Step 7/8  Create RTOS tasks      ← xTaskCreate() 前
Step 8/8  Gateway ready          ← 所有任务创建完毕，即将切换到运行时界面
```

初始化全部完成后，切换到**运行时固定界面**，此后按 `TIME_REFRESH`（5 秒）周期刷新动态内容。

`oled_ok` 标志位保护所有 OLED 调用：若 `oled_init()` 失败，所有后续 OLED 操作均跳过，不影响网关核心功能。

---

## 3. 新增辅助函数

所有辅助函数均定义在 `packet_forwarder/lora_pkt_fwd.c` 中，位于现有 `oled_show_one_line()` 之后。

### 3.1 `oled_draw_one_line_norefresh()`

```c
static void oled_draw_one_line_norefresh(uint8_t col, uint8_t row, const char *str)
```

功能：将字符串写入帧缓冲，**不触发 `oled_refresh()`**。  
用途：批量更新多行后统一刷新，避免多次 I2C 传输造成的画面撕裂或闪烁。

实现要点：
- 将 `str` 拷贝到 `N_CHAR_A_ROW`（21）字符宽度的缓冲区，不足处补空格，超出截断。
- 直接调用 `oled_draw_string(col, row, buf)`，**不调用** `oled_refresh()`。

### 3.2 `oled_boot_progress()`

```c
static void oled_boot_progress(uint8_t step, uint8_t total, const char *msg)
```

功能：在 OLED 上显示标准化的启动进度页，一次 `oled_refresh()` 完成全部更新。

显示布局（启动阶段，128×64，8 行）：

```
Row 0: STM32 LORA GATEWAY
Row 1: Loading x/y
Row 2: <当前步骤描述>
Row 3: Please wait...
Row 4: (空)
Row 5: (空)
Row 6: (空)
Row 7: (空)
```

调用顺序：
1. `oled_draw_one_line_norefresh(0, 0, "STM32 LORA GATEWAY")` — 固定标题
2. `oled_draw_one_line_norefresh(0, 1, "Loading x/y")` — 进度计数
3. `oled_draw_one_line_norefresh(0, 2, msg)` — 当前阶段说明
4. `oled_draw_one_line_norefresh(0, 3, "Please wait...")` — 提示行
5. `oled_refresh()` — 一次性推送到 SSD1306

---

## 4. 启动阶段 OLED 完整流程

```
pkt_fwd_main()
│
├── xSemaphoreCreateMutex() × 5          [互斥锁创建，无 OLED]
│
├── oled_init()                          ← OLED 初始化点（最早可能时机）
│     ├── 成功 → oled_ok = true
│     │         oled_boot_progress(1/8, "Power-on init")
│     └── 失败 → oled_ok = false，后续所有 OLED 调用跳过
│
├── uart_cli_init()
├── oled_boot_progress(2/8, "Load flash config")   [if oled_ok]
├── config_load()
│
├── json_set_allocation_functions(...)
│
├── 区域 JSON 选择
│     oled_boot_progress(3/8, "Load region profile")   [if oled_ok]
│
├── oled_boot_progress(4/8, "Parse JSON config")   [if oled_ok]
├── parse_SX130x_configuration()
├── parse_gateway_configuration()
├── parse_debug_configuration()
│
├── Flash 配置覆盖 (EUI, NS IP/port)
│
├── oled_boot_progress(5/8, "Init W5500 network")   [if oled_ok]
├── net_init()
├── net_udp_open() × 2
├── net_set_dest() × 2
├── sntp_task_start()
│
├── oled_boot_progress(6/8, "Start concentrator")   [if oled_ok]
├── lgw_reset()
├── lgw_start()
├── jit_queue_init() × 2
│
├── oled_boot_progress(7/8, "Create RTOS tasks")   [if oled_ok]
├── xTaskCreate(thread_up)
├── xTaskCreate(thread_down)
├── xTaskCreate(thread_jit)
├── xTaskCreate(thread_cli)
│
├── oled_boot_progress(8/8, "Gateway ready")   [if oled_ok]
│
└── 切换到运行时界面（见第 5 节）
```

---

## 5. 运行时 OLED 完整布局

初始化完成后，绘制固定的运行时界面并刷新。布局如下（每格 21 字符宽）：

```
┌─────────────────────┐
│Row 0: STM32 LORA GATEWAY  │  固定标题，oled_draw_string，随其他行一起刷新
│Row 1: EUI:AABBCCDD 11223344│  网关 EUI（高/低 32 位），oled_draw_string
│Row 2: IP:192.168.71.110   │  本机 eth_ip，oled_draw_string
│Row 3: Concentrator OK     │  射频集中器状态，oled_draw_string
│Row 4: NS_IP=192.168.71.1  │  NS 服务器 IP，oled_show_one_line（含 refresh）
│Row 5: NS_PORT=1700        │  NS 上行端口，oled_show_one_line（含 refresh）
│Row 6: 2026-03-26 08:00 Z  │  UTC 时间 / 系统 Uptime，每 5 秒刷新
│Row 7: Temp=26.1C          │  SX1302 温度，每 stat_interval 秒刷新
└─────────────────────┘
```

绘制顺序（`oled_boot_progress(8/8)` 之后）：

| 步骤 | 函数 | 行 | 内容 | 是否立即刷新 |
|------|------|----|------|------------|
| 1 | `oled_draw_string` | 0 | `"STM32 LORA GATEWAY  "` | 否 |
| 2 | `oled_draw_string` | 1 | `"EUI:AABBCCDD 11223344"` | 否 |
| 3 | `oled_draw_string` | 2 | `"IP:x.x.x.x"` | 否 |
| 4 | `oled_draw_string` | 3 | `"Concentrator OK "` | 否 |
| 5 | `oled_show_one_line` | 4 | `"NS_IP=x.x.x.x"` | **是** |
| 6 | `oled_show_one_line` | 5 | `"NS_PORT=1700"` | **是** |
| 7 | `oled_show_one_line` | 6 | 初始 Uptime | **是** |

> Row 7（温度）在主统计循环（每 `stat_interval` 秒）读取 SX1302 温度后写入。

---

## 6. 运行时刷新逻辑

主统计循环（`while (!exit_sig && !quit_sig)`）内有两级刷新：

### 6.1 TIME_REFRESH 级（每 5 秒）

```c
while (time_count < stat_interval) {
    vTaskDelay(pdMS_TO_TICKS(1000 * TIME_REFRESH));
    time_count += TIME_REFRESH;

    // Row 6 刷新
    t = time(NULL);
    if (t > 1700000000) {
        // SNTP 已同步 → 显示 UTC 时间 "YYYY-MM-DD HH:MM:SS Z"
        strftime(stat_timestamp, ..., "%Y-%m-%d %H:%M:%S Z", gmtime_r(...));
    } else {
        // 未同步 → 显示系统运行时间 "Up HH:MM:SS"
        get_uptime_sec() → snprintf("Up %02lu:%02lu:%02lu", ...);
    }
    oled_show_one_line(0, 6, stat_timestamp);
}
```

`gmtime_r()` 替代非线程安全的 `gmtime()`，避免与 SNTP 后台任务竞争。

### 6.2 stat_interval 级（默认 30 秒）

```c
// Row 7 刷新（温度）
lgw_get_temperature(&temperature);
snprintf(out_info, 22, "Temp=%.1fC", temperature);
oled_show_one_line(0, 7, out_info);
```

### 6.3 刷新时序总结

```
上电
 │
 ├─ oled_init()
 ├─ boot progress 1~8（逐步显示加载阶段）
 ├─ 运行时界面初始绘制（Row 0-6）
 │
 └─ 主循环
       │
       ├─ 每 5 s: Row 6 ← UTC 时间 / Uptime
       │
       └─ 每 30 s: Row 7 ← 温度
```

---

## 7. 代码变更摘要

**修改文件**：`packet_forwarder/lora_pkt_fwd.c`

| 类型 | 位置 | 说明 |
|------|------|------|
| 新增函数 | `oled_draw_one_line_norefresh()` | 写帧缓冲不刷新，供批量更新使用 |
| 新增函数 | `oled_boot_progress()` | 标准化启动进度页，4 行内容一次刷新 |
| 新增变量 | `pkt_fwd_main()` 局部 `bool oled_ok` | OLED 初始化成功标志 |
| 新增变量 | `pkt_fwd_main()` 局部 `const uint8_t boot_total_steps = 8` | 总步数常量 |
| 移动位置 | `oled_init()` | 从互斥锁创建后立即调用（原位置在后） |
| 新增调用 | 8 处 `oled_boot_progress()` | 分布于各初始化节点 |

内存影响（编译结果）：

```
RAM:    97128 B / 128 KB  (74.10%)   无变化
CCMRAM: 27800 B /  64 KB  (42.42%)   无变化
FLASH: 328464 B / 896 KB  (35.80%)   无变化（新增代码在误差范围内）
```
