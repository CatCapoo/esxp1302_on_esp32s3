# test_pkt_fwd NS 上行对接测试备忘录

**日期：** 2026-02-21  
**测试目的：** 验证网关通过 UDP（Semtech 协议）与 NS 的完整对接流程，
重点排查 `PUSH_DATA acknowledged` 低、`out-of-sync ACK` 告警的根因并验证修复效果。

---

## 一、测试配置

### 硬件

| 角色 | 设备 |
|------|------|
| 网关 | ESP32-S3 + SX1302（ESXP1302 板） |
| 发包端 | 原始 LoRa 发包仪（SX1278，非 LoRaWAN 节点） |
| NS | ChirpStack（远端，`10.67.9.238:1700`） |

### 网络环境

| 参数 | 值 |
|------|----|
| 上网方式 | **手机热点（4G）** |
| 网关 IP | 10.67.9.11（DHCP） |
| NS IP | 10.67.9.238:1700 |
| 实测 UDP RTT | ~270–400 ms（高延迟，波动大） |

> ⚠️ **手机热点注意事项**  
> - UDP RTT 典型值 300–400 ms，远高于有线或企业 WiFi（< 50 ms）  
> - 运营商 CGNAT 不保证 UDP 包不乱序/不丢包  
> - NAT 表 UDP 超时通常约 30 s，长时间无包会导致映射失效  
> - 生产环境应使用有线/稳定 WiFi，手机热点仅适合调试

### 频率配置（CN490，存 NVS）

| 参数 | 值 |
|------|----|
| Radio 0 中心频率 | 480.400 MHz |
| Radio 1 中心频率 | 481.200 MHz |
| 测试信道 | 2（480.500 MHz，Radio 0 +100 kHz） |

---

## 二、测试现象

### 2.1 问题现象（修复前）

```
INFO: [up] PUSH_ACK received in 0 ms          ← 立刻收到，实为上轮遗留的过期 ACK
WARNING: [up] ignored out-of sync ACK packet  ← 反复出现，约 3 次 / 分钟
INFO: [down] PULL_ACK received in 272 ms
INFO: [down] PULL_ACK received in 382 ms
PUSH_DATA acknowledged: 22.22%               ← 统计窗口内极低
```

典型规律：
- 第 1 轮：`PUSH_ACK received in 0 ms`（瞬间收到旧包，实为上一轮过期 ACK）
- 第 2–4 轮：`WARNING: out-of sync`（真正的 ACK 在超时后才到，token 已换）
- 偶发 1 轮正常确认

### 2.2 修复后现象（预期）

```
INFO: [up] PUSH_ACK received in 320 ms        ← 在超时窗口内正常收到
INFO: [down] PULL_ACK received in 280 ms
PUSH_DATA acknowledged: 100.00%              ← 统计稳定
```

---

## 三、根本原因分析

### 原因 1：socket 路由 bug（代码缺陷，与网络无关）

`sock_up` 和 `sock_down` 均未调用 `connect()`。
`recvfrom(..., &dest_addr, &socklen)` 将**回包发送方地址写入全局 `dest_addr`**，
后续 `sendto(..., &dest_addr, ...)` 的目标地址因此被破坏。

此外无 `connect()` 时 OS 无法按五元组路由，PULL_ACK 可能被 `sock_up` 抢先读走，
导致 token 不匹配。

**修复：** 启用 `connect()`，将 `sendto/recvfrom` 全部替换为 `send/recv`。

### 原因 2：ACK 超时窗口远小于 NS 实际延迟

| 参数 | 原值 | 修复后 |
|------|------|--------|
| `PUSH_TIMEOUT_MS` | 100 ms | 2000 ms |
| `PULL_TIMEOUT_MS` | 200 ms | 1000 ms |
| `push_timeout_half`（实际 per-recv） | 50 ms | 1000 ms |
| 总等待（2 次循环） | **100 ms** | **2000 ms** |
| NS 实际 RTT | ~300–400 ms | — |

原代码总等待 100 ms 远小于 NS RTT 300–400 ms，
ACK 每轮都超时被丢入缓冲区，下一轮 `recv()` 读出时 token 已是新值 → out-of-sync。

### 原因 3：stale ACK 污染（上轮超时遗留）

即使扩大了超时窗口，若某轮因偶发丢包超时，
下一轮发出 PUSH_DATA 之前缓冲区中仍有旧 ACK 等待被读。
新轮 `recv()` 优先读到旧 ACK → token 不匹配。

**修复：** 每次发送新 PUSH_DATA 之前用 1 ms 超时非阻塞 drain 清空 `sock_up` 缓冲区。

---

## 四、修复内容（`lora_pkt_fwd.c`）

