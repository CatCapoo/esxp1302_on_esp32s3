# ChirpStack v4 使用笔记

> 写作背景：2026-02-22 完成 ESXP1302 接入 ChirpStack v4 的全链路调试，
> 记录 ChirpStack 的配置体系、常见操作和调试技巧。

---

## 一、ChirpStack v4 整体架构

ChirpStack v4 相比 v3 做了重大重构：

| 对比项 | v3 | v4 |
|--------|----|----|
| 组件数量 | NS + AS + GW Bridge（3个独立服务）| 合并为单一 `chirpstack` 进程 |
| 数据库 | PostgreSQL + Redis | 同上 |
| 配置格式 | 各服务独立 TOML | 单一 chirpstack.toml + region_xxx.toml |
| Region 支持 | 编译时决定 | 运行时配置，可多 region 同时运行 |
| API | gRPC + REST | 同上，REST via chirpstack-rest-api |

### 1.1 Docker 部署结构（本项目）

```
~/chirpstack-docker/
├── docker-compose.yml           ← 所有容器定义
└── configuration/
    ├── chirpstack/
    │   ├── chirpstack.toml      ← NS 主配置
    │   ├── region_cn470_10.toml ← CN470_10 区域配置
    │   └── region_*.toml        ← 其他区域（可选）
    ├── chirpstack-gateway-bridge/
    │   └── chirpstack-gateway-bridge.toml
    └── mosquitto/
        └── mosquitto.conf
```

### 1.2 各容器端口

| 容器 | 端口 | 用途 |
|------|------|------|
| chirpstack | **8080** | Web GUI + gRPC API |
| chirpstack-rest-api | **8090** | REST API（代理 gRPC）|
| chirpstack-gateway-bridge | **1700/udp** | 接收网关 UDP 包 |
| mosquitto | **1883/tcp** | MQTT Broker |
| redis | 6379 | 内部缓存（不对外暴露）|
| postgres | 5432 | 持久化数据库（不对外暴露）|

---

## 二、关键配置文件详解

### 2.1 chirpstack.toml（NS 主配置）

**启用区域（必须与 gateway-bridge topic_prefix 一致）：**

```toml
[network]
  enabled_regions=[
    "cn470_10",
    # 可以同时启用多个 region，每个 region 独立处理对应 topic 的数据
  ]
```

**API 和 Web UI：**

```toml
[api]
  bind="0.0.0.0:8080"
  secret="your-secret-key"  # JWT 签名密钥，改掉默认值
```

**数据库：**

```toml
[postgresql]
  dsn="postgres://chirpstack:chirpstack@postgresql/chirpstack?sslmode=disable"

[redis]
  servers=["redis://redis/"]
```

### 2.2 region_cn470_10.toml（区域配置）

**核心字段：**

```toml
[regions.cn470_10]
  description="CN470 (LoRa Alliance plan, channels 80-87)"

  [regions.cn470_10.gateway]
    force_gws_private=false

  [[regions.cn470_10.network.extra_channels]]
    # CN470_10 的8个上行信道
    frequency=486300000
    min_dr=0
    max_dr=5
    # ... 其余7个类似

  [regions.cn470_10.network]
    enabled_uplink_channels=[80, 81, 82, 83, 84, 85, 86, 87]

  [[regions.cn470_10.rx2]]
    frequency=505300000
    dr=0

  [regions.cn470_10.gateway_topic_prefix]
    topic_prefix="cn470_10"    # 必须与 gateway-bridge 的 topic 前缀一致
```

### 2.3 docker-compose.yml（gateway-bridge 关键配置）

```yaml
chirpstack-gateway-bridge:
  image: chirpstack/chirpstack-gateway-bridge:4
  environment:
    # 这三行的前缀必须与 region TOML 中的 topic_prefix 一致
    - INTEGRATION__MQTT__EVENT_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/event/{{ .EventType }}
    - INTEGRATION__MQTT__STATE_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/state/{{ .StateType }}
    - INTEGRATION__MQTT__COMMAND_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/command/#
  ports:
    - "1700:1700/udp"   # 接收网关 UDP
```

> ⚠️ **常见坑**：YAML 中长字符串不能有换行，某些编辑器会在80列自动折行，
> 导致 template 字符串被截断，topic 解析失败。用 Python/sed 修改更安全：
> ```bash
> python3 -c "
> content = open('docker-compose.yml').read()
> content = content.replace('cn470/', 'cn470_10/')
> open('docker-compose.yml', 'w').write(content)
> "
> ```

