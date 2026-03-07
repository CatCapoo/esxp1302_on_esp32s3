# 06 — 多区域频率计划、Flash 配置 v4 与 UART CLI 频率命令

## 概述

本文档记录以下三项功能的设计、实现逻辑与代码结构：

1. **多区域频率计划预设系统**（`gw_config_presets.h/c`、`gateway_defaults.h`）
2. **Flash 配置结构体 v4**（`gateway_config.h/c`）：新增频率字段、增强诊断
3. **UART CLI 频率命令**（`uart_cli.c`）：`config set freq_region`、`region list`
4. **链接脚本 Flash 区域保护**（`STM32F407ZGTx_FLASH_cmake.ld`）

这些改动是在原有 Flash 配置功能（见 [06_flash_config_uart_cli_testing.md](../testing/06_flash_config_uart_cli_testing.md)）基础上的扩展，将频率配置从编译期硬编码改为运行时可配置。

---

## Part 1：背景与需求

### 1.1 问题：频率固定在代码里

原始 Packet Forwarder 直接使用 `global_cn_conf`（CN470 470.6/471.4 MHz）作为唯一配置，没有任何运行时切换机制：

```c
// 原始代码（lora_pkt_fwd.c）
const char *conf_array = (const char *)global_cn_conf;  // 始终 CN470 SB0
```

实际部署中，ChirpStack 使用 `cn470_10` 区域（SB10，486.6/487.4 MHz），与默认值相差约 16 MHz——节点与网关根本不在同一频段，即便所有软件都正常，也收不到任何数据包。

### 1.2 CN470 频率计划结构

LoRaWAN Alliance CN470 标准定义了 96 个上行信道，按 200 kHz 间隔排列：

$$f_{up}(k) = 470.3 + k \times 0.2 \text{ MHz},\quad k = 0, 1, \ldots, 95$$

96 个信道被均匀分为 **12 个子带（Sub-Band）**，每个子带 8 个信道：

| 子带 | 信道 | 频率范围 | radio_0 中心 | radio_1 中心 |
|------|------|---------|-------------|-------------|
| SB0  | CH0-7   | 470.3–471.7 MHz | 470.6 MHz | 471.4 MHz |
| SB1  | CH8-15  | 471.9–473.3 MHz | 472.2 MHz | 473.0 MHz |
| SB2  | CH16-23 | 473.5–474.9 MHz | 473.8 MHz | 474.6 MHz |
| SB3  | CH24-31 | 475.1–476.5 MHz | 475.4 MHz | 476.2 MHz |
| SB4  | CH32-39 | 476.7–478.1 MHz | 477.0 MHz | 477.8 MHz |
| SB5  | CH40-47 | 478.3–479.7 MHz | 478.6 MHz | 479.4 MHz |
| SB6  | CH48-55 | 479.9–481.3 MHz | 480.2 MHz | 481.0 MHz |
| SB7  | CH56-63 | 481.5–482.9 MHz | 481.8 MHz | 482.6 MHz |
| SB8  | CH64-71 | 483.1–484.5 MHz | 483.4 MHz | 484.2 MHz |
| SB9  | CH72-79 | 484.7–486.1 MHz | 485.0 MHz | 485.8 MHz |
| **SB10** | **CH80-87** | **486.3–487.7 MHz** | **486.6 MHz** | **487.4 MHz** |
| SB11 | CH88-95 | 487.9–489.3 MHz | 488.2 MHz | 489.0 MHz |

每个子带的 radio_0/radio_1 计算规律：

$$\text{radio}_0(N) = 470.6 + N \times 1.6 \text{ MHz}$$
$$\text{radio}_1(N) = \text{radio}_0(N) + 0.8 \text{ MHz}$$

SX1302 有 8 个 IF 通道，分别以两个 radio 为中心，±100/300/500 kHz 偏移覆盖 8 个信道。

**ChirpStack 命名**：`cn470_N`（N = 0..11）对应 SB_N，与本项目 CLI 名称 `CN470_N` 完全一致。

---

## Part 2：频率计划预设系统

### 2.1 文件结构

