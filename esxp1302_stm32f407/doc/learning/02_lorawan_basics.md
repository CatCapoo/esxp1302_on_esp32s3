# 02 — LoRaWAN 协议基础与 CN470 频段

## LoRa vs LoRaWAN

| | LoRa | LoRaWAN |
|---|------|---------|
| 层级 | 物理层 (PHY) | MAC 层协议 |
| 功能 | 射频调制/解调 | 入网、密钥管理、ADR、MAC 命令 |
| 标准 | Semtech 专利 | LoRa Alliance 标准 |
| 用法 | P2P 直连 | 星形网络 (节点→网关→NS) |

SX1302 网关工作在 **LoRa PHY 层**，收到的是原始 LoRa 帧。LoRaWAN MAC 帧的解析由上层 Packet Forwarder 或 Network Server 完成。

---

## LoRaWAN 帧结构

```
┌──────┬──────────────────────┬───────┬─────────┬─────┐
│ MHDR │        FHDR          │ FPort │ Payload │ MIC │
│ (1B) │ DevAddr+FCtrl+FCnt   │ (1B)  │ (N B)   │(4B) │
│      │ (7-22 B)             │       │         │     │
└──────┴──────────────────────┴───────┴─────────┴─────┘
```

- **MHDR**: Message Header (MType + Major)
  - `0x40` = Confirmed Data Up
  - `0x80` = Confirmed Data Down
- **DevAddr**: 4 字节设备地址 (ABP 模式下静态配置)
- **FCnt**: 帧计数器 (2 字节, MSB-first)
- **MIC**: Message Integrity Code (基于 NwkSKey 计算)

---

## CN470 频率计划

CN470 是中国 LoRaWAN 区域频率计划，工作在 470-510 MHz ISM 频段。

### 上行信道 (96 个)

```
CH0  = 470.3 MHz
CH1  = 470.5 MHz
...
CH95 = 489.3 MHz

频率公式: freq = 470.3 + k × 0.2  (MHz),  k = 0..95
```

### Sub-band 划分

96 个信道按每 8 个分为 12 个 Sub-band:

| Sub-band | 信道 | 频率范围 (MHz) |
|----------|------|---------------|
| SB1 | CH0–CH7 | 470.3–471.7 |
| SB2 | CH8–CH15 | 471.9–473.3 |
| ... | ... | ... |
| **SB10** | **CH80–CH87** | **486.3–487.7** |
| SB11 | CH88–CH95 | 487.9–489.3 |

### 本项目使用: Sub-band 10

选择 SB10 的原因:
- 远离 470 MHz 附近的常见干扰
- 适合测试 (该频段使用较少)

| 信道 | 频率 | E77 信道掩码位 |
|------|------|----------------|
| CH80 | 486.3 MHz | Bit 0 |
| CH81 | 486.5 MHz | Bit 1 |
| CH82 | 486.7 MHz | Bit 2 |
| CH83 | 486.9 MHz | Bit 3 |
| CH84 | 487.1 MHz | Bit 4 |
| CH85 | 487.3 MHz | Bit 5 |
| CH86 | 487.5 MHz | Bit 6 |
| CH87 | 487.7 MHz | Bit 7 |

E77 信道掩码: `0000:0000:0000:0000:0000:00FF` (仅使能 SB10)

---

## DataRate 定义 (CN470)

| DR | SF | BW (kHz) | 比特率 (bps) | 最大负载 (B) |
|----|-----|---------|-------------|-------------|
| 0 | 12 | 125 | 250 | 59 |
| 1 | 11 | 125 | 440 | 59 |
| 2 | 10 | 125 | 980 | 59 |
| 3 | 9 | 125 | 1760 | 123 |
| 4 | 8 | 125 | 3125 | 230 |
| 5 | 7 | 125 | 5470 | 230 |

DR 值越大 → SF 越小 → 速率越快 → 距离越短

---

## ABP vs OTAA

| | ABP | OTAA |
|---|-----|------|
| 全称 | Activation By Personalization | Over-The-Air Activation |
| 入网 | 无需握手，静态配置密钥 | Join Request/Accept 握手 |
| 密钥 | DevAddr + NwkSKey + AppSKey 静态写入 | 由 JoinEUI + AppKey 派生 |
| 安全性 | 较低 (密钥固定) | 较高 (会话密钥动态生成) |
| 用途 | 测试、简单场景 | 生产部署 |

本项目 HAL 测试使用 **ABP + dummy keys**，因为:
1. 不需要真实的 Network Server
2. E77 只需要发出有效 LoRaWAN 帧让网关能收到即可
3. MIC 验证不在网关侧进行

---

## SyncWord

| 值 | 含义 |
|----|------|
| 0x34 | LoRa public network (LoRaWAN) |
| 0x12 | LoRa private network (P2P) |

SX1302 HAL 中 `boardconf.lorawan_public = true` 会设置 SyncWord = 0x34。
SX1278 RadioLib 接收端也必须设置 `0x34` 才能匹配。
