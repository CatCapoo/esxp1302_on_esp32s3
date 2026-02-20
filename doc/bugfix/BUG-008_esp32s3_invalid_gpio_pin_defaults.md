# BUG-008：ESP32-S3 无效 GPIO 引脚默认值导致运行时错误

- **日期**：2026-02-20  
- **文件**：`main/libloragw/loragw_i2c.h`、`main/packet_forwarder/lora_pkt_fwd.c`、`main/board_config.h`（新建）  
- **严重级别**：运行时致命（I2C / GPIO 初始化失败）

---

## 现象

固件烧录后串口输出多个运行时错误：

```
E (xxx) i2c: i2c_set_pin(xxx): invalid GPIO number
E (xxx) gpio: gpio_set_direction(308): GPIO number error
E (xxx) gpio: gpio_pullup_en(78): GPIO number error
```

设备随后无法启动 packet forwarder。

---

## 根本原因

代码中各模块的 GPIO 引脚默认值沿用自原始 Linux / ESP32（Classic）移植版本：

| 模块 | 默认引脚 | ESP32-S3 存在？ |
|------|----------|----------------|
| I2C SCL | GPIO 22 | ❌ 不存在（ESP32-S3 GPIO 范围 0–21 + 26–48，**不含 22–25**） |
| I2C SDA | GPIO 21 | ✅（恰好有效，但引脚被 SPI Flash 复用，不宜使用） |
| USER_BUTTON_1 | GPIO 23 | ❌ 不存在 |
| USER_BUTTON_2 | GPIO 25 | ❌ 不存在 |

`gpio_set_direction(308)` 中的 308 是 `GPIO_NUM_23`（枚举值 = 23，但框架内部检验时数值不匹配），`gpio_pullup_en(78)` 同理。

---

## 修复

### 1. 新增 `main/board_config.h`

将全部硬件引脚定义集中到一个文件，方便针对不同板卡修改：

```c
#define I2C_MASTER_SDA_IO   4
#define I2C_MASTER_SCL_IO   5
#define SX1302_RESET_PIN    2
#define BLINK_GPIO          1
#define LED_GREEN_GPIO      7
#define USER_BUTTON_1       0    /* IO0: boot/config button */
#define USER_BUTTON_2       6    /* IO6: reserved (GPIO_NUM_NC 等效) */
```

所有相关模块头文件（`loragw_i2c.h`、`loragw_gpio.h`、`loragw_spi.h`、`loragw_gps.h`、`led_indication.h`）及 `lora_pkt_fwd.c` 均 `#include "board_config.h"`，不再各自硬编码引脚号。

### 2. GPIO_NUM_NC 保护

对于设计上不连接的按键引脚，在初始化前加 NC 判断：

```c
if (USER_BUTTON_1 != GPIO_NUM_NC) {
    gpio_set_direction(USER_BUTTON_1, GPIO_MODE_INPUT);
    gpio_pullup_en(USER_BUTTON_1);
}
```

---

## 验证

修复后固件启动无 GPIO 相关错误，I2C OLED 初始化通过，SX1302 正常复位并启动。