```
libloragw/
  gw_config_presets.h   — freq_region_t 枚举 + gw_freq_preset_t 结构体 + API 声明
  gw_config_presets.c   — 16 个预设条目 + gw_preset_get() / gw_preset_find_by_name()
  gateway_defaults.h    — 编译期默认值（NS host、EUI、IP、频率区域）
```

### 2.2 `freq_region_t` 枚举设计

```c
// gw_config_presets.h
typedef enum {
    FREQ_REGION_CN470_SB0  =  0,  // CH0-7:   470.3-471.7 MHz
    FREQ_REGION_CN470_SB1  =  1,  // CH8-15:  471.9-473.3 MHz
    // ... SB2-SB9 ...
    FREQ_REGION_CN470_SB10 = 10,  // CH80-87: 486.3-487.7 MHz ← ChirpStack cn470_10
    FREQ_REGION_CN470_SB11 = 11,  // CH88-95: 487.9-489.3 MHz

    FREQ_REGION_EU868      = 12,
    FREQ_REGION_US915_SB0  = 13,
    FREQ_REGION_AU915_SB0  = 14,
    FREQ_REGION_AS923      = 15,

    FREQ_REGION_CUSTOM     = 255  // 手动设置 radio0_freq/radio1_freq
} freq_region_t;

#define GW_FREQ_PRESET_COUNT  16
```

**关键设计决策**：
- 枚举值从 0 开始连续编号，CN470_SB_N 的值就是 N，与 ChirpStack `cn470_N` 中的 N 直接对应
- `FREQ_REGION_CUSTOM = 255` 使用最大 uint8 值，与任何有效预设不冲突
- 枚举值存在 `gateway_config_t.freq_region`（uint8_t），一字节即可表达

### 2.3 预设表实现

```c
// gw_config_presets.c
const gw_freq_preset_t gw_freq_presets[GW_FREQ_PRESET_COUNT] = {
    { FREQ_REGION_CN470_SB0,  "CN470_0",  470600000UL, 471400000UL,
      "CN470 SB0  (CH0-7,   470.3-471.7 MHz)" },
    // ...
    { FREQ_REGION_CN470_SB10, "CN470_10", 486600000UL, 487400000UL,
      "CN470 SB10 (CH80-87, 486.3-487.7 MHz) = ChirpStack cn470_10" },
    // ...
    { FREQ_REGION_EU868,      "EU868",    867500000UL, 868500000UL,
      "EU868 (867.1-868.5 MHz)" },
};
```

频率以 **Hz 整数**存储（而非 MHz 浮点），与 SX1302 HAL 的 `rfconf.freq_hz` 类型保持一致，避免浮点精度问题。

### 2.4 查找 API

```c
// 按枚举值查找（用于 config_print、lora_pkt_fwd 运行时）
const gw_freq_preset_t *gw_preset_get(freq_region_t region);

// 按名称查找，大小写不敏感（用于 CLI "config set freq_region CN470_10"）
freq_region_t gw_preset_find_by_name(const char *name);
```

`gw_preset_find_by_name()` 使用逐字符 `tolower()` 比较，允许用户输入 `cn470_10`、`CN470_10`、`Cn470_10` 均能匹配。

### 2.5 `gateway_defaults.h` 的作用

以前的默认值硬编码在 `gateway_config.h` 中，与结构定义混在一起难以维护。现在独立为 `gateway_defaults.h`：

```c
// gateway_defaults.h — 修改此文件即可更改编译期默认值
#define GW_DEFAULT_NS_HOST          "192.168.71.100"
#define GW_DEFAULT_NS_PORT_UP       1700
#define GW_DEFAULT_EUI              0xAA555A00000021FBULL
#define GW_DEFAULT_ETH_IP           {192, 168, 71, 110}
#define GW_DEFAULT_FREQ_REGION      FREQ_REGION_CN470_SB0
#define GW_DEFAULT_RADIO0_FREQ      470600000UL
#define GW_DEFAULT_RADIO1_FREQ      471400000UL
```

`gateway_config.h` 通过宏转发引用这些定义：

