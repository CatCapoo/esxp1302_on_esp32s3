# test_pkt_fwd 测试备忘录

**日期：** 2026-02-20  
**测试目的：** 验证完整的 Packet Forwarder 流水线（SX1302 LoRa 收包 → ESP32-S3 组帧 → WiFi UDP 上报 NS）

---

## 一、测试配置

### 硬件

| 角色 | 设备 |
|------|------|
| 网关 | ESP32-S3 + SX1302（ESXP1302 板） |
| 发包端 | 原始 LoRa 发包仪（非 LoRaWAN 节点） |

### 频率计划（CN490）

| 参数 | 值 |
|------|----|
| Radio 0 中心频率 | 480.400 MHz |
| Radio 1 中心频率 | 481.200 MHz |
| 信道带宽 | 125 kHz |
| 信道 0–3 | Radio 0：480.1 / 480.3 / 480.5 / 480.7 MHz |
| 信道 4–7 | Radio 1：480.9 / 481.1 / 481.3 / 481.5 MHz |
| LoRa std 信道 | Radio 1，481.0 MHz，250 kHz BW，SF7 Explicit |
| FSK 信道 | Radio 1，481.5 MHz，125 kHz BW，50 kbps |

### 发包端参数（与本次测试对应）

| 参数 | 值 | 说明 |
|------|----|------|
| 频率 | 480.500 MHz | 信道 2，Radio 0 +100 kHz |
| BW | 125 kHz | |
| SF | 12 | |
| CR | 4/5 | |
| SyncWord | 0x34 | LoRaWAN 公共网络 |
| CRC | 开启 | pkt_fwd 默认只转发 CRC 正确的包 |
| Preamble | ≥ 8 | |
| Payload | `hello world` | |

> **注意 CRC**：pkt_fwd 从配置文件读取 `forward_crc_valid = true`、`forward_crc_error = false`、
> `forward_crc_disabled = false`，因此发包端**必须开启 CRC**，否则包会被静默丢弃。

---

## 二、pkt_fwd CLI 用法

网关启动后进入 ESP-IDF 控制台，命令名为 `pkt_fwd`。
**任何带参数的调用均会将配置写入 NVS，然后立即重启设备。**

### 2.1 帮助

```
pkt_fwd -h
```

### 2.2 配置 WiFi 并连接

```
pkt_fwd -u <SSID> -p <密码>
```

示例：
```
pkt_fwd -u MyRouterSSID -p MyPassword123
```

执行后设备保存配置到 NVS 并重启，重启后自动以 Station 模式连接 WiFi。

### 2.3 配置 NS（网络服务器）地址

```
pkt_fwd --host <NS_IP或域名> --port <端口号>
```

示例：
```
pkt_fwd --host 192.168.1.10 --port 1700
```

默认配置文件（`global_conf.cn490.json`）中 NS 地址为 `192.168.1.202:1680`，若未通过此命令覆盖且 NVS 中无值，则使用配置文件的缺省值。

### 2.4 配置网关 ID

```
pkt_fwd --gwid <16位十六进制字符串>
```

示例：
```
pkt_fwd --gwid AA555A0000001234
```

未配置时网关ID从配置文件读取，默认为 `AA555A00000021FB`。

### 2.5 同时配置多个参数

各参数可在一条命令中组合：

```
pkt_fwd -u MySSID -p MyPass --host 10.0.0.1 --port 1700 --gwid AA555A0000001234
```

### 2.6 Web 配置界面

连接同一 WiFi 后，浏览器访问 `http://<设备IP>/`，弹出 HTTP Basic Auth 认证框：

- **用户名**：`iot`
- **密码**：`lora`

通过 Web 界面可以配置与 CLI 相同的参数，保存后设备自动重启。

> **注意**：若浏览器返回 **431 Request Header Fields Too Large**，
> 需在 `sdkconfig.defaults` 中增加 `CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048`
> 并重新编译烧录。详见 [BUG-009](../bugfix/BUG-009_http_431_httpd_max_req_hdr_len_too_small.md)。

---

## 三、测试流程

1. 烧录固件，上电
2. 用 CLI 配置 WiFi：`pkt_fwd -u <SSID> -p <Password>`
3. 等待设备重启并连接 WiFi，观察串口输出  
   `INFO: [main] concentrator started, packet can now be received`
4. 发包端以 480.5 MHz SF12 BW125 CRC=ON 发送原始 LoRa 包
5. 串口监视包接收日志

---

## 四、验证结果

SX1278 原始 LoRa 发包端发送 `"hello world"`（SF12, BW125, 480.5 MHz, CRC ON），
网关成功接收并打印 JSON 上报 UDP：

```
INFO: Received pkt from mote: 6F6C6C65 (fcnt=28535)

JSON up: {"rxpk":[{
  "jver":1,
  "tmst":2668949,
  "chan":2,
  "rfch":0,
  "freq":480.500000,
  "mid": 0,
  "stat":1,
  "modu":"LORA",
  "datr":"SF12BW125",
  "codr":"4/5",
  "rssis":-64,
  "lsnr":5.2,
  "foff":780,
  "rssi":-64,
  "size":11,
  "data":"aGVsbG8gd29ybGQ="
}]}
```

`data` 字段 Base64 解码 = `hello world`，与发送端一致。

### 信号质量

| 指标 | 典型值 |
|------|--------|
| RSSI | -64 dBm |
| SNR | +5.0 ~ +5.5 dB |
| 频偏 foff | ~778 Hz（发包端与网关晶振误差） |

---

## 五、已知非阻塞性告警

| 告警信息 | 原因 | 影响 |
|---------|------|------|
| `ERROR: failed to read I2C device 0x38` | 无 STTS751 温度传感器硬件 | 无，温度上报为 0 |
| `MQTT_EVENT_ERROR / MQTT_EVENT_DISCONNECTED` | 无 MQTT Broker | 无，LoRa 包仍正常通过 UDP 上报 |
| `failed to configure temperature sensor` | 同上 | 无 |

---

## 六、进度状态

- ✅ CMakeLists.txt INCLUDE_DIRS 修复（见 [BUG-006](../bugfix/BUG-006_cmake_include_dirs_multiple_declaration.md)）
- ✅ webpage.h 生成方式确认（见 [BUG-007](../bugfix/BUG-007_webpage_h_not_generated_by_idf_build.md)）
- ✅ ESP32-S3 无效 GPIO 引脚修复（见 [BUG-008](../bugfix/BUG-008_esp32s3_invalid_gpio_pin_defaults.md)）
- ✅ HTTP 431 修复（见 [BUG-009](../bugfix/BUG-009_http_431_httpd_max_req_hdr_len_too_small.md)）
- ✅ WiFi 连接正常（SSID: 222）
- ✅ SX1302 集中器启动成功（`concentrator started`）
- ✅ LoRa 原始包接收验证通过（CN490 480.5 MHz SF12 BW125）
- ⏳ LoRaWAN NS 对接（无可用 NS，待后续）
- ⏳ MQTT 对接（无 Broker，待后续）
