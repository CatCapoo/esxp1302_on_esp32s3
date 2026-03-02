# LoRaWAN 节点调试学习笔记：E77-400M22S OTAA 全链路

> 写作背景：2026-02-27 使用成都亿佰特 E77-400M22S（CN470）通过 AT 指令接入
> ESXP1302 网关 + ChirpStack v4，经历多轮 JOIN FAILED 后最终完整打通。
> 完整过程记录见 [test_notes/test_e77_lorawan_node_validation_memo.md](../test_notes/test_e77_lorawan_node_validation_memo.md)

---

## 一、CN470 chanmask 的双重作用

这是本次调试最核心的 Learning。

CN470 的 chanmask 不只是决定节点**上行频率**，还同时决定节点如何**计算 RX1 下行窗口频率**。

### 1.1 上行频率（容易想到）

chanmask 告诉节点哪些信道可用，节点在这些信道上轮转发送 JoinRequest / 数据帧。
如果 chanmask 与网关实际监听信道不对齐，网关收不到上行包。

### 1.2 RX1 下行频率（容易忽略）

CN470 的 RX1 下行频率由上行信道的**绝对编号**决定：

$$f_{RX1} = 500.3 + (ch_{uplink} \bmod 48) \times 0.2 \text{ MHz}$$

节点必须知道自己发出的那包 JoinRequest 走的是 CN470 第几号信道，
才能在正确的 RX1 频率上等待 JoinAccept。

如果 chanmask 告诉节点"你在 CH0~CH7"，但实际上网关在 CH80~CH87：

| 情形 | 节点认为自己在 | 节点计算 RX1 | ChirpStack 实际发 JoinAccept |
|------|-------------|------------|---------------------------|
| chanmask 错（00FF...） | CH0~CH7（内部序 0~7） | 500.3~501.7 MHz | 506.7~508.1 MHz |
| chanmask 对（...00FF） | CH80~CH87 | **506.7~508.1 MHz** ✅ | 506.7~508.1 MHz ✅ |

6.4 MHz 的频率差距，JoinAccept 永远收不到，一直 JOIN FAILED。

### 1.3 本工程的正确 chanmask

```
--chanmask 0000:0000:0000:0000:0000:00FF
```

**chanmask 结构：** 6 段 × 16bit，mask0=CH0~15，mask1=CH16~31，...，mask5=CH80~95

`00FF` in mask5 = bit0~7 of the 6th segment = **CH80~CH87**

> ⚠️ CN470 与 EU868/US915 的重要区别：EU868 的 RX1 是固定频率偏移，US915 也是
> 固定公式，不依赖节点"知道自己在哪个信道"。CN470 的 RX1 基于绝对信道号，
> 因此 chanmask 的正确性直接影响下行能否收到。

---

## 二、OTAA JOIN FAILED 分层诊断法

JOIN 失败可能在三个独立位置断链，需要从底层向上逐层排除：

```
节点发送 JoinRequest
        │
        ▼
┌───────────────────────────────────────┐
│ 层 1：物理层（RF 频率）               │
│ 诊断：网关 Monitor 有无 JSON up？     │
│       gateway-bridge event=up 计数？  │
│ 无 → chanmask 错，上行频率不对        │
└───────────────────────────────────────┘
        │ 有 JSON up
        ▼
┌───────────────────────────────────────┐
│ 层 2：网络服务器层                    │
│ 诊断：chirpstack 日志有无 join 事件？ │
│       Device Events 有无 join？       │
│ 无 → AppKey 不匹配 / DevEUI 未注册   │
└───────────────────────────────────────┘
        │ 有 join，看到 JoinAccept 下发
        ▼
┌───────────────────────────────────────┐
│ 层 3：下行接收层（RX1 频率）         │
│ 诊断：网关有无 JSON down？            │
│       下行频率是否与 RX1 公式一致？   │
│ 节点仍 JOIN FAILED → RX1 频率偏移   │
│ → chanmask 导致节点 RX1 计算错误     │
└───────────────────────────────────────┘
        │ 节点收到 JoinAccept
        ▼
  +EVT:JOINED ✅
```

**关键：** gateway-bridge 日志是 NS 侧的第一个监控点。出问题时先看它，
比直接看 chirpstack 日志定位更快。

---

## 三、E77-400M22S AT 接口要点

### 3.1 pyserial 打开串口触发硬件复位

pyserial `serial.Serial()` 默认会操作 DTR/RTS 引脚，很多 USB-UART 芯片将
这两根线接到模块的 NRST/BOOT，导致模块复位。打开串口后立即发 AT 必然失败。