```c
#include "gateway_defaults.h"
#define CONFIG_DEFAULT_FREQ_REGION  GW_DEFAULT_FREQ_REGION
#define CONFIG_DEFAULT_RADIO0_FREQ  GW_DEFAULT_RADIO0_FREQ
// ...
```

---

## Part 3：Flash 配置结构体 v4

### 3.1 结构体变化

```c
// gateway_config.h
typedef struct {
    uint32_t magic;              // 0xC0FFEE04 (v4)
    char     ns_host[64];
    uint16_t ns_port_up;
    uint16_t ns_port_down;
    uint64_t gateway_eui;
    uint8_t  eth_ip[4];
    uint8_t  eth_gw[4];
    uint8_t  eth_sn[4];
    /* === v3 新增 === */
    uint32_t radio0_freq;        // SX1302 radio_0 中心频率，Hz
    uint32_t radio1_freq;        // SX1302 radio_1 中心频率，Hz
    uint8_t  freq_region;        // freq_region_t 枚举值
    uint8_t  _pad[3];            // 对齐填充，必须置 0
    /* =============== */
    uint32_t checksum;           // 所有前序字节的 32 位累加和
} gateway_config_t;
```

**为什么要 `_pad[3]`**：`freq_region` 是 1 字节，`checksum` 是 4 字节，若不填充，`checksum` 会在偏移 `+1` 处，导致非对齐访问。填充 3 字节后 `checksum` 对齐到 4 字节边界，Flash 按 word 写入时无额外处理。同时 `sizeof(gateway_config_t)` 为 4 的倍数，满足 `HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, ...)` 要求。

### 3.2 CONFIG_MAGIC 版本管理

```c
// v1: 原始结构
// v2: ns_port 默认值从 1680 改为 1700
// v3: 添加 radio0_freq, radio1_freq, freq_region, _pad[3]
// v4: CN470 从 8 个子带扩展到 12 个子带，EU868/US915 枚举值位移(+4)
#define CONFIG_MAGIC  0xC0FFEE04U
```

每次 `gateway_config_t` 布局改变时 magic 必须更新，否则 `config_load()` 会用旧 Flash 数据填充新字段。版本不匹配时：

```c
// gateway_config.c — config_load()
if (flash->magic != CONFIG_MAGIC) {
    printf("[CFG] Flash magic mismatch: found 0x%08X, expected 0x%08X\r\n",
           (unsigned)flash->magic, (unsigned)CONFIG_MAGIC);
    printf("[CFG] (firmware update changed config layout; defaults will be applied)\r\n");
    apply_defaults(&s_config);
}
```

**实际遇到的问题**：在 v4 之前的固件中，某次提交只扩展了枚举（SB8-SB11），导致 EU868 的枚举值从 8 变成了 12，同一个 Flash 字节 `freq_region=8` 在旧固件里是 EU868，在新固件里变成了 CN470_SB8。magic 升版强制重新应用默认值，避免此类静默错误。

### 3.3 增强的诊断输出

**config_load 诊断（magic 不匹配 vs. checksum 不匹配）**：

```c
if (flash->magic != CONFIG_MAGIC) {
    // magic 不匹配：固件更新导致结构变化，这是正常的
    printf("[CFG] Flash magic mismatch: found 0x%08X, expected 0x%08X\r\n", ...);
} else {
    // magic 相同但 checksum 错：数据损坏（极罕见）
    uint32_t expected = checksum_compute(flash);
    printf("[CFG] Flash checksum mismatch: stored 0x%08X, computed 0x%08X\r\n", ...);
}
```

**config_save 回读验证**：

```c
// gateway_config.c — config_save()
const gateway_config_t *flash = (const gateway_config_t *)CONFIG_FLASH_ADDR;
if (flash->magic == s_config.magic &&
    flash->checksum == s_config.checksum &&
    memcmp(flash->ns_host, s_config.ns_host, sizeof(s_config.ns_host)) == 0) {
    printf("[CFG] Saved to Flash OK (verified).\r\n");
} else {
    printf("[CFG] WARNING: Flash read-back mismatch!\r\n");
}
```

