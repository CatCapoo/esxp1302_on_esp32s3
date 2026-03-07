# ChirpStack 对接与 CN470_10 频率计划配置备忘录

**日期：** 2026-02-22  
**测试目的：** 排查网关通过 ChirpStack Gateway Bridge 接入 ChirpStack NS 后，
GUI 始终显示网关"离线"的问题，并完成 CN470_10 频率计划的完整配置。

---

## 一、系统架构

```
LoRa 节点（普通 LoRa / LoRaWAN 节点）
  │  上行：486.3 ~ 487.7 MHz（CN470_10 ch80~ch87）
  ▼
ESP32-S3 + SX1302（ESXP1302 网关）
  │  Semtech UDP 协议，端口 1700
  ▼
chirpstack-gateway-bridge（Docker 容器）
  │  协议转换：UDP → MQTT
  │  发布 topic: cn470_10/gateway/<GW_ID>/event/up|stats
  ▼
mosquitto（MQTT Broker，端口 1883）
  ▼
chirpstack（Network Server，端口 8080）
  │  订阅 topic: cn470_10/gateway/...
  │  按 region_cn470_10 规则解析上行包
  ▼
chirpstack-rest-api（端口 8090）/ Web GUI（端口 8080）
```

### 服务部署

所有服务通过 Docker Compose 部署，路径：`~/chirpstack-docker/docker-compose.yml`

---

## 二、遇到的问题

### 问题 1：网关 GUI 始终显示离线

**现象：**
- 网关日志显示 `PUSH_DATA acknowledged: 100%`，UDP 通信完全正常
- ChirpStack GUI 中网关一直显示 "Never seen"（从未上线）
- `chirpstack-gateway-bridge` 日志显示发布 topic 为 `cn470/gateway/.../event/stats`

**根因：MQTT topic 前缀不匹配**

| 组件 | topic 前缀（修复前）|
|------|-------------------|
| gateway-bridge 发布 | `cn470/...` |
| ChirpStack NS 订阅 | `cn470_10/...` |

两者 topic 不在同一频道，NS 收不到 stats 消息，网关永远显示离线。

**附加问题：** `docker-compose.yml` 中 `EVENT_TOPIC_TEMPLATE` 配置项存在非法换行符，
导致模板字符串被截断，进一步导致 topic 解析异常。

**修复方法：**

编辑 `~/chirpstack-docker/docker-compose.yml`，将 `chirpstack-gateway-bridge` 服务的
环境变量中 topic 前缀从 `cn470` 改为 `cn470_10`，同时修复换行符：

```yaml
  chirpstack-gateway-bridge:
    environment:
      - INTEGRATION__MQTT__EVENT_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/event/{{ .EventType }}
      - INTEGRATION__MQTT__STATE_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/state/{{ .StateType }}
      - INTEGRATION__MQTT__COMMAND_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/command/#
```

修复后重启容器：
```bash
cd ~/chirpstack-docker
docker compose up -d --no-deps chirpstack-gateway-bridge
```

**验证：** 查看 gateway-bridge 日志，确认 topic 已更新：
```
topic=cn470_10/gateway/aa555a00000021fb/event/stats
topic=cn470_10/gateway/aa555a00000021fb/state/conn
```

---

### 问题 2：网关可显示在线，但节点上行数据被 NS 丢弃

**现象：**
```
ERROR chirpstack::uplink: Deduplication error error=MACPayload requires at least 7 bytes
```

**原因 A（已解决）：频率不匹配**

修复前网关 radio 频率（NVS 中）为 480.4/481.2 MHz（约为 CN470_6 范围），
而 NS 配置的 `region_cn470_10` 期望信道为 486.3~487.7 MHz，两者相差约 6 MHz，
上行包频率不在 NS 的信道列表内，被直接丢弃。

修复方法：通过网页配置界面（Soft AP 模式）更新 NVS 中的 radio 频率（详见第三节）。

**原因 B（测试阶段预期现象）：裸 LoRa 包没有 LoRaWAN MAC 帧头**

测试节点发送的是裸 LoRa 数据（payload = "hello world"），
不包含 LoRaWAN 规定的 MHDR + DevAddr + FCtrl + FCnt + MIC 等字段（最少 12 字节），
NS 在解析 MAC 层时失败报错。

这是**预期现象**，等正式 LoRaWAN 节点上线后自动解决，无需处理。

---

## 三、CN470_10 完整频率配置方案

### 3.1 CN470 频段计划说明

CN470 是 470~510 MHz 频段的统称。LoRa Alliance 将其细分为 12 个子信道计划
（`cn470_0` ~ `cn470_11`），每个计划覆盖 8 个上行信道（200 kHz 间距）。

ChirpStack 中每个子计划对应一个独立的 region 配置文件和 MQTT topic 前缀，
**三者必须保持一致：网关硬件频率 = Gateway Bridge topic 前缀 = NS region**。

本项目使用 **CN470_10**（LoRa Alliance ch80~ch87）。

---

