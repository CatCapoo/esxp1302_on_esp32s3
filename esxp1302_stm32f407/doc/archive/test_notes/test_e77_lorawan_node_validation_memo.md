# E77-400M22S LoRaWAN 节点完整功能验证备忘录

**日期：** 2026-02-27  
**测试目的：** 使用成都亿佰特 E77-400M22S 节点验证 ESXP1302 网关完整 LoRaWAN
功能链路，包括 OTAA 入网、上行数据、下行 ACK、ADR 自适应速率。

---

## 一、测试配置

### 硬件

| 角色 | 设备 |
|------|------|
| 网关 | ESXP1302（ESP32-S3 + SX1302 + SX1250） |
| LoRaWAN 节点 | 成都亿佰特 E77-400M22S（CN470，STM32WLE5） |
| 节点连接方式 | UART 9600 8N1，`/dev/ttyUSB0` |

### 软件/服务

| 组件 | 版本/配置 |
|------|---------|
| 网关固件 | 本工程 ESXP1302 |
| ChirpStack | v4，Docker 部署，`~/chirpstack-docker/` |
| ChirpStack region | `cn470_10` |
| Gateway Bridge | UDP 端口 `1680`（注意：非默认 1700） |
| 控制脚本 | `scripts/e77_node_ctrl.py` |

### 网关 RF 配置（cn490.json，已验证）

| 参数 | 值 |
|------|----|
| Radio 0 中心频率 | 486.6 MHz |
| Radio 1 中心频率 | 487.4 MHz |
| 上行信道 | CH80~CH87（CN470 标准编号） |
| 上行频率范围 | 486.3 ~ 487.7 MHz |
| 下行 RX1 范围 | 506.7 ~ 508.1 MHz |
| 下行 RX2 固定频率 | 505.3 MHz |
| Gateway EUI | `AA555A00000021FB` |

### E77 节点关键参数（测试用，已替换）

| 参数 | 值 |
|------|----|
| DevEUI | 通过 `AT+CDEVEUI=?` 查询获取 |
| AppKey | 在 ChirpStack Device 详情页生成 |
| chanmask | `0000:0000:0000:0000:0000:00FF`（CH80~CH87） |
| Region | `2`（CN470） |
| 串口波特率 | 9600 bps |

---

## 二、控制脚本用法

脚本路径：`scripts/e77_node_ctrl.py`，依赖：`pip3 install pyserial`

### 2.1 子命令一览

| 子命令 | 说明 |
|--------|------|
| `otaa` | OTAA 入网并周期发包 |
| `abp` | ABP 本地入网并周期发包 |
| `query` | 查询模块当前所有参数（不入网） |
| `send` | 仅发一包（已入网前提） |
| `restore` | 恢复出厂配置 |

### 2.2 快速测试命令（实际可用）

以下命令已根据实际设备和 ChirpStack 配置填入参数，可直接复制执行：

| 参数来源 | 值 |
|---------|-----|
| **DevEUI** | 从模块查询：`AT+CDEVEUI=?` |
| **AppKey** | 自定义，需在 ChirpStack Device 详情页设置为同一值 |
| **AppEUI** | 固定 `0000000000000000`（自建测试） |
| **chanmask** | 本工程网关固定 `0000:0000:0000:0000:0000:00FF`（CH80~CH87） |

> **⚠️ 重要：** 首次测试或发现 AT 命令被拒绝时，务必先执行下面的 **⑨ 清除旧配置** 命令清空模块持久化配置，再进行其他测试。

#### ① 查询模块当前状态
```bash
python3 scripts/e77_node_ctrl.py query --port /dev/ttyUSB0
```

**输出示例：**
```
DevEUI: AABBCCDD11223344
AppEUI: 0000000000000000
Region: 2:CN470
```

#### ② OTAA 入网 + 周期发包（30秒间隔）

**前置条件：** 在 ChirpStack 中创建设备，设置 AppKey = `00112233445566778899AABBCCDDEEFF`

```bash
python3 scripts/e77_node_ctrl.py otaa \
    --port /dev/ttyUSB0 \
    --region 2 \
    --deveui AABBCCDD11223344 \
    --appeui 0000000000000000 \
    --appkey 00112233445566778899AABBCCDDEEFF \
    --chanmask 0000:0000:0000:0000:0000:00FF \
    --adr 1 \
    --interval 30 \
    --payload DEADBEEF01020304
```