写完后立即从 Flash 地址（内存映射，直接读）验证 magic + checksum + ns_host 三个字段，无需再次擦写，属于零成本检查。

---

## Part 4：链接脚本 Flash 区域保护

### 4.1 问题

`STM32F407ZGTx_FLASH_cmake.ld` 原始配置：

```
FLASH (rx) : ORIGIN = 0x8000000, LENGTH = 1024K
```

STM32F407ZGTx 总 Flash 为 1024KB（Sector 0-11），我们将 Sector 11（`0x080E0000`，128KB）作为配置存储区。如果链接脚本允许代码使用全部 1024KB，当固件增长超过 896KB 时，链接器会把代码/只读数据**覆盖到 Sector 11**，导致 `config_load()` 读出的是固件数据而非配置，产生几乎不可调试的错误。

### 4.2 修复

```
/* STM32F407ZGTx_FLASH_cmake.ld */
/* FLASH limited to 896K (0xE0000) = Sectors 0-10.
 * Sector 11 (0x080E0000, 128K) is reserved for gateway config storage. */
FLASH (rx) : ORIGIN = 0x8000000, LENGTH = 896K
```

修改后，链接器会在固件超过 896KB 时报错，而不是静默覆盖配置区。当前固件大小约 284KB，使用率 31.7%，有充足余量。

### 4.3 与 STM32CubeProgrammer 的协同

`STM32_Programmer_CLI --download xxx.elf --start` 在烧录时只擦除 ELF 文件所覆盖的扇区。烧录日志确认：

```
Erasing internal memory sectors [0 6]
```

仅擦除 Sector 0-6，Sector 11 保持不变。Flash 配置在重新烧录固件后仍然完好。

---

## Part 5：lora_pkt_fwd.c 中的区域选择与频率覆盖

### 5.1 启动时选择 JSON 配置

```c
// lora_pkt_fwd.c — pkt_fwd_main()
const char *conf_array;
{
    freq_region_t _region = (freq_region_t)config_get()->freq_region;
    if (_region == FREQ_REGION_EU868) {
        conf_array = (const char *)global_eu_conf;
    } else if (_region == FREQ_REGION_US915_SB0) {
        conf_array = (const char *)global_us_conf;
    } else {
        // CN470 所有子带 + CUSTOM 均使用 CN470 基础 JSON
        // 信道中心频率在下一步由 Flash 配置覆盖
        conf_array = (const char *)global_cn_conf;
    }
}
```

CN470 各子带共用同一个 JSON（信道偏移量不变，只有 radio 中心频率不同），因此只需选对 JSON 类型，再用 Flash 存储的 `radio0_freq/radio1_freq` 覆盖即可。

### 5.2 radio 中心频率覆盖

```c
// lora_pkt_fwd.c — parse_SX130x_configuration()
rfconf.freq_hz = (unsigned int)json_object_get_number(conf_obj, "freq");

// Apply Flash freq override
const gateway_config_t *_fcfg = config_get();
if (i == 0 && _fcfg->radio0_freq != 0) {
    rfconf.freq_hz = (unsigned int)_fcfg->radio0_freq;
} else if (i == 1 && _fcfg->radio1_freq != 0) {
    rfconf.freq_hz = (unsigned int)_fcfg->radio1_freq;
}
```

`radio0_freq == 0` 时跳过覆盖，保留 JSON 中的默认值，这样 `FREQ_REGION_CUSTOM` 模式下若用户没有设置 radio 频率，仍能降级到 JSON 默认值。

---

## Part 6：UART CLI 频率命令

### 6.1 `config set freq_region <name>`

```
> config set freq_region CN470_10
[CLI] freq_region = 10 (CN470_10)
[CLI]   radio0_freq = 486600000 Hz
[CLI]   radio1_freq = 487400000 Hz
[CLI] ** Run 'config save' then 'reboot' to apply **
```