### 3.2 网关硬件配置（NVS 或 global_conf.json）

SX1302 内部有 2 个射频前端（radio_0 / radio_1），每个覆盖 ±400 kHz，
合计 8 个上行信道，间距 200 kHz。

**radio 中心频率公式：**

$$f_{radio\_1} = f_{radio\_0} + 800\,\text{kHz}$$

**CN470_10 的 radio 配置：**

| 参数 | 值 |
|------|----|
| `radio_0` 中心频率 | `486600000` Hz（486.6 MHz）|
| `radio_1` 中心频率 | `487400000` Hz（487.4 MHz）|

**8 个上行接收信道（IF 偏移固定）：**

| 信道名 | LoRa ch | Radio | IF 偏移 | 实际接收频率 |
|--------|---------|-------|---------|------------|
| multiSF_0 | ch80 | radio_0 | -300 kHz | **486.3 MHz** |
| multiSF_1 | ch81 | radio_0 | -100 kHz | **486.5 MHz** |
| multiSF_2 | ch82 | radio_0 | +100 kHz | **486.7 MHz** |
| multiSF_3 | ch83 | radio_0 | +300 kHz | **486.9 MHz** |
| multiSF_4 | ch84 | radio_1 | -300 kHz | **487.1 MHz** |
| multiSF_5 | ch85 | radio_1 | -100 kHz | **487.3 MHz** |
| multiSF_6 | ch86 | radio_1 | +100 kHz | **487.5 MHz** |
| multiSF_7 | ch87 | radio_1 | +300 kHz | **487.7 MHz** |

> 这 8 个频率即为节点上行发射频率，节点只需配置这 8 个，下行由 NS 自动计算。

**通过网页配置界面修改 radio 频率（推荐方式）：**

1. 上电时按住 **IO0（左键）** 进入 Soft AP 模式
2. 手机/电脑连接 WiFi：SSID=`esp32`，密码=`esp32wifi`
3. 浏览器访问 `http://192.168.4.1`，登录（用户名 `iot`，密码 `lora`）
4. 填写：
   - **radio0 频率**：`486600000`
   - **radio1 频率**：`487400000`
5. 点击 **Apply** → **Reboot**
6. Soft AP 模式有 **10 分钟超时**，务必在超时前完成操作

---

### 3.3 Gateway Bridge 配置

文件路径：`~/chirpstack-docker/docker-compose.yml`

```yaml
chirpstack-gateway-bridge:
  environment:
    - INTEGRATION__MQTT__EVENT_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/event/{{ .EventType }}
    - INTEGRATION__MQTT__STATE_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/state/{{ .StateType }}
    - INTEGRATION__MQTT__COMMAND_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/command/#
```

> **关键：topic 前缀必须与 NS 的 region id 完全一致**，包括下划线和数字。
> `cn470` ≠ `cn470_10`，差一个字符就全部失效。

---

### 3.4 ChirpStack NS 配置

**主配置文件：** `~/chirpstack-docker/configuration/chirpstack/chirpstack.toml`

确认 `cn470_10` 在 `enabled_regions` 列表中：
```toml
enabled_regions=[
  "cn470_10",
  ...
]
```

**区域配置文件：** `~/chirpstack-docker/configuration/chirpstack/region_cn470_10.toml`

关键参数：

| 参数 | 值 | 说明 |
|------|----|------|
| `topic_prefix` | `cn470_10` | 必须与 gateway-bridge 一致 |
| `enabled_uplink_channels` | `[80,81,82,83,84,85,86,87]` | ch80~ch87 |
| `rx2_frequency` | `505300000` Hz | RX2 窗口固定频率 |
| `rx1_delay` | 1 秒 | 上行后 RX1 窗口时延 |

---

### 3.5 上下行频率关系

CN470 采用**频分双工（FDD）**设计，上下行故意分开到不同频段，防止网关自发自收干扰。

| 方向 | 频率范围 |
|------|---------|
| 上行（节点→网关）| 470~490 MHz |
| 下行（网关→节点）| 500~510 MHz |

**CN470_10 的 RX1 下行频率映射（NS 自动计算，无需手动配置）：**

| 上行信道 | 上行频率 | 对应 RX1 下行频率 |
|---------|---------|-----------------|
| ch80 | 486.3 MHz | 506.7 MHz |
| ch81 | 486.5 MHz | 506.9 MHz |
| ch82 | 486.7 MHz | 507.1 MHz |
| ch83 | 486.9 MHz | 507.3 MHz |
| ch84 | 487.1 MHz | 507.5 MHz |
| ch85 | 487.3 MHz | 507.7 MHz |
| ch86 | 487.5 MHz | 507.9 MHz |
| ch87 | 487.7 MHz | 508.1 MHz |

RX1 映射公式：

$$f_{RX1} = 500.3 + (ch \bmod 48) \times 0.2 \text{ MHz}$$

例：ch82 mod 48 = 34，$500.3 + 34 \times 0.2 = 507.1$ MHz ✅

