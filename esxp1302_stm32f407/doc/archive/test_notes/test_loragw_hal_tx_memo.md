# test_loragw_hal_tx 测试备忘录

**日期：** 2026-02-20  
**测试目的：** 验证 SX1302 HAL TX 射频发送功能（CN470 频段，SF12）

---

## 一、测试命令

```
test_loragw_hal_tx -r 1250 -k 0 -c 0 -f 505.3 -m LORA -s 12 -b 125 -n 3 -z 16 -p 14
```

### 参数说明

| 参数 | 值 | 说明 |
|------|----|------|
| `-r 1250` | SX1250 | 射频芯片型号 |
| `-k 0` | Radio A | 时钟源选 RF chain 0 |
| `-c 0` | RF chain 0 | TX 使用 RF chain 0 |
| `-f 505.3` | 505.3 MHz | TX 频率（CN470 频段） |
| `-m LORA` | LoRa | 调制方式 |
| `-s 12` | SF12 | 扩频因子 |
| `-b 125` | 125 kHz | 带宽 |
| `-n 3` | 3 包 | 发送包数 |
| `-z 16` | 16 字节 | payload 大小 |
| `-p 14` | 14 dBm | 发射功率 |

---

## 二、注意事项

### 2.1 SyncWord（最关键！）

- TX 端（SX1302 HAL）`boardconf.lorawan_public = true`，对应 SyncWord = **0x34**（LoRaWAN public network）
- **接收端必须将 SyncWord 配置为 0x34**，否则无法完成包头同步（HdrValid 永远为 0），导致接收超时
- 私有 LoRa 网络默认 SyncWord 为 **0x12**，这是之前 RX 验证不通过的根本原因

```
RX 初始化日志中可确认：
[DRV] RX_INIT done ... SyncWord=0x34   ← 接收端配置正确后的输出
```

### 2.2 CRC 设置

- TX 代码中固定 `pkt.no_crc = true`（**关闭 CRC**）
- 接收端若开启了 CRC 校验，会因缺少 CRC 字段而丢弃数据包
- 纯射频验证时接收端也应关闭 CRC

### 2.3 Payload 首字节

- TX 代码硬编码 `pkt.payload[0] = 0x40`（ASCII `@`）
- 0x40 为 LoRaWAN MHDR Unconfirmed Data Up（代码注释有误，写的是 Confirmed）
- 对普通 LoRa 接收端无影响，仅作为数据标识

### 2.4 包头模式

- 默认使用**显式包头（Explicit Header）**，TX/RX 必须一致
- 若使用 `--nhdr` 则切换为隐式包头，接收端同样需要对应配置

### 2.5 RX 窗口时序

- SF12/BW125 单包空中时间约 **1318 ms**（16字节 payload）
- 接收端 timeout 须大于单包空中时间，建议设 ≥ 3000 ms
- RX 窗口与 TX 发送时机未同步时，第一包可能被 timeout 截断，属正常现象

---

## 三、接收端 ModemStat 正常接收流程

| ModemStat | 含义 | 状态 |
|-----------|------|------|
| `0x04` / `0x24` | RxOngoing | 射频接收中，未检测到信号 |
| `0x05` / `0x25` | + SigDetect | 检测到前导码信号 |
| `0x07` / `0x27` | + SigSync | 前导码同步完成 |
| `0x0F` / `0x2F` | **+ HdrValid** | **包头校验通过** ✅ |
| DIO0=1, IRQ=0x40 | RxDone | payload 接收完毕 |

> `HdrValid=0` 一直到超时 → 通常是 SyncWord 不匹配导致

---

## 四、测试结果

**测试环境：** ESP32-S3 + SX1302，接收端为普通 SX127x LoRa 模块

| 项目 | 结果 |
|------|------|
| TX 发送 | ✅ 正常，发 3 包均完成 |
| 接收端收包 | ✅ 成功接收 2/3 包（1 包被 RX 窗口截断，正常） |
| 收到字节数 | ✅ `bytes=16`，与 `-z 16` 一致 |
| payload 数据 | ✅ `data:@`（0x40），与 TX 首字节一致 |
| HdrValid | ✅ 接收成功的包均经过 HdrValid=1 阶段 |
| RSSI | -68 ~ -76 dBm（近距离测试） |

### 问题排查历程

1. **初始问题：** 接收端一直 `ModemStat=0x07`（SigSync），但 HdrValid 始终为 0，最终超时
2. **排查方向：** 检查 CRC、包头模式、payload 内容→均无问题
3. **根本原因：** 接收端 SyncWord 配置为默认值 `0x12`，与 TX 端的 `0x34` 不匹配
4. **解决方案：** 将接收端 SyncWord 改为 `0x34`，重新测试后成功收包

---

## 五、快速验证检查清单

- [ ] 接收端 SyncWord = `0x34`
- [ ] 接收端 CRC 关闭（或 TX 开启 CRC）
- [ ] 频率一致：505.3 MHz
- [ ] SF 一致：SF12
- [ ] BW 一致：125 kHz
- [ ] 包头模式一致：Explicit Header
- [ ] 接收端 timeout > 1400 ms（SF12/BW125/16byte 空中时间）
