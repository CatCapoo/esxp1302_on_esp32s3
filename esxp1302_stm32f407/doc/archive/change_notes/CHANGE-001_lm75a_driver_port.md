# CHANGE-001：移植 NXP LM75A 温度传感器驱动（替换 ST STTS751）

- **日期**：2026-02-27  
- **分支**：`bringup/temp`  
- **涉及文件**：`main/libloragw/loragw_lm75a.c/.h`、`loragw_i2c.c/.h`、`loragw_hal.c`、`main/CMakeLists.txt`、`test/test_loragw_i2c.c`

---

## 背景

原工程使用 ST STTS751 作为 SX1302 HAL 层的温度传感器，用于：
1. **RSSI 温度补偿**：每次 `lgw_receive()` 后调用 `lgw_get_temperature()`，按 cn490 系数多项式修正 RSSI
2. **状态上报**：packet forwarder 每 30 秒在统计 JSON 中上报 `"temp"` 字段

硬件板上实际焊接的是 NXP LM75A，与 STTS751 引脚/寄存器均不兼容，需要重新移植驱动。

---

## LM75A 与 STTS751 差异对比

| 项目 | STTS751 | LM75A |
|------|---------|-------|
| 分辨率 | 12-bit，0.0625°C | 11-bit，0.125°C |
| 温度寄存器地址 | MSB=0x00，LSB=0x02（分两次读） | 0x00（一次读 2 字节） |
| 配置寄存器 | 0x03 | 0x01 |
| 转换速率寄存器 | 0x04（需单独设置） | 无（连续转换，固定） |
| 产品/厂商 ID 寄存器 | 0xFD/0xFE/0xFF | 无 |
| I2C 地址范围 | 0x48~0x4F（3 位地址引脚） | 0x48~0x4F（3 位地址引脚） |

---

## 实现内容

### 1. 新增 `loragw_lm75a.h` / `loragw_lm75a.c`

- **`lm75a_configure(uint8_t i2c_addr)`**  
  读配置寄存器（0x01）确认设备在线，写 `0x00` 设为正常工作模式（非 shutdown、OS 比较器模式、低电平有效）

- **`lm75a_get_temperature(uint8_t i2c_addr, float *temperature)`**  
  调用 `i2c_esp32_read_word()` 一次读取 2 字节，拼装为 `int16_t` 后除以 256.0 得到 °C  
  公式：`raw = (buf[0]<<8 | buf[1])`（有符号），`temp = raw / 256.0f`

- **地址扫描数组**（`loragw_lm75a.h` 中定义）：  
  `{0x48, 0x49, 0x4A, 0x4B}`，0x48 排第一（避免 NACK 污染后续地址扫描的总线状态）

### 2. 新增 `i2c_esp32_read_word()`（`loragw_i2c.c/.h`）

STTS751 两个温度字节在不同寄存器地址，可用两次 `i2c_esp32_read()` 读取。  
LM75A 的 16-bit 温度寄存器需要在同一 I2C 事务内连续读出 MSB + LSB，否则两次读之间传感器可能更新数据导致字节不一致。

```c
// 单事务读 2 字节：先写寄存器地址，重新 START 后连读 MSB(ACK)+LSB(NACK)
esp_err_t i2c_esp32_read_word(uint8_t device_addr, uint8_t reg_addr, uint8_t *data);
```

### 3. 更新 `loragw_hal.c`

```c
// 旧
#include "loragw_stts751.h"
err = stts751_configure(ts_addr);
err = stts751_get_temperature(ts_addr, temperature);

// 新
#include "loragw_lm75a.h"
err = lm75a_configure(ts_addr);
err = lm75a_get_temperature(ts_addr, temperature);
```

### 4. 更新 `main/CMakeLists.txt`

```cmake
# 旧
"libloragw/loragw_stts751.c"
# 新
"libloragw/loragw_lm75a.c"
```

注：`loragw_stts751.c/.h` 文件保留在文件系统，不参与编译，仅作历史参考。

### 5. 重写 `test/test_loragw_i2c.c`

移除 STTS751 的 Product ID / Manufacturer ID 校验逻辑，改为：
- 读 LM75A 配置寄存器（0x01）验证设备在线
- 循环 100 次读取温度，每次调用 `i2c_esp32_read_word()`，打印原始字节和换算后温度

---

## 调试过程中遇到的问题

### 问题 1：地址引脚悬空

**现象**：测试程序对 0x48 能读到数据，主程序扫描失败  
**原因**：地址引脚 A0/A1/A2 悬空，实际地址不确定；后确认 A0=A1=A2=GND → 地址 0x48  
**解决**：将地址引脚确实接地，同时将扫描数组首位改为 0x48

### 问题 2：NACK 污染总线

**现象**：主程序扫描时，如果先扫描到无效地址产生 NACK，后续对正确地址的访问也失败  
**原因**：ESP-IDF legacy I2C driver 在 NACK 后未完整释放总线（SDA 被从设备拉低），导致下一次事务启动失败  
**解决**：将已知存在的 0x48 放在扫描数组第一位，避免先扫到无效地址

### 问题 3：`pkt_fwd` 任务栈溢出

**现象**：主程序运行一段时间后崩溃，日志出现 stack overflow  
**原因**：任务栈设为 `1*4096`（4 KB），HAL + I2C + JSON 组帧操作超出  
**解决**：改为 `4*4096`（16 KB）

### 问题 4：`CHECK_NULL` 宏使用了错误的返回值常量

**现象**：编译失败，`LGW_REG_ERROR` 未声明  
**原因**：从 `loragw_stts751.c` 复制宏定义时未替换返回值常量  
**解决**：改为 `LGW_I2C_ERROR`

---

## 验证结果

| 验证项 | 结果 |
|--------|------|
| 测试程序 100 次读温 | ✅ 22~23.75°C，分辨率 0.125°C |
| 主程序找到传感器 | ✅ `INFO: found temperature sensor on port 0x48` |
| 统计 JSON 温度字段 | ✅ `"temp":22.5` |
| RSSI 温度补偿生效 | ✅ `RSSI temperature offset applied: 0.918 dB (current temperature 22.9 C)` |
| 编译无错误/警告 | ✅ |

---

## 硬件接线备注

| 信号 | GPIO |
|------|------|
| SDA | GPIO 4 |
| SCL | GPIO 5 |
| VCC | 3.3 V |
| GND | GND |
| A0/A1/A2 | 全部接 GND → 地址 0x48 |

**注意事项**：
- LM75A VCC 引脚需并联 100nF 陶瓷去耦电容，否则可能引起 I2C 总线噪声，进而导致 SX1302 收包 CRC 失败率上升
- I2C 连线应尽量短（<10cm），远离天线和 SX1302 RF 走线
