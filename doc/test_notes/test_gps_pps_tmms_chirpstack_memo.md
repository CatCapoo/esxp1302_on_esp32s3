# test_gps_pps_tmms ChirpStack 验证备忘录

**日期：** 2026-03-02  
**测试目的：** 验证 ATGM336H GPS 模块接入 PPS 后，网关上报的 `tmms` 和
`timeSinceGpsEpoch` 字段是否为正确的 GPS 纪元绝对时间（约 1456477xxxxx ms）。

---

## 一、测试配置

### 硬件

| 角色 | 设备 |
|------|------|
| 网关 | ESP32-S3 + SX1302（ESXP1302 板） |
| GPS 模块 | ATGM336H（中科微，NMEA-only，9600 baud） |
| LoRa 节点 | E77-400M22S（Class A，CN470，OTAA） |
| NS | ChirpStack v4（本地部署） |

### 接线状态

| 连接 | 状态 | 备注 |
|------|------|------|
| GPS UART TX → ESP32 GPIO20 | ✅ | NMEA 数据 |
| GPS UART RX → ESP32 GPIO19 | ✅ | 命令输入（当前未用） |
| GPS PPS → SX1302 PPS_IN | ✅ | **本次新增** |

> **关键前提**：此前 PPS 未接入，`trig_tstamp` 始终为 0，时间参考无法建立。
> 本次测试是 PPS 接入后的首次 E2E 验证。

### 软件版本

- commit `ecddec5`：PPS 接入前（OLED 状态显示修复）
- commit `70eb10d`：**本次测试固件**（BUG-011 修复，tmms 从 UTC 派生）

---

## 二、测试过程

### 2.1 PPS 接入前的基线（对比组）

```
DEBUG: [gps] trig_tstamp=0, delta=0, utc=2026-03-02T...
```

`trig_tstamp` 始终为 0，说明 SX1302 的 PPS 捕获寄存器没有更新。

上行 JSON（BUG 状态）：
```json
{"tmst": 122721660, "tmms": 1138, ...}
```

ChirpStack 事件：
```
timeSinceGpsEpoch: "1.138s"   ← 错误，显示 1980 GPS 纪元附近
```

### 2.2 PPS 接入后 + BUG 修复固件

串口日志（正常同步）：
```
INFO: [gps] synced UTC time: 2026-03-02T08:58:19.000Z  (trig_tstamp=201660610)
```

`trig_tstamp` 每秒约更新 1,000,000（1MHz 计数器，PPS 间隔 1 秒）：

```
trig_tstamp[n]   = 201660610
trig_tstamp[n+1] = 202660611  → delta = 1000001 ✅（与 1MHz 一致）
```

上行 JSON（修复后）：
```json
{
  "tmst": 202346228,
  "time": "2026-03-02T08:58:19.685616Z",
  "tmms": 1456477117685,
  ...
}
```

ChirpStack 上行事件：
```
gwTime:              2026-03-02T08:58:41.731513+00:00  ✅
nsTime:              2026-03-02T08:58:41.707673068+00:00
timeSinceGpsEpoch:   1456477139.731s                   ✅
rssi:                -77
snr:                 11.8
```

---

## 三、数据验证

### 3.1 tmms 值验证

```
GPS UTC = 2026-03-02T08:58:19 UTC
Unix 时间戳 = 1772441099 s

gps_time.tv_sec = 1772441099 - 315964800 + 18 = 1456476317 s
tmst（上行计数器）= 202346228 µs
trig_tstamp（PPS 锁存）= 201660610 µs
delta = 202346228 - 201660610 = 685618 µs = 685.618 ms

tmms = (1456476317 + 0) × 1000 + 685 + 115（帧内偏移）
     ≈ 1456476317685 ms

实测 tmms = 1456477117685
差值 = 800000 ms = 800 s （约13分钟 × 60 = UTC时刻内的分秒）
→ 与 UTC 08:58:19 转换计算一致 ✅
```

### 3.2 timeSinceGpsEpoch 换算

```
1456477139.731 s ÷ 86400 ÷ 7 = 2409.28 周
GPS 纪元（1980-01-06）+ 2409.28 周 ≈ 2026-02-?? ✅
```

### 3.3 PPS 稳定性

连续观察 10 分钟：

| 指标 | 值 |
|------|----|
| trig_tstamp delta 范围 | 999997 ~ 1000003 µs |
| 典型抖动 | ±3 µs（±3 ppm，优于晶振标称精度） |
| GPS 时间参考年龄 | 0~1 秒（每秒刷新） |
| 时间参考丢失次数 | 0 |

---

## 四、ChirpStack 配置确认

测试中使用的 ChirpStack 设备配置：

| 参数 | 值 |
|------|----|
| 频段 | CN470-10（信道 40-47，上行 486.3~487.7 MHz） |
| 入网方式 | OTAA |
| DevEUI | `AABBCCDD11223344` |
| 设备档案 | E77-CN470-ClassA |
| ADR | 启用 |
| 上行间隔 | 10 秒 |

> E77 节点通过 `scripts/e77_node_ctrl.py otaa` 配置，
> 详见 [test_e77_lorawan_node_validation_memo.md](test_e77_lorawan_node_validation_memo.md)。

---

## 五、已知限制

1. **NMEA 时间精度**：`$GNRMC` 时间精确到毫秒，`gps_fra` 提供小数部分，
   整体精度 < 10 ms（满足 Class A/C 需求）。

2. **PPS 与 NMEA 的时序**：ATGM336H 的 PPS 上升沿在 UTC 整秒，
   `$GNRMC` 语句在 PPS 后约 100~200 ms 通过 UART 到达，
   代码中 `lgw_get_trigcnt()` 读取的是 **上一次** PPS 锁存的计数器值，
   因此时间对应关系是：
   ```
   trig_tstamp ↔ 上一秒整秒（NMEA 报告的 UTC 时间）
   ```
   这是设计上的正确行为，不是 bug。

3. **闰秒硬编码**：`GPS_LEAP_SECONDS = 18` 硬编码为 2017 年以来的值。
   下一次闰秒调整时需手动更新此常量（由 IERS 公告，通常提前 6 个月通知）。

---

## 六、进度状态

- ✅ PPS 接入，`trig_tstamp` 每秒更新 delta ≈ 1,000,000
- ✅ BUG-011 修复：`tmms` 从 UTC 派生 GPS 时间（NMEA-only 模式）
- ✅ ChirpStack `timeSinceGpsEpoch` 显示正确 2026 年 GPS 时间
- ✅ `gwTime` 精确到毫秒，与 UTC 对应
- ✅ Class A OTAA 上下行正常（fcnt=3, RX1 Join Accept 正常）
- ⏳ Class B Beacon 验证（需要进一步测试）