---

## 三、在 ChirpStack 中注册设备：完整流程与参数说明

ChirpStack 注册一台新设备需要依次完成 4 步，后面的步骤依赖前面的步骤创建的对象：

```
① 注册网关（Gateway）
       ↓
② 创建设备配置文件（Device Profile）
       ↓
③ 创建应用（Application）
       ↓
④ 在应用内注册设备（Device）并填写密钥
```

---

### 3.1 注册网关（Gateway）

Web UI → **Gateways** → **Add gateway**

#### 字段说明

| 字段 | 示例值 | 说明 |
|------|--------|------|
| **Name** | `ESXP1302-Lab` | 仅用于显示，无功能影响 |
| **Gateway ID（EUI-64）** | `AA555A00000021FB` | **关键字段**。8字节 EUI，全局唯一标识网关 |
| Description | 随意 | 备注 |
| Tags | 可选 | Key-Value 元数据，可用于过滤 |

**Gateway ID 从哪来？**  
本工程固件在启动时通过 `esp_read_mac()` 读取 ESP32-S3 WiFi MAC 地址，
拼接后打印到串口 Monitor：
```
Gateway EUI: AA:55:5A:00:00:00:21:FB
```
去掉冒号即为 `AA555A00000021FB`。也可在 `global_conf.cn490.json` 中的
`gateway_ID` 字段查到。

**注册后验证：**  
几秒后刷新页面，**Last seen** 应显示当前时间（而不是 "Never"）；
状态指示变为绿色 **Online**。若仍显示离线，检查 gateway-bridge 的
MQTT topic 前缀是否与 NS region id 一致（见第二节）。

---

### 3.2 创建设备配置文件（Device Profile）

**Device Profile** 定义了"一类节点"的 LoRaWAN 参数，同型号的多台设备共享同一个
Profile，不需要每台单独设置。

Web UI → **Device profiles** → **Add device profile**

#### General 选项卡

| 字段 | 推荐值 | 说明 |
|------|--------|------|
| **Name** | `E77-CN470-OTAA` | 命名建议包含型号+频段+入网方式，便于区分 |
| **Region** | `CN470_10` | **关键字段**。必须与网关 radio 频率对应的子频段一致，本工程固定选 CN470_10 |
| **MAC version** | `LoRaWAN 1.0.3` | **关键字段**。必须与节点固件版本严格匹配。E77-400M22S 固件为 LoRaWAN 1.0.3，选错会导致 MIC 校验失败或 FCnt 逻辑不兼容 |
| **Regional parameters revision** | `A`（即 RP002-1.0.1） | 与 MAC version 配套的区域参数版本。1.0.3 对应选 A |
| **ADR algorithm** | `Default ADR algorithm (LoRa only)` | ChirpStack 内置 ADR 策略：收集历史 SNR 样本，自动下发 LinkADRReq 调整 DR 和 TxPower。选 Default 即可 |
| **Expected uplink interval** | `3600`（秒） | 预期节点上行间隔，用于判断设备是否活跃（超时后 Device 状态变为 inactive）。测试时可填较小值如 `120` |
| **Device-status request interval** | `0`（禁用）| 自动发 DevStatusReq 查电池和 SNR 的间隔，0 = 禁用。测试不需要开 |

#### Join (OTAA) 选项卡

| 字段 | 值 | 说明 |
|------|----|------|
| **Supports OTAA** | ✅ 勾选 | 表示此 Profile 的设备使用 OTAA 入网。OTAA 优于 ABP：每次入网动态派发 DevAddr 和 Session Key，防重放攻击 |

#### Class B / Class C 选项卡

| 字段 | 值 | 说明 |
|------|----|------|
| Supports Class B | 不勾 | Class B 需要网关同步 Beacon，目前未测试 |
| Supports Class C | 不勾 | Class C 节点常开接收窗口，功耗高，需要节点固件支持。E77 入网后可通过 `AT+CCLASS=C` 切换，届时再创建单独 Profile |

#### Codec 选项卡

| 字段 | 说明 |
|------|------|
| Payload codec | 用于在 GUI 中自动解码 payload。选 `None` 则显示原始 hex；选 `CayenneLPP` 则自动解析传感器数据格式；也可填自定义 JavaScript 解码器 |

