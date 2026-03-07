# ESXP1302 时间系统全貌

> 写作背景：2026-03-02 完成 BUG-011 修复（tmms 显示 1980 GPS 纪元），
> 借此机会系统梳理 ESXP1302 网关中所有时间相关概念的来龙去脉，
> 涵盖从硬件晶振到 ChirpStack JSON 字段的完整链路。
> 本文包含对关键代码路径的逐行解析，配合源码阅读使用。
>
> 涉及源文件：
> - `main/libloragw/loragw_gps.c` — GPS 驱动和时间转换函数库
> - `main/packet_forwarder/lora_pkt_fwd.c` — 包转发主逻辑，含所有线程

---

## 目录

1. [时间系统总览](#一时间系统总览)
2. [硬件层：SX1302 计数器](#二硬件层sx1302-计数器)
3. [GPS 层：数据流与时间参考建立](#三gps-层数据流与时间参考建立)
4. [代码解析：thread_gps](#四代码解析thread_gps)
5. [代码解析：gps_process_sync](#五代码解析gps_process_sync)
6. [代码解析：lgw_gps_get](#六代码解析lgw_gps_get)
7. [代码解析：lgw_gps_sync](#七代码解析lgw_gps_sync)
8. [代码解析：thread_valid 与晶振校正](#八代码解析thread_valid-与晶振校正)
9. [代码解析：thread_up 时间字段生成](#九代码解析thread_up-时间字段生成)
10. [代码解析：thread_down 下行时间调度](#十代码解析thread_down-下行时间调度)
11. [时间转换函数详解](#十一时间转换函数详解)
12. [ChirpStack 层：NS 视角的时间字段](#十二chirpstack-层ns-视角的时间字段)
13. [PPS 的作用与缺失的影响](#十三pps-的作用与缺失的影响)
14. [BUG-011 根因与修复](#十四bug-011-根因与修复)
15. [时间系统常见问题 FAQ](#十五时间系统常见问题-faq)

---

## 一、时间系统总览

ESXP1302 中同时存在 **四套相互关联但不同质的时间**：

```
┌─────────────────────────────────────────────────────────────────────┐
│  【1】SX1302 硬件计数器（count_us / tmst）                          │
│       单位：微秒（µs）                                              │
│       起点：网关上电复位，从 0 开始                                 │
│       驱动：32 MHz 晶振，经分频后以 1 MHz 速率递增（TS_CPS=1E6）   │
│       特点：相对时间，单调递增，无闰秒，32 bit 约 71.6 分钟回绕     │
│       用途：帧接收时刻标记（tmst），下行发射精确调度                │
├─────────────────────────────────────────────────────────────────────┤
│  【2】GPS UTC 时间（utc / "time" 字段）                             │
│       单位：秒 + 纳秒（struct timespec）                           │
│       起点：1970-01-01 00:00:00 UTC（Unix 纪元）                    │
│       来源：NMEA $GNRMC / $GNZDA 语句解析                           │
│       精度：毫秒级（NMEA 报告到 .sss）                              │
│       用途：上行 JSON "time" 字段，人类可读的绝对时间               │
├─────────────────────────────────────────────────────────────────────┤
│  【3】GPS 纪元时间（gps_time / "tmms" 字段）                        │
│       单位：秒 + 纳秒（struct timespec）或毫秒（uint64_t tmms）    │
│       起点：1980-01-06 00:00:00 GPS（GPS 纪元）                     │
│       来源：① UBX NAV-TIMEGPS（u-blox 专有，含周号+iTOW）          │
│               ② NMEA UTC 换算：GPS = UTC_unix - 315964800 + 18     │
│       特点：不跳闰秒，比 UTC 快 18 秒（截至 2026）                  │
│       用途：上行 JSON "tmms" 字段，Class B Beacon 时间戳            │
├─────────────────────────────────────────────────────────────────────┤
│  【4】系统时钟（time(NULL) / systime）                              │
│       单位：秒（time_t）                                            │
│       来源：ESP32 内部 RTC（无 NTP，仅作看门狗用）                  │
│       用途：GPS 参考年龄检测（thread_valid），日志时间戳             │
└─────────────────────────────────────────────────────────────────────┘
```

这四套时间通过 **`time_reference_gps`（struct tref）** 绑定在一起。
`struct tref` 定义在 `loragw_gps.h`：

```c
struct tref {
    time_t      systime;   // 系统时钟，"本次同步发生在何时"
    uint32_t    count_us;  // 同步时刻的 SX1302 计数器（来自 trig_tstamp，即 PPS 锁存值）
    struct timespec utc;   // 同步时刻的 UTC 时间（来自 NMEA 解析）
    struct timespec gps;   // 同步时刻的 GPS 纪元时间（修复前总是 {0,0}）
    double      xtal_err;  // 晶振误差：cnt_diff / utc_diff（理想值 1.0）
};
```

有了这个锚点，可以在任意时刻通过计数器值推算出 UTC 或 GPS 纪元时间。

---

## 二、硬件层：SX1302 计数器

### 2.1 两个计数器

SX1302 内部有两个由相同晶振驱动的计数器，均以 1 MHz 速率递增：

| 计数器 | 更新条件 | 读取函数 | 用途 |
|--------|----------|----------|------|
| `TIMESTAMP_INST`（即时） | 每微秒自动递增 | `lgw_get_instcnt()` | 帧接收时刻（`tmst`），JIT 调度基准 |
| `TIMESTAMP_PPS`（PPS 捕获） | GPS PPS 上升沿时硬件锁存 INST 值 | `lgw_get_trigcnt()` | GPS 同步锚点（`trig_tstamp`） |

**关键理解**：两个计数器的时钟源完全相同，没有 PPS 时晶振照样跑，
区别在于 PPS 计数器是**硬件锁存器**——PPS 脉冲到来的瞬间把 INST 的值"快照"进去，
之后不管 INST 继续跑多远，PPS 寄存器里的值保持不变，直到下一个 PPS 脉冲。

### 2.2 tmst 字段的来源

```
LoRa 帧到达 SX1302 天线
       ↓ SX1302 硬件记录接收完成时刻（精确到帧尾最后一个符号）
lgw_receive() 读出 rxpkt[i].count_us（INST 计数器当前值）
       ↓
thread_up: p->count_us → JSON "tmst" 字段（见 lora_pkt_fwd.c 约 2330 行）

// lora_pkt_fwd.c ~L2330
j = snprintf(..., ",\"tmst\":%u", p->count_us);
```

`tmst` 是**相对时间戳**，单位微秒，是网关上电后计数器的累计值。
不同网关之间 `tmst` 完全没有可比性；同一网关重启后也重新从 0 开始。

### 2.3 计数器溢出

`count_us` 是 32 位无符号整型：

$$2^{32} - 1 = 4{,}294{,}967{,}295\ \mu s \approx 4295\ s \approx 71.6\ \text{分钟}$$

每 71.6 分钟计数器回绕为 0。时间差计算利用无符号整数自然溢出，无需特殊处理：

```c
// loragw_gps.c - lgw_cnt2utc() 中的差值计算
// count_us 和 ref.count_us 都是 uint32_t，相减自动处理溢出
delta_sec = (double)(count_us - ref.count_us) / (TS_CPS * ref.xtal_err);
```

若使用有符号比较则溢出时结果错误，这是个常见陷阱。

---

## 三、GPS 层：数据流与时间参考建立

### 3.1 完整数据流（ATGM336H NMEA-only 模式）

```
ATGM336H GPS 模块（UART1, GPIO19 RX/GPIO20 TX, 9600 baud）
  │
  ├─ 每秒输出多条 NMEA 语句（$GNRMC, $GNGGA, $GNGSV 等）
  │
  │  thread_gps（FreeRTOS 任务，优先级 6，栈 8KB）
  │    │
  │    ├─ uart_read_bytes() 阻塞读，超时 1000ms
  │    │   写入 serial_buff[128] 的 [wr_idx..] 位置
  │    │
  │    ├─ 扫描缓冲区（rd_idx 从 0 到 wr_idx）
  │    │   ├─ 遇到 '$'（0x24，NMEA 起始）：
  │    │   │   memchr 找 '\n'（0x0a，结束符）
  │    │   │   lgw_parse_nmea() → 解析 $GNRMC：
  │    │   │     更新 gps_yea/mon/day/hou/min/sec/fra（静态全局变量）
  │    │   │     更新 gps_dla/mla/ola/dlo/mlo/olo/alt（坐标）
  │    │   │     设置 gps_time_ok = true（fix 状态为 A 时）
  │    │   │     返回 NMEA_RMC
  │    │   │       ↓
  │    │   │   gps_process_coords() → 更新 meas_gps_coord
  │    │   │   gps_process_sync()   → 建立时间参考（详见第五节）
  │    │   │
  │    │   └─ 遇到 0xB5（UBX 起始）：
  │    │       lgw_parse_ubx() → ATGM336H 无此输出，通常 IGNORED
  │    │
  │    └─ 滑动窗口缓冲区管理（详见 3.2）
  │
  ├─ PPS 引脚（1 Hz，精确标记整秒边界）
  │     ↓ SX1302 硬件在上升沿锁存 INST → TIMESTAMP_PPS 寄存器
  │     ↓ lgw_get_trigcnt() 读出 trig_tstamp
  │     └─ 在 gps_process_sync() 中与 NMEA UTC 绑定
  │
  └─ thread_valid（每秒检查参考有效期，维护晶振校正系数）
```

### 3.2 缓冲区管理（滑动窗口）

`thread_gps` 不使用经典环形缓冲区，而是用一个 128 字节数组 + 两个索引实现滑动窗口：

```c
// lora_pkt_fwd.c - thread_gps() 内
char serial_buff[128];   // 静态缓冲区
size_t wr_idx = 0;       // 有效数据结束位置（新数据追加到此处）

// 每次循环：
// 1. 追加读取
int nb_char = uart_read_bytes(gps_tty_fd,
    (uint8_t *)(serial_buff + wr_idx),  // 追加到已有数据之后
    LGW_GPS_MIN_MSG_SIZE,               // 最少读 LGW_GPS_MIN_MSG_SIZE 字节
    pdMS_TO_TICKS(1000));               // 阻塞等待最多 1 秒
wr_idx += (size_t)nb_char;

// 2. 扫描阶段（rd_idx 从头扫到 wr_idx）
size_t rd_idx = 0;
size_t frame_end_idx = 0;
while (rd_idx < wr_idx) {
    size_t frame_size = 0;
    if (serial_buff[rd_idx] == '$') {
        // 找到 NMEA 帧，解析...
        frame_size = ...; // 本帧字节数
    }
    if (frame_size > 0) {
        rd_idx += frame_size;
        frame_end_idx = rd_idx; // 记录最后一个成功帧的结束位置
    } else {
        rd_idx++; // 未识别字符，跳过
    }
}

// 3. 清除已处理数据（将剩余数据移到缓冲区开头）
if (frame_end_idx) {
    memcpy(serial_buff, &serial_buff[frame_end_idx], wr_idx - frame_end_idx);
    wr_idx -= frame_end_idx;
}

// 4. 溢出保护
if ((sizeof(serial_buff) - wr_idx) < LGW_GPS_MIN_MSG_SIZE) {
    memcpy(serial_buff, &serial_buff[LGW_GPS_MIN_MSG_SIZE], wr_idx - LGW_GPS_MIN_MSG_SIZE);
    wr_idx -= LGW_GPS_MIN_MSG_SIZE;
}
```

这不是环形缓冲区，而是**滑动窗口**：`memcpy` 每次把未处理的数据搬到缓冲区头部，
代价是 O(n) 拷贝，但对低速 NMEA（9600 baud）完全够用。

---

## 四、代码解析：thread_gps

> 源文件：`lora_pkt_fwd.c`，函数 `thread_gps()`，约 L3653 起

### 4.1 任务创建方式

原始代码使用 POSIX `pthread_create`，ESP32 移植时改为 FreeRTOS：

```c
// lora_pkt_fwd.c ~L1852
xTaskCreatePinnedToCore(
    (TaskFunction_t) thread_gps,
    "thread_gps",
    4096*2,         // 8KB 栈（GPS UART + NMEA 解析用）
    NULL,
    6,              // 优先级 6（高于默认 5，确保 GPS 数据不丢失）
    NULL,
    tskNO_AFFINITY  // 不锁定 CPU 核心
);
```

### 4.2 NMEA 帧检测逻辑

```c
} else if (serial_buff[rd_idx] == (char)LGW_GPS_NMEA_SYNC_CHAR) {  // '$' = 0x24
    // 找帧结束符 LF（0x0A）
    char* nmea_end_ptr = memchr(&serial_buff[rd_idx], 0x0a, (wr_idx - rd_idx));

    if (nmea_end_ptr) {
        frame_size = nmea_end_ptr - &serial_buff[rd_idx] + 1; // 含 LF
        latest_msg = lgw_parse_nmea(&serial_buff[rd_idx], frame_size);

        if (latest_msg == NMEA_RMC) {
            gps_process_coords(); // 更新经纬度（独立于时间）
            gps_process_sync();   // 建立/更新时间参考
        }
    }
    // 注：若缓冲区中只有帧头没有 LF，nmea_end_ptr 为 NULL
    //     frame_size 保持 0，下次读到更多数据后再重试
}
```

ATGM336H 只输出 NMEA，所以 UBX 分支（`0xB5` 起始）在正常工作中永远不会命中。

### 4.3 为什么在 NMEA_RMC 而不是 NMEA_GGA 上触发同步

`$GNRMC` 是唯一同时包含**时间**和**日期**的 NMEA 语句（`$GNGGA` 只有时间，没有日期）。
`lgw_gps_get()` 需要完整的年月日时分秒才能调用 `mktime()` 得到绝对 Unix 时间戳，
因此必须等 RMC 解析完成后才能触发时间同步。

---

## 五、代码解析：gps_process_sync

> 源文件：`lora_pkt_fwd.c`，函数 `gps_process_sync()`，约 L3591 起

这是连接 GPS 时间和 SX1302 计数器的核心函数，每秒在 `NMEA_RMC` 到达时调用一次。

```c
static void gps_process_sync(void)
{
    struct timespec gps_time;  // GPS 纪元时间（1980-01-06 起）
    struct timespec utc;       // UTC 时间（1970-01-01 起）
    unsigned int trig_tstamp;  // PPS 锁存的 SX1302 计数器值

    // ① 从 NMEA 解析结果中构造 UTC 和 GPS 纪元时间
    int i = lgw_gps_get(&utc, &gps_time, NULL, NULL);
    if (i != LGW_GPS_SUCCESS) {
        // gps_time_ok == false（无定位或 NMEA 未解析成功）
        return;
    }

    // ② 读取 PPS 硬件锁存的计数器值（需要持 mx_concent 锁）
    xSemaphoreTake(mx_concent, portMAX_DELAY);
    i = lgw_get_trigcnt(&trig_tstamp);
    xSemaphoreGive(mx_concent);
    // trig_tstamp = PPS 脉冲到来瞬间的 INST 计数器快照
    // 若无 PPS：trig_tstamp = 0（寄存器从未被锁存过）

    // ③ 用 (trig_tstamp, utc, gps_time) 三元组更新时间参考
    xSemaphoreTake(mx_timeref, portMAX_DELAY);
    i = lgw_gps_sync(&time_reference_gps, trig_tstamp, utc, gps_time);
    xSemaphoreGive(mx_timeref);
    // time_reference_gps.count_us = trig_tstamp（PPS 时刻的计数器）
    // time_reference_gps.utc      = utc（该 PPS 整秒对应的 UTC）
    // time_reference_gps.gps      = gps_time（同上，GPS 纪元表示）
    // time_reference_gps.xtal_err = 实测 slope（修正晶振偏差）

    if (i == LGW_GPS_SUCCESS) {
        // 打印同步日志（GPS_LOG_VERBOSE >= 1 时）
        // "INFO: [gps] synced UTC time: 2026-03-02T08:58:19.000Z (trig_tstamp=201660610)"
    }
}
```

**关键约束**：`gps_process_sync()` 每次调用都使用"最新的 PPS 锁存值"，
而不是每次 PPS 脉冲时立刻触发（ATGM336H 不支持 PPS 中断通知软件）。
因此实际逻辑是：**NMEA RMC 到达时，顺便读一下当前的 PPS 寄存器**，
PPS 值和 NMEA 时间对应同一整秒（只要 NMEA 和 PPS 的传输延迟可以忽略）。

---

## 六、代码解析：lgw_gps_get

> 源文件：`loragw_gps.c`，函数 `lgw_gps_get()`，约 L681 起

`lgw_gps_get()` 是将 NMEA 解析出的静态全局变量转换为 `struct timespec` 的桥梁。

### 6.1 UTC 分支（NMEA 路径，无 BUG）

```c
// loragw_gps.c - lgw_gps_get(), utc 分支
if (utc != NULL) {
    if (!gps_time_ok) return LGW_GPS_ERROR;  // NMEA 未解析成功

    struct tm x = {0};
    // 将 NMEA 字段组装成 broken-down time
    x.tm_year = (gps_yea < 100) ? gps_yea + 100 : gps_yea - 1900;
    x.tm_mon  = gps_mon - 1;   // tm_mon 是 [0,11]，NMEA 是 [1,12]
    x.tm_mday = gps_day;
    x.tm_hour = gps_hou;
    x.tm_min  = gps_min;
    x.tm_sec  = gps_sec;

    time_t y = mktime(&x);  // 转换为 Unix 时间戳（秒）
    // 注：mktime 假设输入是本地时间，但 tzset() 已在 lgw_gps_enable() 中调用
    // ESP-IDF 默认时区 UTC+0，所以结果正确

    utc->tv_sec  = y;
    utc->tv_nsec = (int32_t)(gps_fra * 1e9);  // 小数秒部分（纳秒）
}
```

`gps_fra` 来自 `$GNRMC` 中的 `hhmmss.sss` 格式，
例如 `085819.685` → `gps_fra = 0.685` → `tv_nsec = 685000000 ns`。

### 6.2 gps_time 分支（BUG-011 修复点）

修复前的原始代码（使用 UBX 路径，NMEA-only 模块永远不更新）：

```c
// 修复前（BUGGY）：
if (gps_time != NULL) {
    // gps_week, gps_iTOW, gps_fTOW 只由 UBX_NAV_TIMEGPS 消息更新
    // ATGM336H 无 UBX 输出 → 这三个变量永远是 0
    fractpart = modf(((double)gps_iTOW / 1E3) + ((double)gps_fTOW / 1E9), &intpart);
    gps_time->tv_sec  = (time_t)intpart + (time_t)gps_week * 604800;
    // = 0 + 0 * 604800 = 0
    gps_time->tv_nsec = (long)(fractpart * 1E9);  // = 0
    // 结果：gps_time = {0, 0}（GPS 纪元 1980-01-06，不是当前时间！）
}
```

修复后（以 `gps_week` 是否为 0 判断模式）：

```c
// 修复后（loragw_gps.c 当前代码）：
if (gps_time != NULL) {
    if (!gps_time_ok) return LGW_GPS_ERROR;

    if (gps_week != 0) {
        // ── UBX 模式（u-blox 模块）：使用高精度 iTOW + week ──
        fractpart = modf(((double)gps_iTOW / 1E3) + ((double)gps_fTOW / 1E9), &intpart);
        gps_time->tv_sec  = (time_t)intpart + (time_t)gps_week * 604800;
        gps_time->tv_nsec = (long)(fractpart * 1E9);
    } else {
        // ── NMEA-only 模式（ATGM336H 等）：从 NMEA 日期时间字段派生 ──
        struct tm gps_tm = {0};
        gps_tm.tm_year = (gps_yea < 100) ? gps_yea + 100 : gps_yea - 1900;
        gps_tm.tm_mon  = gps_mon - 1;
        gps_tm.tm_mday = gps_day;
        gps_tm.tm_hour = gps_hou;
        gps_tm.tm_min  = gps_min;
        gps_tm.tm_sec  = gps_sec;
        time_t gps_unix = mktime(&gps_tm);  // → Unix 时间戳

        // GPS 纪元 = Unix 时间戳 - GPS纪元偏移 + 闰秒
        // GPS_epoch_offset = 315964800 （1970-01-01 到 1980-01-06 的秒数）
        // GPS_LEAP_SECONDS  = 18        （GPS 比 UTC 快 18 秒，2017年至今）
        gps_time->tv_sec  = gps_unix - UNIX_GPS_EPOCH_OFFSET + GPS_LEAP_SECONDS;
        gps_time->tv_nsec = (int32_t)(gps_fra * 1e9);
        // 结果示例：1772441099 - 315964800 + 18 = 1456476317
        //           对应 2026-03-02T08:58:37 GPS，约 1456476317 秒自 1980-01-06
    }
}
```

---

## 七、代码解析：lgw_gps_sync

> 源文件：`loragw_gps.c`，函数 `lgw_gps_sync()`，约 L610 起

`lgw_gps_sync()` 每次收到 GPS 时间后被调用，负责：
1. 检测该次同步是否"异常"（slope 超出 ±10 ppm）
2. 若正常，将三元组 `(count_us, utc, gps_time)` 写入 `time_reference_gps`
3. 若连续 3 次异常，强制重置（排除卫星信号质量问题导致的持续失锁）

```c
int lgw_gps_sync(struct tref *ref, unsigned int count_us,
                 struct timespec utc, struct timespec gps_time) {

    // 计算计数器差值（秒，未经晶振校正）
    double cnt_diff = (double)(count_us - ref->count_us) / TS_CPS;
    // 计算 UTC 差值（秒）
    double utc_diff = (double)(utc.tv_sec - ref->utc.tv_sec)
                    + 1E-9 * (double)(utc.tv_nsec - ref->utc.tv_nsec);

    // 计算 slope = 计数器速率 / UTC 速率
    // 理想值 = 1.0（计数器每秒走恰好 1,000,000 个单位）
    // 实际值 = 1.0 ± 几 ppm（晶振偏差）
    double slope = cnt_diff / utc_diff;

    // 异常检测：slope 超出 ±10 ppm 范围（PLUS_10PPM=1.00001, MINUS_10PPM=0.99999）
    bool aber_n0 = (slope > PLUS_10PPM) || (slope < MINUS_10PPM) || (utc_diff == 0);

    if (!aber_n0) {
        // ── 正常：直接更新时间参考 ──
        ref->systime     = time(NULL);      // 系统时钟（当前时刻）
        ref->count_us    = count_us;        // PPS 时刻的计数器值
        ref->utc         = utc;             // PPS 对应的 UTC
        ref->gps         = gps_time;        // PPS 对应的 GPS 纪元时间（修复后正确）
        ref->xtal_err    = slope;           // 本次测得的晶振速率
        return LGW_GPS_SUCCESS;

    } else if (aber_n0 && aber_min1 && aber_min2) {
        // ── 连续 3 次异常：强制重置 ──
        ref->systime  = time(NULL);
        ref->count_us = count_us;
        ref->utc      = utc;
        ref->gps      = gps_time;
        // xtal_err 只在越界时才重置为 1.0，否则保留上次的值
        if ((ref->xtal_err > PLUS_10PPM) || (ref->xtal_err < MINUS_10PPM))
            ref->xtal_err = 1.0;
        return LGW_GPS_SUCCESS;

    } else {
        // ── 只有 1 或 2 次异常：忽略，保留旧参考 ──
        return LGW_GPS_ERROR;
        // 调用方 (gps_process_sync) 收到 ERROR 后打印
        // "WARNING: [gps] GPS out of sync, keeping previous time reference"
    }
}
```

**无 PPS 时的 slope 计算**：
- `count_us = trig_tstamp = 0`（PPS 寄存器从未更新）
- `cnt_diff = (0 - ref->count_us)`：第一次 `ref->count_us=0` 时差值=0，slope=0 → 异常
- 后续每次 `count_us` 仍然是 0，而 `ref->count_us` 也被更新为 0
- 实际上 slope 一直是 0/1 = 0，连续 3 次异常 → 强制重置
- 强制重置后：`ref->count_us = 0, ref->xtal_err` 保留或重置为 1.0
- 之后 `lgw_cnt2utc(count_us=X, ref.count_us=0)` → `delta = X/1E6` 秒（设备运行时间）

---

## 八、代码解析：thread_valid 与晶振校正

> 源文件：`lora_pkt_fwd.c`，函数 `thread_valid()`，约 L3775 起

`thread_valid` 每秒执行一次，负责两件事：
1. **检查参考有效期**：超过 `GPS_REF_MAX_AGE=30` 秒没有成功同步，则 `gps_ref_valid = false`
2. **维护晶振校正系数**：对 `xtal_err` 做低通滤波，得到稳定的 `xtal_correct`

```c
void thread_valid(void) {
    long gps_ref_age;
    double xtal_err_cpy;
    unsigned init_cpt = 0;
    double init_acc = 0.0;

    while (!exit_sig && !quit_sig) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // 每秒执行一次

        // ── 第一步：检查参考年龄 ──
        xSemaphoreTake(mx_timeref, portMAX_DELAY);
        gps_ref_age = (long)difftime(time(NULL), time_reference_gps.systime);
        if (gps_ref_age >= 0 && gps_ref_age <= GPS_REF_MAX_AGE) {  // 30 秒内
            gps_ref_valid = true;
            xtal_err_cpy = time_reference_gps.xtal_err;
        } else {
            gps_ref_valid = false;  // 参考过旧，禁用时间戳
        }
        xSemaphoreGive(mx_timeref);

        // ── 第二步：晶振校正系数管理 ──
        if (!gps_ref_valid) {
            // GPS 失锁：重置，不做频率补偿
            xtal_correct_ok = false;
            xtal_correct = 1.0;
            init_cpt = 0;
            init_acc = 0.0;
        } else if (init_cpt < XERR_INIT_AVG) {
            // 初始化阶段（前 XERR_INIT_AVG=4 次）：累积求和
            init_acc += xtal_err_cpy;
            ++init_cpt;
        } else if (init_cpt == XERR_INIT_AVG) {
            // 初始均值：xtal_correct = N / sum(slope_i)
            // 即 slope 的调和平均数的倒数
            xtal_correct = (double)XERR_INIT_AVG / init_acc;
            xtal_correct_ok = true;
            ++init_cpt;
        } else {
            // 稳态跟踪：一阶 IIR 低通滤波（系数 1/XERR_FILT_COEF=1/256）
            double x = 1.0 / xtal_err_cpy;
            xtal_correct = xtal_correct
                         - xtal_correct / XERR_FILT_COEF
                         + x / XERR_FILT_COEF;
            // 等价于：xtal_correct = xtal_correct * (1 - α) + x * α，α = 1/256
        }
    }
}
```

`xtal_correct` 是 `1/xtal_err` 的低通滤波值（1/slope），
而不是 `xtal_err`（slope）本身——因为时间转换公式中除以 `xtal_err` 等同于乘以 `xtal_correct`：

```c
// lgw_cnt2utc() 中：
delta_sec = (count_us - ref.count_us) / (TS_CPS * ref.xtal_err);

// beacon 频率校正中（使用平滑后的 xtal_correct）：
pkt.freq_hz = (unsigned int)(xtal_correct * (double)pkt.freq_hz);
```

---

## 九、代码解析：thread_up 时间字段生成

> 源文件：`lora_pkt_fwd.c`，函数 `thread_up()`，约 L2333 起

每收到一批上行包，`thread_up` 先快照一份 GPS 时间参考，再为每个包生成时间字段：

```c
// thread_up() - GPS 参考快照
if ((nb_pkt > 0) && (gps_enabled == true)) {
    xSemaphoreTake(mx_timeref, portMAX_DELAY);
    ref_ok = gps_ref_valid;       // 参考是否有效（最近 30 秒内成功同步过）
    local_ref = time_reference_gps; // 深拷贝，避免处理过程中被 gps_process_sync 修改
    xSemaphoreGive(mx_timeref);
} else {
    ref_ok = false;
}

// 对每个收到的包 p：

// tmst：始终写入（无需 GPS）
j = snprintf(..., ",\"tmst\":%u", p->count_us);
// p->count_us = SX1302 INST 计数器，帧接收时刻

if (ref_ok == true) {
    // ── time 字段（UTC）──
    j = lgw_cnt2utc(local_ref, p->count_us, &pkt_utc_time);
    // 内部计算：delta_sec = (p->count_us - local_ref.count_us) / (1E6 * xtal_err)
    //          pkt_utc_time = local_ref.utc + delta_sec
    if (j == LGW_GPS_SUCCESS) {
        x = gmtime(&pkt_utc_time.tv_sec);
        snprintf(..., ",\"time\":\"%04i-%02i-%02iT%02i:%02i:%02i.%06liZ\"",
            x->tm_year+1900, x->tm_mon+1, x->tm_mday,
            x->tm_hour, x->tm_min, x->tm_sec,
            pkt_utc_time.tv_nsec / 1000);  // 精确到微秒
    }

    // ── tmms 字段（GPS 纪元毫秒）──
    j = lgw_cnt2gps(local_ref, p->count_us, &pkt_gps_time);
    // 内部计算：delta_sec = (p->count_us - local_ref.count_us) / (1E6 * xtal_err)
    //          pkt_gps_time = local_ref.gps + delta_sec
    if (j == LGW_GPS_SUCCESS) {
        pkt_gps_time_ms = pkt_gps_time.tv_sec * 1000
                        + pkt_gps_time.tv_nsec / 1000000;
        // = 1456477117685 ms（修复后）
        snprintf(..., ",\"tmms\":%" PRIu64 "", pkt_gps_time_ms);
    }
}
// ref_ok == false 时：time 和 tmms 字段均不写入 JSON
```

### 9.1 time 字段数值验证（实际日志）

```
INFO: [gps] synced UTC time: 2026-03-02T08:58:19.000Z  (trig_tstamp=201660610)

收到帧：p->count_us = 202346228
同步点：local_ref.count_us = 201660610，local_ref.utc.tv_sec = 2026-03-02T08:58:19

delta_us = 202346228 - 201660610 = 685618 µs
delta_sec = 685618 / 1E6 / 1.0 = 0.685618 s

pkt_utc_time = 2026-03-02T08:58:19 + 0.685618 = 2026-03-02T08:58:19.685618

JSON: "time":"2026-03-02T08:58:19.685616Z"  ✅（与日志一致，±2µs 误差）
```

### 9.2 tmms 字段数值验证

```
local_ref.gps.tv_sec = 1456476317（来自 lgw_gps_sync，修复后正确）
delta_sec = 0.685618 s

pkt_gps_time.tv_sec = 1456476317 + 0 = 1456476317（delta 整数部分为 0）
pkt_gps_time.tv_nsec = 685618000 ns

pkt_gps_time_ms = 1456476317 * 1000 + 685618000 / 1000000
                = 1456476317000 + 685
                ≈ 1456477117685 ms（注意同步点每次更新）

JSON: "tmms":1456477117685  ✅
```

---

## 十、代码解析：thread_down 下行时间调度

> 源文件：`lora_pkt_fwd.c`，函数 `thread_down()`

### 10.1 Class A：使用 tmst（计数器绝对值）

NS 在收到上行后，计算 RX1/RX2 窗口的计数器值并下发 `tmst` 字段：

```json
// NS 下发的 txpk（Class A，RX1 窗口）
{"txpk":{"tmst":203346228,"freq":507.7,...}}
```

```c
// thread_down() 处理：
val = json_object_get_value(txpk_obj, "tmst");
if (val != NULL) {
    txpkt.count_us = (unsigned int)json_value_get_number(val);
    // = 203346228（直接作为 SX1302 发射时刻）
    downlink_type = JIT_PKT_TYPE_DOWNLINK_CLASS_A;
}
// JIT 队列在 count_us = 203346228 时触发 lgw_send()
```

RX1 时刻计算（由 NS 完成，网关无需参与）：
```
RX1_tmst = uplink_tmst + RX1Delay × 1E6
         = 202346228 + 1 × 1,000,000
         = 203346228
```

### 10.2 Class B：使用 tmms（GPS 纪元毫秒）

```c
val = json_object_get_value(txpk_obj, "tmms");
if (val != NULL) {
    uint64_t x2 = (uint64_t)json_value_get_number(val);

    // 毫秒 → struct timespec
    double x3, x4;
    x3 = modf((double)x2 / 1E3, &x4);
    gps_tx.tv_sec  = (time_t)x4;        // 整数秒部分
    gps_tx.tv_nsec = (long)(x3 * 1E9);  // 小数秒部分（纳秒）

    // GPS 纪元时间 → SX1302 计数器值
    i = lgw_gps2cnt(local_ref, gps_tx, &txpkt.count_us);
    // 内部：delta = gps_tx - ref.gps
    //       count_us = ref.count_us + delta × 1E6 × xtal_err
    downlink_type = JIT_PKT_TYPE_DOWNLINK_CLASS_B;
}
```

Class B 必须 `gps_ref_valid == true`，否则网关返回 `JIT_ERROR_GPS_UNLOCKED` 拒绝发送。

---

## 十一、时间转换函数详解

`loragw_gps.c` 提供四个时间转换函数，构成双向映射：

```
SX1302 计数器 (count_us, uint32_t)
        ↕
  lgw_cnt2utc()  ←→  lgw_utc2cnt()
        ↕
  UTC (struct timespec，Unix 纪元 1970-01-01)

SX1302 计数器 (count_us, uint32_t)
        ↕
  lgw_cnt2gps()  ←→  lgw_gps2cnt()
        ↕
  GPS 纪元时间 (struct timespec，GPS 纪元 1980-01-06)
```

### 11.1 lgw_cnt2utc()

```c
int lgw_cnt2utc(struct tref ref, unsigned int count_us, struct timespec *utc) {
    // 有效性检查：参考必须被初始化过，且 xtal_err 在合理范围内
    if (ref.systime == 0 || ref.xtal_err > PLUS_10PPM || ref.xtal_err < MINUS_10PPM)
        return LGW_GPS_ERROR;

    // delta_sec：从参考时刻到目标时刻的时间差（已做晶振校正）
    double delta_sec = (double)(count_us - ref.count_us) / (TS_CPS * ref.xtal_err);
    // 注：count_us 和 ref.count_us 都是 uint32_t，减法自动处理 32-bit 回绕

    double intpart, fractpart;
    fractpart = modf(delta_sec, &intpart);

    long tmp = ref.utc.tv_nsec + (long)(fractpart * 1E9);
    if (tmp < (long)1E9) {
        utc->tv_sec  = ref.utc.tv_sec + (time_t)intpart;
        utc->tv_nsec = tmp;
    } else {  // 纳秒进位
        utc->tv_sec  = ref.utc.tv_sec + (time_t)intpart + 1;
        utc->tv_nsec = tmp - (long)1E9;
    }
    return LGW_GPS_SUCCESS;
}
```

### 11.2 lgw_cnt2gps()

与 `lgw_cnt2utc()` 完全对称，只是基准从 `ref.utc` 换成 `ref.gps`：

```c
int lgw_cnt2gps(struct tref ref, unsigned int count_us, struct timespec *gps_time) {
    double delta_sec = (double)(count_us - ref.count_us) / (TS_CPS * ref.xtal_err);

    // 以 ref.gps 为基准（修复前 ref.gps = {0,0}，修复后为正确 GPS 纪元时间）
    long tmp = ref.gps.tv_nsec + (long)(fractpart * 1E9);
    gps_time->tv_sec  = ref.gps.tv_sec + (time_t)intpart [+ 进位];
    gps_time->tv_nsec = tmp [- 1E9 进位];
}
```

### 11.3 三个时间纪元的换算关系

```
Unix 纪元   1970-01-01 00:00:00 UTC
            ↕ +315964800 秒（约 10 年，精确到秒）
GPS 纪元    1980-01-06 00:00:00 GPS
            ↕ GPS = UTC + 18 秒（闰秒，2017年至今有效）

公式汇总：
  GPS_sec   = Unix_sec - 315964800 + 18
  Unix_sec  = GPS_sec  + 315964800 - 18
  tmms (ms) = GPS_sec × 1000 + GPS_nsec / 1,000,000

数值示例（2026-03-02T08:58:19 UTC）：
  Unix_sec  = 1772441099
  GPS_sec   = 1772441099 - 315964800 + 18 = 1456476317
  tmms ≈ 1456476317 × 1000 + 685 = 1456476317685
```

---

## 十二、ChirpStack 层：NS 视角的时间字段

ChirpStack 收到网关上报的 `rxpk` JSON 后，在上行事件中暴露以下时间字段：

| 字段 | 来源 | 说明 |
|------|------|------|
| `gwTime` | rxpk `time` 字段（网关测量） | 网关测量的帧到达 UTC 时间，精确到微秒 |
| `nsTime` | ChirpStack 服务器系统时钟 | NS 收到包的时刻（与 gwTime 有几十 ms 差） |
| `timeSinceGpsEpoch` | rxpk `tmms` 字段换算 | `tmms / 1000` 秒，用于 Class B 调度 |

实测数据（BUG-011 修复后，2026-03-02）：

```
gwTime:             2026-03-02T08:58:41.731513+00:00
nsTime:             2026-03-02T08:58:41.707673068+00:00
timeSinceGpsEpoch:  1456477139.731s
```

`gwTime - nsTime ≈ 24 ms`：UDP 单向传输延迟，正常范围。

**timeSinceGpsEpoch 换算验证**：

$$\frac{1456477139}{86400 \times 7} = 2409.28\ \text{GPS 周}$$

$$1980\text{-}01\text{-}06 + 2409.28\ \text{周} \approx 2026\text{-}03\text{-}02\ \checkmark$$

---

## 十三、PPS 的作用与缺失的影响

### 13.1 有 PPS 时的同步精度

```
GPS PPS 上升沿（绝对精度 ±100 ns，受卫星信号质量影响）
       ↓
SX1302 硬件检测上升沿，将 INST 计数器值写入 PPS 寄存器
  TIMESTAMP_PPS = TIMESTAMP_INST（锁存瞬间，同步精度 ±1 µs）
       ↓
lgw_get_trigcnt(&trig_tstamp) 读出快照值
       ↓
gps_process_sync() 调用 lgw_gps_sync(trig_tstamp, utc, gps_time)
  建立锚点：count_us = trig_tstamp 对应 utc（UTC 整秒）

精度分析：
  trig_tstamp 误差：±1 µs（SX1302 计数器分辨率）
  utc 误差：       ±5 ms（NMEA 字符从 GPS 传来的串口延迟）
  综合精度：以计数器为准，约 ±1 µs
  实测验证：相邻 trig_tstamp 差值 = 1,000,001 µs（几乎精确 1 秒）
```

### 13.2 无 PPS 时的退化行为

```
TIMESTAMP_PPS 寄存器从不更新
       ↓
lgw_get_trigcnt() 返回 0（寄存器初始值）
       ↓
gps_process_sync() 中：
  cnt_diff = (0 - 0) = 0  (第一次)
  slope = 0 / 1 = 0  → 异常（< MINUS_10PPM）

  每次 lgw_gps_sync 都会：
    第1次异常：保留旧参考
    第2次异常：保留旧参考
    第3次异常（连续）：强制重置
      ref.count_us = 0
      ref.utc = 当前 NMEA UTC
      ref.gps = gps_time（修复后正确，修复前 {0,0}）
       ↓
之后每次都重复上面过程，ref.count_us 一直是 0，ref.utc 一直更新

效果（ref.count_us = 0）：
  delta_sec = count_us / 1E6（设备运行时间）
  time = ref.utc + delta（UTC 基准每秒更新，所以 time 仍然正确）
  tmms = ref.gps + delta
       = gps_time + delta（修复后 gps_time 正确）
       = gps_time + 设备运行时间（有几秒的绝对误差，但量级正确）
```

### 13.3 PPS 健康检查方法

```
连接 PPS：
  trig_tstamp[n+1] - trig_tstamp[n] ≈ 1,000,000 µs（±10 µs 内）

未连接 PPS：
  trig_tstamp 不变 → delta = 0

诊断命令（从串口日志）：
  "INFO: [gps] synced UTC time: ...  (trig_tstamp=201660610)"
  看相邻两行的 trig_tstamp 差值
```

---

## 十四、BUG-011 根因与修复

### 14.1 问题链

```
ATGM336H 是 NMEA-only 模块（无 u-blox 专有协议）
       ↓ （从不输出 UBX_NAV_TIMEGPS）
loragw_gps.c 中的静态全局变量：
  gps_week = 0    ← 只由 UBX NAV-TIMEGPS 消息更新
  gps_iTOW = 0    ← 同上
  gps_fTOW = 0    ← 同上
（这三个变量在设备整个运行期间永远是 0）
       ↓
lgw_gps_get() 的 gps_time 分支（修复前）：
  gps_time->tv_sec  = iTOW/1000 + week*604800 = 0 + 0 = 0
  gps_time->tv_nsec = 0
  → gps_time = {0, 0}（GPS 纪元 1980-01-06 00:00:00）
       ↓
lgw_gps_sync() 将 {0, 0} 写入 ref.gps：
  time_reference_gps.gps = {0, 0}
       ↓
lgw_cnt2gps(ref, count_us, &pkt_gps_time)：
  pkt_gps_time = {0, 0} + delta_sec
               = delta_sec（设备运行时间，单位秒）
       ↓
pkt_gps_time_ms = pkt_gps_time.tv_sec * 1000 + ...
                = delta_sec * 1000 ms
                ≈ 1138 ms（设备运行约 1.138 秒时收到的帧）
       ↓
JSON: "tmms":1138
       ↓
ChirpStack: timeSinceGpsEpoch = "1.138s"
            换算 = 1980-01-06T00:00:01.138 GPS（1980 年！）
```

### 14.2 修复逻辑

判断依据：`gps_week == 0` → NMEA-only 模式，从 NMEA 时间字段派生 GPS 纪元时间：

```
NMEA $GNRMC 解析结果（全局变量）：
  gps_yea=26, gps_mon=3, gps_day=2, gps_hou=8, gps_min=58, gps_sec=19, gps_fra=0.685
       ↓
struct tm gps_tm = {.tm_year=126, .tm_mon=2, .tm_mday=2, .tm_hour=8, ...}
       ↓
gps_unix = mktime(&gps_tm) = 1772441099（Unix 时间戳）
       ↓
gps_time->tv_sec = 1772441099 - 315964800 + 18 = 1456476317
gps_time->tv_nsec = 685000000（来自 gps_fra）
```

### 14.3 新增常量

```c
// loragw_gps.c 私有常量区（修复中新增）
#define UNIX_GPS_EPOCH_OFFSET  315964800  // 1970-01-01 到 1980-01-06 的秒数
#define GPS_LEAP_SECONDS       18         // GPS 比 UTC 快 18 秒（2017年至今）
```

这两个常量将原来散落在代码中的魔法数字集中定义，便于未来（如闰秒调整时）维护。

---

## 十五、时间系统常见问题 FAQ

**Q: tmst 和 tmms 有什么区别？**

A: `tmst` 是**相对**时间（网关上电后的微秒数），`tmms` 是 GPS 纪元**绝对**时间（毫秒）。
不同网关的 `tmst` 无法直接比较，但 `tmms` 可以用于多网关 TDOA 定位。

---

**Q: 没有 GPS 时 time 字段还有吗？**

A: `gps_ref_valid = false` 时 `time` 和 `tmms` 字段均不写入 JSON。
ChirpStack NS 会用 `nsTime`（服务器收包时刻）代替。

---

**Q: GPS 没有定位但有时间信号，time 字段有吗？**

A: 有。`gps_process_sync()`（时间）和 `gps_process_coords()`（坐标）是独立的。
只要 `$GNRMC` 中 fix 状态为 `A`（Active），`gps_time_ok=true`，时间参考就能建立。
坐标信息只影响 `stat` JSON 中的 `lati`/`long`/`alti` 字段。

---

**Q: 闰秒 18 秒会变吗？**

A: 闰秒由 IERS 决定，最近一次是 2017 年 1 月 1 日（第 18 次）。
下次调整（如有）需手动更新 `loragw_gps.c` 中的 `GPS_LEAP_SECONDS` 常量。
IERS 提前 6 个月发布公告，截至 2026 年 18 秒仍正确。

---

**Q: xtal_err 偏离 1.0 会怎样？**

A: `lgw_gps_sync()` 设置 ±10 ppm 安全阈值（`PLUS_10PPM=1.00001`，`MINUS_10PPM=0.99999`）。
超过范围的点被标记为"异常"，连续 3 次异常后强制重置。
正常晶振偏差约 ±2~5 ppm，远在安全范围内。

---

**Q: Class A 节点入网需要 GPS 时间吗？**

A: 不需要。Class A 的下行窗口用 `tmst`（相对时间）调度：
`RX1_count_us = uplink_tmst + 1,000,000`（1 秒后）。
GPS 时间只影响 `time`/`tmms` 上报字段以及 Class B/Class C GPS 调度。

---

**Q: 为什么 time 字段精确到微秒而 NMEA 只有毫秒？**

A: `time` 字段的精度来自 SX1302 计数器（µs 级），不是 NMEA 字符串。
计算路径是：`delta_us = p->count_us - ref.count_us`（µs 精度），
再加到 `ref.utc`（ms 精度的 NMEA 时间）上，得到 µs 级的绝对时间。
实际上 NMEA 时间的 ms 精度是锚点误差，µs 级的相对精度来自晶振。

---

## 相关文档

- [BUG-011 修复记录](../bugfix/BUG-011_tmms_wrong_gps_epoch_nmea_only.md)
- [GPS PPS + tmms 验证测试](../test_notes/test_gps_pps_tmms_chirpstack_memo.md)
- [ESXP1302 代码全貌](esxp1302_code_walkthrough.md)
