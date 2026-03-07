# LoRaWAN 网关与 Network Server 协议规定

> 写作背景：2026-02-22 调试 ESXP1302 接入 ChirpStack，
> 深入理解了 Semtech UDP 包转发协议、Gateway Bridge 的作用和 MQTT 话题规范。

---

## 一、LoRaWAN 网络架构总览

LoRaWAN 采用星形拓扑，由四层组成：

```
┌──────────┐   LoRa RF    ┌──────────┐   UDP/IP   ┌─────────────────┐   MQTT   ┌────────────────┐
│ End Node │ ──────────── │ Gateway  │ ─────────── │ Gateway Bridge  │ ──────── │ Network Server │
│（节点）  │              │（网关）  │ 1700/1680  │（协议转换）     │          │（NS，ChirpStack）│
└──────────┘              └──────────┘             └─────────────────┘          └────────────────┘
```

每层职责：

| 层 | 职责 |
|----|------|
| **End Node** | 传感器/执行器，通过 LoRa 发送数据，实现 ClassA/B/C |
| **Gateway** | 纯物理层转发，收 LoRa 包→封 UDP，不解密、不判断真假 |
| **Gateway Bridge** | 将 Semtech UDP 协议转换为 MQTT，两端都可以配置 |
| **Network Server** | 解密、去重、ADR、下行调度、设备鉴权 |
| **Application Server** | 处理业务数据，NS 通过 gRPC/HTTP 对接 |

---

## 二、Semtech UDP 包转发协议（Semtech Packet Forwarder Protocol）

本协议是网关和 Gateway Bridge/NS 之间的标准协议，
由 Semtech 设计并开源（basic_pkt_fwd、lora_pkt_fwd），
LoRaWAN 生态中几乎所有网关都支持。

### 2.1 UDP 端口约定

| 方向 | 端口 | 说明 |
|------|------|------|
| 网关 → NS（发送方） | 随机（本地端口）| 动态分配 |
| NS → 网关（接收方，上行端口） | **1700**（默认）| 接收 PUSH_DATA |
| NS → 网关（接收方，下行端口） | **1700**（默认）| 接收 PULL_RESP |
| 网关本地（接收 PULL_RESP）| **1680**（默认）| 部分实现用不同端口 |

> 本项目配置：serv_port_up=1680，serv_port_down=1680（见 global_conf.json）

### 2.2 五种消息类型

#### PUSH_DATA（网关 → NS，上行数据）

网关收到 LoRa 包后，打包成 JSON 通过 UDP 发给 NS：

```
字节0：协议版本（0x02）
字节1-2：随机 token（2字节，用于匹配 ACK）
字节3：消息类型 0x00 = PUSH_DATA
字节4-11：网关 EUI（8字节，大端）
字节12+：JSON 字符串（rxpk 数组）
```

JSON 示例：
```json
{
  "rxpk": [{
    "tmst": 12345678,     // 网关本地时间戳（微秒，32位循环）
    "freq": 486.7,        // 接收频率（MHz）
    "chan": 2,            // 信道编号（IF0~IF7）
    "rfch": 0,            // radio 编号（0或1）
    "stat": 1,            // CRC 状态（1=OK, -1=FAIL, 0=NO_CRC）
    "modu": "LORA",       // 调制方式
    "datr": "SF12BW125",  // 数据率（SFxBWyyy）
    "codr": "4/5",        // 编码率
    "rssi": -73,          // RSSI（dBm）
    "lsnr": 4.0,          // SNR（dB）
    "size": 11,           // payload 字节数
    "data": "aGVsbG8="   // Base64 编码的 payload
  }]
}
```

#### PUSH_ACK（NS → 网关，确认收到上行）

```
字节0：0x02
字节1-2：与 PUSH_DATA 相同的 token
字节3：0x01 = PUSH_ACK
```

**ackr（acknowledged ratio）** 就是这个 ACK 的回复率，
`lora_pkt_fwd.c` 统计 30 秒内发出 PUSH_DATA 次数和收到 PUSH_ACK 次数之比。