| 位置 | 修改 | 原因 |
|------|------|------|
| socket 初始化 | 启用 `connect(sock_up/sock_down, &dest_addr, ...)` | 绑定对端，OS 精确路由回包 |
| `send_tx_ack()` | `sendto → send` | `connect()` 后无需重复指定对端地址 |
| `thread_up` 发送 | `sendto → send` | 同上 |
| `thread_up` 接收 | `recvfrom(&dest_addr) → recv` | 避免 dest_addr 被回包地址覆盖 |
| `thread_down` 发送 | `sendto → send` | 同上 |
| `thread_down` 接收 | `recvfrom(&dest_addr) → recv` | 同上 |
| `PUSH_TIMEOUT_MS` | `100 → 2000` | 覆盖手机热点延迟 |
| `PULL_TIMEOUT_MS` | `200 → 1000` | 同上 |
| `thread_up` 发送前 | 新增 1 ms drain 清空 stale ACK | 防止上轮超时遗留 ACK 污染 token 校验 |

---

## 五、本地调试工具：UDP NS Mock 脚本

当无法连接远端 NS（ChirpStack 不可用、网络不稳定）时，
可用本地 Mock 脚本模拟 NS 响应，快速验证网关上行链路。

### 5.1 脚本位置

```
scripts/lora_ns_mock.py
```

### 5.2 功能

- 监听 UDP，回应 `PUSH_DATA` → `PUSH_ACK`（token 匹配）
- 回应 `PULL_DATA` → `PULL_ACK`（token 匹配）
- 打印收到的 uplink JSON payload（含 rxpk）
- 打印统计信息（包数、ACK 率等）

### 5.3 使用方法

**步骤 1：** 在本机（与网关同一局域网）运行 Mock 服务器：

```bash
python3 scripts/lora_ns_mock.py 1700
```

**步骤 2：** 查到本机 IP，例如 `10.67.9.100`。

**步骤 3：** 在网关 CLI 将 NS 地址改为本机 IP：

```
pkt_fwd --host 10.67.9.100 --port 1700
```

设备自动保存到 NVS 并重启。

**步骤 4：** 观察 Mock 服务器输出，确认 PUSH_DATA/PULL_DATA 均已到达并 ACK。

### 5.4 预期输出示例

```
[NS Mock] Listening on UDP 0.0.0.0:1700
[NS Mock] PULL_DATA  from ('10.67.9.11', 52314) token=0xA3B2 -> PULL_ACK sent
[NS Mock] PUSH_DATA  from ('10.67.9.11', 52313) token=0x1C7F -> PUSH_ACK sent
[NS Mock]   rxpk[0]: freq=480.5 sf=SF12BW125 rssi=-70 data=aGVsbG8gd29ybGQ=
[NS Mock] PUSH_DATA  from ('10.67.9.11', 52313) token=0x4D21 -> PUSH_ACK sent
```

### 5.5 注意事项

- 脚本需与网关在**同一局域网**（或有路由可达）
- 本机防火墙需放行 UDP 1700 端口
- Mock 不实现下行，仅用于验证上行链路
- MQTT 默认已通过 `ENABLE_MQTT=0` 宏禁用，无需 Broker 即可测试

---

## 六、与 ChirpStack 对接注意事项

1. **注册网关 EUI**：ChirpStack → Gateways → Add，EUI 填 `AA555A00000021FB`
2. **确认 gateway bridge 端口**：NS 侧需监听 UDP 1700（Semtech UDP 协议）
3. **检查 NS 是否在线**：
   ```bash
   # 在 NS 服务器上确认 gateway bridge 在监听
   ss -ulnp | grep 1700
   ```
4. **统计正常标志**：串口出现以下内容说明上行链路正常：
   ```
   INFO: [up] PUSH_ACK received in XXX ms
   INFO: [down] PULL_ACK received in XXX ms
   PUSH_DATA acknowledged: 100.00%
   PULL_DATA sent: N (100.00% acknowledged)
   ```
5. **网关离线原因排查**：PULL_ACK 能收到说明 UDP 通了，若 ChirpStack 仍显示离线，
   检查 gateway bridge → ChirpStack gRPC/MQTT 内部转发是否正常

---

## 七、进度状态

- ✅ LoRa 原始包接收验证通过（CN490 480.5 MHz SF12 BW125）
- ✅ socket 路由 bug 修复（`connect + send/recv`）
- ✅ ACK 超时扩大（2000/1000 ms）+ stale ACK drain
- ✅ MQTT 禁用（`ENABLE_MQTT=0`，无 Broker 时不影响主流程）
- ✅ 本地 UDP Mock 脚本可用（`scripts/lora_ns_mock.py`）
- ⏳ 在稳定局域网环境下验证 100% ACK 率
- ⏳ ChirpStack 对接验证（网关上线确认）
- ⏳ LoRaWAN 节点入网测试（下行 Join Accept）
