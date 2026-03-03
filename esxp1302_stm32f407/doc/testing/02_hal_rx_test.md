# 02 — HAL RX 接收测试

## 概述

HAL RX 测试验证完整的 **SX1302 集中器接收链路**：
- `lgw_start()` → `lgw_receive()` → `lgw_stop()` 生命周期
- CN470 Sub-band 10 八信道同时接收
- 与 E77 节点配合的端到端测试

源文件: `test/test_loragw_hal_rx.c`

---

## 频率计划: CN470_10

| 信道 | 频率 (MHz) | 射频链路 | IF 偏移 (kHz) |
|------|-----------|---------|--------------|
| CH80 | 486.3 | Radio A (radio_0) | -300 |
| CH81 | 486.5 | Radio A | -100 |
| CH82 | 486.7 | Radio A | +100 |
| CH83 | 486.9 | Radio A | +300 |
| CH84 | 487.1 | Radio B (radio_1) | -300 |
| CH85 | 487.3 | Radio B | -100 |
| CH86 | 487.5 | Radio B | +100 |
| CH87 | 487.7 | Radio B | +300 |

- **Radio A 中心频率**: 486.6 MHz
- **Radio B 中心频率**: 487.4 MHz
- 每个 Radio 覆盖 4 个 multi-SF 信道，IF 偏移 ±100/±300 kHz

---

## 测试流程

### 1. 配置阶段

```c
/* Board */
boardconf.lorawan_public = true;
boardconf.clksrc          = 0;

/* Radio A (RX only) */
rfconf.freq_hz = 486600000;
rfconf.type    = LGW_RADIO_TYPE_SX1250;

/* Radio B (RX only) */
rfconf.freq_hz = 487400000;

/* 8 multi-SF channels */
for (i = 0; i < 8; i++) {
    ifconf.rf_chain = channel_rfchain[i];
    ifconf.freq_hz  = channel_if[i];
    ifconf.datarate = DR_LORA_SF7;  /* multi-SF 最小 SF */
}
```

### 2. 收包循环

- **外循环**: 10 次 start/stop 周期
- **内循环**: 每次启动后监听 30 秒
- 每 10ms 调用 `lgw_receive()` 轮询
- 最大一次接收 16 个包

### 3. 输出信息

每个接收到的包打印以下字段:

| 字段 | 说明 |
|------|------|
| `count_us` | SX1302 内部 µs 时间戳 |
| `size` | 负载长度 (字节) |
| `if_chain` | IF 信道编号 (0–7) |
| `status` | CRC 状态 (0x10 = CRC OK) |
| `datarate` | SF 值 |
| `coderate` | CR 值 |
| `rf_chain` | 射频链路 (0 或 1) |
| `freq_hz` | 实际频率 |
| `snr_avg` | 平均 SNR (dB) |
| `rssi_chan` | 信道 RSSI |
| `rssi_sig` | 信号 RSSI |
| `crc` | CRC 值 |
| `payload` | 十六进制负载 |

---

## 发射端: E77 节点配置

E77-400M22S 模块**仅支持 LoRaWAN 模式**（不支持 P2P 原始 LoRa），因此使用 **ABP + dummy keys** 方式发包。

### 运行命令

```powershell
python scripts/e77_node_tx.py --port COM11 --interval 5 --count 20 --dr 5
```

> **注意**: E77 的串口是 COM11，不是 COM3（COM3 是蓝牙虚拟串口）。

### E77 发送的 LoRaWAN 帧结构

网关收到的是完整 LoRaWAN MAC 帧（MHDR + FHDR + FPort + Payload + MIC），作为原始 LoRa 包解码。

---

## 实际测试结果

```
===== sx1302 HAL RX test (CN470_10) =====
Radio A: 486600000 Hz   Radio B: 487400000 Hz
Channels CH80–CH87 (486.3–487.7 MHz)

--- Loop 1/10 ---
INFO: concentrator started, waiting for packets ...

----- LoRa packet #1 -----
  size:     21
  chan:     1
  status:   0x10
  datr:     10
  rf_chain  0
  freq_hz   486500000
  snr_avg:  2.8
  rssi_chan:-47.0
  ...
  payload:  40 34 12 01 26 00 19 00 02 DE AD BE EF 01 02 03 04 ...

----- LoRa packet #2 -----
  snr_avg:  -1.5
  ...

----- LoRa packet #3 -----
  snr_avg:  -0.5
  ...
```

### 结果分析

- E77 发送 DR2(SF10)，网关在 CH81(486.5 MHz) 上正确接收
- SNR 值 2.8 / -1.5 / -0.5 dB 表明信号质量正常（室内短距离）
- `status = 0x10` 表示 CRC 校验通过
- `payload` 中可见 LoRaWAN 帧: DevAddr=26011234, FCnt 递增

---

## 开发过程中遇到的问题

### 问题 1: LM75A 地址配置错误

**现象**: I2C 温度传感器读取失败，HAL 启动日志报错  
**原因**: 原始 ESP32 代码适配的是 STTS751 温度传感器，扫描地址是 `{0x39, 0x3B, 0x38, 0x3A}`，但板载的是 **LM75A** 传感器  
**修复**: 将温度传感器地址列表改为 `{0x48, 0x49, 0x4A, 0x4B}`

```c
/* Before (STTS751): */
uint8_t dev_list[] = {0x39, 0x3B, 0x38, 0x3A};

/* After (LM75A): */
uint8_t dev_list[] = {0x48, 0x49, 0x4A, 0x4B};
```

### 问题 2: 浮点数 printf 输出为空

**现象**: `printf("%.1f", snr)` 不输出任何内容  
**原因**: newlib-nano 默认不包含浮点 printf 支持  
**修复**: 在 CMakeLists.txt 链接选项中添加 `-u_printf_float`

```cmake
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -u_printf_float)
```

### 问题 3: E77 串口识别

**现象**: 发送 AT 命令无反应  
**原因**: Windows 设备管理器显示多个 COM 端口，COM3 是蓝牙虚拟串口  
**修复**: 使用 `e77_probe.py` 脚本逐一探测各 COM 端口，确认 E77 实际对应 COM11

```powershell
python scripts/e77_probe.py --port COM11
# 输出: ✓ 找到! 波特率=9600  行尾=b'\r\n'
```