**预期结果：**
```
[19:41:02] [OK  ] ✓ OTAA 入网成功！
[19:41:03] [INFO] ─── 第 1 包 ───────────────────
[19:41:04] [OK  ] TX 确认: +EVT:SEND_CONFIRMED
[19:41:05] [DOWN] 下行收到: +EVT:RX_1, PORT 0, DR 5, RSSI -74, SNR 9
[19:41:35] [INFO] ─── 第 2 包 ───────────────────
...（每 30 秒发一包，无限循环）
```

#### ③ OTAA 入网 + Confirmed uplink（验证双向通信，10秒间隔）

```bash
python3 scripts/e77_node_ctrl.py otaa \
    --port /dev/ttyUSB0 \
    --region 2 \
    --deveui AABBCCDD11223344 \
    --appeui 0000000000000000 \
    --appkey 00112233445566778899AABBCCDDEEFF \
    --chanmask 0000:0000:0000:0000:0000:00FF \
    --adr 1 \
    --interval 10 \
    --ack 1 \
    --payload DEADBEEF01020304
```

> `--ack 1` = Confirmed uplink（MAC layer ACK），每包都等待下行 ACK。  
> 用于验证双向链路正常。

#### ④ OTAA 入网 + 发固定次数（5包后退出）

```bash
python3 scripts/e77_node_ctrl.py otaa \
    --port /dev/ttyUSB0 \
    --region 2 \
    --deveui AABBCCDD11223344 \
    --appeui 0000000000000000 \
    --appkey 00112233455566778899AABBCCDDEEFF \
    --chanmask 0000:0000:0000:0000:0000:00FF \
    --interval 20 \
    --count 5 \
    --ack 1
```

**结果：** 发 5 包后脚本自动退出。用于快速验证链路状态。

#### ⑤ ABP 入网（不需要 Join 流程，直接使用静态密钥）

**前置条件：** 在 ChirpStack Device → **Activation** 标签页手动填写以下值：
- DevAddr: `26011234`
- NwkSEncKey: `00112233445566778899AABBCCDDEEFF`
- AppSKey: `FFEEDDCCBBAA99887766554433221100`

```bash
python3 scripts/e77_node_ctrl.py abp \
    --port /dev/ttyUSB0 \
    --region 2 \
    --devaddr 26011234 \
    --nwkskey 00112233445566778899AABBCCDDEEFF \
    --appskey FFEEDDCCBBAA99887766554433221100 \
    --chanmask 0000:0000:0000:0000:0000:00FF \
    --adr 0 \
    --dr 2 \
    --interval 60
```

> ABP 跳过 Join 流程，直接入网。用于对比测试或网络调试。

#### ⑥ 恢复出厂配置（清除旧设置）

```bash
python3 scripts/e77_node_ctrl.py restore --port /dev/ttyUSB0
```

**用途：** 测试前清除旧配置，避免历史参数干扰（如 chanmask、AppKey 等）。

#### ⑦ verbose 模式（调试串口通信）

```bash
python3 scripts/e77_node_ctrl.py query --port /dev/ttyUSB0 --verbose
```

**效果：** 打印所有串口收发的原始字符串，用于诊断通信问题。

#### ⑧ 仅发一包（已入网后）

```bash
python3 scripts/e77_node_ctrl.py send \
    --port /dev/ttyUSB0 \
    --payload CAFEBABE \
    --port-fwd 10 \
    --ack 0
```

**前置条件：** 节点已成功 OTAA 或 ABP 入网。  
**效果：** 发送一个 FPort=10、payload=CAFEBABE 的 unconfirmed 上行，然后退出。

#### ⑨ 清除旧配置（开始新测试前必执行）

```bash
python3 scripts/e77_node_ctrl.py restore --port /dev/ttyUSB0
```

**用途：** 清除模块中所有旧的持久化配置（DevEUI、AppKey、AppEUI、频段、信道掩码等），
并触发硬件重启。开始任何新的测试前必须执行此命令，避免历史参数干扰。

**等待时间：** 命令执行后等待 2~3 秒让模块完成重启，再执行后续命令。

**症状：** 如果遇到 `AT+REGION=X` 返回 `AT_PARAM_ERROR` 或 `AT+CDEVEUI/CAPPEUI/CAPPKEY` 返回 `AT_ERROR`，
说明模块有旧配置冲突，立即执行此命令清除。

