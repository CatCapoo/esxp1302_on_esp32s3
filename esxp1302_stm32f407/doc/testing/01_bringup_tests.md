# 01 — 底层外设 Bringup 测试

## 概述

移植 SX1302 驱动的第一步是验证底层外设通信链路。以下 5 项测试按序执行，每项通过后再进行下一项。

---

## 测试切换方法

在 `CMakeLists.txt` 的 `target_compile_definitions` 中取消注释需要的测试宏，注释掉其他的：

```cmake
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    TEST_LORAGW_SPI             # ← 取消注释要运行的测试
    # TEST_LORAGW_SPI_SX1250
    # TEST_LORAGW_REG
    # TEST_LORAGW_I2C_OLED
    # TEST_LORAGW_I2C_LM75A
)
```

然后重新编译烧录：
```powershell
cmake --build --preset Debug -j8
```

---

## 测试 1: TEST_LORAGW_SPI — SPI 总线压力测试

### 目的
验证 SPI3 到 SX1302 的基本读写链路。

### 测试内容
- 50 轮循环
- 每轮写入 4 KB 随机数据到 SX1302 internal FIFO
- 读回并逐字节比较
- 任何一个字节不匹配则报错

### 期望结果
```
SPI link opened (STM32 HAL)
cycle  0: write 4096 bytes, read back OK
cycle  1: write 4096 bytes, read back OK
...
cycle 49: write 4096 bytes, read back OK
All 50 cycles PASSED
```

### 常见问题

| 现象 | 原因 | 修复 |
|------|------|------|
| 全部读回 0xFF | NSS 引脚配置错误或未拉低 | 检查 `board_config.h` NSS 引脚映射 |
| 全部读回 0x00 | MISO 线未连接 | 检查硬件接线 |
| 随机字节错误 | SPI 时钟太快 | 增大分频系数 |
| 超时 | SX1302 未上电或 RESET 未正确释放 | 检查电源和 RESET GPIO |

---

## 测试 2: TEST_LORAGW_SPI_SX1250 — SX1250 寄存器读写

### 目的
验证通过 SX1302 的 SPI bridge 访问 SX1250 射频芯片。

### 测试内容
- 发送 `GET_STATUS` (0xC0) 到 Radio0 和 Radio1
- 读取状态字节
- 20 轮寄存器 R/W 循环

### 期望结果
```
Radio 0 GET_STATUS: 0x32
Radio 1 GET_STATUS: 0x32
R/W cycle 0 ... OK
...
R/W cycle 19 ... OK
```

### 关键说明

`GET_STATUS` 的 opcode = 0xC0 (≥ 0x80)。如果原始代码中有 `& 0x7F` 掩码，会导致命令错误。
- 正确返回: `0x32` = STDBY_RC, command OK
- 错误返回: `0x00` 或 `0xA2` = opcode 被截断

**这是一个关键 bug**，修复方法见 [impl/02_platform_adapt.md §4](../impl/02_platform_adapt.md#4-radio_spi_rbwb-opcode-掩码-bug-修复)。

---

## 测试 3: TEST_LORAGW_REG — SX1302 寄存器测试

### 目的
验证 SX1302 寄存器系统的正确性。

### 测试内容

**TEST#1 — 默认值验证**：
- 复位 SX1302
- 读取所有已知寄存器
- 检查默认值是否与 spec 一致

**TEST#2 — R/W 功能测试**：
- 对可写寄存器写入测试值
- 读回验证
- 恢复原值

### 期望结果
```
===== SX1302 REG Test =====

TEST#1: read all registers and check default values ...
TEST#1: PASSED

TEST#2: read-modify-write test on selected registers ...
TEST#2: PASSED
```

### 常见问题

| 现象 | 原因 | 修复 |
|------|------|------|
| TEST#1 大量 FAIL | SX1302 未正确复位 | 检查 `lgw_reset()` 时序 |
| 部分寄存器 version FAIL | 版本寄存器读取时 SX1302 未就绪 | 增加连接轮询等待 (200ms) |

---

## 测试 4: TEST_LORAGW_I2C_OLED — SSD1306 OLED 显示

### 目的
验证 I2C2 总线和 OLED 驱动。

### 测试内容
1. 全屏填充白色 → 等待 1s → 清屏
2. 在 8 行文本区域显示 ASCII 字符串
3. 100 帧弹球动画（小球在屏幕边界弹跳）

### 期望结果
- 肉眼可见 OLED 亮起、填充、显示文字
- 弹球动画流畅运行 100 帧

### 常见问题

| 现象 | 原因 | 修复 |
|------|------|------|
| 屏幕不亮 | I2C 地址错误（部分 SSD1306 是 0x3D） | 修改 `OLED_I2C_ADDR` |
| 显示花屏 | 初始化序列不完整 | 对照 SSD1306 datasheet 检查 |
| 无反应 | I2C2 引脚 SDA/SCL 接线错误 | 检查 PF0/PF1 |

---

## 测试 5: TEST_LORAGW_I2C_LM75A — LM75A 温度传感器

### 目的
验证 I2C 温度传感器读取。

### 测试内容
1. 自动扫描 0x48–0x4F 查找 LM75A
2. 配置为连续转换模式
3. 20 次温度读取，每次间隔 500ms

### 期望结果
```
LM75A scan: found at 0x48
Read  0: 22.625 C
Read  1: 22.750 C
...
Read 19: 23.000 C
TEST PASSED
```

### 常见问题

| 现象 | 原因 | 修复 |
|------|------|------|
| 扫描不到设备 | A0/A1/A2 引脚电平不同 | 改变扫描地址范围 |
| 温度值异常 (+256°C 等) | 字节序解析错误 | LM75A 是 MSB-first, 11-bit 有符号 |
| 温度值为 0 | I2C 通信失败 | 检查上拉电阻和引脚配置 |
