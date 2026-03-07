# Learning 学习笔记目录

本目录收录项目开发过程中沉淀的专题学习笔记，每篇对应一个具体知识点或调试事件，
侧重"为什么"和"是什么"，与 `test_notes/`（测试过程记录）和 `bugfix/`（问题修复记录）互为补充。

---

## 文档列表

### LoRaWAN 协议与频率计划

| 文件 | 内容摘要 | 来源事件 |
|------|---------|---------|
| [lora_cn470_frequency_plan.md](lora_cn470_frequency_plan.md) | CN470 频率体系：12 个子计划、radio 中心频率计算、RX1/RX2 公式、FDD 上下行分离机制、chanmask 结构 | 2026-02-22 ChirpStack 接入调试 |
| [lorawan_gateway_ns_protocol.md](lorawan_gateway_ns_protocol.md) | Semtech UDP 包转发协议（PUSH_DATA / PULL_DATA / PULL_RESP）、Gateway Bridge MQTT topic 结构、上下行数据流全链路 | 2026-02-22 网关 NS 对接调试 |

### ChirpStack

| 文件 | 内容摘要 | 来源事件 |
|------|---------|---------|
| [chirpstack_v4_usage.md](chirpstack_v4_usage.md) | ChirpStack v4 配置体系（Region / Gateway / Device Profile / Application / Device）、OTAA/ABP 注册步骤、MQTT topic 与 region_id 的对应关系、常见故障排查 | 2026-02-22 ChirpStack 接入调试 |

### LoRaWAN 节点调试

| 文件 | 内容摘要 | 来源事件 |
|------|---------|---------|
| [lorawan_node_e77_otaa_debug.md](lorawan_node_e77_otaa_debug.md) | E77-400M22S AT 指令接口要点、OTAA JOIN FAILED 的分层诊断法（RF层→NS层→RX1层）、CN470 chanmask 双重作用（上行频率 + RX1 计算基准）、pyserial DTR/RTS 复位陷阱 | 2026-02-27 E77 节点全链路验证 |

### 固件代码

| 文件 | 内容摘要 | 来源事件 |
|------|---------|---------|
| [esxp1302_code_walkthrough.md](esxp1302_code_walkthrough.md) | ESXP1302 工程完整代码解析：任务架构、SX1302 HAL 初始化流程、packet forwarder 收发逻辑、NVS 配置读写 | 2026-02-22 代码走读 |

### 驱动移植

| 文件 | 内容摘要 | 来源事件 |
|------|---------|---------|
| [lm75a_i2c_driver_porting.md](lm75a_i2c_driver_porting.md) | LM75A 温度传感器 I2C 驱动移植：ESP-IDF I2C API、11-bit 有符号温度数据解析、I2C 总线卡死恢复机制、RSSI 温度补偿接入 | 2026-02-27 LM75A 驱动移植 |

### Bug 深度复盘

| 文件 | 内容摘要 | 来源事件 |
|------|---------|---------|
| [BUG-010_notes.md](BUG-010_notes.md) | PUSH_DATA ackr 低（22%）根因全分析：MQTT TLS 阻塞、`tv_usec` 溢出（1,000,000 μs 非法值）、drain setsockopt 竞态、WiFi Modem Sleep AP 缓冲延迟 | 2026-02-21 BUG-010 |

---

## 知识索引

按关键词快速定位：

| 关键词 | 相关文档 |
|--------|---------|
| CN470 子频段 / chanmask / RX1 公式 | [lora_cn470_frequency_plan.md](lora_cn470_frequency_plan.md)、[lorawan_node_e77_otaa_debug.md](lorawan_node_e77_otaa_debug.md) |
| OTAA JOIN FAILED 诊断 | [lorawan_node_e77_otaa_debug.md](lorawan_node_e77_otaa_debug.md) |
| ChirpStack MQTT topic 不匹配 / region_id | [chirpstack_v4_usage.md](chirpstack_v4_usage.md)、[lorawan_gateway_ns_protocol.md](lorawan_gateway_ns_protocol.md) |
| Semtech UDP / PUSH_DATA / PULL_RESP | [lorawan_gateway_ns_protocol.md](lorawan_gateway_ns_protocol.md)、[BUG-010_notes.md](BUG-010_notes.md) |
| pyserial DTR/RTS 复位 | [lorawan_node_e77_otaa_debug.md](lorawan_node_e77_otaa_debug.md) |
| E77 AT 指令 / AT_CCLASS 入网前报错 | [lorawan_node_e77_otaa_debug.md](lorawan_node_e77_otaa_debug.md) |
| ADR / LinkADRReq | [lorawan_node_e77_otaa_debug.md](lorawan_node_e77_otaa_debug.md) |
| ESP-IDF I2C / LM75A | [lm75a_i2c_driver_porting.md](lm75a_i2c_driver_porting.md) |
| tv_usec 溢出 / ackr 低 | [BUG-010_notes.md](BUG-010_notes.md) |
| SX1302 HAL / packet forwarder 架构 | [esxp1302_code_walkthrough.md](esxp1302_code_walkthrough.md) |

---

*最后更新：2026-02-27*