---

## 三、测试结果

### 3.1 OTAA 入网

**结论：✅ 成功**

入网耗时约 5~8 秒，串口日志示例：
```
[19:41:01.234] [INFO] ✓ 设置频段: AT+REGION=2
[19:41:01.389] [INFO] ✓ 设置 DevEUI: AT+CDEVEUI=XXXXXXXXXXXX
[19:41:01.549] [INFO] ✓ 设置 AppEUI: AT+CAPPEUI=0000000000000000
[19:41:01.704] [INFO] ✓ 设置 AppKey: AT+CAPPKEY=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
[19:41:01.859] [INFO] ✓ 发射功率: AT+CTXP=0
[19:41:02.017] [INFO] ✓ ADR: AT+CADR=1
[19:41:02.176] [INFO] ✓ 手动掩码使能: AT+CMANUALMASK=1
[19:41:02.334] [INFO] ✓ 信道掩码: AT+CFREQBANDMASK=0000:0000:0000:0000:0000:00FF
[19:41:02.334] [INFO] ──────────────────────────────────────────────────
[19:41:02.335] [INFO] 触发 OTAA 入网 (region=CN470) ...
[19:41:12.001] [OK  ] ✓ OTAA 入网成功！
```

网关 Monitor 同步出现 JoinRequest 上行和 JoinAccept 下行：
```
JSON up:  freq:487.100000  stat:1  data:"AAAAAA...RDM..."   ← JoinRequest
PULL_RESP received  ← ChirpStack 推送下行
JSON down: freq:507.5 size:33                               ← JoinAccept
```

### 3.2 上行数据

**结论：✅ 成功，57+ 包连续发送零丢包**

典型单包日志：
```
[19:41:13.002] [INFO] ─── 第 1 包 ───────────────────────────────
[19:41:13.003] [INFO] 发送上行 → port=2 ack=1 payload=DEADBEEF01020304
[19:41:14.891] [OK  ] TX 确认: OK+SENT:01
[19:41:14.892] [OK  ] TX 确认: +EVT:SEND_CONFIRMED
[19:41:15.201] [DOWN] 下行收到: +EVT:RX_1, PORT 0, DR 5, RSSI -74, SNR 9
```

| 字段 | 含义 |
|------|------|
| `OK+SENT:01` | 发送完成，重传 1 次（Confirmed uplink 正常行为） |
| `+EVT:SEND_CONFIRMED` | 收到 ChirpStack 下发的 ACK，双向链路正常 |
| `+EVT:RX_1, PORT 0` | 下行在 RX1 窗口收到，PORT=0 表示 MAC 层 ACK |
| `DR 5` | 下行数据速率 DR5（SF7），ADR 已将速率优化到最快 |
| `RSSI -74` | 接收信号强度 -74 dBm（室内测试，良好） |
| `SNR 9` | 信噪比 9 dB（优秀，SF7 最低可用 -7.5 dB） |

ChirpStack Device Events 截面可见连续 `uplink` 和 `ack` 事件，帧计数递增无跳号。

### 3.3 下行数据（手动入队）

**结论：✅ 成功**

在 ChirpStack GUI → Device → Queue 入队 FPort=2 Payload=`AABBCC`，
等待节点下一次上行触发 RX1，节点串口出现：
```
[DOWN] 下行收到: +EVT:RX_1, PORT 2, DR 5, RSSI -72, SNR 9
```

### 3.4 ADR 自适应速率

**结论：✅ 成功，SF12 → SF7 自动调整**

网关近距离放置（约 1~2 米），ChirpStack ADR 算法通过 LinkADRReq MAC 命令在
前 3~4 包内完成调速：
- 入网初始：DR0（SF12BW125）
- 第 3 包后：DR5（SF7BW125）
- 稳定后 RSSI：-72 ~ -74 dBm，SNR：9 dB

### 3.5 综合信号质量

| 指标 | 典型值 | 评价 |
|------|--------|------|
| RSSI | -72 ~ -74 dBm | 良好（室内近距离，天线直连型） |
| SNR | 9 dB | 优秀（SF7 容限范围内有大量余量） |
| 频率偏差 foff | ~300~500 Hz | 正常，晶振误差范围 |
| 丢包率（57包样本） | 0% | 稳定 |

---

## 四、遇到的问题与复现

