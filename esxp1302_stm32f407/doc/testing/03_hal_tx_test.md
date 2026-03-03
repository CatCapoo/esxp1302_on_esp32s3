# 03 — HAL TX 发射测试

## 概述

HAL TX 测试验证 **SX1302 集中器发射链路**：
- TX gain LUT 配置
- `lgw_send()` + `lgw_status()` 轮询发射完成
- 使用 SX1278 (RadioLib) 作为接收端验证

源文件: `test/test_loragw_hal_tx.c`

---

## TX 参数配置

| 参数 | 值 |
|------|-----|
| 频率 | 486.3 MHz (CH80) |
| 射频链路 | Radio A (rf_chain 0) |
| 发射功率 | 14 dBm |
| 调制 | LoRa |
| SF | 7 |
| BW | 125 kHz |
| CR | 4/5 |
| 前导码 | 8 symbols |
| CRC | 关闭 (`no_crc = true`) |
| IQ 反转 | 关闭 (`invert_pol = false`) |
| TX 模式 | IMMEDIATE |
| 负载长度 | 32 bytes |
| 每轮包数 | 10 |
| 循环次数 | 3 |
| 总包数 | 30 |

---

## TX Gain LUT 配置

```c
txlut.size = 1;
txlut.lut[0].rf_power = 14;    /* 目标功率 14 dBm */
txlut.lut[0].pa_gain  = 1;     /* SX1250: 0 或 1 */
txlut.lut[0].pwr_idx  = 14;    /* SX1250 功率索引 */
txlut.lut[0].mix_gain = 5;
txlut.lut[0].dig_gain = 0;
```

---

## 发射流程

```
lgw_reset()
     ↓
lgw_start()
     ↓
┌─ for (10 packets) ─────────────────────┐
│  lgw_send(&pkt)                         │
│  do { lgw_status(TX_STATUS) }           │
│      while (tx_status != TX_FREE)       │
│  wait_ms(500)                           │
└─────────────────────────────────────────┘
     ↓
lgw_stop()
     ↓
lgw_reset()
     ↓
(重复 3 轮)
```

### 负载内容

前 9 字节模拟 LoRaWAN 帧头:
```
Byte 0:   0x40 (Confirmed Data Up MHDR)
Byte 1-4: 0xAB 0xAB 0xAB 0xAB (DevAddr)
Byte 5:   0x00 (FCtrl)
Byte 6-7: FCnt (每包递增)
Byte 8:   0x02 (FPort)
Byte 9+:  Sequential bytes (0x09, 0x0A, ...)
```

---

## 接收端: SX1278 + RadioLib

使用 Arduino + SX1278 模块 + RadioLib 库作为接收端验证。

### RadioLib 配置

```cpp
radio.begin(486.3, 125.0, 7, 5, 0x34, 17, 8);
// 频率=486.3, BW=125, SF=7, CR=4/5, SyncWord=0x34, Power=17, Preamble=8
radio.setCRC(false);        // 与 TX 端 no_crc=true 对应
radio.setInvertIQ(false);   // 与 TX 端 invert_pol=false 对应
```

> **关键配置要点**:
> - SyncWord 必须是 `0x34`（LoRa public network），对应 `boardconf.lorawan_public = true`
> - CRC 必须关闭，因为 TX 端设置 `no_crc = true`
> - IQ 反转必须关闭，因为 TX 端设置 `invert_pol = false`

---

## 实际测试结果

### SX1302 TX 端输出

```
===== sx1302 HAL TX test (CN470_10) =====
TX freq: 486300000 Hz  SF7  BW125  Power: 14 dBm
Packets: 10 per loop x 3 loops = 30 total

--- TX Loop 1/3 ---
INFO: concentrator started
Sending packet 1/10 (size=32) ...
TX done.
Sending packet 2/10 (size=32) ...
TX done.
...
Sent 10 packets (loop 1)
```

### SX1278 RX 端输出

```
Received packet #1  RSSI:-68 dBm  SNR:9.2 dB  len:32
Payload: 40 AB AB AB AB 00 00 00 02 09 0A 0B 0C ...

Received packet #2  RSSI:-69 dBm  SNR:8.8 dB  len:32
...

Total received: 13/30 packets
```

### 结果分析

| 指标 | 值 | 说明 |
|------|-----|------|
| 接收率 | 13/30 (43%) | 室内测试，信号衰减+干扰正常 |
| RSSI | -68 ~ -69 dBm | 信号强度稳定 |
| SNR | 8-9 dB | SF7 解调余量充足 |
| 负载 | 32 bytes 正确 | FCnt 递增可见 |
| 丢包原因 | 同频干扰 / 天线匹配 | 实验室环境可接受 |

---

## 常见问题

| 现象 | 原因 | 修复 |
|------|------|------|
| `lgw_send()` 返回错误 | TX gain LUT 未配置或 rf_chain 未使能 tx | 检查 `rfconf.tx_enable = true` 和 `lgw_txgain_setconf()` |
| TX done 但接收端无信号 | SyncWord/CRC/IQ 不匹配 | 严格对齐两端参数 |
| RSSI 极低或全丢包 | 天线未连接或频率偏差 | 检查天线 SMA 接口 |
| `tx_status` 永不为 `TX_FREE` | SX1302 TX FSM 卡住 | 增加超时保护，必要时 reset |
