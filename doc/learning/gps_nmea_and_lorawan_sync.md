# GPS、NMEA 协议与 LoRaWAN 网关时间同步

> 写作背景：2026-02-28 完成 ESXP1302 GPS 线程的 ESP32 移植，
> 过程中深入了解了 NMEA 0183 协议帧格式、GPS/北斗定位原理、
> GPS 时间与 UTC 的关系，以及 LoRaWAN 网关如何利用 GPS PPS
> 和时间戳实现精准时间同步。  
> 对应变更：[CHANGE-002](../change_notes/CHANGE-002_gps_thread_esp32_port.md)

---

## 目录

- [GPS、NMEA 协议与 LoRaWAN 网关时间同步](#gpsnmea-协议与-lorawan-网关时间同步)
  - [目录](#目录)
  - [一、GPS 系统基础](#一gps-系统基础)
    - [1.1 GPS 定位原理](#11-gps-定位原理)
    - [1.2 GPS 时间与 UTC 的关系](#12-gps-时间与-utc-的关系)
    - [1.3 北斗（BDS）与 GPS 双模](#13-北斗bds与-gps-双模)
  - [二、NMEA 0183 协议详解](#二nmea-0183-协议详解)
    - [2.1 协议基本格式](#21-协议基本格式)
    - [2.2 校验和计算](#22-校验和计算)
    - [2.3 常见语句详解](#23-常见语句详解)
      - [`$GNRMC` — 推荐最小定位信息（Recommended Minimum）](#gnrmc--推荐最小定位信息recommended-minimum)
      - [`$GNGGA` — 定位系统固定数据（Fix Data）](#gngga--定位系统固定数据fix-data)
      - [`$GNZDA` — 时间和日期](#gnzda--时间和日期)
      - [`$GPGSV` / `$BDGSV` — 卫星视图（Satellites in View）](#gpgsv--bdgsv--卫星视图satellites-in-view)
    - [2.4 多系统前缀规则](#24-多系统前缀规则)
  - [三、ATGM336H 模块特性](#三atgm336h-模块特性)
    - [3.1 与 u-blox 的主要差异](#31-与-u-blox-的主要差异)
    - [3.2 UBX 私有协议概述](#32-ubx-私有协议概述)
  - [四、LoRaWAN 网关的 GPS 时间同步机制](#四lorawan-网关的-gps-时间同步机制)
    - [4.1 为什么网关需要 GPS 时间？](#41-为什么网关需要-gps-时间)
    - [4.2 PPS 脉冲与时间戳关联](#42-pps-脉冲与时间戳关联)
    - [4.3 XTAL 误差补偿](#43-xtal-误差补偿)
    - [4.4 本项目的折中方案（无 PPS）](#44-本项目的折中方案无-pps)
  - [五、lora\_pkt\_fwd GPS 相关代码结构](#五lora_pkt_fwd-gps-相关代码结构)
    - [5.1 thread\_gps：NMEA 解析主循环](#51-thread_gpsnmea-解析主循环)
    - [5.2 thread\_valid：时间参考有效性验证](#52-thread_valid时间参考有效性验证)
    - [5.3 gps\_process\_sync：时间同步与 XTAL 校正](#53-gps_process_sync时间同步与-xtal-校正)
    - [5.4 GPS 时间如何用于下行帧时间戳](#54-gps-时间如何用于下行帧时间戳)
  - [参考资料](#参考资料)

---

## 一、GPS 系统基础

### 1.1 GPS 定位原理

GPS（全球定位系统）通过至少 4 颗卫星的信号测量**伪距**（pseudo-range）来确定三维坐标和精确时间。

**关键过程：**

1. 每颗卫星以精确的 GPS 时间广播其位置和发射时间
2. 接收机记录接收时间，计算信号传播时间 × 光速 = 伪距
3. 4 颗卫星方程组联立，解出接收机的 (x, y, z, 时钟偏差)

```
卫星 1：(x₁-x)² + (y₁-y)² + (z₁-z)² = (c·(t₁-τ))²
卫星 2：...
卫星 3：...
卫星 4：（第4颗用于消除接收机时钟偏差 τ）
```

只有 3 颗卫星时可以得到平面坐标（高度固定），4 颗才能得到三维坐标+时间。

**精度指标：**

| 指标 | 说明 |
|------|------|
| HDOP | 水平精度因子，< 2 优秀，2~5 良好 |
| VDOP | 垂直精度因子 |
| PDOP | 三维精度因子 = √(HDOP²+VDOP²) |
| fix 状态 A | Active，有效定位 |
| fix 状态 V | Void，定位无效 |

### 1.2 GPS 时间与 UTC 的关系

**GPS 时间（GPS Time）** 是一个从 1980 年 1 月 6 日 00:00:00 开始计数、  
从不跳秒的原子时系统。

**UTC** 会通过插入**闰秒**（leap second）来跟踪地球自转速度的变化。

**偏差关系：**

```
GPS Time = UTC + 闰秒数
截至 2024 年，GPS Time = UTC + 18 秒
```

`$GNZDA` 语句报告的时间已经是 UTC，`$GNRMC` 中的时间也是 UTC，  
二者的闰秒转换由 GPS 模块内部完成，应用程序不需要手动处理闰秒。

**GPS 周（GPS Week）：**

GPS 时间内部以"周数（week number）+ 周内秒数（TOW, Time of Week）"表示。  
u-blox UBX 协议的 `NAV-TIMEGPS` 消息直接给出 GPS 周和 TOW，  
可以精确计算 GPS 纪元秒数（用于 LoRaWAN Class B beacon 时间戳）。

### 1.3 北斗（BDS）与 GPS 双模

ATGM336H 支持 GPS + 北斗双模，NMEA 语句前缀有区别：

| 前缀 | 系统 |
|------|------|
| `$GP` | 仅 GPS |
| `$BD` / `$GB` | 仅北斗 |
| `$GN` | 多系统融合结果（GPS + 北斗） |

定位输出（`$GNGGA`、`$GNRMC`）使用 `$GN` 前缀，表示多星融合结果，精度更高。  
卫星视图（`$GPGSV`、`$BDGSV`）分系统输出，可以看到各自的卫星信号强度。

---

## 二、NMEA 0183 协议详解

### 2.1 协议基本格式

NMEA 0183 是一种 ASCII 文本协议，每条语句（sentence）格式如下：

```
$TalkerIdSentenceId,field1,field2,...,fieldN*XX\r\n
│                  │                       │  │
│ 前缀（$开头）    │ 逗号分隔的数据字段     │校│
│                  │                       │验│
│ TalkerId：2字符  │ 字段可以为空           │和│
│ SentenceId：3字符│                       │  │
└──────────────────┴───────────────────────┴──┘
```

关键规则：
- 以 `$` 开头（0x24），以 `\r\n`（0x0D 0x0A）结束
- `*` 后面跟 2 位十六进制校验和
- 校验和是 `$` 和 `*` 之间所有字节的异或值
- 字段可以为空（两个逗号相邻），但逗号数量固定

### 2.2 校验和计算

```c
// 计算 $GNRMC,...*XX 中的 XX
uint8_t checksum = 0;
// 从 $ 后第一个字符开始，到 * 前最后一个字符
for (const char *p = sentence + 1; *p != '*' && *p != '\0'; p++) {
    checksum ^= (uint8_t)*p;
}
// checksum 转为两位大写十六进制即为 XX
```

验证示例：
```
$GNRMC,052325.000,A,3118.66793,N,12122.63244,E,0.00,44.93,280226,,,A*46
                                                                      ^^
G^N^R^M^C^,^0^...（对逗号之间所有字符做异或）= 0x46 ✓
```

### 2.3 常见语句详解

#### `$GNRMC` — 推荐最小定位信息（Recommended Minimum）

```
$GNRMC,052325.000,A,3118.66793,N,12122.63244,E,0.00,44.93,280226,,,A*46
        │          │ │           │ │            │ │    │    │        │
        时间(UTC)  │ 纬度       │ 经度         │ 速度  航向 日期     定位模式
               fix │     方向N/S │       方向E/W│
               A=有效,V=无效                  knots
```

| 字段 | 含义 | 示例 |
|------|------|------|
| 字段1 | UTC 时间 HHMMSS.sss | `052325.000` = 05:23:25.000 |
| 字段2 | 定位状态 A=有效 V=无效 | `A` |
| 字段3 | 纬度 DDMM.MMMMM | `3118.66793` = 31°18.66793' |
| 字段4 | 半球 N/S | `N` |
| 字段5 | 经度 DDDMM.MMMMM | `12122.63244` = 121°22.63244' |
| 字段6 | 半球 E/W | `E` |
| 字段7 | 速度（节） | `0.00` |
| 字段8 | 航向（度） | `44.93` |
| 字段9 | UTC 日期 DDMMYY | `280226` = 2026-02-28 |
| 字段12 | 定位模式 A=自主 D=差分 N=无效 | `A` |

**坐标解析注意事项：**

NMEA 坐标格式是 `DDMM.MMMMM`（度 + 分，不是十进制度），需要转换：

```c
// 解析纬度 3118.66793 N
// DD = 31（度）
// MM.MMMMM = 18.66793（分）
// 十进制度 = 31 + 18.66793 / 60.0 = 31.31133°

int degrees = (int)(raw / 100);
double minutes = raw - degrees * 100;
double decimal_degrees = degrees + minutes / 60.0;
```

#### `$GNGGA` — 定位系统固定数据（Fix Data）

```
$GNGGA,052325.000,3118.66793,N,12122.63244,E,1,07,2.8,9.2,M,0.0,M,,*74
                                              │  │  │   │
                                         质量指示│  │  海拔高度（M=米）
                                              1=GPS定位│
                                              卫星数    HDOP
```

| 质量指示 | 含义 |
|----------|------|
| 0 | 无定位 |
| 1 | GPS 定位 |
| 2 | 差分 GPS |
| 4 | RTK 固定解 |
| 5 | RTK 浮动解 |

#### `$GNZDA` — 时间和日期

```
$GNZDA,052325.000,28,02,2026,00,00*45
        UTC时间    日 月  年  时区偏移（小时,分钟）
```

这是最干净的时间语句，只含时间和日期，没有位置信息。  
时区偏移在 GNSS 模块中通常输出 `00,00`（即 UTC）。

#### `$GPGSV` / `$BDGSV` — 卫星视图（Satellites in View）

```
$GPGSV,2,1,07,01,73,051,,02,38,041,,03,30,138,33,07,33,206,30*7A
        │  │  │   └─────────────────────────── 卫星1信息（PRN,仰角,方位,SNR）
        │  │  └── 本次消息中总卫星数
        │  └───── 本条消息序号（第1条，共2条）
        └──────── 总消息条数
```

每条 GSV 最多携带 4 颗卫星的信息，卫星总数多时分多条发送。  
SNR（信噪比）单位 dB-Hz，30 以上视为良好。

### 2.4 多系统前缀规则

```
$GP... → GPS 单系统
$GL... → GLONASS
$GA... → Galileo
$GB... / $BD... → BeiDou（北斗）
$GN... → 多系统融合（GNSS）
```

---

## 三、ATGM336H 模块特性

### 3.1 与 u-blox 的主要差异

| 特性 | ATGM336H（中科微） | u-blox M8/M9 |
|------|-------------------|--------------|
| 协议 | 标准 NMEA 0183 | NMEA + UBX 私有协议 |
| UBX 支持 | 无 | 有（可配置输出） |
| 北斗支持 | ✅ 原生双模 | ✅（部分型号） |
| 默认波特率 | 9600 | 9600 |
| PPS 引脚 | 有（1PPS 输出） | 有 |
| 价格 | 低（国产） | 较高 |
| 文档 | 中文资料为主 | 英文完整文档 |

在 lora_pkt_fwd 的原始实现（面向 u-blox）中，时间同步依赖 `UBX_NAV_TIMEGPS`——  
这是 u-blox 私有二进制帧，含有精确的 GPS 周号（week）和周内秒数（TOW），  
可实现纳秒级时间参考。ATGM336H 没有此帧，只能从 NMEA `$GNRMC` 中获取时间。

### 3.2 UBX 私有协议概述

u-blox UBX 是二进制协议，帧格式如下：

```
┌──────┬──────┬────────────┬──────┬──────────┬──────────┬──────┐
│ 0xB5 │ 0x62 │ Class(1B)  │ ID   │ Length   │ Payload  │ CK_A │ CK_B
│ 同步1 │ 同步2│            │ (1B) │ (2B LE)  │          │ 2B Fletcher校验
└──────┴──────┴────────────┴──────┴──────────┴──────────┴──────┘
```

`UBX_NAV_TIMEGPS`（Class=0x01, ID=0x20）的 Payload 包含：
- `iTOW`：GPS 毫秒时刻（周内毫秒）
- `fTOW`：亚毫秒精度（纳秒）
- `week`：GPS 周号
- `leapS`：闰秒数
- `valid`：有效标志位

`lgw_parse_ubx()` 解析此帧后更新内部 `gps_week`、`gps_iTOW`、`gps_fTOW` 变量，  
这些变量在 `lgw_gps_get()` 的 `gps_time` 部分使用，提供 GPS 纪元秒数（精度更高）。  
而从 NMEA `$GNRMC` 解析的时间只精确到毫秒，但对于 LoRaWAN Class A/C 已经足够。

---

## 四、LoRaWAN 网关的 GPS 时间同步机制

### 4.1 为什么网关需要 GPS 时间？

LoRaWAN 网关需要 GPS 时间的主要场景：

| 场景 | 说明 |
|------|------|
| **Class B 下行** | Beacon 严格按 GPS 时间周期发送（128 秒），节点和网关必须同步 |
| **Class B 时隙** | Ping slot 时间基于 GPS epoch，网关必须知道 GPS 时间才能打开正确的下行窗口 |
| **统计上报** | JSON stat 字段含 GPS 坐标，用于多网关覆盖地图展示 |
| **精确时间戳** | 上行帧携带精确的 GPS 时间戳，Network Server 可做 TDOA 定位 |

Class A 只要网关能正常收包就行，不依赖 GPS 时间，  
但 Class B 和精确下行时序必须有有效的 GPS 时间参考。

### 4.2 PPS 脉冲与时间戳关联

**PPS（Pulse Per Second）** 是 GPS 模块输出的每秒一次的精确脉冲，  
上升沿对应 UTC 整秒时刻，精度通常在 ±100ns 以内。

lora_pkt_fwd 的时间同步机制：

```
GPS 模块
  ├─ UART：发送 $GNRMC 时间     → 告知"这一秒是几点"
  └─ PPS 引脚：发送 1 Hz 脉冲   → 精确标记"这一秒的边界"

SX1302
  └─ PPS 引脚输入 → 触发内部计数器捕获 → lgw_get_trigcnt() 读取

结合：
  UTC 时间（来自 RMC）+ SX1302 计数器（来自 trigcnt）
  → 建立 GPS UTC ↔ SX1302 硬件计数器 的时间参考
  → lgw_gps_sync() 写入 time_reference_gps
  → lgw_cnt2utc() / lgw_utc2cnt() 可以双向转换
```

### 4.3 XTAL 误差补偿

SX1302 内部使用晶振驱动计数器，但晶振存在频率偏差（ppm 级别），  
长时间运行后计数器与真实时间会有偏差。

`thread_valid` 的职责就是持续监测这个偏差：

```c
// 每次 GPS 同步更新 time_reference_gps.xtal_err
// xtal_err = 计数器差值 / UTC 差值（理想值为 1.0）

// XTAL 误差滤波（低通）
xtal_correct = xtal_correct * (1 - 1/COEF) + (1/xtal_err) * (1/COEF);

// 下行时间戳转换时使用 xtal_correct 校正
delta_us = (count_us - ref.count_us) / (TS_CPS * xtal_correct);
```

前 16 次（`XERR_INIT_AVG=16`）同步取平均值作为初始校正，  
之后用低通滤波器（系数 `XERR_FILT_COEF=256`）跟踪缓慢变化。

### 4.4 本项目的折中方案（无 PPS）

ESXP1302 当前配置中，SX1302 的 PPS 引脚并未接到 ATGM336H 的 PPS 引脚，  
所以 `lgw_get_trigcnt()` 读取的捕获计数器无法对应精确的 PPS 时刻。

**实际影响：**

- `gps_process_sync()` 中 `lgw_get_trigcnt()` 读取的是调用时刻的即时计数器，  
  而不是上一次 PPS 上升沿的计数器
- 时间同步精度从 PPS 的 ±100ns 级别下降到取决于 `$GNRMC` 传输延迟（< 100ms 级别）
- Class A 完全不受影响
- Class B Beacon 的精度会降低，但对于大多数农业/工业场景（百毫秒级窗口）仍然可用

```
# 现状（无 PPS 接入）
SX1302 PPS_IN ─── [未连接] ─── ATGM336H PPS_OUT
       → lgw_get_trigcnt() 返回当前计数器（非 PPS 时刻）
       → 时间同步精度约 10~100ms

# 改善方案（接入 PPS）
SX1302 PPS_IN ─── [连接] ─── ATGM336H PPS_OUT
       → lgw_get_trigcnt() 返回上一次 PPS 上升沿的计数器快照
       → 时间同步精度约 < 1ms（主要受 UART 传输延迟限制）
```

---

## 五、lora_pkt_fwd GPS 相关代码结构

### 5.1 thread_gps：NMEA 解析主循环

`thread_gps` 以 8 字节（`LGW_GPS_MIN_MSG_SIZE`）步进从 UART 读取数据，  
填入环形缓冲区 `serial_buff[128]`，逐字节扫描同步字符：

```
缓冲区扫描逻辑：

rd_idx 从 0 扫描到 wr_idx：
  ├─ 遇到 0xB5（UBX 同步字节1）→ 尝试 lgw_parse_ubx()
  │    ├─ INCOMPLETE：等待更多数据
  │    ├─ UBX_NAV_TIMEGPS → gps_process_sync()
  │    └─ 其他 UBX 帧 → 忽略
  └─ 遇到 0x24（$ ，NMEA 同步字节）→ 查找 0x0A（LF，行尾）
       ├─ 未找到 LF：等待更多数据
       └─ 找到 LF → lgw_parse_nmea()
            ├─ NMEA_RMC → gps_process_coords() + gps_process_sync()
            ├─ NMEA_GGA → （坐标内部更新，不在这里单独处理）
            └─ 其他 → 忽略

处理完的帧从缓冲区移除（memcpy 前移）
缓冲区剩余空间不足 8 字节时丢弃最旧的 8 字节（防溢出）
```

### 5.2 thread_valid：时间参考有效性验证

每秒检查 `time_reference_gps.systime`（最后一次成功同步的系统时间）：

```c
gps_ref_age = difftime(time(NULL), time_reference_gps.systime);
if (gps_ref_age >= 0 && gps_ref_age <= GPS_REF_MAX_AGE) {  // 30秒内有效
    gps_ref_valid = true;
} else {
    gps_ref_valid = false;
    xtal_correct_ok = false;  // 同时使 XTAL 校正失效
}
```

`GPS_REF_MAX_AGE = 30` 秒，超过 30 秒没有成功同步就认为时间参考失效，  
日志中显示 `Invalid time reference (age: XXX sec)`。

### 5.3 gps_process_sync：时间同步与 XTAL 校正

```c
static void gps_process_sync(void) {
    // 1. 从 lgw_parse_nmea 更新的内部变量读取 UTC 时间
    lgw_gps_get(&utc, &gps_time, NULL, NULL);
    //   内部：gps_time_ok=true 时，将 gps_yea/mon/day/hou/min/sec 转为 timespec

    // 2. 读取 SX1302 "触发计数器"（理想情况下是 PPS 时刻的快照）
    lgw_get_trigcnt(&trig_tstamp);

    // 3. 建立/更新时间参考
    lgw_gps_sync(&time_reference_gps, trig_tstamp, utc, gps_time);
    //   内部：计算 slope = 计数器差 / UTC 差，检测异常点，
    //         连续 3 次异常才强制重置，否则只更新 systime/utc/count_us/xtal_err
}
```

### 5.4 GPS 时间如何用于下行帧时间戳

当 Network Server 下发 Class B / Class C 帧时，JSON 中可包含 `"tmms"` 字段  
（GPS 毫秒时间戳），`thread_down` 将其转换为 SX1302 计数器值：

```c
// JSON 中的 GPS 毫秒时间戳 → timespec
gps_tx.tv_sec  = (time_t)(x2 / 1000);
gps_tx.tv_nsec = (long)((x2 % 1000) * 1E6);

// GPS 时间 → SX1302 计数器（使用 time_reference_gps 和 xtal_correct）
lgw_gps2cnt(local_ref, gps_tx, &txpkt.count_us);

// SX1302 在 count_us 时刻发射该帧（精度取决于 PPS 是否接入）
jit_enqueue(&jit_queue[rf_chain], current_time, &txpkt, JIT_PKT_TYPE_DOWNLINK_CLASS_B);
```

整个链路：

```
NS 给出 GPS 时间戳（毫秒）
  → lgw_gps2cnt() 利用 time_reference_gps 换算为 SX1302 计数器值
  → JIT 队列在精确计数器时刻触发发射
  → lgw_send() 将包交给 SX1302，硬件在 count_us 到达时自动发射
```

---

## 参考资料

- NMEA 0183 标准：https://www.nmea.org/content/FORUM/NMEA_0183_Standard.pdf
- u-blox M8 协议手册（UBX 帧格式参考）：u-blox M8 Receiver Description
- ATGM336H 数据手册：中科微电子
- Semtech lora_pkt_fwd 源码：https://github.com/Lora-net/sx1302_hal
