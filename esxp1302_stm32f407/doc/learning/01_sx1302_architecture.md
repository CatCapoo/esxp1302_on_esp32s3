# 01 — SX1302 内部架构与驱动层次

## SX1302 芯片架构

SX1302 是 Semtech 的第二代 LoRa 网关集中器芯片，相比 SX1301 的主要改进:

- 更低功耗 (~40% 降低)
- 支持 Fine Timestamp
- 内置时钟管理 (不需要外部 MCU 维护精确时钟)
- 支持全双工

### 内部模块

```
┌─────────────────────────────────────────────┐
│                  SX1302                      │
│ ┌──────────┐  ┌──────────┐  ┌────────────┐ │
│ │ Radio A  │  │ Radio B  │  │ AGC+ARB    │ │
│ │ (SX1250) │  │ (SX1250) │  │ MCU core   │ │
│ └────┬─────┘  └────┬─────┘  └──────┬─────┘ │
│      │              │               │        │
│      ▼              ▼               ▼        │
│ ┌──────────────────────────────────────────┐ │
│ │       Digital Front-End (DFE)            │ │
│ │  8× multi-SF + 1× single-SF + 1× FSK   │ │
│ └──────────────────────────────────────────┘ │
│ ┌──────────────────────────────────────────┐ │
│ │         TX path (via Radio A or B)       │ │
│ └──────────────────────────────────────────┘ │
│ ┌──────────────────────────────────────────┐ │
│ │         SPI slave interface              │ │
│ └──────────────────────────────────────────┘ │
└─────────────────────────────────────────────┘
```

### SX1250 射频前端

SX1302 内部集成 2 个 SX1250 射频收发器:
- **Radio A (radio_0)**: 覆盖一半 IF 信道，可用于 TX
- **Radio B (radio_1)**: 覆盖另一半 IF 信道
- 通过 SX1302 的 **SPI bridge** 间接访问（HOST → SX1302 SPI → SX1250 internal SPI）
- 每个 SX1250 有自己的 PLL，中心频率独立设置

### AGC + ARB 固件

SX1302 内部有一个 MCU 核心运行 AGC (自动增益控制) 和 ARB (仲裁) 固件:
- 固件以 `.var` 文件形式存储在主机端
- 启动时通过 SPI 加载到 SX1302 内部 RAM
- 在本项目中: `agc_fw_sx1250.var` 和 `arb_fw.var`

---

## 驱动层次结构

```
┌─────────────────────────────────────┐
│           Application               │
│    (test_loragw_hal_rx/tx.c)        │
├─────────────────────────────────────┤
│          HAL Layer                   │
│    (loragw_hal.c)                   │
│  lgw_start / lgw_stop               │
│  lgw_receive / lgw_send             │
├─────────────────────────────────────┤
│       Mid-Level Drivers             │
│  loragw_sx1302.c   - 寄存器操作      │
│  loragw_sx1250.c   - SX1250 控制     │
│  loragw_cal.c      - 校准            │
│  loragw_stts751.c  - 温度传感器      │
│  loragw_sx1302_rx.c- RX buffer      │
│  loragw_sx1302_timestamp.c          │
├─────────────────────────────────────┤
│      Low-Level Platform Layer       │
│  loragw_spi.c      - SPI 通信       │
│  loragw_i2c.c      - I2C 通信       │
│  loragw_gpio.c     - RESET GPIO     │
│  loragw_aux.c      - 延时/时间戳     │
│  loragw_reg.c      - 寄存器抽象      │
├─────────────────────────────────────┤
│      STM32 HAL / FreeRTOS           │
│  HAL_SPI / HAL_I2C / HAL_GPIO      │
│  HAL_TIM / osDelay / HAL_GetTick   │
└─────────────────────────────────────┘
```

### 关键数据流

**RX 路径**:
```
RF → SX1250 → SX1302 DFE → RX Buffer → SPI Read → lgw_receive()
```

**TX 路径**:
```
lgw_send() → SPI Write → SX1302 TX Buffer → SX1250 → RF
```

---

## 启动序列 (`lgw_start()`)

1. SX1302 SPI 连接检查
2. 加载 AGC 固件 (`agc_fw_sx1250.var`)
3. 加载 ARB 固件 (`arb_fw.var`)
4. 配置 SX1250 Radio A/B (PLL 频率)
5. 配置 DFE (IF 信道、datarate)
6. 校准 (`lgw_cal()`) — TX/RX 增益校准
7. 温度传感器初始化
8. 启动 AGC + ARB MCU
9. 使能 RX

---

## 寄存器访问模型

SX1302 使用分页寄存器模式:

```c
/* 直接寄存器: 地址 < 0x100 */
lgw_reg_w(SX1302_REG_COMMON_CTRL0, value);

/* 分页寄存器: 需要先设置页面 */
lgw_reg_w(SX1302_REG_AGC_MCU_CTRL, page);
lgw_reg_w(SX1302_REG_AGC_MCU_DATA, value);
```

寄存器定义在 `loragw_reg.h` 中通过宏生成，包含:
- 页地址
- 寄存器偏移
- 位域 mask + shift
- 是否只读

---

## 固件文件说明

| 文件 | 大小 | 用途 |
|------|------|------|
| `agc_fw_sx1250.var` | ~8 KB | SX1250 模式专用的 AGC 固件 |
| `arb_fw.var` | ~8 KB | 信道仲裁固件 |
| `cal_fw.var` | ~2 KB | 校准固件 (仅校准阶段使用) |

这些文件在编译时通过 `xxd -i` 或直接包含为 C 数组，链接进最终固件。