> **小结**：Device Profile 描述的是节点"能做什么"（版本、Class、ADR 支持），
> 不涉及具体的 DevEUI / AppKey，可以被多台同型号设备复用。

---

### 3.3 创建应用（Application）

**Application** 是设备的逻辑分组容器。同一应用内的设备共享：
- 同一个 MQTT uplink topic（`application/<id>/device/+/event/up`）
- 同一套 HTTP integration / webhook 配置
- 同一个 API 鉴权视图

Web UI → **Applications** → **Add application**

| 字段 | 说明 |
|------|------|
| **Name** | 如 `GW-Validation-Test` |
| Description | 随意 |
| Tags | 可选 Key-Value 元数据 |

> 生产场景中一个业务系统对应一个 Application；
> 调试场景一般一个项目建一个 Application 即可。

---

### 3.4 注册设备（Device）并填写密钥

在 Application 内为每台物理节点创建一个 Device 条目。

**路径：** Web UI → **Applications** → 选择应用 → **Add device**

#### General 选项卡

| 字段 | 示例值 | 说明 |
|------|--------|------|
| **Name** | `E77-Node-01` | 显示名，随意 |
| **DevEUI** | `AABBCCDD11223344` | **关键字段**。8字节全局唯一设备标识符，烧写在节点芯片内，通过 `AT+CDEVEUI=?` 查询 |
| **AppEUI（JoinEUI）** | `0000000000000000` | 标识 Join Server 的 8字节 EUI。LoRaWAN 1.0.x 称 AppEUI，1.1 称 JoinEUI。自建测试全填 0 即可；生产场景由部署方分配 |
| **Device profile** | 选择步骤 3.2 创建的 Profile | 决定 MAC 版本、Region、ADR 等 |
| **Skip frame-counter checks** | 调试期间勾选 | **调试必勾**。不勾时若节点重烧后 FCnt 从 0 重置，NS 会因为 FCnt 回退而拒绝所有上行包（防重放保护）。生产环境不勾 |
| Tags | 可选 | |
| Variables | 可选 | 可在 JS codec 中引用的自定义变量 |

**DevEUI 从哪来？**  
对于 E77-400M22S，通过 AT 指令查询：
```bash
python3 scripts/e77_node_ctrl.py query --port /dev/ttyUSB0
# 或直接发 AT 指令：
AT+CDEVEUI=?
# → 返回: AT+CDEVEUI=AABBCCDD11223344
```
每块模块出厂烧写唯一 DevEUI，不能修改。

---

#### 填写密钥（OTAA）

点击 **Submit** 添加设备后，自动跳转到设备详情页。  
进入 **Keys (OTAA)** 标签页：

| 字段 | 字节数 | 说明 |
|------|--------|------|
| **Application key（AppKey）** | 16B | **最关键的密钥**。节点侧和 NS 侧共同持有，用于推导所有 Session Key。必须与节点固件中烧写的 AppKey 完全一致（大小写不敏感） |
| **NwkKey**（仅 1.1）| 16B | LoRaWAN 1.1 新增的网络密钥，与 AppKey 分离。1.0.x 设备无此字段 |

**AppKey 是什么，为什么重要？**

AppKey 是根密钥（Root Key），OTAA 入网时：
```
NwkSKey = aes128_encrypt(AppKey, 0x01 || AppNonce || NetID || DevNonce || pad)
AppSKey = aes128_encrypt(AppKey, 0x02 || AppNonce || NetID || DevNonce || pad)
```
- `NwkSKey`（Network Session Key）：用于 MAC 层帧的 MIC 计算和加解密
- `AppSKey`（Application Session Key）：用于应用 payload 的加解密

AppKey 泄露 = Session Key 可被推算 = 所有历史和未来数据被解密。
**不同设备必须使用不同的 AppKey**（出厂时各自生成，或由 NS 批量生成后烧写）。

测试时可在 ChirpStack 设备详情页点击 **Generate** 按钮随机生成，
然后将生成的 32 位 hex 字符串填入节点 `--appkey` 参数。

---

#### 填写密钥（ABP）

ABP 不执行 Join 流程，Session Key 直接预置，需要手动填写激活参数。

进入 **Activation** 标签页：

