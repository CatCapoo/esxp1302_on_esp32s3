# 04 — 测试脚本参考

## 脚本概览

| 脚本 | 用途 | 依赖 |
|------|------|------|
| `scripts/e77_node_tx.py` | E77 节点 ABP 发包 (配合 HAL RX 测试) | pyserial |
| `scripts/e77_probe.py` | E77 串口诊断探测 | pyserial |

安装依赖:
```powershell
pip install pyserial
```

---

## e77_node_tx.py — E77 ABP 发包脚本

### 功能

通过 AT 指令控制 E77-400M22S 模块，以 ABP 模式在 CN470 Sub-band 10 上周期发送 LoRaWAN 上行包。

E77 模块 **仅支持 LoRaWAN**（不支持 P2P 原始 LoRa），所以使用 dummy keys 实现无需网络服务器的发包测试。

### 命令行参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--port` | (必选) | 串口号，如 `COM11` 或 `/dev/ttyUSB0` |
| `--baud` | 9600 | 波特率 |
| `--interval` | 10 | 发包间隔 (秒) |
| `--count` | 0 | 总包数，0=无限 |
| `--dr` | 2 | DataRate: 0=SF12, 1=SF11, 2=SF10, 3=SF9, 4=SF8, 5=SF7 |
| `--payload` | `DEADBEEF01020304` | 发送的 hex 负载 |
| `--txp` | 0 | 发射功率档 (0=最大) |
| `--devaddr` | `26011234` | ABP DevAddr |
| `--nwkskey` | dummy 全 `00-FF` | NwkSKey (32 hex) |
| `--appskey` | dummy 全 `FF-00` | AppSKey (32 hex) |
| `--verbose` | false | 显示 AT 交互细节 |

### 典型用法

```powershell
# 基本用法: SF10, 每 10 秒发一包, 无限循环
python scripts/e77_node_tx.py --port COM11

# 快速测试: SF7, 间隔 5s, 发 20 包
python scripts/e77_node_tx.py --port COM11 --interval 5 --count 20 --dr 5

# 详细调试模式
python scripts/e77_node_tx.py --port COM11 --verbose

# 指定自定义 payload
python scripts/e77_node_tx.py --port COM11 --payload "0102030405060708"
```

### 工作流程

1. 打开串口 → AT 通信测试 (3 次重试)
2. ABP 配置:
   - `AT+REGION=2` (CN470)
   - `AT+CDEVADDR=26:01:12:34`
   - `AT+CNWKSKEY=...`
   - `AT+CAPPSKEY=...`
   - `AT+CTXP=0`
   - `AT+CADR=0`
   - `AT+CDATARATE=<dr>`
   - `AT+CMANUALMASK=1`
   - `AT+CFREQBANDMASK=0000:0000:0000:0000:0000:00FF`
3. `AT+CJOIN=0:0` 触发 ABP 本地入网
4. 循环 `AT+SEND=2:1:0:<payload>` 发包

### 输出示例

```
[10:23:45.123] [INFO] 打开串口 COM11 @ 9600 bps
[10:23:46.789] [INFO] AT 通信正常 ✓
[10:23:47.000] [INFO] =======================================================
[10:23:47.001] [INFO]   配置 E77 ABP 模式 (CN470 Sub-band 10)
[10:23:47.002] [INFO] =======================================================
[10:23:47.100] [INFO] ✓ 设置频段 CN470: AT+REGION=2
...
[10:23:48.500] [OK]   ✓ ABP 入网成功!
[10:23:49.600] [INFO] 开始发包: DR2(SF10) 间隔=10s payload=DEADBEEF01020304
[10:23:49.700] [INFO] ── 第 1 包 ──
[10:23:51.200] [OK]     TX 确认: +EVT:SEND_OK
```

---

## e77_probe.py — 串口探测脚本

### 功能

自动尝试多种波特率 (9600, 115200, 57600, 38400, 19200) 和行尾格式 (`\r\n`, `\r`, `\n`)，向指定 COM 端口发送 `AT` 指令，检测 E77 模块是否响应。

### 命令行参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--port` | (必选) | 串口号 |
| `--baud` | 0 | 指定波特率 (0=自动扫描全部) |

### 典型用法

```powershell
# 自动探测
python scripts/e77_probe.py --port COM11

# 只测试 9600 bps
python scripts/e77_probe.py --port COM11 --baud 9600
```

### 输出示例

```
[9600 bps, ending=b'\r\n']
  发送 b'AT\r\n'
  收到 (6 字节): b'AT\r\nOK'

  ✓ 找到! 波特率=9600  行尾=b'\r\n'
```

---

## 串口识别注意事项

Windows 系统上可能存在多个 COM 端口:

| 端口 | 设备 |
|------|------|
| COM3 | 蓝牙虚拟串口 (不是 E77!) |
| COM11 | E77-400M22S 模块 |

**确认方法**:
1. 设备管理器查看 "端口 (COM 和 LPT)" 类别
2. 插拔 E77 USB 连接线，观察哪个 COM 端口出现/消失
3. 或使用 `e77_probe.py` 逐个端口探测
