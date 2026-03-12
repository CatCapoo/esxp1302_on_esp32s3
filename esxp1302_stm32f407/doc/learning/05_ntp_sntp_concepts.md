# NTP / SNTP 概念与原理

> **所属项目**：ESXP1302 STM32F407 移植  
> **关联实现**：[impl/07_sntp_implementation.md](../impl/07_sntp_implementation.md)  
> **适用**：理解为什么 LoRa 网关需要 NTP、NTP 协议本身是怎么运作的

---

## 目录

1. [为什么 LoRa 网关需要精确时间](#1-为什么-lora-网关需要精确时间)
2. [NTP 与 SNTP 的区别](#2-ntp-与-sntp-的区别)
3. [时间纪元：Unix Epoch 与 NTP Epoch](#3-时间纪元unix-epoch-与-ntp-epoch)
4. [SNTPv4 协议报文结构](#4-sntpv4-协议报文结构)
5. [NTP 时间戳的解析](#5-ntp-时间戳的解析)
6. [软件时钟模型（Software Clock）](#6-软件时钟模型software-clock)
7. [时钟漂移与重同步策略](#7-时钟漂移与重同步策略)
8. [UDP 与 ARP：局域网 NTP 的特殊性](#8-udp-与-arparp-局域网-ntp-的特殊性)
9. [Newlib 的 time() 调用链](#9-newlib-的-time-调用链)
10. [与 ESP-IDF esp_sntp 的设计对比](#10-与-esp-idf-esp_sntp-的设计对比)

---

## 1. 为什么 LoRa 网关需要精确时间

LoRa 网关本身不需要亚毫秒精度（那是 GPS PPS 的领域）。但有三个功能对时间有强依赖：

### 1.1 Semtech UDP Stat 上报

`PUSH_DATA` 协议的 `stat` JSON 包含 `time` 字段：

```json
"stat": {
  "time": "2026-03-12 10:23:45 GMT",
  "rxnb": 12, "rxok": 10, ...
}
```

ChirpStack / TTN 等 NS 端用这个时间戳做网关健康监控。若时间戳是 `2000-01-01 00:00:05 GMT`（设备上电后 5 秒，RTC 未同步），NS 会将网关标记为"时钟异常"或直接丢弃该 stat。

### 1.2 OLED 时间显示

第 6 行显示 `2026-03-12 10:23:45 Z`，没有 NTP 就只能显示 `Up 00:00:05`（运行时长），信息量大幅下降。

### 1.3 日志可读性

串口日志前缀带 UTC 时间，便于问题定位：

```
##### 2026-03-12 10:23:45 UTC #####
### [UPSTREAM] ###
```

没有 NTP 时退化为：`##### Uptime 00:05:30 #####`

---

## 2. NTP 与 SNTP 的区别

| 项目 | NTP (RFC 5905) | SNTP (RFC 4330) |
|------|---------------|-----------------|
| 复杂度 | 完整实现：多层级、对称模式、环路过滤、相位锁定 | 极简：单次 client-server 查询 |
| 精度 | 局域网 < 1 ms，广域网 ~10 ms | 几十 ms 级（完全够用） |
| 实现代码量 | 数千行（ntpd） | ~100 行 |
| 适用场景 | 精密时间服务器、精密测量 | 嵌入式设备、IoT 终端 |
| 兼容性 | 完全兼容 | 与标准 NTP 服务器完全兼容（协议格式相同） |

**本项目用 SNTPv4**，理由：STM32 RAM/Flash 有限，且时间精度要求只是"秒级正确"，SNTP 完全满足。

---

## 3. 时间纪元：Unix Epoch 与 NTP Epoch

这是理解 NTP 时间戳解析的关键基础知识。

### 3.1 两种纪元

```
NTP  Epoch: 1900-01-01 00:00:00 UTC  (NTP 规范定义)
Unix Epoch: 1970-01-01 00:00:00 UTC  (POSIX/C 标准)
```

两者之差（秒数）：

$$\Delta = (1970 - 1900) \text{ 年} = 70 \text{ 年} = 2{,}208{,}988{,}800 \text{ 秒}$$

精确计算：70年 × 365.25天 × 86400秒 ≈ 2,208,988,800，其中闰年已计入。

在代码中定义为：

```c
#define NTP_UNIX_OFFSET     2208988800UL
```

### 3.2 换算公式

$$t_{\text{Unix}} = t_{\text{NTP}} - 2{,}208{,}988{,}800$$

### 3.3 时间范围

NTP 时间戳是 32 位无符号整数，最大值 $2^{32} - 1 = 4{,}294{,}967{,}295$。

$$t_{\text{NTP,max}} \rightarrow t_{\text{Unix}} = 4{,}294{,}967{,}295 - 2{,}208{,}988{,}800 = 2{,}085{,}978{,}495$$

即：NTP 32 位时间戳会在 **2036-02-07** 发生溢出回绕（NTP Era 1 问题），实际 NTPv4 用 64 位时间戳解决了这个问题，本项目 SNTP 只取高 32 位（秒字段），在 2036 年前完全安全。

---

## 4. SNTPv4 协议报文结构

SNTPv4 报文通过 **UDP，端口 123** 传输，固定 48 字节：

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|LI | VN  |Mode |    Stratum    |     Poll      |   Precision   |  byte 0-3
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Root Delay                             |  byte 4-7
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Root Dispersion                           |  byte 8-11
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Reference Identifier                       |  byte 12-15
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Reference Timestamp (64)                   |  byte 16-23
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Originate Timestamp (64)                   |  byte 24-31
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Receive Timestamp (64)                    |  byte 32-39
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Transmit Timestamp (64)                    |  byte 40-47  ← 我们读这个
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### 关键字段说明

| 字段 | 位置 | 本项目关注 | 说明 |
|------|------|-----------|------|
| LI | byte[0] bits 7-6 | 否 | Leap Indicator，闰秒警告 |
| VN | byte[0] bits 5-3 | 是（发送时设为 4） | Version Number，版本号 |
| Mode | byte[0] bits 2-0 | 是（发送时设为 3） | 3=Client，4=Server |
| Stratum | byte[1] | 否（可验证） | 0=Kiss-o'-Death，1=原子钟，2-15=同步层级 |
| Transmit Timestamp | byte[40-47] | **是（核心）** | 服务器发出应答时的时间，64位NTP格式，高32位=秒 |

### 本项目发送的请求包 (byte[0])

```c
ntp_pkt[0] = 0x23;
// 二进制: 0 0 1 0 0 0 1 1
//         └┬┘ └─┬─┘ └─┬─┘
//          LI=0  VN=4  Mode=3(Client)
```

其余 47 字节全 0，SNTP 客户端不需要填充时间戳字段。

---

## 5. NTP 时间戳的解析

服务器回应的 48 字节中，我们只需 **Transmit Timestamp** 的高 32 位（秒部分）：

```
byte[40]  byte[41]  byte[42]  byte[43]  ← 秒（NTP epoch）
byte[44]  byte[45]  byte[46]  byte[47]  ← 亚秒（1/2^32 秒分辨率，本项目忽略）
```

解析为 big-endian 32 位整数：

$$t_{\text{NTP}} = \text{pkt}[40] \ll 24 \;|\; \text{pkt}[41] \ll 16 \;|\; \text{pkt}[42] \ll 8 \;|\; \text{pkt}[43]$$

转为 Unix 时间：

$$t_{\text{Unix}} = t_{\text{NTP}} - 2{,}208{,}988{,}800$$

合法性校验：若 $t_{\text{NTP}} < 2{,}208{,}988{,}800$，则表示服务器返回了 NTP Epoch 之前的时间，视为无效。

---

## 6. 软件时钟模型（Software Clock）

STM32F407 没有带电池的 RTC（本板设计如此），每次上电时间从零开始。NTP 同步后，需要一种轻量机制持续提供时间，而不是每次调 `time()` 都发一个 UDP 包。

### 6.1 核心思想

以"同步时刻的 UTC 时间"加上"此后经过的毫秒数"计算当前时间：

$$t_{\text{now}} = t_{\text{sync}} + \left\lfloor \frac{\text{HAL\_GetTick()} - \text{tick\_sync}}{1000} \right\rfloor$$

### 6.2 状态变量

```c
static volatile time_t   s_epoch_at_sync;  // NTP 成功时的 Unix 时间（秒）
static volatile uint32_t s_tick_at_sync;   // NTP 成功时的 HAL_GetTick()（毫秒）
static volatile bool     s_synced;         // 是否已同步
```

### 6.3 为什么用 HAL_GetTick() 而非 RTC？

| 方案 | 优点 | 缺点 |
|------|------|------|
| HAL_GetTick() | 永远可用，无需外设配置 | 精度受 FreeRTOS tick 分辨率限制（1ms），长时间有晶振漂移 |
| RTC（STM32内部） | 断电保持（需电池），精度高 | 本板无备用电池，掉电丢时间；需 CubeMX 配置 LSE/LSI |
| GPS PPS | 亚微秒精度 | 需要 GPS 模块，成本高，本板未配置 |

HAL_GetTick() 是 SysTick 中断驱动的毫秒计数器，基于 HSE 晶振（本板 8 MHz），长时间漂移约 ±20 ppm（每小时约 ±0.07 秒）。每小时重同步一次完全抵消漂移。

### 6.4 写入顺序与并发安全

```c
// sntp_sync() 中的写入顺序——关键：synced 最后写
s_epoch_at_sync = unix_time;   // 1. 先写时间基准
s_tick_at_sync  = tick_now;    // 2. 再写 tick 基准
s_synced        = true;        // 3. 最后置位 synced
```

`sntp_get_utc()` 读取时：

```c
if (!s_synced) return 0;       // 读 synced=false → 直接返回 0，不读脏数据
uint32_t elapsed_ms = HAL_GetTick() - s_tick_at_sync;
return s_epoch_at_sync + (time_t)(elapsed_ms / 1000);
```

这是典型的"**松散可见性（relaxed visibility）**"写法：由于 ARM Cortex-M4 的自然字（32 位）写入是原子的，且写入顺序符合顺序一致性，不需要显式内存屏障。

---

## 7. 时钟漂移与重同步策略

### 7.1 漂移来源

$$\Delta_{\text{drift}} = t_{\text{elapsed}} \times \text{ppm}_{\text{crystal}}$$

STM32F407 外部晶振（8 MHz HSE）标称精度 ±20 ppm：

- 1 小时后误差：$3600 \times 20 \times 10^{-6} = 0.072$ 秒
- 24 小时后误差：$86400 \times 20 \times 10^{-6} = 1.73$ 秒

### 7.2 重同步周期

```c
#define SNTP_RESYNC_INTERVAL    3600   // 成功后 1 小时重同步
#define SNTP_RETRY_INTERVAL     60     // 失败后 60 秒重试
```

1 小时重同步可将累计误差控制在 0.07 秒以内，满足本项目所有需求。

---

## 8. UDP 与 ARP：局域网 NTP 的特殊性

### 8.1 ARP 的影响

NTP 使用 UDP（无连接），发包前 W5500 需要通过 ARP 获取目标 IP 的 MAC 地址。如果 ARP 缓存为空（刚上电），第一个 `sendto()` 需要先广播 ARP 请求并等待应答，在 W5500 驱动中这可能触发 ARP 超时错误（`SOCKERR_TIMEOUT = -13`）。

### 8.2 为什么要延迟 5 秒后才开始第一次同步

```c
vTaskDelay(pdMS_TO_TICKS(5000));  // 等 5 秒让 ARP 缓存预热
```

数据包转发器（主任务）在启动后立刻发送 `PULL_DATA` UDP 包到 NS（同一 LAN），W5500 在这个过程中完成了对网关 IP 的 ARP 并缓存了 MAC 地址。5 秒后 SNTP 任务再发 NTP 包时，ARP 缓存已有网关 MAC，不会再超时。

### 8.3 为什么局域网 NTP 更可靠

公网 NTP（如 120.25.115.20）需要：路由器路由 → 互联网出口 → 域名解析（本项目用 IP 跳过了这步）→ 服务器应答 → 路由回来。每一步都有丢包风险。

局域网网关（路由器）运行 NTP 守护进程（stratum 2-3），同 LAN ARP 直达，丢包率极低，延迟 < 1 ms，是首选。

---

## 9. Newlib 的 time() 调用链

在 STM32 项目中使用 Newlib（nano.specs），`time(NULL)` 的内部路径：

```
time(NULL)                    // Newlib libc (syscalls/time.c)
  └─> gettimeofday()          // Newlib POSIX wrapper
        └─> _gettimeofday()   // Newlib syscall (弱符号 weak symbol)
              └─> 通常返回 0，需要用户覆盖 (override)
```

### 9.1 弱符号与强符号

CubeMX 在 `Core/Src/syscalls.c` 生成：

```c
__attribute__((weak)) int _gettimeofday(struct timeval *tv, void *tz) {
    return 0;  // 弱实现：时间永远是 0
}
```

`__attribute__((weak))` 表示"如果链接器找到另一个同名的强符号（non-weak），优先用强符号"。

本项目在独立文件 `sntp_gettimeofday.c` 中提供强实现：

```c
// 无 __attribute__((weak)) → 强符号
int _gettimeofday(struct timeval *tv, void *tz) {
    if (tv) { tv->tv_sec = sntp_get_utc(); tv->tv_usec = 0; }
    return 0;
}
```

### 9.2 为什么用独立文件，不改 syscalls.c？

CubeMX 重新生成工程时会覆盖 `syscalls.c`，放在独立文件 `sntp_gettimeofday.c` 中，只要该文件在 CMakeLists.txt 的 SOURCES 列表里，就不会被 CubeMX 覆盖。

---

## 10. 与 ESP-IDF esp_sntp 的设计对比

| 特性 | ESP-IDF esp_sntp | 本项目 sntp_client |
|------|-----------------|-------------------|
| 网络层 | LwIP UDP socket | WIZnet W5500 硬件 socket |
| 异步模型 | 是（后台任务） | 是（后台 FreeRTOS 任务） |
| 时间设置 | `settimeofday()` 写 LwIP 内部 RTC | 写 `s_epoch_at_sync` 软件时钟 |
| `time(NULL)` | LwIP 内部 RTC → 直接返回 | `sntp_get_utc()` 软件时钟计算 |
| 服务器配置 | `esp_sntp_setservername()` 支持域名 | IP only（无 DNS 解析器） |
| 服务器回退 | 单服务器循环 | 5 级自动回退（gw → .1 → .254 → ns → pub） |
| 重同步 | 默认 1 小时（可配） | 硬编码 1 小时 |
| 初始等待 | 无（LwIP 自管理 ARP） | 5 秒等 ARP 预热 |
| 回调通知 | `esp_sntp_set_time_sync_notification_cb()` | 无（轮询 `sntp_is_synced()`） |

本项目设计是 esp_sntp 在裸机 W5500 平台上的最小化移植，保留了核心的"后台任务 + 软件时钟"模型，去掉了 LwIP 依赖。
