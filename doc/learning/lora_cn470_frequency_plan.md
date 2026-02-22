# LoRa CN470 频率计划详解

> 写作背景：2026-02-22 调试 ESXP1302 网关接入 ChirpStack 时，
> 排查了 MQTT topic 前缀不匹配和 radio 频率偏差约 6 MHz 两个问题，
> 顺带彻底搞清楚 CN470 频率体系。

---

## 一、LoRa 物理层基础

### 1.1 三个关键参数

LoRa 调制由三个参数决定一次通信能否成功，发射端和接收端必须完全对齐：

| 参数 | 含义 | 常见值 |
|------|------|--------|
| **频率（freq）** | 载波中心频率 | 486.7 MHz |
| **扩频因子（SF）** | 符号扩展倍数，影响速率和灵敏度 | SF7 ~ SF12 |
| **带宽（BW）** | 信号占用带宽 | 125 / 250 / 500 kHz |

**SF 和速率的关系**（BW=125 kHz）：

| SF | 空中速率（约） | 灵敏度（约）| 适用场景 |
|----|-------------|-----------|---------|
| SF7 | ~5.5 kbps | -123 dBm | 近距离、高频发包 |
| SF9 | ~1.8 kbps | -129 dBm | 中等距离 |
| SF12 | ~250 bps | -137 dBm | 远距离、低频发包 |

> LoRaWAN 的 ADR（自适应速率）会根据 RSSI/SNR 自动选择 SF，近了用 SF7，远了用 SF12。

---

## 二、CN470 频段整体框架

### 2.1 频段划分

CN470（又称 CN470-510）是中国 470~510 MHz 非授权频段，
LoRa Alliance 在 **LoRaWAN Regional Parameters**（区域参数规范）中对其做了标准化定义。

```
470 MHz ───────────────────────────────────────── 510 MHz
│← ─ ─ ─ ─  上行（UL）: 470 ~ 490 MHz ─ ─ ─ ─ →│← ─ ─ 下行（DL）: 500 ~ 510 MHz ─ ─ →│
```

这是**频分双工（FDD）**设计：
- **上行**（节点→网关）：470.3 ~ 489.3 MHz，共 96 个信道，步进 200 kHz
- **下行**（网关→节点）：500.3 ~ 509.7 MHz，共 48 个信道，步进 200 kHz

FDD 的好处：网关可以同时收发，上下行不会互相干扰（频率相差 30 MHz+）。

### 2.2 为什么上行 96 个信道但只用 8 个？

实际部署时，一个**信道计划（Channel Plan）**只用上行 96 个信道中的连续 8 个，
对应的网关只需要接收这 8 个频率，节点也只在这 8 个频率上轮流发包。

这 8 个信道被分成 12 组，每组叫一个**子信道计划**，编号 cn470_0 ~ cn470_11。

不同运营商/部署场景选择不同的子计划，互不干扰。

---

## 三、12 个子信道计划一览

LoRa Alliance 定义的 12 个子计划（以 LoRaWAN Regional Parameters RP002 为准）：

| 计划 | 上行信道编号 | 上行频率范围 | 下行信道编号 | 下行频率范围 |
|------|------------|------------|------------|------------|
| cn470_0 | ch0~ch7 | 470.3~471.7 MHz | ch0~ch7 | 500.3~501.7 MHz |
| cn470_1 | ch8~ch15 | 472.3~473.7 MHz | ch8~ch15 | 502.3~503.7 MHz |
| cn470_2 | ch16~ch23 | 474.3~475.7 MHz | ch16~ch23 | 504.3~505.7 MHz |
| cn470_3 | ch24~ch31 | 476.3~477.7 MHz | ch0~ch7 | 500.3~501.7 MHz |
| cn470_4 | ch32~ch39 | 478.3~479.7 MHz | ch8~ch15 | 502.3~503.7 MHz |
| cn470_5 | ch40~ch47 | 480.3~481.7 MHz | ch16~ch23 | 504.3~505.7 MHz |
| cn470_6 | ch48~ch55 | 482.3~483.7 MHz | ch0~ch7 | 500.3~501.7 MHz |
| cn470_7 | ch56~ch63 | 484.3~485.7 MHz | ch8~ch15 | 502.3~503.7 MHz |
| cn470_8 | ch64~ch71 | 486.3~487.7 MHz | ch16~ch23 | 504.3~505.7 MHz |
| cn470_9 | ch72~ch79 | 488.3~489.7 MHz | ch0~ch7 | 500.3~501.7 MHz |
| **cn470_10** | **ch80~ch87** | **486.3~487.7 MHz** | **ch0~ch7** | **506.7~508.1 MHz** |
| cn470_11 | ch88~ch95 | 488.3~489.7 MHz | ch8~ch15 | 502.3~503.7 MHz |