### 问题 1：首次运行立即返回 `AT_ERROR`

**现象：**
```
[INFO] 打开串口 /dev/ttyUSB0 @ 9600 bps
[WARN] 指令失败: AT  响应: ['AT_ERROR']
[WARN] AT 测试第 1/3 次失败
```

**根本原因：pyserial 打开串口时默认拉动 DTR/RTS 电平，触发模块硬件复位**

E77-400M22S 的 NRST 引脚连接到 USB-UART 转换芯片的 DTR/RTS（常见接法），
pyserial 在 `serial.Serial()` 构造时会将这两根线拉高/拉低，使模块进入复位状态。
复位完成（约 1 秒）前发送 AT 必然返回 `AT_ERROR`。

**复现步骤：**
1. 代码中 `serial.Serial(port, baud, timeout=0.3)` — 不设 dsrdtr/rtscts
2. 立刻发送 `AT` 指令

**修复：**
```python
self.ser = serial.Serial(
    port=port, baudrate=baud,
    dsrdtr=False,   # 禁止自动操作 DSR/DTR
    rtscts=False,   # 禁止硬件流控
    xonxoff=False,  # 禁止软件流控
)
time.sleep(1.5)                     # 等待模块上电就绪
self.ser.reset_input_buffer()       # 丢弃启动消息
```
同时 `at_test()` 增加 3 次重试，兼容其他触发复位的场景。

---

### 问题 2：`AT+CCLASS=A` 返回 `AT_NO_NETWORK_JOINED`

**现象：**
```
[WARN] 指令失败: AT+CCLASS=A  响应: ['AT_NO_NETWORK_JOINED']
```
配置步骤被中断，后续命令未执行。

**根本原因：E77 手册规定 Class A 是入网默认模式，在未入网状态下无法设置 CCLASS**

> 手册原文：Class A 类型适用于上行数据报文，E77 默认为 Class A，设备在入网前不支持更改 Class 类型。

**复现步骤：**
1. 在 `config_otaa()` 中添加 `steps.append(("AT+CCLASS=A", "设置类型"))`
2. 未入网时发送该命令

**修复：**  
直接从 `config_otaa()` 和 `config_abp()` 中删除 `AT+CCLASS=A` 配置步骤，
入网后如需切换 Class C 才需再执行。

---

### 问题 3：OTAA JOIN FAILED（第一阶段）——上行频率完全不对

**现象：**
```
[19:xx:xx] [ERROR] ✗ OTAA 入网失败！响应: ['+EVT:JOIN FAILED']
```
网关 Monitor 无任何 `JSON up` 日志（gateway-bridge 日志 `event=up` 计数为零）。

**根本原因：chanmask `00FF:0000:0000:0000:0000:0000` 对应 CH0~CH7（470.3~471.7 MHz），
而网关实际监听 CH80~CH87（486.3~487.7 MHz），相差 16 MHz**

初始传入 `--chanmask 00FF:0000:0000:0000:0000:0000`（SubBand1），
E77 认为上行在 470.3~471.7 MHz，网关根本没有这些频率的收发能力，
Join Request 从未到达网关。

**诊断过程：**
1. 网关 Monitor：无 `JSON up` 输出 → 射频层完全没有收到包
2. 检查网关实际 radio 频率：`radio 0 center=486600000, radio 1 center=487400000`
3. 8 个上行信道：486.3~487.7 MHz → CN470 CH80~CH87
4. 对应 chanmask：mask5 低8位 = `0xFF` → `0000:0000:0000:0000:0000:00FF`

**修复：** `--chanmask 0000:0000:0000:0000:0000:00FF`

---

### 问题 4：OTAA JOIN FAILED（第二阶段）——RX1 下行频率不对齐 ⭐ 最核心

> 这是本次调试中最隐蔽也最具学习价值的问题。

**现象：**  
更正 chanmask 后，网关 Monitor 确认上行已收到 JoinRequest，
ChirpStack 也推送了 JoinAccept 下行，但节点始终 JOIN FAILED：
```
JSON up:  freq:487.100000  stat:1  ← JoinRequest 收到 ✅
PULL_RESP received                 ← ChirpStack 推送 JoinAccept ✅
JSON down: freq:507.5 size:33      ← 网关发出下行 ✅
+EVT:JOIN FAILED                   ← 节点就是收不到 ❌
```

