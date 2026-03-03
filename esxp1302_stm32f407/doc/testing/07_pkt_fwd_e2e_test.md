# 07 — Packet Forwarder 端到端测试

## 概述

本测试验证 **ESXP1302（STM32F407 + SX1302 + W5500）Packet Forwarder** 的完整上行链路：

```
E77-400M22S 节点
  │  LoRa RF (CN470, 470.3 MHz, SF10BW125)
  ▼
SX1302 集中器 → libloragw 接收 → lora_pkt_fwd.c 打包
  │  UDP PUSH_DATA (Semtech UDP 协议)
  ▼
W5500 以太网 (192.168.10.15) ────── 直连网线 ──────  PC (192.168.10.1)
  │                                                       │
  │                                                  UDP :1680 监听
  │                                               lora_ns_mock.py / ChirpStack
  └─ UDP PULL_DATA (10s 心跳) ◄── PULL_ACK ─────────────┘
```

分两阶段测试：

| 阶段 | NS | 目的 |
|------|----|----|
| **阶段一（已完成）** | `lora_ns_mock.py` mock NS | 验证 GW→NS UDP 链路、上行转发、统计信息 |
| **阶段二（待做）** | ChirpStack（Linux 以太网）| 验证 OTAA 入网、下行 ACK、ADR |

---

## 测试环境

### 硬件

| 角色 | 设备 |
|------|------|
| 网关 | ESXP1302（STM32F407ZGTx + SX1302 + SX1250，W5500 以太网） |
| LoRaWAN 节点 | 成都亿佰特 E77-400M22S（CN470，STM32WLE5，UART 9600 8N1） |
| NS（阶段一）| PC 本机 `scripts/lora_ns_mock.py` |
| NS（阶段二）| Linux 主机 ChirpStack v4（Docker），端口 1700 |

### 网络（阶段一 — Mock NS）

| 设备 | IP | 说明 |
|------|----|------|
| PC 以太网口 | `192.168.10.1` | 与 W5500 直连，静态 |
| W5500 | `192.168.10.15` | 固件静态配置 |
| Mock NS 监听 | `0.0.0.0:1680` | PC 本机 |

### 网关固件配置（Flash v2，CONFIG_MAGIC=0xC0FFEE02）

| 参数 | 值 |
|------|----|
| Gateway EUI | `AA:55:5A:00:00:00:21:FB` |
| NS Host | `192.168.10.1` |
| NS Port Up/Down | `1680` |
| W5500 IP | `192.168.10.15` |
| 上行信道 | CH0~7（470.3~471.7 MHz，对应 CN470 Sub-Band 1） |

### E77 节点参数（阶段一 ABP，dummy keys）

| 参数 | 值 |
|------|----|
| DevEUI（出厂固化）| 通过 `AT+CDEVEUI=?` 查询（本次 `AABBCCDD11223344`） |
| DevAddr（测试用）| `26011234` |
| NwkSKey（测试用）| `00112233445566778899AABBCCDDEEFF` |
| AppSKey（测试用）| `FFEEDDCCBBAA99887766554433221100` |
| chanmask | `0001:0000:0000:0000:0000:0000`（SubBand1，CH0~7） |
| DataRate | DR2（SF10BW125） |
| 发送间隔 | 5 s |
| FPort | 2，Unconfirmed |

---

## 阶段一：Mock NS 测试（ABP 上行）

### 步骤

#### 1. 启动 Mock NS（PC 终端 A）

```powershell
cd esxp1302_stm32f407
python scripts/lora_ns_mock.py 1680
```

预期输出：
```
[INFO] Listening on 0.0.0.0:1680 ...
```

#### 2. 给网关上电，确认连接

串口 Monitor 应出现：
```
[down] PULL_ACK received in 5 ms
```

#### 3. E77 ABP 发包（PC 终端 B）

```powershell
python scripts/e77_node_ctrl.py abp `
    --port COM17 `
    --devaddr 26011234 `
    --nwkskey 00112233445566778899AABBCCDDEEFF `
    --appskey FFEEDDCCBBAA99887766554433221100 `
    --chanmask 0001:0000:0000:0000:0000:0000 `
    --adr 0 --dr 2 `
    --interval 5 `
    --payload DEADBEEF01020304
```

> **注意**：若 E77 出现 `AT_PARAM_ERROR` / `AT_ERROR` 说明有历史配置冲突，先执行
> `python scripts/e77_node_ctrl.py restore --port COM17` 清除后重新运行。

### 测试结果（2026-03-04，已验证 ✅）

#### E77 节点侧

```
[00:48:20.481] [INFO] AT 通信正常
[00:48:22.550] [OK]   ✓ ABP 入网成功！
[00:48:22.551] [INFO] 开始周期发送：间隔=5s  payload=DEADBEEF01020304  无限循环
[00:48:22.551] [INFO] ─── 第 1 包 ──────────
[00:48:22.551] [INFO] 发送上行 → port=2 ack=0 payload=DEADBEEF01020304
[00:48:25.229] [OK]   TX 确认: OK+SENT:00
```

#### Mock NS 侧（收到上行帧示例）