> **注意**：cn470_8 和 cn470_10 的**上行频率完全相同**（486.3~487.7 MHz），
> 区别在于下行频率和信道编号不同，ChirpStack 靠 topic 前缀区分。
> 本项目使用 **cn470_10**。

---

## 四、CN470_10 详细信道表

### 4.1 上行 8 个信道

| LoRa ch | 信道中心频率 | SF 支持 | BW |
|---------|------------|--------|----|
| ch80 | **486.3 MHz** | SF7~SF12 | 125 kHz |
| ch81 | **486.5 MHz** | SF7~SF12 | 125 kHz |
| ch82 | **486.7 MHz** | SF7~SF12 | 125 kHz |
| ch83 | **486.9 MHz** | SF7~SF12 | 125 kHz |
| ch84 | **487.1 MHz** | SF7~SF12 | 125 kHz |
| ch85 | **487.3 MHz** | SF7~SF12 | 125 kHz |
| ch86 | **487.5 MHz** | SF7~SF12 | 125 kHz |
| ch87 | **487.7 MHz** | SF7~SF12 | 125 kHz |

节点上行时在这 8 个频率中**随机跳频**，防止冲突（CSMA/CA 协议不适用 LoRa，靠跳频降低碰撞概率）。

### 4.2 下行：RX1 窗口（上行后 1 秒）

下行 RX1 与上行信道有映射关系，NS 自动计算，网关无需额外配置：

$$f_{RX1} = 500.3 + (ch \bmod 48) \times 0.2 \text{ MHz}$$

| 上行 ch | 上行频率 | ch mod 48 | RX1 频率 |
|--------|---------|-----------|---------|
| ch80 | 486.3 MHz | 32 | 506.7 MHz |
| ch81 | 486.5 MHz | 33 | 506.9 MHz |
| ch82 | 486.7 MHz | 34 | **507.1 MHz** |
| ch83 | 486.9 MHz | 35 | 507.3 MHz |
| ch84 | 487.1 MHz | 36 | 507.5 MHz |
| ch85 | 487.3 MHz | 37 | 507.7 MHz |
| ch86 | 487.5 MHz | 38 | 507.9 MHz |
| ch87 | 487.7 MHz | 39 | 508.1 MHz |

### 4.3 下行：RX2 窗口（上行后 2 秒）

RX2 是**固定频率**，所有节点通用：

| 参数 | 值 |
|------|----|
| RX2 频率 | **505.3 MHz** |
| RX2 SF | **SF12** |
| RX2 BW | **125 kHz** |

> 如果 RX1 没成功（网关来不及发，或节点没收到），
> NS 会在 RX2 窗口再发一次下行包，用固定参数重传。

---

## 五、SX1302 的 radio_0 / radio_1 架构

理解 radio_0/radio_1 是配置网关的关键。

### 5.1 硬件架构

SX1302 芯片内部有 2 个射频前端（Rx Radio），每个有一个本振（LO），
各自能覆盖 ±500 kHz 的接收范围，8 个数字解调信道（IF0~IF7）分配给这两个前端：

```
                  radio_0（LO = 486.6 MHz）
                  │
          ┌───────┼───────────────────────────────┐
          │       │ 4个IF信道，每个±300kHz以内     │
          │  IF0(-300k)  IF1(-100k)  IF2(+100k)  IF3(+300k)
          │  486.3 MHz   486.5 MHz   486.7 MHz   486.9 MHz
          └──────────────────────────────────────┘
          
                  radio_1（LO = 487.4 MHz）
                  │
          ┌───────┼───────────────────────────────┐
          │       │ 4个IF信道                      │
          │  IF4(-300k)  IF5(-100k)  IF6(+100k)  IF7(+300k)
          │  487.1 MHz   487.3 MHz   487.5 MHz   487.7 MHz
          └──────────────────────────────────────┘
```

### 5.2 radio 中心频率计算规律

对于任意 8 信道计划（步进 200 kHz），radio 中心频率选取规律：

$$f_{radio\_0} = f_{ch0} + 300\,\text{kHz}$$