#### PULL_DATA（网关 → NS，保持连接 / 拉取下行）

网关每隔 ~10 秒发一次，用途：
1. 告诉 NS 网关的 UDP 出口 IP:Port（NAT 穿透）
2. 让 NS 知道网关在线

```
字节0：0x02
字节1-2：token
字节3：0x02 = PULL_DATA
字节4-11：网关 EUI
```

#### PULL_ACK（NS → 网关，确认收到 PULL_DATA）

```
字节0：0x02
字节1-2：与 PULL_DATA 相同 token
字节3：0x04 = PULL_ACK
```

#### PULL_RESP（NS → 网关，下行发包指令）

NS 要发下行帧时，通过之前 PULL_DATA 确认的地址，发 PULL_RESP 给网关：

```json
{
  "txpk": {
    "imme": false,       // 是否立即发（false = 用 tmst 精确定时）
    "tmst": 12346678,    // 发射时间戳（上行 tmst + 1000000 for RX1）
    "freq": 507.1,       // 下行频率（MHz）
    "rfch": 0,           // 使用哪个 radio 发射
    "powe": 14,          // 发射功率（dBm）
    "modu": "LORA",
    "datr": "SF12BW125",
    "codr": "4/5",
    "ipol": true,        // 下行极化反转（LoRaWAN 规定下行必须 ipol=true）
    "size": 17,
    "data": "YAFCAQABAAAAc..."
  }
}
```

> **极化反转（ipol）** 是 LoRaWAN 的一个重要细节：
> 节点上行 ipol=false，网关下行 ipol=true，
> 这样两个节点相互之间收不到对方的上行包，只有网关（ipol=true）才能收到节点上行。

---

### 2.3 连接状态维护

Semtech UDP 是无连接的，网关靠以下机制维护"在线"状态：

```
网关                         NS (Gateway Bridge)
 │───── PUSH_DATA（stats）────→│  每30秒一次，含网关统计数据
 │←──── PUSH_ACK ─────────────│
 │                             │
 │───── PULL_DATA ─────────────→│  每10秒一次
 │←──── PULL_ACK ─────────────│
```

NS 通过连续收到 PUSH_DATA 或 PULL_DATA 来判断网关是否在线，
超过一定时间没收到就标记为离线。

---

## 三、Gateway Bridge 的作用

### 3.1 为什么需要 Gateway Bridge？

ChirpStack NS 原生使用 **MQTT** 与网关通信，
而 99% 的 LoRa 网关硬件实现的是 Semtech UDP 协议，二者不兼容。

Gateway Bridge 作为适配器，把 UDP 协议转换成 MQTT：

```
网关（UDP）──→ Gateway Bridge ──→ MQTT Broker ──→ ChirpStack NS
                ↑ 也可反向：
                NS 通过 MQTT 发命令 → Bridge 转成 UDP PULL_RESP → 网关
```

### 3.2 MQTT Topic 规范

Gateway Bridge 发布和订阅的 topic 格式：

```
{topic_prefix}/gateway/{gateway_eui}/event/{event_type}
{topic_prefix}/gateway/{gateway_eui}/state/{state_type}
{topic_prefix}/gateway/{gateway_eui}/command/#
```

| 字段 | 含义 | 示例 |
|------|------|------|
| `topic_prefix` | 频率计划标识符 | `cn470_10` |
| `gateway_eui` | 网关 EUI，**小写十六进制** | `aa555a00000021fb` |
| `event_type` | 事件类型 | `up`（上行）、`stats`（统计）、`ack` |
| `state_type` | 状态类型 | `conn`（连接状态）|

**完整 topic 示例：**

```
cn470_10/gateway/aa555a00000021fb/event/up
cn470_10/gateway/aa555a00000021fb/event/stats
cn470_10/gateway/aa555a00000021fb/state/conn
cn470_10/gateway/aa555a00000021fb/command/#
```

### 3.3 topic_prefix 与频率计划的关系