```
[2026-03-04 00:48:40.183] ← 192.168.10.15:1680  PUSH_DATA  token=c4af
  网关 EUI : AA:55:5A:00:00:00:21:FB
  上行帧数量: 1
  [rxpk 0]
    freq: 470.3   modu: LORA   datr: SF10BW125   codr: 4/5
    rssi: -79     lsnr: 6.8    size: 21
    data (hex): 40341201260004000223660e49aa300e96cc5b1282
  → 已回复 PUSH_ACK
```

#### 网关串口（关键行）

```
INFO: Received pkt from mote: 26011234 (fcnt=4)
JSON up: {"rxpk":[{"freq":470.300000,"datr":"SF10BW125","rssi":-79,"lsnr":6.8,"size":21,"data":"..."}]}
INFO: [up] PUSH_ACK received in 5 ms
INFO: [down] PULL_ACK received in 5 ms
```

#### 汇总指标（稳定后，Uptime ~02:10）

| 指标 | 值 |
|------|----|
| 接收包数（CRC_OK） | 100% |
| 上行转发率（RF→NS） | **100%** |
| PUSH_DATA ACK 率 | **100%** |
| PULL_DATA ACK 率 | **100%** |
| PUSH_ACK RTT | **5 ms** |
| 接收频率 | 470.3 MHz（CH0）|
| RSSI 范围 | -72 ~ -79 dBm |
| SNR 范围 | 6.5 ~ 10.8 dB |
| `ackr`（stat 字段）| 50.0% → **100.0%**（稳定）|

> **ackr 第一条为 50%** 是正常现象：启动时 stat 帧比第一个 rxpk 先发出，第一帧
> PUSH_ACK 尚未收到即已统计，第二条 stat 起稳定 100%。

#### 已知非关键错误

| 错误 | 原因 | 影响 |
|------|------|------|
| `failed to read LM75A temperature (0x4B)` | 板上未焊 0x4B 地址 LM75A（硬件接 0x48）| 无，`temp` 字段输出 `0.0` |
| `AT+REGION=2` → `AT_PARAM_ERROR` | E77 已存有历史 CN470 配置冲突 | 无，入网和发包均正常；新测试前先 `restore` 可规避 |

---

## 阶段二：ChirpStack OTAA 测试（待做）

> **前提**：切换到 Linux 以太网环境后，通过 UART CLI 修改 NS 地址（无需重新烧录）。

### 切换 NS 地址

用任意串口终端连接网关 UART（115200 bps），执行：

```
ns <linux_server_ip> 1700
save
```

其中 `<linux_server_ip>` 为 Linux 主机在以太网上的实际 IP，Gateway Bridge 默认端口 1700。

确认后网关串口会出现：
```
[cfg] ns_host = <linux_server_ip>
[cfg] ns_port_up = 1700 / ns_port_down = 1700
[down] PULL_ACK received in X ms
```

### E77 OTAA 命令

ChirpStack 中需预先创建设备，配置与下面参数一致：

| 参数 | 值 |
|------|----|
| DevEUI | E77 出厂值（`AT+CDEVEUI=?` 查询）|
| AppEUI（JoinEUI）| `0000000000000000` |
| AppKey | 自定义 32 hex，填入 ChirpStack Device |
| chanmask | `0000:0000:0000:0000:0000:00FF`（CN470-10，CH80~CH87）|
| Region | `2`（CN470）|

```bash
# Linux 端运行（将 COM17 换为 /dev/ttyUSBx）
python3 scripts/e77_node_ctrl.py restore --port /dev/ttyUSB0

python3 scripts/e77_node_ctrl.py otaa \
    --port /dev/ttyUSB0 \
    --region 2 \
    --deveui <AT+CDEVEUI=? 查询结果> \
    --appeui 0000000000000000 \
    --appkey <ChirpStack 中配置的 AppKey> \
    --chanmask 0000:0000:0000:0000:0000:00FF \
    --adr 1 \
    --interval 30 \
    --payload DEADBEEF01020304
```

> **chanmask 注意**：阶段一（SubBand1 = `0001:...:0000`，470.3~471.7 MHz）与  
> ChirpStack CN470-10（SubBand10 = `0000:...:00FF`，486.3~487.7 MHz）**不同**，  
> 切换到 ChirpStack 时必须更新 chanmask，否则节点发包频率与网关 RF 配置不匹配。

---

## 故障排查

| 现象 | 原因 | 处理 |
|------|------|------|
| PULL_ACK 收不到 | NS IP/Port 配置错误 | UART CLI `ns <ip> <port>` 修改 |
| PUSH_ACK 收不到 | PC 防火墙拦截 UDP 1680 | 放行入站 UDP 1680（或 1700）|
| E77 `AT_PARAM_ERROR` | 历史 REGION 配置冲突 | `e77_node_ctrl.py restore` 后重试 |
| `ackr` 始终 0% | Mock NS 未启动 / 端口不对 | 确认 mock NS 在正确端口监听 |
| OTAA 无 `+EVT:JOINED` | chanmask 与 GW RF 不匹配 / ChirpStack 中 AppKey 不一致 | 检查两侧 AppKey，检查频率计划 |
