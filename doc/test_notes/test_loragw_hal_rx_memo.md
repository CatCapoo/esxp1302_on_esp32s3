# test_loragw_hal_rx 使用备忘录

**日期**：2026-02-20

---

## 接收端（SX1302）启动命令

```
test_loragw_hal_rx -r 1250 -a 480.0 -b 480.0 -k 0 -m 1
```

| 参数 | 值 | 说明 |
|------|----|------|
| `-r` | 1250 | 射频前端类型 SX1250 |
| `-a` / `-b` | 480.0 | Radio A / B 中心频率 (MHz) |
| `-k` | 0 | 时钟源选 Radio A |
| `-m` | 1 | 信道模式 1：所有信道集中在中心频率 -400 kHz 偏移处 (= 480.0 MHz) |

---

## 发送端（SX1278）配置（RadioLib）

```cpp
radio.begin(480.0, 125.0, 12, 5, 0x12, 17, 8);
radio.setCRC(false);
```

| 参数 | 值 | 说明 |
|------|----|------|
| 频率 | 480.0 MHz | 与接收端信道对齐 |
| BW | 125 kHz | LoRa 标准带宽 |
| SF | 12 | 最大扩频因子 |
| CR | 4/5 | 编码率 |
| 同步字 | 0x12 | LoRa 私有网络默认值 |
| CRC | 关闭 | `setCRC(false)` |

---

## RX 验证关键点

**问题**：代码默认 `lorawan_public = true`，对应同步字 `0x34`（LoRaWAN 公共网络），
而 SX1278 非 LoRaWAN 模式默认同步字为 `0x12`，导致 SX1302 无法识别数据包，收不到任何内容。

**修复**：在 `main/test/test_loragw_hal_rx.c` 中将：
```c
boardconf.lorawan_public = true;
```
改为：
```c
boardconf.lorawan_public = false;  // 使用私有同步字 0x12
```

---

## 验证结果

SX1278 发送 `"hello world"`，SX1302 成功接收：

```
68 65 6C 6C 6F 20 77 6F 72 6C 64
```