| 字段 | 字节数 | 说明 |
|------|--------|------|
| **Device address（DevAddr）** | 4B | 网络内唯一地址，ABP 时由用户自定，如 `26011234`。OTAA 时由 NS 动态分配，无需手填 |
| **NwkSEncKey** | 16B | LoRaWAN 1.1：网络层加密 Session Key（1.0.x 中等同于 NwkSKey） |
| **SNwkSIntKey** | 16B | LoRaWAN 1.1：服务器侧网络层完整性 Key（1.0.x 与 NwkSKey 相同） |
| **FNwkSIntKey** | 16B | LoRaWAN 1.1：转发侧网络层完整性 Key（1.0.x 与 NwkSKey 相同） |
| **AppSKey** | 16B | 应用层 Session Key，用于 payload 加解密 |
| **Uplink frame-counter（FCntUp）** | 4B int | 节点上行帧计数器初始值。通常填 `0`，与节点侧同步 |
| **Downlink frame-counter（NFCntDown）** | 4B int | NS 下行帧计数器初始值，通常 `0` |

> **ABP 注意**：ABP Session Key 是静态的，永不更新，安全性低于 OTAA。
> 且 FCntUp 一旦超过上限（2^32）或设备重烧（FCntUp 归零），
> 若 NS 未关闭 FCnt 检查，所有包会被丢弃。
> **调试 ABP 时务必勾选 Skip frame-counter checks**，
> 并在 Device Profile 中关闭 FCnt rollover 检查。

---

### 3.5 验证设备注册成功

注册并填写密钥后，运行节点脚本触发 OTAA 入网：

```bash
python3 scripts/e77_node_ctrl.py otaa \
    --port /dev/ttyUSB0 --region 2 \
    --deveui <DEVEUI> --appkey <APPKEY> \
    --chanmask 0000:0000:0000:0000:0000:00FF
```

**成功标志（按顺序）：**

| 序号 | 观察位置 | 预期现象 |
|------|---------|---------|
| 1 | 节点串口 | `+EVT:JOINED` |
| 2 | ChirpStack GUI → Device → **Events** | 出现 `join` 事件 |
| 3 | GUI → Device → **Activation** | 显示当前 DevAddr / NwkSKey / AppSKey |
| 4 | 发送上行后 → **LoRaWAN frames** | 出现 `UnconfirmedDataUp` 帧，payload 已解密 |
| 5 | **Events** 标签 | 出现 `up` 事件，data 字段显示 base64 payload |

---

## 四、OTAA 入网流程（Join）

OTAA（Over-The-Air Activation）是推荐的入网方式：

```
节点                            NS (ChirpStack)
 │
 │── JoinRequest（含 AppEUI/DevEUI/DevNonce）──→
 │                                              │ 查 AppKey，验证 MIC
 │                                              │ 生成 NwkSKey, AppSKey
 │                                              │ 分配 DevAddr
 │←── JoinAccept（含 AppNonce/NetID/DevAddr）──│
 │
 │ 节点用 AppKey 解密 JoinAccept，
 │ 推导出 NwkSKey / AppSKey，
 │ 此后上行数据用这两个 Session Key 加密
```

**关键参数：**

| 参数 | 大小 | 存储位置 |
|------|------|---------|
| DevEUI | 8B | 节点硬件（通常烧写）|
| AppKey | 16B | 节点固件 + ChirpStack |
| DevAddr | 4B | NS 动态分配 |
| NwkSKey | 16B | 推导自 AppKey（每次 Join 更新）|
| AppSKey | 16B | 推导自 AppKey（每次 Join 更新）|

---

## 五、日志查看与调试

### 5.1 查看实时日志

```bash
cd ~/chirpstack-docker

# 查看 NS 日志（最常用）
docker compose logs -f chirpstack

# 查看 gateway-bridge 日志
docker compose logs -f chirpstack-gateway-bridge

# 查看所有容器
docker compose logs -f

# 查看最近 N 分钟
docker compose logs --since=5m chirpstack
```

### 5.2 关键日志关键字

| 关键字 | 含义 |
|--------|------|
| `Gateway partially updated` | NS 收到 stats，网关上线 ✅ |
| `Uplink received` | 收到上行包 ✅ |
| `region_id="cn470_10"` | 消息属于 cn470_10 region ✅ |
| `MACPayload requires at least 7 bytes` | 上行包不是合法 LoRaWAN 帧（裸 LoRa 包）⚠️ |
| `MIC error` | MIC 校验失败，AppKey/NwkSKey 不对 ❌ |
| `DevAddr not found` | 设备未注册或 DevAddr 错误 ❌ |
| `frame-counter did not increment` | FCnt 计数器未递增（重放攻击防护触发）❌ |

