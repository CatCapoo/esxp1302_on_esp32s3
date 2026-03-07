# Packet Forwarder OTAA 端到端测试备忘录

**日期：** 2026-03-07  
**测试目的：** 验证 STM32F407 版 ESXP1302 Packet Forwarder 的完整 OTAA
下行链路，排查并修复 JoinAccept 发送失败的根本原因（B18）。

---

## 一、测试配置

### 硬件

| 角色 | 设备 |
|------|------|
| 网关 | ESXP1302-STM32F407（SX1302 + SX1250 + W5500 以太网） |
| LoRaWAN 节点 | 成都亿佰特 E77-400M22S（CN470，STM32WLE5） |
| 节点串口 | `/dev/ttyUSB1`，9600 bps 8N1 |
| 网关串口（调试/CLI） | `/dev/ttyUSB0`，115200 bps |

### 软件/服务

| 组件 | 版本/配置 |
|------|---------|
| 网关固件 | 本工程 esxp1302_stm32f407（STM32CubeIDE + CMake） |
| ChirpStack | v4，Docker 部署，`~/chirpstack-docker/` |
| ChirpStack region | `cn470_10` |
| Gateway Bridge topic 前缀 | `cn470_10` |
| 控制脚本 | `scripts/e77_node_ctrl.py` |

### 网关 RF 配置（Flash 存储，`freq_region=CN470_10`）

| 参数 | 值 |
|------|----|
| Radio 0 中心频率 | 486.6 MHz |
| Radio 1 中心频率 | 487.4 MHz |
| 上行信道 | CH80~CH87（486.3 ~ 487.7 MHz） |
| 下行 RX1 范围 | 506.7 ~ 508.1 MHz |
| TX 频率范围（JSON 配置） | 470.0 ~ 510.0 MHz |
| Gateway EUI | `AA555A00000021FB` |
| NS 地址 | `192.168.71.100:1700` |

### E77 节点参数（ChirpStack 中已注册）

| 参数 | 值 | 备注 |
|------|----|------|
| DevEUI | `AABBCCDD11223344` | 脚本写入值，非出厂值 |
| AppEUI | `0000000000000000` | |
| AppKey | `00112233445566778899AABBCCDDEEFF` | ChirpStack 配置值 |
| chanmask | `0000:0000:0000:0000:0000:00FF` | CH80~CH87，CN470-10 |
| Region | `2`（CN470） | |

---

## 二、测试过程与问题定位

### 2.1 初始现象

使用 `e77_node_ctrl.py otaa` 触发 OTAA 入网，节点持续返回 `+EVT:JOIN FAILED`。

### 2.2 三层诊断法（来自 `doc/archive/learning/lorawan_node_e77_otaa_debug.md`）

```
层 1：物理层  → 看 gateway-bridge event=up      → SX1302 是否收到 JoinRequest
层 2：NS 层   → 看 chirpstack join 事件          → NS 是否处理并下发 JoinAccept
层 3：下行层  → 看 E77 +EVT:JOINED               → 节点是否收到 JoinAccept
```

**第一轮诊断**（修复 B18 前）：

| 层 | 诊断命令 | 结果 |
|----|---------|------|
| Layer 1 | `docker logs chirpstack-gateway-bridge-1 \| grep event` | 只有 `event=stats`，**无 `event=up`** |
| — | — | ❌ 上行未到达 NS，RF 层问题 |

继续排查 RF 层：

- `config show`（网关 UART CLI）确认：`radio0=486600000 Hz`、`radio1=487400000 Hz`、`ns_host=192.168.71.100:1700` 均正确。
- 修改 E77 天线朝向、缩短距离无效。

**查看网关统计输出**（`/tmp/otaa_log.txt` 监控网关串口）：

```
# RF packets received by concentrator: 1     ← 网关收到 1 包（JoinRequest）
# RF packets forwarded: 1 (23 bytes)          ← PUSH_DATA 上报 NS
# RF packets sent to concentrator: 1 (33 bytes)
# TX errors: 1                                ← JoinAccept 发送失败！
```

**第二轮诊断**（发现收包后 TX 失败）：

