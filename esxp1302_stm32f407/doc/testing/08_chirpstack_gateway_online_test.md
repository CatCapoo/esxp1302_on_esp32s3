# 08 — ChirpStack 网关上线测试与 stat 时间戳 Bug 修复

## 概述

本文档记录将 ESXP1302（STM32F407 + SX1302 + W5500）接入 **ChirpStack v4** 网络服务器的调试过程，以及发现并修复的关键 Bug：**stat 报文 `time` 字段格式错误导致网关无法上线**。

相关代码修改：`lora_pkt_fwd.c`（commit `cbbbc34`）

---

## 测试环境

### 硬件与网络

```
STM32F407 + SX1302 + W5500
  IP: 192.168.71.110 (静态)
  EUI: AA555A00000021FB
  频段: CN470_10 (radio0=486.6MHz, radio1=487.4MHz)
         │
         │  UDP 1700（PUSH_DATA / PULL_DATA）
         ▼
Linux 主机 (192.168.71.100)
  ├─ chirpstack-gateway-bridge:4   (0.0.0.0:1700/udp → MQTT)
  ├─ chirpstack:4                  (0.0.0.0:8080/tcp)
  ├─ eclipse-mosquitto:2           (0.0.0.0:1883/tcp)
  └─ postgres:14 + redis:7
```

### 软件版本

| 组件 | 版本 |
|------|------|
| ChirpStack | v4 (Docker) |
| ChirpStack Gateway Bridge | v4 |
| Gateway Bridge 后端 | Semtech UDP (`0.0.0.0:1700/udp`) |
| Gateway Bridge MQTT prefix | `cn470_10` |

---

## 问题现象

在 ChirpStack 中注册网关（EUI: `AA555A00000021FB`，Region: `cn470_10`）后，网关始终显示 **离线（never seen）**，无法上线。

### 网关侧串口日志

```
[CFG] Loaded from Flash (valid).
INFO: [up] PUSH_DATA datagrams sent: 0 (0.00% acknowledged)
INFO: [down] PULL_DATA sent: 4 (100.00% acknowledged)
INFO: [down] PULL_ACK received in 5 ms
```

**关键观察**：
- `PULL_DATA sent: 4 (100.00% acknowledged)` — 说明网关与 NS 之间 UDP 通信完全正常，PULL_ACK 在 5ms 内收到
- `PUSH_DATA datagrams sent: 0` — 因为没有 RF 包到来，没有发送过 PUSH_DATA（这是正常的）

但统计 JSON 是会每隔 30 秒通过 PUSH_DATA 发送的，网关没上线说明即使发了 stat，NS 也没有接受。

---

## 根因定位

### 查看 Gateway Bridge 日志

由于 ChirpStack 运行在本机 Docker 中，直接查看容器日志：

```bash
docker logs --tail 50 chirpstack-docker-chirpstack-gateway-bridge-1
```

日志输出：

```
time="...Z" level=error msg="backend/semtechudp: could not handle packet"
  addr="192.168.71.110:1700"
  data_base64="Ap9pAKpVWgAAACH7eyJzdGF0Ijp7InRpbWUiOiIwMDowMDozNiIs..."
  error="parsing time \"\\\"00:00:36\\\"\" as \"\\\"2006-01-02 15:04:05 MST\\\"\": cannot parse \"00:00:36\\\"\" as \"2006\""
```

Base64 解码后的 stat JSON：

```json
{"stat":{"time":"00:00:36","rxnb":0,"rxok":0,"rxfw":0,"ackr":0.0,"dwnb":0,"txnb":0,"temp":0.0}}
```

**根因**：`time` 字段只有 `"00:00:36"`（设备运行时长 HH:MM:SS），而 ChirpStack Gateway Bridge 要求完整的日期时间格式 `"YYYY-MM-DD HH:MM:SS TZD"`（参考 Go 的时间解析模式 `"2006-01-02 15:04:05 MST"`）。Gateway Bridge 无法解析此格式，**每一条 PUSH_DATA 统计报文都被直接丢弃**，网关始终不显示上线。

### 原始代码

```c
// lora_pkt_fwd.c — 修复前
char stat_timestamp[24];
// ...
snprintf(stat_timestamp, sizeof stat_timestamp, "%02lu:%02lu:%02lu",
         (unsigned long)(up / 3600),
         (unsigned long)((up % 3600) / 60),
         (unsigned long)(up % 60));
// 产生：  "00:00:36"
// 需要：  "2000-01-01 00:00:36 GMT"
```

---

## 修复方案

### 方案选择

STM32F407 没有 RTC（或 RTC 未校时），无法获得真实 UTC 时间。可能的方案：

| 方案 | 优点 | 缺点 |
|------|------|------|
| 使用固定日期占位 `2000-01-01 HH:MM:SS GMT` | 简单，无依赖 | 时间不准，但 NS 只用于判断"是否在线" |
| 通过 SNTP 获取 UTC 时间 | 时间准确 | 需要 SNTP 客户端实现，增加复杂度 |
| 通过 GPS PPS 获取 UTC | 最准确 | GPS 模块尚未移植到 STM32 |

ChirpStack Gateway Bridge 接收 stat 后，只检查格式是否合法，**不校验时间值是否合理**。使用固定日期占位完全可行。

### 代码修复