### 5.3 MQTT 实时监听（调试 topic）

```bash
# 安装 mosquitto 客户端
sudo apt install mosquitto-clients

# 订阅所有 cn470_10 网关消息（从宿主机访问 Docker 的 mosquitto）
mosquitto_sub -h localhost -p 1883 -t "cn470_10/#" -v

# 只看上行数据
mosquitto_sub -h localhost -p 1883 -t "cn470_10/gateway/+/event/up" -v
```

> Gateway Bridge 发布的消息是 **Protobuf 序列化**的，
> 直接 `mosquitto_sub` 看到的是二进制，需要 protobuf 工具解码，
> 或者通过 ChirpStack Web GUI 的设备 LoRaWAN frames 页面查看解码后的内容。

### 5.4 Web GUI 查看上行帧

Web UI → **Applications** → 选择应用 → **Devices** → 选择设备 → **LoRaWAN frames** 标签

这里会显示：
- 帧时间
- 接收网关（多网关时显示所有网关 EUI 和 RSSI/SNR）
- 上行帧类型（JoinRequest / UnconfirmedDataUp 等）
- 解密后的 payload（如果 AppSKey 正确）

---

## 六、常见操作命令

### 6.1 容器管理

```bash
cd ~/chirpstack-docker

# 启动所有服务
docker compose up -d

# 停止所有服务
docker compose down

# 重启单个服务（不影响其他容器）
docker compose up -d --no-deps chirpstack-gateway-bridge

# 重新加载配置（修改 TOML 后需重启 chirpstack 容器）
docker compose restart chirpstack

# 查看容器状态
docker compose ps
```

### 6.2 REST API 常用请求

ChirpStack 提供完整 REST API（通过 chirpstack-rest-api 代理，端口 8090）：

```bash
BASE="http://localhost:8090"
# 先获取 API Key（Web UI → API Keys → Add API key，或使用 admin 密码直接登录）
TOKEN="Bearer <your-api-key>"

# 列出网关
curl -H "Authorization: $TOKEN" $BASE/api/gateways

# 查看网关详情（替换 EUI）
curl -H "Authorization: $TOKEN" "$BASE/api/gateways/aa555a00000021fb"

# 列出 Device Profile
curl -H "Authorization: $TOKEN" $BASE/api/device-profiles
```

---

## 七、多 Region 同时运行

ChirpStack v4 支持在同一实例中运行多个 Region，每个 Region 独立处理：

**chirpstack.toml：**
```toml
[network]
  enabled_regions=["cn470_10", "eu868", "us915"]
```

**docker-compose.yml 需要多个 gateway-bridge 实例：**

```yaml
  # CN470_10 网关
  chirpstack-gateway-bridge-cn470:
    environment:
      - INTEGRATION__MQTT__EVENT_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/event/{{ .EventType }}
    ports:
      - "1700:1700/udp"

  # EU868 网关
  chirpstack-gateway-bridge-eu868:
    environment:
      - INTEGRATION__MQTT__EVENT_TOPIC_TEMPLATE=eu868/gateway/{{ .GatewayID }}/event/{{ .EventType }}
    ports:
      - "1701:1700/udp"   # 注意端口不能冲突
```

---

## 八、注意事项汇总

| 注意点 | 说明 |
|--------|------|
| topic_prefix 大小写敏感 | `CN470_10` ≠ `cn470_10`，必须小写 |
| YAML 不能换行 | 长字符串模板不能跨行，否则被截断 |
| GW EUI 大小写 | gateway-bridge 发布时转成小写，NS 存储时不区分大小写 |
| FCnt 检查 | 调试期间可关闭，否则重烧节点后 FCnt 从头开始会导致包被丢 |
| LoRaWAN 1.0 vs 1.1 | Device Profile 的 MAC version 必须与节点固件一致 |
| ADR 和信道掩码 | NS 会通过 ADRReq 和 LinkADRReq 下行命令调整节点信道，初次接入可能需要几次上行才稳定 |

---

## 参考资料

- ChirpStack 官方文档：https://www.chirpstack.io/docs/
- ChirpStack GitHub：https://github.com/chirpstack/chirpstack
- Docker 部署示例：https://github.com/chirpstack/chirpstack-docker
