# BUG-010：PUSH_DATA acknowledged 低（ackr 22–60%）

- **日期**：2026-02-21  
- **文件**：`main/packet_forwarder/lora_pkt_fwd.c`、`main/packet_forwarder/global_conf.json/global_conf.cn490.json`  
- **严重级别**：功能性错误（NS 无法正常确认上行包，网关统计 ackr 极低）
- **commit**：`55c1e82` fix: improve ackr by fixing WiFi PS, drain loop, and timeouts

---

## 现象

网关运行后 ackr（PUSH_DATA acknowledged）长期卡在 22–60%，
即使网络服务器日志显示 100% 收到并回复了 PUSH_ACK。

```
INFO: [up] PUSH_ACK received in 0 ms      ← 立刻收到（实为上轮遗留的过期 ACK）
WARNING: [up] ignored out-of sync ACK packet
PUSH_DATA acknowledged: 22.22%            ← 极低
PULL_DATA sent: 5 (100.00% acknowledged)  ← PULL_ACK 正常
```

---

## 根本原因（四个独立问题）

### 问题 1：MQTT TLS 连接阻塞（最严重）

`ENABLE_MQTT` 未关闭，`mqtt_task` 启动后尝试向 `mqtt://192.168.1.202` 建立 TLS 连接。
无 Broker 时 TLS 握手超时长达 **10 秒**，期间 FreeRTOS 调度器被占用，
`thread_up` 无法在 `recv()` 窗口内运行，ackr 跌至 22–40%。

**修复：**

```c
#define ENABLE_MQTT  0   /* set to 1 to enable MQTT, 0 to disable */
```

所有 MQTT 相关代码用 `#if ENABLE_MQTT ... #endif` 包裹，不启用时完全不编译。

---

### 问题 2：`struct timeval.tv_usec` 溢出（静默 bug）

`push_timeout_half` 的计算方式为：

```c
static struct timeval push_timeout_half = {0, (PUSH_TIMEOUT_MS * 500)};
```

`tv_usec` 合法范围是 `[0, 999999]`。当 `PUSH_TIMEOUT_MS = 2000` 时：

```
tv_usec = 2000 × 500 = 1,000,000  ← 溢出！
```

ESP32 lwIP 将无效的 `tv_usec = 1,000,000` 处理为 `0`，
`recv()` 每次立即返回 `EAGAIN`，永远等不到 ACK。

**修复：**

```c
// 原值（溢出）：
#define PUSH_TIMEOUT_MS  2000   // tv_usec = 1,000,000 → 溢出
#define PULL_TIMEOUT_MS  1000   // tv_usec = 1,000,000 → 溢出

// 修正后（安全）：
#define PUSH_TIMEOUT_MS  500    // tv_usec = 250,000 (250ms/次，共 500ms)
#define PULL_TIMEOUT_MS  400    // tv_usec = 400,000 (400ms)
```

同步修正 `global_conf.cn490.json`：

```json
// 原值（100ms 对任何真实网络都太短）：
"push_timeout_ms": 100

// 修正后：
"push_timeout_ms": 500
```

并重新生成 `main/global_json.h`。

---

### 问题 3：drain 循环 `setsockopt` 失败风险

每次发送新 PUSH_DATA 前，旧代码用双 `setsockopt` 清空 stale ACK：

```c
// 旧代码（有风险）：
struct timeval drain_tv = {0, 1000};
setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &drain_tv, sizeof drain_tv);
while (recv(sock_up, _tmp, sizeof _tmp, 0) > 0) {}
setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &push_timeout_half, sizeof push_timeout_half);
// ^ 若此次 setsockopt 失败，socket 超时永远卡在 1ms！
```

ESP32 lwIP 在内存压力下偶发 `setsockopt` 失败，第二次调用失败会导致
`sock_up` 的超时永久停在 1 ms，此后每轮 `recv()` 均立即超时。

**修复：** 改用 `MSG_DONTWAIT` 标志，完全不接触 `SO_RCVTIMEO`：

```c
// 新代码（安全）：
uint8_t _tmp[4];
while (recv(sock_up, (void *)_tmp, sizeof _tmp, MSG_DONTWAIT) > 0) {}
```

---

### 问题 4：WiFi Modem Sleep 导致 ACK 延迟到达（最关键）

ESP32 WiFi 默认开启 `WIFI_PS_MIN_MODEM`（最小调制解调器睡眠模式）。
WiFi 无线电会周期性休眠，AP 在此期间缓冲所有发往网关的单播包，
直到下一个 DTIM Beacon 才释放。典型缓冲延迟 **100–300 ms**。

交叉验证：NS Mock 服务器日志显示 100% 已发出 PUSH_ACK，
但网关 `recv()` 在 250 ms 窗口内仍未收到，说明 ACK 被 AP 缓冲到窗口之外。

**修复：** 在 `wifi_init_sta()` 中 `esp_wifi_start()` 之后立即禁用 Modem Sleep：

```c
ESP_ERROR_CHECK(esp_wifi_start());
/* Disable modem sleep so incoming UDP ACKs are not buffered by the AP.
 * Without this, ACKs can be delayed 100-300ms and miss the recv() window. */
ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
```

> **功耗说明**：网关整机（ESP32-S3 + SX1302）功耗约 400–600 mA，
> 禁用 Modem Sleep 增加约 20–50 mA（< 10%），
> 对常供电的基础设施设备可以忽略。

---

## 效果

| 阶段 | ackr 典型值 | 主要原因 |
|------|------------|---------|
| 修复前 | 22–40% | MQTT TLS 阻塞 |
| 禁用 MQTT 后 | 53–70% | 超时溢出 + WiFi PS |
| 修正超时值后 | 60–80% | WiFi PS |
| 禁用 WiFi PS 后 | **86–100%** | — |

---

## 验证方法

修复并烧录后，观察串口统计行：

```
PUSH_DATA acknowledged: 100.00%   ← 目标
PULL_DATA sent: N (100.00% acknowledged)
```

可用本地 Mock 脚本在局域网内做交叉对比验证
（详见 [test_pkt_fwd_ns_uplink_memo.md](../test_notes/test_pkt_fwd_ns_uplink_memo.md)）。

---

## 相关文件

- `main/packet_forwarder/lora_pkt_fwd.c`
- `main/packet_forwarder/global_conf.json/global_conf.cn490.json`
- `main/global_json.h`（重新生成）
- `scripts/lora_ns_mock.py`（本地验证工具）
