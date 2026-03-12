# SNTP 实现剖析

> **所属项目**：ESXP1302 STM32F407 移植  
> **概念背景**：[learning/05_ntp_sntp_concepts.md](../learning/05_ntp_sntp_concepts.md)  
> **配置参考**：[impl/07_sntp_configuration.md](07_sntp_configuration.md)

---

## 目录

1. [文件清单与职责](#1-文件清单与职责)
2. [软件时钟状态机](#2-软件时钟状态机)
3. [API 速查](#3-api-速查)
4. [sntp_sync() 单次同步流程](#4-sntp_sync-单次同步流程)
5. [sntp_background_task() 5 级回退机制](#5-sntp_background_task-5-级回退机制)
6. [sntp_task_start() 启动入口](#6-sntp_task_start-启动入口)
7. [_gettimeofday() Newlib 桥接](#7-_gettimeofday-newlib-桥接)
8. [完整调用链图](#8-完整调用链图)
9. [时间在 pkt_fwd 中的使用场景](#9-时间在-pkt_fwd-中的使用场景)
10. [已知诊断输出说明](#10-已知诊断输出说明)

---

## 1. 文件清单与职责

| 文件 | 路径 | 职责 |
|------|------|------|
| `sntp_client.h` | `packet_forwarder/sntp_client.h` | API 声明、常量定义 |
| `sntp_client.c` | `packet_forwarder/sntp_client.c` | 核心实现：单次查询、后台任务、软件时钟 |
| `sntp_gettimeofday.c` | `Core/Src/sntp_gettimeofday.c` | Newlib `_gettimeofday()` 强符号覆盖 |

> **注意**：`sntp_gettimeofday.c` 放在 `Core/Src/` 而非 `packet_forwarder/`，是因为它属于 Newlib 系统调用层，与 CubeMX 生成的 `syscalls.c` 在同一目录，便于维护。

---

## 2. 软件时钟状态机

### 2.1 状态变量

```c
// sntp_client.c (文件作用域静态变量)
static volatile time_t   s_epoch_at_sync = 0;   // NTP 同步成功时的 Unix 时间（秒）
static volatile uint32_t s_tick_at_sync  = 0;   // 同步成功时的 HAL_GetTick()（毫秒）
static volatile bool     s_synced        = false; // 是否已同步（至少一次）
```

### 2.2 当前时间计算

```c
time_t sntp_get_utc(void) {
    if (!s_synced) return 0;
    uint32_t elapsed_ms = HAL_GetTick() - s_tick_at_sync;
    return s_epoch_at_sync + (time_t)(elapsed_ms / 1000);
}
```

数学表达：

$$t_{\text{now}} = t_{\text{sync}} + \left\lfloor \frac{\text{HAL\_GetTick}() - \text{tick\_sync}}{1000} \right\rfloor$$

### 2.3 状态转换图

```
          上电
            │
            ▼
    ┌───────────────┐
    │  未同步状态    │  sntp_get_utc() = 0
    │  s_synced=F   │  time(NULL) = 0
    └───────┬───────┘  OLED 显示 "Up HH:MM:SS"
            │ sntp_sync() 成功
            │ 写入 s_epoch_at_sync
            │        s_tick_at_sync
            │        s_synced = true
            ▼
    ┌───────────────┐
    │  已同步状态    │  sntp_get_utc() = 真实 UTC
    │  s_synced=T   │  time(NULL) = 真实 UTC
    └───────┬───────┘  OLED 显示 "2026-03-12 10:23:45 Z"
            │
            │ 每 SNTP_RESYNC_INTERVAL (3600s) 重新调用 sntp_sync()
            │ 刷新 s_epoch_at_sync 和 s_tick_at_sync
            └──────────────────────────────────► （保持已同步状态）
```

---

## 3. API 速查

```c
// sntp_client.h

// 启动后台同步任务（调用一次，非阻塞）
void sntp_task_start(const uint8_t *gw_ip,   // 网关 IP（4字节）
                     const uint8_t *eth_ip,  // 本机 IP（4字节）
                     const uint8_t *ns_ip);  // NS IP（4字节）

// 单次 SNTP 查询（阻塞，最长 NTP_TIMEOUT_MS=1500ms）
int sntp_sync(const uint8_t *ntp_ip);  // NULL → 使用 NTP_SERVER_IP_DEFAULT

// 读取当前 UTC 时间（不阻塞）
time_t sntp_get_utc(void);   // 返回 0 表示未同步

// 查询同步状态
bool sntp_is_synced(void);

// 查询上次同步距今秒数
uint32_t sntp_age(void);     // 返回 UINT32_MAX 表示未同步
```

---

## 4. sntp_sync() 单次同步流程

### 4.1 流程图

```
sntp_sync(ntp_ip)
    │
    ├─ [1] 打开 W5500 UDP socket 2 (NTP_LOCAL_PORT=10123)
    │       socket(NET_SOCK_NTP=2, Sn_MR_UDP, 10123, 0)
    │       失败 → return -1
    │
    ├─ [2] 构造 NTP 请求包 (48字节全0, 仅 pkt[0]=0x23)
    │       0x23 = LI=0, VN=4, Mode=3(client)
    │
    ├─ [3] sendto(NET_SOCK_NTP, pkt, 48, ntp_ip, 123)
    │       失败 SOCKERR_TIMEOUT=-13 → 打印 ARP 超时警告
    │       其他失败 → return -1
    │
    ├─ [4] 等待响应 (最长 NTP_TIMEOUT_MS=1500ms)
    │       每 10ms 检查一次 getSn_RX_RSR() ≥ 48
    │       超时 → 打印警告, return -1
    │
    ├─ [5] recvfrom() 读取 48 字节
    │
    ├─ [6] 解析 Transmit Timestamp (bytes 40-43, big-endian)
    │       ntp_sec = pkt[40]<<24 | pkt[41]<<16 | pkt[42]<<8 | pkt[43]
    │       校验: ntp_sec < NTP_UNIX_OFFSET → 无效, return -1
    │
    ├─ [7] 转为 Unix 时间
    │       unix_time = ntp_sec - NTP_UNIX_OFFSET (2208988800)
    │
    ├─ [8] 更新软件时钟（写入顺序严格保证并发安全）
    │       s_epoch_at_sync = unix_time   ← 先写时间
    │       s_tick_at_sync  = HAL_GetTick() ← 再写 tick
    │       s_synced        = true        ← 最后置位
    │
    └─ return 0 (成功)
```

### 4.2 关键代码片段

```c
// ① 构造请求包，仅需设置第一字节
memset(ntp_pkt, 0, NTP_PACKET_SIZE);
ntp_pkt[0] = 0x23;   // LI=00, VN=100(4), Mode=011(client)

// ② 发送
ret = sendto(NET_SOCK_NTP, ntp_pkt, NTP_PACKET_SIZE,
             (uint8_t *)ntp_ip, NTP_PORT);

// ③ 轮询接收
uint32_t t_start = HAL_GetTick();
while ((HAL_GetTick() - t_start) < NTP_TIMEOUT_MS) {
    if (getSn_RX_RSR(NET_SOCK_NTP) >= NTP_PACKET_SIZE) {
        ret = recvfrom(NET_SOCK_NTP, ntp_pkt, NTP_PACKET_SIZE,
                       peer_ip, &peer_port);
        if (ret >= NTP_PACKET_SIZE) goto parse_ok;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
}

// ④ 解析（parse_ok:）
uint32_t ntp_sec = ((uint32_t)ntp_pkt[40] << 24) |
                   ((uint32_t)ntp_pkt[41] << 16) |
                   ((uint32_t)ntp_pkt[42] <<  8) |
                   ((uint32_t)ntp_pkt[43]);

// ⑤ 写入软件时钟
time_t unix_time  = (time_t)(ntp_sec - NTP_UNIX_OFFSET);
uint32_t tick_now = HAL_GetTick();
s_epoch_at_sync = unix_time;
s_tick_at_sync  = tick_now;
s_synced        = true;
```

---

## 5. sntp_background_task() 5 级回退机制

### 5.1 候选服务器选择逻辑

| 级别 | 候选 IP | 来源 | 说明 |
|------|---------|------|------|
| 1 | `cfg->eth_gw` | Flash 配置 | 用户配置的网关（最快，同 LAN ARP） |
| 2 | `eth_ip[0..2].1` | 自动推导 | 从本机 IP 推导 `.1`，处理网关配 IP 写错的情况 |
| 3 | `eth_ip[0..2].254` | 自动推导 | 部分路由器用 `.254` 作为默认地址 |
| 4 | `ns_ip` | 来自 pkt_fwd 配置 | NS 在同 LAN，ARP 可达；NS 主机通常运行 chrony/ntpd |
| 5 | `120.25.115.20` | 硬编码公网 | 阿里云 NTP，需要路由器能上网 |

> **级别去重**：推导的 `.1` 地址若与 `eth_gw` 相同则跳过；`.254` 同理。避免对同一服务器发两次请求。

### 5.2 任务流程

```
sntp_background_task()
    │
    ├─ vTaskDelay(5000ms)  ← 等 pkt_fwd 主任务发出第一个 UDP 包，预热 ARP
    │
    └─ for(;;)
           │
           ├─ ok = -1
           ├─ 候选1: sntp_sync(eth_gw)      → ok=0 → 跳到 sleep
           ├─ 候选2: sntp_sync(.1)           → ok=0 → 跳到 sleep
           ├─ 候选3: sntp_sync(.254)         → ok=0 → 跳到 sleep
           ├─ 候选4: sntp_sync(ns_ip)        → ok=0 → 跳到 sleep
           ├─ 候选5: sntp_sync(120.25.115.20)→ ok=0 → 跳到 sleep
           │
           ├─ if ok==0:
           │     vTaskDelay(3600s)  ← 成功：1小时后重同步
           └─ else:
                 print "[SNTP] Both NTP sources failed, retry in 60s"
                 vTaskDelay(60s)    ← 失败：60秒后重试
```

### 5.3 FreeRTOS 任务参数

```c
xTaskCreate(sntp_background_task,
            "sntp",           // 任务名
            512,              // 栈大小（字，= 2KB）
            NULL,             // 参数
            tskIDLE_PRIORITY + 1,  // 最低非 IDLE 优先级
            NULL);            // 句柄不保存
```

优先级 `tskIDLE_PRIORITY+1` 确保 SNTP 任务不会抢占数据包转发器（`osPriorityAboveNormal`）。

---

## 6. sntp_task_start() 启动入口

```c
// lora_pkt_fwd.c line 2255
sntp_task_start(cfg->eth_gw, cfg->eth_ip, ns_ip);
```

调用位置：在 `net_init()` 成功、UDP socket 已绑定之后，`lgw_start()` 之前。

`sntp_task_start()` 做三件事：
1. 校验并保存 `gw_ip`、`eth_ip`、`ns_ip` 到模块静态变量
2. 打印启动日志，列出所有候选 IP
3. 调用 `xTaskCreate()` 创建后台任务（非阻塞立即返回）

---

## 7. _gettimeofday() Newlib 桥接

### 7.1 文件：`Core/Src/sntp_gettimeofday.c`

```c
int _gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;
    if (tv) {
        tv->tv_sec  = sntp_get_utc();  // SNTP 软件时钟
        tv->tv_usec = 0;               // 亚秒精度不支持
    }
    return 0;
}
```

### 7.2 链接优先级

```
链接顺序（CMakeLists.txt 中 sntp_gettimeofday.c 在 syscalls.c 前）：

sntp_gettimeofday.o → 提供强符号 _gettimeofday()
syscalls.o          → 提供弱符号 _gettimeofday()（__attribute__((weak))）

链接器规则：强符号 > 弱符号 → 使用 sntp_gettimeofday.o 中的版本
```

### 7.3 调用路径（完整 Newlib 内部路径）

```c
time(NULL)
  └─> __time()               // Newlib libc/time/time.c
        └─> gettimeofday()   // Newlib POSIX wrapper
              └─> _gettimeofday(tv, tz)  // ← 本文件实现（强符号）
                    └─> sntp_get_utc()   // sntp_client.c
                          └─> s_epoch_at_sync + elapsed  // 软件时钟计算
```

---

## 8. 完整调用链图

```
                           ┌─────────────────────────┐
                           │   pkt_fwd_main()         │
                           │   lora_pkt_fwd.c:2255    │
                           └────────────┬────────────┘
                                        │ sntp_task_start(gw,eth,ns)
                                        ▼
                           ┌─────────────────────────┐
                           │   sntp_task_start()      │
                           │   sntp_client.c:241      │
                           │   保存 IP, xTaskCreate() │
                           └────────────┬────────────┘
                                        │ 创建后台任务
                                        ▼
                    ┌──────────────────────────────────────┐
                    │   sntp_background_task()              │
                    │   sntp_client.c:190                   │
                    │   FreeRTOS task, 512 words stack       │
                    │   tskIDLE_PRIORITY+1                  │
                    │                                       │
                    │   vTaskDelay(5000ms)                  │
                    │   loop:                               │
                    │     sntp_sync(gw_ip)  ──────────────►│
                    │     sntp_sync(.1)                     │
                    │     sntp_sync(.254)                   │
                    │     sntp_sync(ns_ip)                  │
                    │     sntp_sync(120.25.115.20)          │
                    │     vTaskDelay(3600s or 60s)          │
                    └──────────────┬───────────────────────┘
                                   │ sntp_sync(ip)
                                   ▼
                    ┌──────────────────────────────────────┐
                    │   sntp_sync()                         │
                    │   sntp_client.c:101                   │
                    │                                       │
                    │   socket(2, UDP, 10123)               │
                    │   sendto(pkt[0]=0x23, ip, 123)        │
                    │   poll getSn_RX_RSR()  (1500ms max)   │
                    │   recvfrom(48 bytes)                  │
                    │   parse bytes[40-43] → ntp_sec        │
                    │   unix = ntp_sec - 2208988800         │
                    │   s_epoch_at_sync = unix  ────────────┤
                    │   s_tick_at_sync  = tick  ────────────┤
                    │   s_synced = true  ───────────────────┤
                    └──────────────────────────────────────┘
                                                            │ 软件时钟已就绪
                                                            │
         ┌──────────────────────────────────────────────────┘
         │ 任意任务调用 time(NULL)
         ▼
┌────────────────────────────────────────────────────────────┐
│   time(NULL)                                                │
│   Newlib libc                                               │
│     └─> gettimeofday()                                      │
│           └─> _gettimeofday(tv, tz)                         │
│                 (sntp_gettimeofday.c:33)                    │
│                   └─> sntp_get_utc()                        │
│                         (sntp_client.c:82)                  │
│                           if (!s_synced) return 0           │
│                           elapsed = HAL_GetTick()-tick_sync │
│                           return epoch + elapsed/1000       │
└────────────────────────────────────────────────────────────┘
```

---

## 9. 时间在 pkt_fwd 中的使用场景

### 9.1 OLED Row 6 时间显示

```c
// lora_pkt_fwd.c — 主循环，每 TIME_REFRESH=5 秒执行一次
time_t t = time(NULL);
bool shown = false;

if (t > 1700000000) {           // 校验：> 2023 年才算有效 UTC
    struct tm tm_buf;
    struct tm *tm_ptr = gmtime_r(&t, &tm_buf);  // 线程安全版本
    if (tm_ptr != NULL &&
        strftime(stat_timestamp, sizeof stat_timestamp,
                 "%Y-%m-%d %H:%M:%S Z", tm_ptr) > 0) {
        shown = true;
    }
}
if (!shown) {
    // SNTP 未同步时退化为运行时长
    uint32_t up = get_uptime_sec();
    snprintf(stat_timestamp, sizeof stat_timestamp, "Up %02lu:%02lu:%02lu",
             up/3600, (up%3600)/60, up%60);
}
oled_show_one_line(0, 6, stat_timestamp);
```

**显示效果对比：**

| SNTP 状态 | OLED Row 6 显示 |
|-----------|----------------|
| 未同步 | `Up 00:00:35` |
| 已同步 | `2026-03-12 10:23:45 Z` |

> `gmtime_r()` 而非 `gmtime()` 的原因：`gmtime()` 返回指向全局静态缓冲区的指针，在 FreeRTOS 多任务环境中（SNTP 后台任务也调用 `gmtime_r()`）存在竞争风险。`gmtime_r()` 将结果写入调用方提供的 `struct tm`，线程安全。

### 9.2 统计报告标题时间戳

```c
// lora_pkt_fwd.c — 统计报告打印（每 DEFAULT_STAT=30 秒）
time_t t = time(NULL);
struct tm tm_buf;
if (t > 1700000000 && gmtime_r(&t, &tm_buf) != NULL &&
    strftime(stat_timestamp, sizeof stat_timestamp,
             "%Y-%m-%d %H:%M:%S UTC", &tm_buf) > 0) {
    printf("\n##### %s #####\n", stat_timestamp);
} else {
    uint32_t up = get_uptime_sec();
    printf("\n##### Uptime %02lu:%02lu:%02lu #####\n", ...);
}
```

### 9.3 stat JSON 时间字段

```c
// lora_pkt_fwd.c — stat JSON 生成
if (t > 1700000000 && gmtime_r(&t, &tm_buf) != NULL &&
    strftime(stat_timestamp, sizeof stat_timestamp,
             "%Y-%m-%d %H:%M:%S GMT", &tm_buf) > 0) {
    // 正常：stat.time = "2026-03-12 10:23:45 GMT"
} else {
    snprintf(stat_timestamp, sizeof stat_timestamp,
             "2000-01-01 %02lu:%02lu:%02lu GMT",
             up/3600, (up%3600)/60, up%60);
    // 降级：stat.time = "2000-01-01 00:00:35 GMT"（ChirpStack 可识别但会告警）
}
```

---

## 10. 已知诊断输出说明

以下 `printf` 是当前代码中残留的临时诊断输出，功能稳定后可移除：

### sntp_client.c 中

```
[SNTP] Querying X.X.X.X ...
```
每次调用 `sntp_sync()` 都打印，用于追踪哪个候选服务器被尝试。

```
[SNTP] DBG W5500 regs: SIPR=...  SUBR=...  GAR=...  Sn_SR[2]=0x22
```
`// --- TEMPORARY DIAGNOSTIC ---` 块，显示 W5500 寄存器状态（确认 IP/GW/子网掩码是否正确写入）。Sn_SR[2]=0x22 表示 UDP socket 已成功打开。

```
[SNTP] WARN: ARP timeout reaching X.X.X.X
```
W5500 无法 ARP 到目标 IP（该 IP 不在同一 LAN 或路由不可达），正常情况（候选服务器不可达时）。

```
[SNTP] WARN: no response from X.X.X.X within 1500 ms
```
目标 IP ARP 成功（包发出去了），但服务器没有在 1500ms 内应答（服务器不运行 NTP，或对应的端口防火墙封闭）。

```
[SNTP] Time synced: 2026-03-12 10:23:45 UTC
```
同步成功，此行之后 `time(NULL)` 返回真实 UTC。

```
[SNTP] Both NTP sources failed, retry in 60s
```
全部 5 个候选均失败，60 秒后重试。