**必须设置：**
```python
serial.Serial(port, baud,
    dsrdtr=False,   # 不自动操作 DSR/DTR
    rtscts=False,   # 不硬件流控
    xonxoff=False,  # 不软件流控
)
time.sleep(1.5)  # 等待上电/复位完成
ser.reset_input_buffer()
```

### 3.2 入网前不能设置 AT+CCLASS

E77 手册：Class A 是入网默认模式，入网前 `AT+CCLASS=X` 返回 `AT_NO_NETWORK_JOINED`。
只有在 `+EVT:JOINED` 之后才能切换 Class B/C。

### 3.3 模块 echo on：响应中包含命令回显

模块默认开启 echo，发送 `AT+REGION=2` 后会先收到 `AT+REGION=2` 再收到 `OK`。
解析响应时需要过滤掉与发送命令相同的行，否则会误判。

```python
echo_str = cmd.strip().upper()
lines = [l for l in raw_lines if l.upper() != echo_str]
```

### 3.4 `AT+CJOIN=1:0` 参数含义

```
AT+CJOIN=<JOIN_MODE>:<AUTO_JOIN>
         1=OTAA      0=不自动重连
         0=ABP       1=上电自动入网
```

OTAA 手动测试用 `AT+CJOIN=1:0`，ABP 用 `AT+CJOIN=0:0`。

### 3.5 `AT+SEND` 格式

```
AT+SEND=<PORT>:<RETRIES>:<ACK>:<PAYLOAD_HEX>
```

- `RETRIES`：Confirmed uplink 未收到 ACK 时的重传次数
- `ACK=1`：Confirmed uplink（双向确认）；`ACK=0`：Unconfirmed
- 响应 `OK+SENT:XX`：XX 为实际重传次数（首次发送算 0）

### 3.6 AT+RESTORE 用于清除旧 chanmask

调试过程中多次修改 chanmask 后，旧配置可能持久化在模块 Flash 中，
导致行为与预期不符。每次切换频段计划前建议先 `AT+RESTORE` 恢复出厂。

---

## 四、CN470 RX1/RX2 速查

### RX1 频率公式

$$f_{RX1} = 500.3 + (ch_{uplink} \bmod 48) \times 0.2 \text{ MHz}$$

### 本工程 CN470_10（CH80~CH87）RX1 映射

| 上行信道 | 上行频率 | ch mod 48 | RX1 下行频率 |
|---------|---------|-----------|------------|
| CH80 | 486.3 MHz | 32 | 506.7 MHz |
| CH81 | 486.5 MHz | 33 | 506.9 MHz |
| CH82 | 486.7 MHz | 34 | 507.1 MHz |
| CH83 | 486.9 MHz | 35 | 507.3 MHz |
| CH84 | 487.1 MHz | 36 | 507.5 MHz |
| CH85 | 487.3 MHz | 37 | 507.7 MHz |
| CH86 | 487.5 MHz | 38 | 507.9 MHz |
| CH87 | 487.7 MHz | 39 | 508.1 MHz |

### RX2（固定）

**505.3 MHz，SF12BW125**，上行后 2 秒。节点未收到 RX1 响应时自动切换到 RX2。

---

## 五、ADR 工作机制

- **开启方法：** `AT+CADR=1`，ChirpStack 侧 Device Profile 也须开启 ADR
- **调速原理：** ChirpStack 收集历史 SNR/RSSI 样本，计算最大可用 DR，通过 `LinkADRReq` MAC 命令下发新 DR 和 TXPower
- **生效时机：** 通常 3~5 包后（需要足够的 SNR 样本）
- **本次测试结果：** 入网 SF12（DR0）→ 第 3~4 包后 SF7（DR5），RSSI -74 dBm，SNR 9 dB

> ADR 调速后节点每包耗时从 ~2.5 秒降到 ~0.1 秒（SF12 vs SF7 on air time），
> 对电池供电节点功耗影响显著。

---

## 六、相关文档

| 文档 | 说明 |
|------|------|
| [lora_cn470_frequency_plan.md](lora_cn470_frequency_plan.md) | CN470 频率计划完整体系 |
| [chirpstack_v4_usage.md](chirpstack_v4_usage.md) | ChirpStack 配置与操作 |
| [test_notes/test_e77_lorawan_node_validation_memo.md](../test_notes/test_e77_lorawan_node_validation_memo.md) | 本次测试完整过程记录（含复现步骤和测试数据） |
| `scripts/e77_node_ctrl.py` | E77 控制脚本源码 |

---

*写作日期：2026-02-27*