**RX2 固定频率：505.3 MHz，SF12BW125**（上行后 2 秒）

---

### 3.6 切换其他频率计划的步骤（手动操作指南）

如需切换到其他 CN470 子计划（如 cn470_0），按以下步骤操作：

**第一步：确定目标计划的 radio 频率**

不同计划的 radio_0 起始频率：

| 计划 | radio_0 | radio_1 | 上行信道 |
|------|---------|---------|---------|
| cn470_0 | 470600000 | 471400000 | 470.3~471.7 MHz |
| cn470_1 | 472600000 | 473400000 | 472.3~473.7 MHz |
| cn470_2 | 474600000 | 475400000 | 474.3~475.7 MHz |
| cn470_3 | 476600000 | 477400000 | 476.3~477.7 MHz |
| cn470_4 | 478600000 | 479400000 | 478.3~479.7 MHz |
| cn470_5 | 480600000 | 481400000 | 480.3~481.7 MHz |
| cn470_6 | 482600000 | 483400000 | 482.3~483.7 MHz |
| cn470_7 | 484600000 | 485400000 | 484.3~485.7 MHz |
| cn470_8 | 484600000 | 485400000 | 485.3~486.7 MHz |（与7有重叠，请查官方文档确认）
| cn470_9 | 486600000 | 487400000 | 486.3~487.7 MHz |（注：与cn470_10相同上行，下行不同）
| **cn470_10** | **486600000** | **487400000** | **486.3~487.7 MHz** |
| cn470_11 | 488600000 | 489400000 | 488.3~489.7 MHz |

> 精确频率以 `~/chirpstack-docker/configuration/chirpstack/region_cn470_XX.toml` 中的 `frequency` 字段为准。

**第二步：更新网关 radio 频率**（见 3.2 节网页配置步骤）

**第三步：更新 gateway-bridge topic 前缀**

```bash
# 编辑 docker-compose.yml，将所有 cn470_10 改为目标计划名
vi ~/chirpstack-docker/docker-compose.yml

# 重启 gateway-bridge
cd ~/chirpstack-docker
docker compose up -d --no-deps chirpstack-gateway-bridge
```

**第四步：确认 NS 已启用目标 region**

```bash
grep "enabled_regions" ~/chirpstack-docker/configuration/chirpstack/chirpstack.toml
```

如目标 region 不在列表中，添加后重启 chirpstack：
```bash
docker compose restart chirpstack
```

**第五步：验证**

```bash
# 查看 gateway-bridge 日志，确认 topic 前缀正确
docker compose logs --tail=20 chirpstack-gateway-bridge | grep topic

# 查看 NS 日志，确认收到来自正确 region 的消息
docker compose logs --tail=20 chirpstack | grep region_id
```

---

## 四、测试现象记录

### 4.1 修复前（topic 前缀 cn470）

| 现象 | 说明 |
|------|------|
| 网关 UDP ackr = 100% | Gateway Bridge 收到 UDP 包并 ACK，正常 |
| NS GUI 显示"Never seen" | NS 从未收到来自网关的 stats，topic 不匹配 |
| chirpstack 日志无任何 gateway 相关输出 | NS 根本没有订阅 cn470/... topic |

### 4.2 修复后（topic 前缀 cn470_10，radio 频率 486.6/487.4 MHz）

| 现象 | 说明 |
|------|------|
| gateway-bridge 日志 | `topic=cn470_10/gateway/.../event/stats` ✅ |
| NS 日志 | `region_id="cn470_10"` 收到消息 ✅ |
| NS Gateway 状态 | `Gateway partially updated` ✅ 网关在线 |
| 节点上行 | `freq=486.700000, chan=2, CRC_OK=100%` ✅ |
| NS 上行处理 | `MACPayload requires at least 7 bytes` ⚠️ 预期内（裸 LoRa 测试包）|

### 4.3 节点信号质量（测试环境）

```json
{
  "freq": 486.700000,
  "chan": 2,
  "datr": "SF12BW125",
  "rssi": -73,
  "lsnr": 4.0,
  "foff": 392
}
```

| 参数 | 值 | 评价 |
|------|----|------|
| RSSI | -73 dBm | 良好（室内近距离）|
| SNR | 4.0 dB | 正常（SF12 可用范围 -20~+10 dB）|
| foff（频率偏差）| ~400 Hz | 正常，晶振误差范围内 |
| CRC | 100% OK | 信号质量优秀 |

---

## 五、下一步

- [ ] 部署 LoRaWAN 协议栈到节点（OTAA 或 ABP）
- [ ] 在 ChirpStack 创建 Application + Device Profile + 注册 Device（填写 DevEUI/AppKey）
- [ ] 验证 OTAA Join 流程（Join Request → Join Accept）
- [ ] 验证上行数据帧在 ChirpStack GUI 中正常显示
- [ ] 验证下行 ACK（RX1 507.1 MHz 或 RX2 505.3 MHz）

---

*记录人：cuckooshan，2026-02-22*