$$f_{radio\_1} = f_{radio\_0} + 800\,\text{kHz}$$

以 cn470_10（486.3 ~ 487.7 MHz）为例：

$$f_{radio\_0} = 486.3 + 0.3 = 486.6\,\text{MHz} = 486600000\,\text{Hz}$$

$$f_{radio\_1} = 486.6 + 0.8 = 487.4\,\text{MHz} = 487400000\,\text{Hz}$$

这样两个 radio 各覆盖 4 个信道，8 个信道刚好全覆盖，且 IF 偏移对称为 ±100 kHz 和 ±300 kHz。

### 5.3 IF 偏移在 global_conf.json 中的体现

```json
"chan_multiSF_0": { "enable": true, "radio": 0, "if": -300000 },
"chan_multiSF_1": { "enable": true, "radio": 0, "if": -100000 },
"chan_multiSF_2": { "enable": true, "radio": 0, "if":  100000 },
"chan_multiSF_3": { "enable": true, "radio": 0, "if":  300000 },
"chan_multiSF_4": { "enable": true, "radio": 1, "if": -300000 },
"chan_multiSF_5": { "enable": true, "radio": 1, "if": -100000 },
"chan_multiSF_6": { "enable": true, "radio": 1, "if":  100000 },
"chan_multiSF_7": { "enable": true, "radio": 1, "if":  300000 }
```

实际接收频率 = `radio[n].freq` + `if`（偏移）

---

## 六、NVS 频率配置

本项目（ESXP1302）使用 NVS（Non-Volatile Storage）在运行时覆盖 JSON 文件中的 radio 频率。

**加载顺序（lora_pkt_fwd.c 逻辑）：**

```
1. 读 global_conf.json → 解析所有配置（包括 radio freq 默认值）
2. 从 NVS 读取 FREQ_RADIO0、FREQ_RADIO1
3. 如果 NVS 中有值，用字符串替换 JSON 中的 "freq" 字段
4. 再次解析替换后的 JSON → 实际生效的 radio 频率来自 NVS
```

**修改 NVS 频率的方式：**

1. **通过 Soft AP 网页**（推荐）：
   - 上电时按住 IO0（USER_BUTTON_1，左键）→ 进入 Soft AP 模式
   - 连接 WiFi：SSID=`esp32`，密码=`esp32wifi`
   - 浏览器打开 `http://192.168.4.1`，用户名 `iot`，密码 `lora`
   - 修改 radio0/radio1 频率 → Apply → Reboot
   - Soft AP 模式有 10 分钟超时，超时后自动重启进入正常模式

2. **通过源码修改 JSON 默认值**：
   编辑 `main/packet_forwarder/global_conf.json/global_conf.cn490.json`，
   修改 `radio_0.freq` 和 `radio_1.freq`，
   **但 NVS 中若已有值则会覆盖此处配置**，需清除 NVS 或同步更新 NVS。

---

## 七、LoRaWAN 帧格式与裸 LoRa 的区别

LoRa 是物理层调制，LoRaWAN 是建立在 LoRa 上的 MAC 层协议。

### 7.1 LoRaWAN 上行帧最小结构

```
┌──────┬─────────┬───────┬───────┬──────────┬─────────┬───┐
│ MHDR │ DevAddr │ FCtrl │ FCnt  │ FPort(可选)│ FRMPayload│ MIC │
│ 1B   │  4B     │  1B   │  2B   │  1B      │ 可变    │ 4B │
└──────┴─────────┴───────┴───────┴──────────┴─────────┴───┘
最小 = 1+4+1+2+4 = 12 字节（MACPayload 部分至少 7 字节）
```

### 7.2 裸 LoRa 发包（无 LoRaWAN 帧头）

测试时直接发 "hello world" = 11 字节，没有 MHDR、DevAddr 等字段。

NS 收到后尝试解析 MAC 帧，发现长度不足（<7 字节的 MACPayload），报错：

```
ERROR chirpstack::uplink: Deduplication error error=MACPayload requires at least 7 bytes
```

**这不是故障，是预期行为。** 等节点运行 LoRaWAN 协议栈后自动消除。

---

## 参考资料

- LoRa Alliance 官方文档：LoRaWAN Regional Parameters RP002
  - 搜索 "CN470-510 channel plan" 可找到完整信道表
- Semtech SX1302 数据手册（SX1302 Datasheet）
- ChirpStack 官方文档：https://www.chirpstack.io/docs/