`topic_prefix` **必须**与 NS 中 region 配置文件的 `topic_prefix` 字段完全一致。

这是 ChirpStack 知道"这个网关属于哪个频率计划"的唯一依据：

```
gateway-bridge 的 topic_prefix="cn470_10"
        ↕ 必须完全一致（包括下划线、数字）
region_cn470_10.toml 的 topic_prefix="cn470_10"
```

如果不一致（哪怕只差一个字符 `cn470` vs `cn470_10`），
NS 就收不到这个网关的任何消息，GUI 永远显示离线。

---

## 四、LoRaWAN 网关的"透明转发"原则

**网关不做任何业务处理**，只负责物理层收发，这是 LoRaWAN 架构的核心设计原则：

| 网关做的 | 网关不做的 |
|---------|----------|
| 接收所有 LoRa 帧（无论是否注册）| 验证 DevAddr |
| 测量 RSSI / SNR / 频偏 | 解密 FRMPayload |
| 打上接收时间戳（tmst）| 判断 MIC 是否合法 |
| 转发给 NS（无过滤）| ADR 决策 |
| 执行 NS 下行命令 | 缓存数据 |

这意味着：
- 一个 LoRa 包可以同时被多个网关收到，NS 负责**去重**
- 网关覆盖范围内所有设备的上行包都会被上报，NS 根据 DevAddr 判断是不是自己的设备
- 安全性完全由 NS 和节点之间的 AES-128 加密保证，网关无法伪造

---

## 五、ClassA / ClassB / ClassC 的下行时序

LoRaWAN 定义了三种节点工作模式，网关需要精确定时发下行包：

### ClassA（最常用）

```
节点发上行
   │
   ├── 1秒后 ──→ RX1 窗口（下行频率=上行频率的映射值，SF=上行SF）
   │
   └── 2秒后 ──→ RX2 窗口（固定频率505.3MHz，SF12）
```

每次上行后才有下行机会，最省电。NS 必须在 RX1 或 RX2 之一发送下行，否则等下次上行。

### ClassB

在 ClassA 基础上，节点周期性打开"ping slot"接收窗口，
用信标（Beacon）同步，允许 NS 在固定时隙主动下行，延迟可预测。

### ClassC

节点除了发送上行，其余时间**持续监听**，NS 随时可以下行。
功耗最高，适合有稳定电源的设备（如智能插座）。

---

## 六、下行发射时序的精确性要求

NS 调度下行包时，会在 PULL_RESP 的 `txpk.tmst` 中指定精确发射时间：

```
tmst_tx = tmst_rx + RX1_delay × 1_000_000（微秒）
```

其中 `tmst_rx` 是网关在 PUSH_DATA 中上报的接收时间戳。

网关必须保证在 `tmst_tx` 时刻（误差 < ±20 μs）开始发射，
否则节点的 RX 窗口关闭，下行包丢失。

SX1302 内置硬件定时器用于保证这个精度，这也是为什么网关不能用纯软件方案的原因。

---

## 七、多网关覆盖与去重

当多个网关都覆盖同一区域时，同一个上行包会被多个网关收到并上报：

```
Node ──上行──→ Gateway A ──PUSH_DATA──→ NS
           └──→ Gateway B ──PUSH_DATA──→ NS  （同一个包，两份）
```

NS 的去重逻辑（ChirpStack 中的 deduplication window）：

1. 收到第一份，启动去重窗口（默认 200ms）
2. 窗口内收到的其他副本记录 metadata（RSSI、SNR、网关EUI等）
3. 窗口结束后，选择信号最强的那份处理，其余丢弃
4. 把所有网关的 metadata 汇总，便于网络优化

---

## 参考资料

- LoRa Alliance：LoRaWAN Specification v1.0.4
- Semtech：[UDP Packet Forwarder Protocol](https://github.com/Lora-net/packet_forwarder/blob/master/PROTOCOL.TXT)
- ChirpStack Gateway Bridge：https://www.chirpstack.io/docs/chirpstack-gateway-bridge/