**根本原因：E77 内部逻辑信道编号与 CN470 绝对信道编号不一致，导致计算 RX1 错误**

E77 的 chanmask 从 mask0 低位（bit0）开始线性编号内部信道，
即 mask5 bit0 = 内部信道 80，但 E77 固件的 RX1 计算公式可能基于的是
**已启用信道的本地序号**（0~7），而不是 CN470 绝对信道号（80~87）：

| E77 内部逻辑编号 | CN470 绝对信道 | E77 计算 RX1（错误）| 正确 RX1 |
|----------------|--------------|---------------------|---------|
| 0 | CH80 | 500.3 + 0×0.2 = 500.3 MHz | 506.7 MHz |
| 4 | CH84 | 500.3 + 4×0.2 = **501.1 MHz** | **507.5 MHz** |

ChirpStack 按 CN470 规范计算：$f_{RX1} = 500.3 + (ch \bmod 48) \times 0.2$

以 CH84 为例：$500.3 + (84 \bmod 48) \times 0.2 = 500.3 + 7.2 = 507.5\ \text{MHz}$

E77 按内部序号 4 计算：$500.3 + 4 \times 0.2 = 501.1\ \text{MHz}$

两者相差 **6.4 MHz**，JoinAccept 在 507.5 MHz 发出，E77 在 501.1 MHz 等待，完全错开。

**诊断过程（关键证据）：**
1. 网关 Monitor 确认 PULL_RESP 下行频率：`freq:507.5` → 网关实际发到 507.5 MHz
2. 对照 RX1 公式推算：507.5 = 500.3 + 36×0.2 → ch=84 (mod 48) ✅
3. 如 E77 按内部序号 4 计算：501.1 MHz，差值 6.4 MHz

**修复：确保 chanmask 明确告诉 E77 自己在 CH80~CH87 的绝对位置**

```
--chanmask 0000:0000:0000:0000:0000:00FF
```
此时 mask5 bit0~7 被激活，对应的是 CN470 CH80~CH87，
E77 内部会将这 8 个信道映射到其 RX1 计算表的正确行，RX1 与 ChirpStack 对齐。

> **此问题的本质**：E77 chanmask 不仅决定上行发射频率，还影响下行 RX1 窗口的
> 频率计算基准。两个独立的问题需要同一个参数（chanmask）同时修正。

---

### 问题 5：AT 配置命令返回 `AT_PARAM_ERROR` / `AT_ERROR` ⭐ 新发现

**现象（实际硬件测试）：**
```
[20:45:25.677] [WARN] 指令失败: AT+REGION=2  响应: ['AT_PARAM_ERROR']
[20:45:25.875] [WARN] 指令失败: AT+CDEVEUI=AABBCCDD11223344  响应: ['AT_ERROR']
[20:45:26.072] [WARN] 指令失败: AT+CAPPEUI=0000000000000000  响应: ['AT_ERROR']
[20:45:26.287] [WARN] 指令失败: AT+CAPPKEY=00112233445566778899AABBCCDDEEFF  响应: ['AT_ERROR']
```

但随后 OTAA 入网仍成功，说明模块使用的是旧的持久化配置而非脚本新传入的参数。

**根本原因：E77 模块将配置参数存储在 NVS（非易失存储），前一次测试的 DevEUI、AppKey、AppEUI、频段等设置仍保留在内存中。
新的 AT 命令试图覆盖这些值时，如果：**

1. **模块状态机已初始化**（之前入过网或配置过）：拒绝接受新的 DevEUI/AppEUI/AppKey 设置，返回 `AT_ERROR`
2. **REGION 参数与上次不同**：可能触发固件的参数校验逻辑，返回 `AT_PARAM_ERROR`

脚本继续执行后续命令（chanmask、ADR、发送），但节点使用的仍是旧的 DevEUI/AppKey，
刚好这些旧配置在 ChirpStack 中仍然注册，所以 OTAA JOIN 意外成功。

**复现步骤：**
1. 连续执行两次 OTAA 命令，不关闭设备电源，不执行 restore
2. 第二次运行时，上述 AT 配置命令返回错误

**修复：**
在任何新的 LoRaWAN 测试之前，**必须先执行**：
```bash
python3 scripts/e77_node_ctrl.py restore --port /dev/ttyUSB0
time.sleep(2)  # 等待模块重启
```

