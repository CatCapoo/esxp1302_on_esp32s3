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

## 三、ChirpStack Web GUI 操作流程

### 3.1 注册网关（Gateway）

Web UI → **Gateways** → **Add gateway**

| 字段 | 值 | 说明 |
|------|----|------|
| Name | 随意 | 显示名称 |
| Gateway ID | `AA555A00000021FB` | 8字节 EUI，从网关串口日志读取 |
| Description | 随意 | |
| Tags | 可选 | |

> 网关 EUI 在 lora_pkt_fwd.c 中通过 `esp_read_mac()` 读取 WiFi MAC 地址生成，
> 串口日志中会打印：`Gateway EUI: AA:55:5A:00:00:00:21:FB`

注册后，若 gateway-bridge 配置正确，几秒后网关状态应变为 **Online**（绿色）。

### 3.2 创建设备配置文件（Device Profile）

**Device Profile** 定义了一类设备的 LoRaWAN 参数，多个设备可共用：

Web UI → **Device profiles** → **Add device profile**

| 字段 | 常用值 | 说明 |
|------|--------|------|
| Name | 随意 | 如 "CN470_10_OTAA_SF7" |
| Region | CN470_10 | 必须与网关频率计划一致 |
| MAC version | LoRaWAN 1.0.4 / 1.1 | 取决于节点固件 |
| Regional parameters | RP002-1.0.3 | |
| ADR algorithm | Default | 根据 RSSI/SNR 自动调整 SF |
| Uplink interval | 3600 | 预期上行间隔（秒），用于判断设备是否活跃 |

**Join 选项卡（OTA）：**

| 字段 | 说明 |
|------|------|
| Supports OTAA | 勾选（推荐用 OTAA，比 ABP 安全）|

### 3.3 创建应用（Application）

**Application** 是设备的逻辑容器，同一应用内的设备共享数据通道：

Web UI → **Applications** → **Add application**

| 字段 | 说明 |
|------|------|
| Name | 如 "Test_App" |
| Description | 随意 |

### 3.4 注册设备（Device）

在 Application 内注册具体设备：

Web UI → **Applications** → 选择应用 → **Add device**

| 字段 | 说明 |
|------|------|
| Name | 随意 |
| DevEUI | 节点固件中的 8字节设备EUI（唯一标识）|
| Device profile | 选择上面创建的 Profile |
| Skip frame-counter checks | 调试期间可勾选，生产环境不要勾 |

添加设备后，进入设备详情 → **Keys (OTAA)** 标签页，填写：

| 字段 | 说明 |
|------|------|
| Application key | 16字节，节点和 NS 共同持有的根密钥（AES-128）|
| NwkKey（1.1+）| LoRaWAN 1.1 额外密钥 |

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