| 层 | 诊断命令 | 结果 |
|----|---------|------|
| Layer 1 | `gateway-bridge event=up` | ✅ 已出现（网关收到 JoinRequest） |
| Layer 2 | `chirpstack NS 日志` | ✅ `join_request dev_eui=aabbccdd11223344`，`join_accept` 已下发 |
| 下行 TX | 网关统计 `TX errors: 1` | ❌ JoinAccept 发送到 SX1302 失败 |

查看网关实时串口输出，找到精确错误：

```
ERROR: SELECTED RF_CHAIN IS DISABLED FOR TX ON SELECTED BOARD
```

### 2.3 根本原因（B18）

定位到 `packet_forwarder/lora_pkt_fwd.c` 的 Radio 配置函数。

从 JSON 读取 `tx_enable` 字段时，只将值存入局部数组 `conf_is_tx_enable[i]`，
**但从未赋值给 `rfconf.tx_enable`**，导致 `lgw_rxrf_setconf()` 传入的
`rfconf.tx_enable = 0`（C 全局变量默认零初始化）。

```c
// 修复前 —— rfconf.tx_enable 始终为 0
conf_is_tx_enable[i] = (bool)json_object_get_boolean(conf_obj, "tx_enable");
// ... 缺少：rfconf.tx_enable = conf_is_tx_enable[i];
if (lgw_rxrf_setconf(i, rfconf) != LGW_HAL_SUCCESS) { ... }

// 修复后
conf_is_tx_enable[i] = (bool)json_object_get_boolean(conf_obj, "tx_enable");
rfconf.tx_enable = conf_is_tx_enable[i];   // ← 补加
if (lgw_rxrf_setconf(i, rfconf) != LGW_HAL_SUCCESS) { ... }
```

**潜伏原因**：HAL RX 测试和 ABP 发包测试均未使用下行路径，此 Bug 从未被触发，
直到 OTAA JoinAccept 需要真正发送下行帧才暴露。

---

## 三、测试结果

### 修复后 OTAA 测试（最终验证）

测试命令：

```bash
# 先确保脚本已加入 auto-restore（已修复 B19）
python3 scripts/e77_node_ctrl.py otaa \
    --port /dev/ttyUSB1 --region 2 \
    --deveui AABBCCDD11223344 \
    --appeui 0000000000000000 \
    --appkey 00112233445566778899AABBCCDDEEFF \
    --chanmask 0000:0000:0000:0000:0000:00FF \
    --adr 1 --count 3
```

**脚本输出（2026-03-07 21:06）：**

```
[21:06:37.936] [INFO] AT 通信正常
[21:06:37.936] [INFO] 恢复出厂配置 ...
[21:06:38.023] [INFO] 出厂配置已恢复，等待重启 ...
[21:06:40.575] [INFO] ✓ 设置频段: AT+REGION=2
[21:06:40.766] [INFO] ✓ 设置 DevEUI: AT+CDEVEUI=AABBCCDD11223344
[21:06:40.957] [INFO] ✓ 设置 AppEUI: AT+CAPPEUI=0000000000000000
[21:06:41.165] [INFO] ✓ 设置 AppKey: AT+CAPPKEY=00112233445566778899AABBCCDDEEFF
[21:06:41.336] [INFO] ✓ 发射功率: AT+CTXP=0
[21:06:41.508] [INFO] ✓ ADR: AT+CADR=1
[21:06:41.710] [INFO] ✓ 手动掩码使能: AT+CMANUALMASK=1
[21:06:41.920] [INFO] ✓ 信道掩码: AT+CFREQBANDMASK=0000:0000:0000:0000:0000:00FF
[21:06:42.071] [INFO] 触发 OTAA 入网 (region=CN470) ...
[21:06:50.609] [OK] ✓ OTAA 入网成功！
[21:06:50.609] [INFO] ─── 第 1 包 ────────────────────────────────────────
[21:06:54.345] [OK]   TX 确认: OK+SENT:00
[21:06:54.345] [INFO]   本次无下行（Class A 正常现象）
[21:07:24.374] [INFO] ─── 第 2 包 ────────────────────────────────────────
[21:07:24.375] [DOWN]   下行收到: +EVT:RX_1, PORT 0, DR 0, RSSI -64, SNR 4
[21:07:25.551] [OK]   TX 确认: OK+SENT:00
[21:07:25.579] [DOWN]   下行收到: +EVT:RX_1, PORT 0, DR 5, RSSI -64, SNR 10
[21:08:06.401] [INFO] ─── 第 3 包 ────────────────────────────────────────
[21:08:07.572] [OK]   TX 确认: OK+SENT:00
[21:08:07.600] [DOWN]   下行收到: +EVT:RX_1, PORT 0, DR 5, RSSI -64, SNR 10
[21:08:18.414] [INFO] 共发送 3 包，结束。
```