restore 执行后，模块回到**出厂默认状态**，所有 NVS 参数（DevEUI 硬编码除外）被清空，
后续新的 AT 配置命令才能被正确接受。

**实际验证（修复后）：**
```bash
# 第 1 步：清除旧配置
$ python3 scripts/e77_node_ctrl.py restore --port /dev/ttyUSB0
[20:45:44.950] [INFO] 出厂配置已恢复，等待重启 ...

# 等待 2 秒

# 第 2 步：重新 OTAA（此时所有 AT 命令都返回 OK）
$ python3 scripts/e77_node_ctrl.py otaa --port /dev/ttyUSB0 --region 2 ...
[20:45:52.584] [INFO] ✓ 设置频段: AT+REGION=2
[20:45:52.775] [INFO] ✓ 设置 DevEUI: AT+CDEVEUI=AABBCCDD11223344
[20:45:52.966] [INFO] ✓ 设置 AppEUI: AT+CAPPEUI=0000000000000000
[20:45:53.174] [INFO] ✓ 设置 AppKey: AT+CAPPKEY=00112233445566778899AABBCCDDEEFF
[20:46:02.618] [OK  ] ✓ OTAA 入网成功！
```

---

## 五、注意事项

### 5.1 chanmask 一定要与网关子频段严格对应

CN470 有 12 个子计划，节点 chanmask 必须精确匹配网关 radio 频率对应的子计划：

| 子计划 | radio_0 | 上行信道 | chanmask |
|--------|---------|---------|---------|
| cn470_0 | 470.6 MHz | CH0~CH7 | `00FF:0000:0000:0000:0000:0000` |
| cn470_5 | 480.6 MHz | CH40~CH47 | `0000:0000:0000:FF00:0000:0000`（待验证） |
| **cn470_10** | **486.6 MHz** | **CH80~CH87** | **`0000:0000:0000:0000:0000:00FF`** |

chanmask 结构：6 段 × 16bit，mask0=CH0~15，...，mask5=CH80~95。  
**本工程固定用 `0000:0000:0000:0000:0000:00FF`，切勿使用默认的 `00FF:...`。**

### 5.2 Gateway Bridge UDP 端口是 1680，不是 1700

本工程 `global_conf.cn490.json` 中 `serv_port_up/down = 1680`，
ChirpStack Gateway Bridge 需监听 **1680** 端口。
默认配置文件和大多数教程用的是 1700，切换时务必检查。

### 5.3 入网前不能设置 AT+CCLASS

E77 手册明确：Class 只能在入网后修改。入网前调用 `AT+CCLASS=X` 会返回 
`AT_NO_NETWORK_JOINED`，导致配置流程异常中断。

### 5.4 pyserial 打开串口会触发 DTR/RTS 复位

必须显式传入 `dsrdtr=False, rtscts=False, xonxoff=False`，并且等待至少 1.5 秒
再发送 AT 命令。无法禁用时（某些 USB-UART 芯片固定拉线），在 `at_test()` 中
加入足够的重试和延迟。

### 5.5 E77 模块 echo on 时会回显命令

模块默认开启 echo，发 `AT+REGION=2` 后，模块会先回显 `AT+REGION=2` 再回
`OK`。脚本中通过过滤同名行处理，若自己写代码需注意不要将回显误判为响应。

### 5.6 AT+CJOIN=1:0 触发入网后需等待最长 30 秒

LoRaWAN OTAA 规范允许节点最多重试多次（DR 递减），
JOIN 超时设置为 30 秒是合理的。若网络条件正常但超时，
优先检查 chanmask 和 ChirpStack Device AppKey 是否匹配。

### 5.7 Confirmed uplink 的 `OK+SENT:XX` 含义

`XX` 为重传计数，例如 `OK+SENT:01` = 发送了 1 次（首次发送算 0，重传 1 次算 1）。
这是 LoRaWAN 标准行为，Confirmed uplink 在收到 ACK 前最多重试 retries 次。

### 5.8 每次新测试前必须清除旧配置

E77 模块在 NVS 中保存 DevEUI、AppKey、AppEUI、频段等参数，断电不会丢失。
如果没有执行 `restore` 命令清除，新的 AT 配置命令可能被拒绝（`AT_ERROR` / `AT_PARAM_ERROR`），
并且节点会继续使用旧的凭据。为避免混淆和隐蔽 Bug，
**每次更换 AppKey 或切换不同的 ChirpStack 实例时，务必先执行一遍 restore**。

