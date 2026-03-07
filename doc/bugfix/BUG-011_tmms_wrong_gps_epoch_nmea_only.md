# BUG-011：tmms 字段返回 1980 GPS 纪元附近时间（NMEA-only 模块）

- **日期**：2026-03-02  
- **文件**：`main/libloragw/loragw_gps.c`  
- **严重级别**：功能性错误（ChirpStack `timeSinceGpsEpoch` 显示为 1.138s/523s，应为 ~1456477139s）  
- **commit**：`70eb10d` fix: derive GPS time from NMEA UTC for NMEA-only modules (ATGM336H)

---

## 现象

PPS 已接入并正常工作（`trig_tstamp` 每秒更新，`delta=1000001`），
但 ChirpStack NS 的上行事件中：

```
timeSinceGpsEpoch: "1.138s"         ← 应为约 1456477139s（2026-03-02）
gwTime: "2026-03-02T08:58:41Z"      ← 正常（来自系统时钟，不依赖 GPS 时间）
```

网关上行 JSON 中 `tmms` 字段：

```json
{"tmms": 523537, ...}   ← 应为约 1456477117685（13 位毫秒数）
```

`523537 ms ÷ 1000 = 523 s`——正好等于设备启动后的运行时间，而非 GPS 纪元绝对时间。

---

## 根本原因

`loragw_gps.c` 中维护两套独立的全局时间变量：

| 变量 | 由谁更新 | ATGM336H 是否更新 |
|------|----------|------------------|
| `gps_yea/mon/day/hou/min/sec` | NMEA `$GNRMC` 解析 | ✅ 每秒更新 |
| `gps_week` / `gps_iTOW` / `gps_fTOW` | UBX `NAV-TIMEGPS` 解析 | ❌ 永远为 0 |

原始 `lgw_gps_get()` 的 `gps_time` 分支（修复前）：

```c
/* 修复前：直接使用 gps_week/iTOW/fTOW 变量 */
fractpart = modf(((double)gps_iTOW / 1E3) + ((double)gps_fTOW / 1E9), &intpart);
gps_time->tv_sec  = (time_t)intpart;
gps_time->tv_sec += (time_t)gps_week * 604800;   // gps_week=0 → tv_sec=0
gps_time->tv_nsec = (long)(fractpart * 1E9);
```

ATGM336H 是 NMEA-only 模块（无 u-blox UBX 协议），`gps_week` 始终为 0，
`gps_iTOW` 始终为 0，导致 `gps_time = {0, 0}`（GPS 纪元 1980-01-06）。

`lgw_gps_sync()` 将此 `{0,0}` 写入 `time_reference_gps.gps`，
随后 `lgw_cnt2gps()` 计算 `tmms` 时：

```
gps_time = ref.gps + delta_from_ref
         = {0, 0}  + (count_us - ref.count_us) / TS_CPS
         ≈ 设备运行时间（秒）
```

**结论**：`tmms` ≈ 设备运行时间 × 1000（毫秒），而非 GPS 纪元绝对毫秒数。

### 调试验证

加入 debug 日志后，以下数据互相吻合：

```
trig_tstamp = 201660610
tmst (uplink count_us) = 202346228
delta from ref = 202346228 - 201660610 = 685618 µs = 685 ms
tmms = 1138 ms  ← 说明 ref.gps.tv_sec = 0，tmms 只有 delta 分量
```

---

## 修复方案

在 `lgw_gps_get()` 中，以 `gps_week` 是否为非零值来区分 UBX 模式和 NMEA-only 模式：

```c
if (gps_week != 0) {
    /* UBX 模式：使用 iTOW + week（原逻辑，精度高，纳秒级） */
    fractpart = modf(((double)gps_iTOW / 1E3) + ((double)gps_fTOW / 1E9), &intpart);
    gps_time->tv_sec  = (time_t)intpart + (time_t)gps_week * 604800;
    gps_time->tv_nsec = (long)(fractpart * 1E9);
} else {
    /* NMEA-only 模式：从 NMEA 日期时间字段派生 GPS 时间
       公式：GPS_time = UTC_unix - UNIX_GPS_EPOCH_OFFSET + GPS_LEAP_SECONDS */
    struct tm gps_tm = {0};
    gps_tm.tm_year = (gps_yea < 100) ? gps_yea + 100 : gps_yea - 1900;
    gps_tm.tm_mon  = gps_mon - 1;
    gps_tm.tm_mday = gps_day;
    gps_tm.tm_hour = gps_hou;
    gps_tm.tm_min  = gps_min;
    gps_tm.tm_sec  = gps_sec;
    time_t gps_unix = mktime(&gps_tm);
    gps_time->tv_sec  = gps_unix - UNIX_GPS_EPOCH_OFFSET + GPS_LEAP_SECONDS;
    gps_time->tv_nsec = (int32_t)(gps_fra * 1e9);
}
```

同步新增两个具名常量（取代魔法数字）：

```c
#define UNIX_GPS_EPOCH_OFFSET   315964800   /* 1970-01-01 到 1980-01-06 的秒数差 */
#define GPS_LEAP_SECONDS        18          /* GPS-UTC 闰秒偏移（2017 年至今有效） */
```

---

## 效果

| 字段 | 修复前 | 修复后 |
|------|--------|--------|
| `tmms` | `523537`（ms，约 8 分钟） | `1456477117685`（ms，2026-03-02） |
| `timeSinceGpsEpoch` | `"1.138s"` | `"1456477139.731s"` |
| `gwTime` | `"2026-03-02T..."` ✅（不受影响） | `"2026-03-02T..."` ✅ |

换算验证：

```
gps_utc_unix = 1772441099  （2026-03-02T08:58:19 UTC 的 Unix 时间戳）
gps_time.tv_sec = 1772441099 - 315964800 + 18 = 1456476317

tmms = (gps_time.tv_sec + delta) × 1000 + ms
     ≈ 1456477117685 ✅
```

---

## 适用范围

此修复专为 NMEA-only GPS 模块设计（如 ATGM336H、中科微 AT6558 系列等），
对 u-blox 模块（`gps_week != 0`）不产生任何影响，原逻辑完整保留。

---

## 相关文件

- `main/libloragw/loragw_gps.c`（主要修改）
- `main/board_config.h`（同步更新，新增 GPS 相关配置注释）
- 测试记录：[test_gps_pps_tmms_chirpstack_memo.md](../test_notes/test_gps_pps_tmms_chirpstack_memo.md)
- 学习笔记：[esxp1302_timestamp_system.md](../learning/esxp1302_timestamp_system.md)