**gateway-bridge 同步日志（节选）：**

```
event=up   uplink_id=9869    ← JoinRequest 收包
event=ack  downlink_id=...   ← JoinAccept 发送 ack（TX 成功）
event=up   uplink_id=8384    ← 第 1 包上行
event=ack                    ← 第 1 包 ACK
event=up   uplink_id=13586   ← 第 2 包上行（含 ADR 下行）
event=ack
event=up   uplink_id=14753   ← 第 3 包上行
event=ack
```

**ChirpStack NS 日志（节选）：**

```
INFO join_request dev_eui="aabbccdd11223344" dev_nonce=19034
INFO Device queue flushed dev_eui=aabbccdd11223344
INFO Sending downlink frame region_id=cn470_10 ...
INFO Publishing event topic=.../event/join        ← NS 触发 join 事件
INFO tx_ack downlink_id=... dev_eui=aabbccdd11223344
INFO DevStatusAns battery=254 margin=7             ← ADR/DevStatus 收到响应
INFO Pending mac-command LinkADRReq
```

### 验证结果汇总

| 检查项 | 结果 |
|--------|------|
| 所有 AT 配置命令成功（无 WARN） | ✅ |
| OTAA 入网 `+EVT:JOINED` | ✅ |
| JoinRequest `event=up`（Layer 1） | ✅ |
| JoinAccept `event=ack`（Layer 2→3 下行） | ✅ |
| ChirpStack NS `event/join` 发布 | ✅ |
| 上行包 3/3 `OK+SENT:00` | ✅ |
| 下行 RX_1 收到 ADR 命令（包 2、3） | ✅ RSSI=-64 SNR=10 |
| ADR 调整 DR（包 2 起 DR5） | ✅ |
| `TX errors: 0`（网关统计） | ✅ |

---

## 四、额外：Confirmed 模式持续发包验证

在完成 OTAA 后追加了 Confirmed 模式测试（`--ack 1`，8 包，间隔 10s）：

```bash
python3 scripts/e77_node_ctrl.py otaa \
    --port /dev/ttyUSB1 --region 2 \
    --deveui AABBCCDD11223344 \
    --appeui 0000000000000000 \
    --appkey 00112233445566778899AABBCCDDEEFF \
    --chanmask 0000:0000:0000:0000:0000:00FF \
    --adr 1 --ack 1 --interval 10 --payload DEADBEEF01020304
```

结果：8 包全部 `OK+SENT:01` + `+EVT:SEND_CONFIRMED`，包 2 起每包均收到
`+EVT:RX_1 DR5 RSSI=-64 SNR=10` 下行确认，说明 ADR 已快速收敛至 DR5，
上下行双向链路稳定。

---

## 五、关联文档

| 文档 | 内容 |
|------|------|
| [bugs_and_fixes.md](../../troubleshooting/bugs_and_fixes.md#b18) | B18 根因分析 |
| [bugs_and_fixes.md](../../troubleshooting/bugs_and_fixes.md#b19) | B19 E77 AT 参数锁定 |
| [lorawan_node_e77_otaa_debug.md](../learning/lorawan_node_e77_otaa_debug.md) | 三层 JOIN 诊断方法 |
| [07_pkt_fwd_e2e_test.md](../../testing/07_pkt_fwd_e2e_test.md) | 完整测试步骤文档 |
| [test_chirpstack_cn470_10_setup_memo.md](test_chirpstack_cn470_10_setup_memo.md) | ChirpStack CN470-10 配置参考 |