---

## 六、Learnings

### L1：LoRaWAN CN470 chanmask 影响两件事

chanmask 不仅决定节点上行**发射频率**，还直接影响节点对 RX1 下行窗口的**频率计算基准**。
这是 CN470 与 EU868/US915 等频段的重要区别——后者的 RX1 是固定的或基于简单偏移，
而 CN470 的 RX1 是从绝对信道号计算的。
节点必须知道自己的上行信道在 CN470 频谱中的**绝对位置**，才能算出正确的 RX1。

### L2：JOIN FAILED 的诊断分层法

JOIN FAILED 可能发生在三个独立位置，需要从底层向上逐层排除：

```
节点发送 JoinRequest
    ↓
1. 网关有没有收到？（看 Monitor 的 JSON up / event=up 计数）
   → 没有：上行频率不对，检查 chanmask 和 gateway radio 频率
    ↓
2. ChirpStack 有没有推送 JoinAccept？（看 chirpstack 日志 / gateway event=down 计数）
   → 没有：AppKey 不匹配，或 DevEUI/AppEUI 未注册
    ↓
3. 节点有没有收到 JoinAccept？（看 +EVT:JOINED 或 JOIN FAILED）
   → 没有：RX1 频率偏移，通常是 chanmask 导致节点计算出错误的 RX1
```

### L3：gateway-bridge 日志是 NS 侧的第一个监控点

在 ChirpStack 侧无任何反应时，第一步应该看 gateway-bridge 的 `event=up`/`event=down` 计数，
而不是直接看 chirpstack 日志。gateway-bridge 是 UDP → MQTT 的网关，
若 `event=up` 为零，说明问题在物理层（RF 频率）；
若有 `event=up` 但 chirpstack 侧无 uplink，说明问题在协议层（AppKey/region）。

### L4：E77 AT 指令接口的健壮性技巧

| 问题 | 解法 |
|------|------|
| 上电 AT_ERROR | `dsrdtr=False` + 等待 1.5s + 重试 3 次 |
| 回显干扰 | 过滤发送命令本身的字符串 |
| CCLASS 报错 | 入网前不设置，入网成功后再切换 |
| JOIN 超时 | 先 `AT+RESTORE` 清除旧配置，再重新配置 chanmask |

### L5：CN470 RX1 频率公式

$$f_{RX1} = 500.3 + (ch_{uplink} \bmod 48) \times 0.2 \text{ MHz}$$

本工程网关 CH80~CH87 对应 RX1：

$$f_{RX1} = 506.7 \sim 508.1\ \text{MHz}$$

RX2 固定：505.3 MHz SF12BW125（上行后 2 秒）。

### L6：ADR 与 chanmask 的相互依赖

开启 ADR（`AT+CADR=1`）时，chanmask 控制节点上行使用哪些信道，
ChirpStack ADR 算法基于历史 RSSI/SNR 计算最优 DR，并通过 `LinkADRReq` MAC 命令下发。
此时节点会在多个信道轮转发包，网关侧 ADR 需要多包样本才能生效，通常 3~5 包后调整完成。
测试时使用短间隔（`--interval 10`）可以加速 ADR 调整。

---

## 七、测试进度状态

- ✅ E77-400M22S AT 指令控制脚本（`scripts/e77_node_ctrl.py`）
- ✅ 解决 DTR/RTS 复位问题
- ✅ 解决 AT+CCLASS 入网前报错
- ✅ 解决 chanmask 上行频率不对（CH0~CH7 → CH80~CH87）
- ✅ 解决 chanmask 导致 RX1 频率偏移（6.4 MHz 错位）
- ✅ 解决 NVS 持久化配置导致 AT 命令被拒绝问题
- ✅ OTAA 入网成功（JOIN 耗时约 5~8 秒）
- ✅ 上行数据验证（57+ 包，0 丢包）
- ✅ Confirmed uplink / 下行 ACK 验证
- ✅ 手动入队下行数据（PORT 2）验证
- ✅ ADR 自适应验证（SF12 → SF7 自动调速）
- ⏳ ABP 入网验证（待后续分支）
- ⏳ Class C 测试（待后续分支）
- ⏳ 多节点并发测试（待后续分支）

---

*记录人：cuckooshan，2026-02-27*