```c
// lora_pkt_fwd.c — 修复后
char stat_timestamp[32];  // 原 24，扩大到 32（"2000-01-01 00:00:36 GMT" = 23字节 + NUL）
// ...
snprintf(stat_timestamp, sizeof stat_timestamp, "2000-01-01 %02lu:%02lu:%02lu GMT",
         (unsigned long)(up / 3600),
         (unsigned long)((up % 3600) / 60),
         (unsigned long)(up % 60));
// 产生：  "2000-01-01 00:00:36 GMT"  ✓
```

同时扩大了缓冲区：`"2000-01-01 00:00:36 GMT"` 为 23 个字节加 NUL，原来的 24 字节实际上刚好够，但扩到 32 字节增加安全余量。

> **注意**：OLED 显示用的是同名变量 `stat_timestamp`，但那段代码在内层循环中单独赋值为 `"Up HH:MM:SS"` 格式，**不受此修复影响**。

---

## 测试结果

### 烧录修复后固件，观察 Gateway Bridge 日志

```bash
sleep 40 && docker logs --tail 20 --since 60s chirpstack-docker-chirpstack-gateway-bridge-1
```

输出：

```
time="2026-03-07T11:29:40.227Z" level=info
  msg="integration/mqtt: publishing event"
  event=stats
  qos=0
  topic=cn470_10/gateway/aa555a00000021fb/event/stats
```

✅ Gateway Bridge 成功解析 stat 报文，通过 MQTT 发布到 `cn470_10/gateway/aa555a00000021fb/event/stats` topic，ChirpStack 主服务接收后将网关标记为在线。

### 网关连接状态日志（确认上线过程）

Gateway Bridge 日志中出现的连接状态序列：

```
[subscribe] topic="cn470_10/gateway/aa555a00000021fb/command/#"
[publish] state=conn topic=cn470_10/gateway/aa555a00000021fb/state/conn
[publish event] event=stats  topic=cn470_10/gateway/aa555a00000021fb/event/stats
```

1. 网关发送 PULL_DATA → Gateway Bridge 建立 UDP 会话 → MQTT subscribe command topic
2. 发布 `state/conn` (online 状态)
3. stat PUSH_DATA 到达 → 解析成功 → 发布 `event/stats`

---

## 涉及的相关知识

### Semtech UDP 协议 stat 报文格式规范

Semtech UDP Packet Forwarder 协议（`PROTOCOL.TXT`）中定义 stat 对象：

```json
{
  "stat": {
    "time": "2014-01-12 08:59:28 GMT",  // 要求: "YYYY-MM-DD HH:MM:SS TZD"
    "rxnb": 2,
    "rxok": 2,
    "rxfw": 2,
    "ackr": 100.0,
    "dwnb": 2,
    "txnb": 2
  }
}
```

协议文档原文：
> `"time"` UTC 'system' time of the gateway, ISO 8601 'expanded' format (e.g `2014-01-12 08:59:28 GMT`)

**ChirpStack Gateway Bridge 的实现**使用 Go 标准库 `time.Parse()` 解析，格式字符串固定为 `"2006-01-02 15:04:05 MST"`，仅支持这一种格式，对 `"00:00:36"` 这样的纯时间字符串没有降级处理，直接返回 error 并丢弃整个报文。

### stat 报文触发网关上线的机制

ChirpStack 判断网关上线的逻辑：

```
网关发送 PUSH_DATA (含 stat) 
  → Gateway Bridge 解析成功
  → Gateway Bridge 通过 MQTT 发布 event/stats
  → ChirpStack 主服务接收到 event/stats
  → 更新网关"最后在线时间"
  → UI 显示"在线"
```

如果 stat 格式错误被 Gateway Bridge 丢弃，整个链路断开，网关永远不上线。这也解释了为什么 `PULL_ACK 100%` 但网关就是不上线：PULL_DATA 是纯心跳，不包含业务数据，Gateway Bridge 无法从它推断网关状态。

### 其他不影响上线的情况

- `PUSH_DATA datagrams sent: 0`：没有 RF 包就没有 PUSH_DATA，这是**正常**的，不影响上线
- stat 报文通过 `thread_up` 每 30 秒发送一次，即使没有 RF 包也会发

### 可扩展方向

1. **SNTP 时钟同步**：W5500 已有 UDP 能力，可以实现 SNTP 客户端来获取准确的 UTC 时间，stat 中的 `time` 字段就能反映真实时间，便于与其他系统的时间戳对齐
2. **stat 内容扩展**：添加 `lati/long/alti` 字段（需 GPS）后，ChirpStack 地图页面可以显示网关地理位置
3. **Gateway Bridge 版本兼容性**：不同版本的 Gateway Bridge 对 stat 格式的要求可能不同，Semtech 官方实现使用 `strftime("%Y-%m-%d %H:%M:%S %Z")` 生成时间戳，可以作为参考

---

## 故障排查检查清单

当网关不上线时，按以下顺序检查：

| 步骤 | 检查项 | 工具 |
|------|--------|------|
| 1 | 确认 UDP 通信正常 | 网关串口：`PULL_ACK received` |
| 2 | 确认 Gateway Bridge 收到数据 | `docker logs chirpstack-docker-chirpstack-gateway-bridge-1` |
| 3 | 检查 Gateway Bridge 日志是否有 error | 关键词：`could not handle packet` |
| 4 | 解码 `data_base64` 查看实际发送内容 | `echo "<base64>" \| base64 -d` |
| 5 | 确认 MQTT topic 前缀与 ChirpStack 配置的 region 一致 | `docker logs ... \| grep topic` |
| 6 | 确认网关 EUI 在 ChirpStack 中已注册 | ChirpStack Web UI |
