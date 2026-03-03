# 芯片与模块数据手册

## 核心芯片

| 芯片 | 角色 | 数据手册 |
|------|------|---------|
| **SX1302** | LoRa 集中器 | [SX1302 Datasheet (Semtech)](https://www.semtech.com/products/wireless-rf/lora-core/sx1302) |
| **SX1250** | 射频收发器 (SX1302 内部) | [SX1250 Datasheet (Semtech)](https://www.semtech.com/products/wireless-rf/lora-connect/sx1250) |
| **STM32F407ZGT6** | 主控 MCU | [STM32F407 Datasheet (ST)](https://www.st.com/en/microcontrollers-microprocessors/stm32f407zg.html) |

## MCU 参考手册

| 文档 | 链接 |
|------|------|
| STM32F407 Reference Manual (RM0090) | [RM0090 (ST)](https://www.st.com/resource/en/reference_manual/dm00031020.pdf) |
| STM32F4 HAL User Manual (UM1725) | [UM1725 (ST)](https://www.st.com/resource/en/user_manual/dm00105879.pdf) |
| Cortex-M4 Programming Manual (PM0214) | [PM0214 (ST)](https://www.st.com/resource/en/programming_manual/dm00046982.pdf) |

## 传感器

| 芯片 | 角色 | 数据手册 |
|------|------|---------|
| **LM75A** | I2C 温度传感器 | [LM75A (NXP)](https://www.nxp.com/docs/en/data-sheet/LM75A.pdf) |
| **SSD1306** | I2C OLED 驱动 | [SSD1306 Datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf) |

## LoRa 模块

| 模块 | 角色 | 文档 |
|------|------|------|
| **E77-400M22S** | LoRaWAN 节点 (测试用) | [E77 User Manual (EBYTE)](https://www.ebyte.com/en/product-view-news.html?id=1952) |
| **E106-470G27P2** | SX1302 网关前端模块 | 供应商附带 PDF (见 `esxp1302_stm32f407/doc/`) |
| **SX1278** | LoRa 收发器 (RadioLib) | [SX1276/77/78 Datasheet](https://www.semtech.com/products/wireless-rf/lora-connect/sx1278) |

## 软件参考

| 项目 | 说明 | 链接 |
|------|------|------|
| lora_gateway (HAL) | Semtech SX1302 HAL 参考实现 | [GitHub: Lora-net/sx1302_hal](https://github.com/Lora-net/sx1302_hal) |
| RadioLib | Arduino LoRa/LoRaWAN 库 | [GitHub: jgromes/RadioLib](https://github.com/jgromes/RadioLib) |
| STM32CubeMX | 代码生成器 | [ST CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html) |
| FreeRTOS | RTOS 内核 | [FreeRTOS.org](https://www.freertos.org/) |
