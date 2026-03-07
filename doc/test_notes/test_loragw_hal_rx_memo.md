# test_loragw_hal_rx 测试备忘录

**日期：** 2026-02-20  
**测试目的：** 验证 SX1302 HAL RX 射频接收功能（480 MHz，SF12，no-CRC）

---

## 一、测试命令

### SX1302 接收端

```
test_loragw_hal_rx -r 1250 -a 480.4 -b 480.4 -k 0 -m 1
```

| 参数 | 值 | 说明 |
|------|----|------|
| `-r 1250` | SX1250 | 射频前端芯片型号 |
| `-a/-b 480.4` | 480.4 MHz | Radio A / B 中心频率，信道偏移 -400 kHz 后实际接收 **480.0 MHz** |
| `-k 0` | Radio A | 时钟源选 RF chain 0 |
| `-m 1` | 模式 1 | 所有 8 个信道偏移均为 -400 kHz，全部集中在同一频率，适合单频测试 |

> **注意 `-m 1` 与 `-a` 的关系**：mode 1 下实际接收频率 = 中心频率 - 0.4 MHz，
> 因此中心频率需设为 **480.4**，才能接收到 480.0 MHz 的信号。
> 若误用 `-a 480.0 -m 1`，实际接收频率为 479.6 MHz，收不到任何包。

### SX1278 发送端（RadioLib）

```cpp
radio.begin(480.0, 125.0, 12, 5, 0x34, 17, 8);
radio.setCRC(false);
```

| 参数 | 值 | 说明 |
|------|----|------|
| 频率 | 480.0 MHz | 与 SX1302 信道实际接收频率对齐 |
| BW | 125 kHz | LoRa 标准带宽 |
| SF | 12 | 最大扩频因子 |
| CR | 4/5 | 编码率 |
| 同步字 | 0x34 | LoRaWAN 公共网络，与 SX1302 `lorawan_public=true` 匹配 |
| CRC | 关闭 | `setCRC(false)`，与接收端 no-CRC 一致 |

---

## 二、注意事项

### 2.1 SyncWord（最关键！）

- SX1302 HAL 代码中 `boardconf.lorawan_public = true`，对应 SyncWord = **0x34**
- 发送端**必须将 SyncWord 配置为 0x34**，否则 SX1302 无法识别包头，收不到任何数据
- 发送端已新增 syncword 配置入口，通过该入口设置即可，**无需修改 SX1302 代码**

### 2.2 信道模式选择

| 模式 | 命令 | 信道分布 | 适合场景 |
|------|------|---------|---------|
| mode 0 | `-m 0 -a 480.0` | 分散 3 个频点，chan 2 / chan 5 接收 480.0 MHz | 模拟 LoRaWAN 多信道 |
| mode 1 | `-m 1 -a 480.4` | 全部 8 个信道集中在 480.0 MHz | 单频点验证（推荐） |

### 2.3 CRC 设置

- 发送端关闭 CRC（`setCRC(false)`），`pkt.status` 中 CRC 字段为 `0x0000`
- SX1302 仍可正常接收无 CRC 的包，`status=0x01` 即为正常收到

---

## 三、验证结果

SX1278 发送 `"hello world"`，SX1302 成功接收：

```
----- LoRa packet -----
  size:     11
  chan:     6
  status:   0x01
  datr:     12
  codr:     1
  rf_chain  0
  freq_hz   480000000
  snr_avg:  -3.0
  rssi_chan: 144.0
  crc:      0x0000
68 65 6C 6C 6F 20 77 6F 72 6C 64
```
