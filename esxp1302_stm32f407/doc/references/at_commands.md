# E77-400M22S AT 指令参考

## 概述

E77-400M22S 是亿佰特 (EBYTE) 生产的 LoRaWAN 模块，基于 STM32WLE5 芯片。
仅支持 LoRaWAN 协议（不支持 P2P LoRa），通过 UART AT 指令控制。

- 默认波特率: 9600 bps
- 行尾: `\r\n`
- 指令回显: 有

---

## 常用指令

### 基本测试

| 指令 | 说明 | 返回 |
|------|------|------|
| `AT` | 通信测试 | `OK` |
| `AT+VER` | 固件版本 | Version string |
| `AT+RESET` | 软复位 | `OK` |

### 入网配置

| 指令 | 说明 | 示例 |
|------|------|------|
| `AT+REGION=<n>` | 设置区域 (2=CN470) | `AT+REGION=2` |
| `AT+CDEVADDR=<addr>` | 设置 DevAddr (ABP) | `AT+CDEVADDR=26:01:12:34` |
| `AT+CNWKSKEY=<key>` | 设置 NwkSKey (32 hex) | `AT+CNWKSKEY=0011...EEFF` |
| `AT+CAPPSKEY=<key>` | 设置 AppSKey (32 hex) | `AT+CAPPSKEY=FFEE...0011` |
| `AT+CJOIN=<mode>:<auto>` | 入网 (0:0 = ABP 手动) | `AT+CJOIN=0:0` |

### 信道配置

| 指令 | 说明 | 示例 |
|------|------|------|
| `AT+CMANUALMASK=1` | 使能手动信道掩码 | |
| `AT+CFREQBANDMASK=<mask>` | 设置信道掩码 | `0000:0000:0000:0000:0000:00FF` |

掩码格式 (12 组 4-hex，每组 16 信道):
```
0000:0000:0000:0000:0000:00FF
                          ↑
                     SB10: CH80-CH87 全部使能
```

### 发送参数

| 指令 | 说明 | 示例 |
|------|------|------|
| `AT+CADR=0` | 关闭 ADR | |
| `AT+CDATARATE=<dr>` | 固定 DR (0=SF12..5=SF7) | `AT+CDATARATE=2` |
| `AT+CTXP=<n>` | 发射功率档 (0=最大) | `AT+CTXP=0` |

### 发送数据

```
AT+SEND=<mode>:<ack>:<port>:<hex_payload>
```

| 参数 | 说明 |
|------|------|
| mode | 2=LoRaWAN |
| ack | 0=无确认, 1=确认 |
| port | FPort (1-223) |
| hex_payload | 十六进制负载 |

示例:
```
AT+SEND=2:1:0:DEADBEEF01020304
```

响应事件:
- `+EVT:SEND_OK` — 发送成功
- `+EVT:SEND_CONFIRMED` — 确认发送成功
- `+EVT:RX_1` / `+EVT:RX_2` — 收到下行
- `AT_ERROR` — 发送失败

---

## ABP 入网流程 (完整)

```
AT                              → OK
AT+REGION=2                     → OK    (CN470)
AT+CDEVADDR=26:01:12:34         → OK
AT+CNWKSKEY=00112233...EEFF     → OK
AT+CAPPSKEY=FFEEDDCC...0011     → OK
AT+CTXP=0                      → OK
AT+CADR=0                       → OK
AT+CDATARATE=2                  → OK    (SF10)
AT+CMANUALMASK=1                → OK
AT+CFREQBANDMASK=0000:...00FF   → OK
AT+CJOIN=0:0                   → +EVT:JOINED
AT+SEND=2:1:0:DEADBEEF...      → +EVT:SEND_OK
```

---

## 注意事项

1. **DevAddr 格式**: AT 指令中使用冒号分隔 `26:01:12:34`，但也接受连续格式 `26011234`
2. **Key 长度**: NwkSKey 和 AppSKey 均为 32 个十六进制字符 (16 字节)
3. **入网后等待**: `AT+CJOIN=0:0` 后建议等待 1-2 秒再发送数据
4. **发送间隔**: CN470 duty cycle 限制较宽松，但建议至少间隔 5 秒
5. **串口回显**: 模块会回显发送的指令，解析响应时需过滤掉回显行
