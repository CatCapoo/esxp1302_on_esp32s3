# SNTP 功能配置清单

> **所属项目**：ESXP1302 STM32F407 移植  
> **相关文档**：
> - 概念原理：[learning/05_ntp_sntp_concepts.md](../learning/05_ntp_sntp_concepts.md)
> - 代码实现：[impl/07_sntp_implementation.md](07_sntp_implementation.md)

本文档列出让 SNTP 功能正常工作所需配置的**全部项目**，分为：硬件/网络前提、编译时配置、运行时配置（Flash）、CMakeLists.txt、CubeMX。

---

## 目录

1. [硬件 / 网络前提](#1-硬件--网络前提)
2. [运行时配置（Flash / UART CLI）](#2-运行时配置flash--uart-cli)
3. [编译时配置（头文件常量）](#3-编译时配置头文件常量)
4. [CMakeLists.txt 源文件配置](#4-cmakeliststxt-源文件配置)
5. [CubeMX / STM32 HAL 配置](#5-cubemx--stm32-hal-配置)
6. [FreeRTOS 堆内存配置](#6-freertos-堆内存配置)
7. [常见问题与诊断](#7-常见问题与诊断)
8. [配置检查清单（Checklist）](#8-配置检查清单checklist)

---

## 1. 硬件 / 网络前提

SNTP 通过 **W5500 以太网芯片**发送 UDP 包，因此需要：

### 1.1 W5500 物理连接

| 要求 | 说明 |
|------|------|
| W5500 已正确接线 | SPI + CS/RST/INT GPIO，参见 `board_config.h` |
| RJ45 网线已插入 | W5500 链路灯亮（绿灯，表示物理层正常） |
| W5500 驱动初始化成功 | `net_init()` 返回 0，串口输出 `INFO: UDP sockets opened` |

### 1.2 局域网环境（推荐配置）

```
┌──────────────────────────────────────────────────────────┐
│  同一交换机 / 路由器 LAN                                   │
│                                                          │
│  ┌────────────┐        ┌──────────────┐                  │
│  │ ESXP1302   │        │ 路由器/NTP服务 │                  │
│  │ W5500      │        │ 192.168.X.1  │                  │
│  │ 192.168.X.Y│──────► │ (或 .254)     │ ← 局域网 NTP     │
│  └────────────┘        └──────────────┘                  │
│                                                          │
│  ┌────────────┐         ← 同 LAN 还有 NS 主机，也可能跑 NTP│
│  │ NS 主机    │                                           │
│  │ 192.168.X.100       │                                  │
│  └────────────┘                                          │
└──────────────────────────────────────────────────────────┘
                              │ 路由器需能访问互联网（备选）
                              ▼
                       公网 NTP: 120.25.115.20
```

> **最低要求**：路由器或同 LAN 内至少有一个设备运行 NTP 服务（chrony、ntpd 或 Windows Time Service 均可）。

---

## 2. 运行时配置（Flash / UART CLI）

以下参数存储在 **Flash Sector 11**（`gateway_config_t` 结构体），通过 UART CLI 配置后执行 `config save` 并重启生效。

### 2.1 必填项

| CLI 命令 | 参数名 | 示例值 | 说明 |
|----------|-------|--------|------|
| `config set eth_ip X.X.X.X` | `eth_ip[4]` | `192.168.71.110` | 网关静态 IP，**必须**在 LAN 子网内 |
| `config set eth_gw X.X.X.X` | `eth_gw[4]` | `192.168.71.1`   | 以太网默认网关 IP，**直接影响 NTP 候选1** |
| `config set eth_sn X.X.X.X` | `eth_sn[4]` | `255.255.255.0`  | 子网掩码，影响 W5500 路由判断 |

> `eth_gw` 是 NTP **候选 1**。填写正确的路由器 IP 可让第一次 NTP 查询即成功。

### 2.2 影响 NTP 候选4 的配置

| CLI 命令 | 说明 |
|----------|------|
| `config set ns_host X.X.X.X` | 设置 NS IP（影响 NTP 候选4：`sntp_sync(ns_ip)`）。若 NS 主机运行 chrony/ntpd 则候选4可成功 |

### 2.3 完整配置示例（对应本项目开发环境）

```
config set eth_ip  192.168.71.110
config set eth_gw  192.168.71.1
config set eth_sn  255.255.255.0
config set ns_host 192.168.71.100
config save
reboot
```

重启后串口应看到：

```
[SNTP] Task start. candidates: gw=192.168.71.1, derived=192.168.71.1, ns=192.168.71.100, pub=120.25.115.20
...（5秒后）...
[SNTP] Querying 192.168.71.1 ...
[SNTP] Time synced: 2026-03-12 10:23:45 UTC
```

---

## 3. 编译时配置（头文件常量）

### 3.1 `sntp_client.h` — NTP 核心参数

| 常量 | 默认值 | 说明 | 修改建议 |
|------|--------|------|----------|
| `NTP_SERVER_IP_DEFAULT` | `{120, 25, 115, 20}` | 公网 NTP（候选5），阿里云 | 如需使用其他公网 NTP，改这里 |
| `NTP_PORT` | `123` | NTP 标准 UDP 端口 | 不要修改 |
| `NET_SOCK_NTP` | `2` | W5500 硬件 socket 编号 | socket 0=上行, 1=下行, 2=NTP，不要冲突 |
| `SNTP_RETRY_INTERVAL` | `60` | 全部候选失败后重试间隔（秒） | 可改大（如 300）减少无效 UDP 流量 |
| `SNTP_RESYNC_INTERVAL` | `3600` | 成功同步后重同步间隔（秒）| 1小时足够补偿晶振漂移，一般不需改 |

### 3.2 `sntp_client.c` — 内部常量

| 常量 | 默认值 | 说明 |
|------|--------|------|
| `NTP_UNIX_OFFSET` | `2208988800UL` | NTP→Unix 纪元偏移，**不要修改** |
| `NTP_PACKET_SIZE` | `48` | SNTPv4 报文固定长度，**不要修改** |
| `NTP_TIMEOUT_MS` | `1500` | 等待服务器应答超时（毫秒）。可适当延长至 3000 以应对慢网络 |
| `NTP_LOCAL_PORT` | `10123` | 本地 UDP 绑定端口（临时端口），与其他 socket 不冲突即可 |
| `WIZNET_SOCKERR_TIMEOUT` | `-13` | W5500 ARP 超时错误码，**不要修改** |

### 3.3 `lora_pkt_fwd.c` — 时间刷新频率

| 常量 | 默认值 | 说明 |
|------|--------|------|
| `TIME_REFRESH` | `5`（秒） | OLED Row 6 时间更新间隔。改小会增加 I2C 刷新频率 |

---

## 4. CMakeLists.txt 源文件配置

需要确认以下两个文件都在 CMake 的 `target_sources` 中：

```cmake
# esxp1302_stm32f407/CMakeLists.txt

target_sources(esxp1302_stm32f407 PRIVATE
    # ...（其他源文件）...

    # SNTP 实现
    packet_forwarder/sntp_client.c

    # Newlib _gettimeofday 强符号覆盖（必须在 syscalls.c 之前，或单独放）
    Core/Src/sntp_gettimeofday.c

    # CubeMX 生成的弱符号（_gettimeofday weak）
    Core/Src/syscalls.c
    # ...
)
```

> **关键**：`sntp_gettimeofday.c` 和 `syscalls.c` 链接时，强符号覆盖弱符号，顺序不影响最终结果（链接器自动处理），但建议 `sntp_gettimeofday.c` 排在前面以明确意图。

---

## 5. CubeMX / STM32 HAL 配置

SNTP 自身不需要额外 CubeMX 配置，但依赖以下已有外设：

### 5.1 SysTick（HAL_GetTick 基础）

| 项目 | 要求 |
|------|------|
| SysTick 中断 | 必须启用（CubeMX 默认开启） |
| 分辨率 | 1 ms（FreeRTOS `configTICK_RATE_HZ = 1000`） |
| 用途 | `HAL_GetTick()` 提供软件时钟的 tick 基准 |

> FreeRTOS 接管 SysTick 后，`HAL_GetTick()` 实际调用的是 FreeRTOS 的 `xTaskGetTickCount()`。这在 `stm32f4xx_hal_timebase_tim.c` 中重定向（若使用 TIM 作为 HAL 时间基）；若使用 SysTick 作为 HAL 时间基则直接读取。本项目使用 TIM2 作为微秒级定时器，SysTick 仍作为 HAL 时间基。

### 5.2 W5500 SPI 接口（间接依赖）

| 外设 | 要求 |
|------|------|
| SPI（W5500） | 已初始化（见 `board_config.h` → `MX_SPI1_Init()`） |
| W5500 CS GPIO | 配置为输出，默认高电平 |
| W5500 RST GPIO | 配置为输出，W5500 初始化时复位 |

### 5.3 不需要的外设

| 外设 | 说明 |
|------|------|
| I2C | SNTP 不用 I2C，与 OLED 驱动完全独立 |
| RTC | 不需要 STM32 内部 RTC，软件时钟替代之 |
| GPS / PPS | 不需要 |

---

## 6. FreeRTOS 堆内存配置

`sntp_task_start()` 调用 `xTaskCreate()` 分配任务栈：

| 参数 | 值 | 实际内存 |
|------|----|---------|
| 栈大小 | 512 words | 512 × 4 = **2048 字节** |
| 堆分配器 | FreeRTOS heap_4 | 从 `configTOTAL_HEAP_SIZE` 中分配 |

### 当前 RAM 占用（已知）

- Build 占用：FLASH 32.90%，**RAM 95.46%**（偏紧）
- SNTP 任务 2KB 栈已包含在此数字中（已通过编译和运行测试）

> 若未来添加新功能导致 RAM 不足，可将 SNTP 栈从 512 字减小到 384 字（仍够用，因为 `sntp_sync()` 调用链最深约 128 字节）。

---

## 7. 常见问题与诊断

### 7.1 OLED Row 6 一直显示 "Up HH:MM:SS"，不显示真实时间

**现象**：即使等很长时间，第 6 行仍然是运行时长。

**诊断步骤**：

1. 检查串口是否有 `[SNTP] Time synced: ...`
   - 有 → 跳到步骤 3
   - 没有 → 继续步骤 2

2. 检查串口是否有以下之一：
   ```
   [SNTP] WARN: ARP timeout reaching X.X.X.X        ← eth_gw 配置不在同一 LAN
   [SNTP] WARN: no response from X.X.X.X within 1500 ms  ← 对方没有 NTP 服务
   [SNTP] Both NTP sources failed, retry in 60s      ← 全部候选均失败
   ```
   - 全是 ARP timeout → `eth_gw` 配置错误，检查 `config set eth_gw`
   - 全是 no response → LAN 内没有 NTP 服务器，检查路由器是否开启 NTP

3. 已同步但 OLED 不显示 → 检查是否 `t > 1700000000` 校验失败（时间被设置为异常值）

### 7.2 串口打印 "[SNTP] DBG W5500 regs: GAR=0.0.0.0"

**含义**：W5500 的网关寄存器（GAR）是全零，说明 `eth_gw` 配置为 0.0.0.0 或未初始化。

**修复**：执行 `config set eth_gw X.X.X.X` 并 `config save; reboot`。

### 7.3 NTP 只能从公网成功（候选5），局域网均失败

**可能原因**：

| 现象 | 原因 | 解决 |
|------|------|------|
| 候选1 ARP timeout | `eth_gw` IP 错误（不是同 LAN 路由器） | 正确配置 `eth_gw` |
| 候选4 no response | NS 主机（如 PC）未运行 NTP 服务 | 安装 chrony：`sudo apt install chrony` |
| 全局 ARP timeout | W5500 物理层未通（网线、交换机） | 检查网线和 LED 状态 |

### 7.4 如何在 NS 主机（Linux）上开启 NTP 服务

```bash
# 安装并启动 chrony（推荐）
sudo apt install chrony
sudo systemctl enable chrony
sudo systemctl start chrony

# 允许 LAN 设备查询（编辑 /etc/chrony.conf，添加）
allow 192.168.71.0/24

sudo systemctl restart chrony

# 验证：从网关发 NTP 请求到该主机应成功
```

### 7.5 候选 IP 推导逻辑

若 `eth_ip = 192.168.71.110`，则：

| 候选 | 推导结果 | 去重检查 |
|------|---------|---------|
| 候选1 | `eth_gw`（配置值，如 `192.168.71.1`） | — |
| 候选2 | `192.168.71.1`（eth_ip[0..2] + .1） | 若与候选1相同则**跳过** |
| 候选3 | `192.168.71.254`（eth_ip[0..2] + .254） | 若与候选1相同则**跳过** |
| 候选4 | `ns_ip`（来自 `ns_host` 配置） | — |
| 候选5 | `120.25.115.20`（硬编码） | — |

---

## 8. 配置检查清单（Checklist）

在部署前逐项确认：

### 编译期

- [ ] `packet_forwarder/sntp_client.c` 已加入 `CMakeLists.txt`
- [ ] `Core/Src/sntp_gettimeofday.c` 已加入 `CMakeLists.txt`
- [ ] `NTP_SERVER_IP_DEFAULT` 为可达的公网 NTP IP（如不能上网，注释掉候选5或改为内网 IP）
- [ ] `NET_SOCK_NTP = 2` 未与其他 W5500 socket 冲突（0=upstream, 1=downstream 已占用）

### Flash 配置（运行时）

- [ ] `eth_ip` 在正确的子网内（与路由器同 LAN）
- [ ] `eth_gw` 是路由器/网关的真实 IP
- [ ] `eth_sn` 子网掩码正确（通常 `255.255.255.0`）
- [ ] `ns_host` 填写 NS 主机 IP（若 NS 主机运行 NTP 则候选4可用）

### 网络环境

- [ ] 路由器开启 NTP 服务（大多数家用/企业路由器默认开启）
- [ ] 网关板与路由器网线连接正常（链路灯亮）
- [ ] 公网 NTP 可达（路由器有互联网接入，或关闭公网候选）

### 运行验证

- [ ] 串口输出 `[SNTP] Task start. candidates: ...`（任务已创建）
- [ ] 约 5 秒后看到 `[SNTP] Querying X.X.X.X ...`（开始查询）
- [ ] 最终看到 `[SNTP] Time synced: 2026-XX-XX XX:XX:XX UTC`（同步成功）
- [ ] OLED Row 6 从 `Up 00:00:05` 切换为 `2026-XX-XX XX:XX:XX Z`
- [ ] ChirpStack/TTN stat 面板时间戳显示真实 UTC（不是 `2000-01-01`）