实现逻辑：
```c
freq_region_t region = gw_preset_find_by_name(val);
if (region == FREQ_REGION_CUSTOM) {
    printf("[CLI] Unknown region '%s'. Use 'region list' to see options.\r\n", val);
    return;
}
const gw_freq_preset_t *p = gw_preset_get(region);
cfg->freq_region  = (uint8_t)region;
cfg->radio0_freq  = p->radio0_freq;  // 同时更新 radio 频率
cfg->radio1_freq  = p->radio1_freq;
```

设置 `freq_region` 的同时自动更新 `radio0_freq/radio1_freq`，避免用户忘记手动更新频率。

### 6.2 `config set radio0_freq / radio1_freq <Hz>`

用于精细调整（不在预设中的频率），自动将 `freq_region` 置为 `CUSTOM`：

```c
cfg->radio0_freq = (uint32_t)atol(val);
cfg->freq_region = (uint8_t)FREQ_REGION_CUSTOM;
```

### 6.3 `region list`

```
Frequency plan presets (CLI name = ChirpStack region):
  Idx   Name        radio0 MHz   radio1 MHz   Description
  0     CN470_0     470.6        471.4        CN470 SB0  (CH0-7, ...)
  ...
  10    CN470_10    486.6        487.4        CN470 SB10 (CH80-87, ...) = ChirpStack cn470_10
  ...
  12    EU868       867.5        868.5        EU868 (867.1-868.5 MHz)
  255   custom      -            -            Manual radio0_freq/radio1_freq
```

---

## Part 7：知识扩展

### 7.1 为什么 CN470 要用 2 个 radio？

SX1302 有两个射频前端（radio_0 和 radio_1），每个可以配置一个中心频率。8 个 IF 通道（IF0-IF7）通过设置相对各自 radio 的频率偏移来覆盖 8 个 LoRa 信道：

```
radio_0 (470.6 MHz)        radio_1 (471.4 MHz)
    │                           │
    ├─ IF0: -300 kHz → 470.3   ├─ IF4: -300 kHz → 471.1
    ├─ IF1: -100 kHz → 470.5   ├─ IF5: -100 kHz → 471.3
    ├─ IF2: +100 kHz → 470.7   ├─ IF6: +100 kHz → 471.5
    └─ IF3: +300 kHz → 470.9   └─ IF7: +300 kHz → 471.7
```

每个 radio 覆盖 4 个信道，合计 8 个，对应 LoRaWAN CN470 一个子带。

### 7.2 ChirpStack Gateway Bridge 的 MQTT topic 命名

ChirpStack Gateway Bridge 使用 `<region>/<prefix>/gateway/<eui>/event/stats` 格式的 MQTT topic，其中 `<region>` 由网关配置中的 region 字段决定。这也是为什么我们的 CLI 名称要与 ChirpStack 的区域名称完全一致：`CN470_10` ↔ `cn470_10`（大小写不敏感匹配）。

如果名称不匹配，Gateway Bridge 会将 stat 发布到错误的 MQTT topic，ChirpStack 主服务器无法订阅到网关状态，网关显示离线。

### 7.3 Flash 配置兼容性策略

| 策略 | 本项目选择 | 说明 |
|------|-----------|------|
| 版本号字段 | 用 CONFIG_MAGIC 区分 | 整个结构版本化，不支持字段级兼容 |
| 向前兼容 | 不支持 | 旧固件无法读取新格式 |
| 向后兼容 | 不支持 | magic 不匹配直接用默认值 |
| 迁移 | 不支持（人工处理）| 版本升级后需重新 `config set` + `config save` |

对于嵌入式网关，这是合理的权衡——固件升级频率低，配置参数少，不值得实现复杂的向后兼容迁移。

### 7.4 可扩展方向

- **DHCP 支持**：`gateway_defaults.h` 中已预留 `GW_DEFAULT_ETH_DNS`，扩展 `eth_dns[4]` 字段和 DHCP 开关只需 magic 升版
- **更多区域**：只需在 `freq_region_t` 枚举和 `gw_freq_presets[]` 表中添加条目，CLI 自动支持
- **配置导出/导入**：CLI 可新增 `config dump`（打印十六进制）和 `config load-hex`（从串口接收十六进制恢复），实现跨设备配置复制
- **配置校验增强**：checksum 可升级为 CRC32，检测能力更强
