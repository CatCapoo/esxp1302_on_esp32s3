# LoRaWAN 标准与频率计划

## LoRa Alliance 标准文档

| 文档 | 版本 | 说明 | 链接 |
|------|------|------|------|
| LoRaWAN Specification | 1.0.3 / 1.0.4 | 核心协议 | [lora-alliance.org](https://lora-alliance.org/resource_hub/lorawan-specification-v1-0-3/) |
| Regional Parameters | RP002-1.0.3 | 各区域频率计划 | [lora-alliance.org](https://lora-alliance.org/resource_hub/rp002-1-0-3-lorawan-regional-parameters/) |

---

## CN470 频率计划详细参数

### 上行 (Uplink)

| 参数 | 值 |
|------|-----|
| 频率范围 | 470.3 – 489.3 MHz |
| 信道数量 | 96 (CH0 – CH95) |
| 信道间隔 | 200 kHz |
| 频率公式 | 470.3 + k × 0.2 MHz (k=0..95) |
| 调制 | LoRa |
| BW | 125 kHz |

### 下行 (Downlink)

| 参数 | 值 |
|------|-----|
| 频率范围 | 500.3 – 509.7 MHz |
| 信道数量 | 48 (CH0 – CH47) |
| 信道间隔 | 200 kHz |

### 上下行频率对应

下行信道 = 上行信道编号 % 48

例: 上行 CH80 → 下行 CH80%48 = CH32 = 500.3 + 32×0.2 = 506.7 MHz

### DataRate

| DR | SF | BW | bit rate |
|-----|-----|------|----------|
| 0 | 12 | 125 kHz | 250 bps |
| 1 | 11 | 125 kHz | 440 bps |
| 2 | 10 | 125 kHz | 980 bps |
| 3 | 9 | 125 kHz | 1760 bps |
| 4 | 8 | 125 kHz | 3125 bps |
| 5 | 7 | 125 kHz | 5470 bps |

### TX Power

| TxPower Index | EIRP (dBm) |
|---------------|-----------|
| 0 | 19 (MaxEIRP) |
| 1 | 17 |
| 2 | 15 |
| 3 | 13 |
| 4 | 11 |
| 5 | 9 |
| 6 | 7 |
| 7 | 5 |

---

## 中国 ISM 频段法规

| 频段 | 用途 | 最大功率 |
|------|------|---------|
| 470-510 MHz | LoRa/IoT | 50 mW (17 dBm) ERP |
| 433.05-434.79 MHz | ISM 通用 | 10 mW |
| 779-787 MHz | 短距离 | 10 mW |

> CN470 频段可免执照使用，但发射功率不得超过 17 dBm ERP。

---

## Semtech SX1302 HAL 参考代码

| 仓库 | 说明 |
|------|------|
| [sx1302_hal](https://github.com/Lora-net/sx1302_hal) | 官方 HAL 实现 (Linux/Raspberry Pi) |
| [lora_gateway](https://github.com/Lora-net/lora_gateway) | SX1301 旧版 HAL (参考用) |
| [packet_forwarder](https://github.com/Lora-net/packet_forwarder) | 官方 Packet Forwarder |

本项目的 HAL 代码基于 `sx1302_hal` 仓库移植，先从 Linux 移植到 ESP32 (ESP-IDF)，再从 ESP32 移植到 STM32F407 (HAL + FreeRTOS)。
