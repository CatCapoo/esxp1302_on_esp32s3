# testing/ — 测试流程文档

| 文档 | 说明 |
|------|------|
| [01_bringup_tests.md](01_bringup_tests.md) | SPI / SX1250 / REG / OLED / LM75A 五项底层外设测试 |
| [02_hal_rx_test.md](02_hal_rx_test.md) | HAL RX 收包测试 — 搭配 E77 LoRaWAN 节点 |
| [03_hal_tx_test.md](03_hal_tx_test.md) | HAL TX 发包测试 — 搭配 SX1278 接收端 |
| [04_scripts_reference.md](04_scripts_reference.md) | Python 测试脚本完整用法手册 |
| [05_w5500_udp_test.md](05_w5500_udp_test.md) | W5500 以太网 + UDP 双向通信测试 |
| [06_flash_config_uart_cli_testing.md](06_flash_config_uart_cli_testing.md) | STM32F407 内部 Flash 配置存储 + UART CLI 环形缓冲（CH340 DTR 复位修复）|

## 测试顺序

按以下顺序逐项验证（每项通过后再进行下一项）：

```
TEST_LORAGW_SPI          ← SPI 链路基本功能
  ↓
TEST_LORAGW_SPI_SX1250   ← SX1250 寄存器 R/W
  ↓
TEST_LORAGW_REG          ← SX1302 寄存器默认值 + R/W
  ↓
TEST_LORAGW_I2C_OLED     ← SSD1306 OLED 显示
  ↓
TEST_LORAGW_I2C_LM75A    ← LM75A 温度读取
  ↓
TEST_LORAGW_HAL_RX       ← 网关收包 (配合 E77 节点)
  ↓
TEST_LORAGW_HAL_TX       ← 网关发包 (配合 SX1278 验证)
  ↓
TEST_W5500_UDP           ← W5500 以太网 + UDP 通信
```

## 测试结果速览

| 测试 | 状态 | 关键指标 |
|------|------|----------|
| SPI | ✅ PASS | 50 轮 4 KB burst R/W 零错误 |
| SX1250 | ✅ PASS | Radio0/1 GET_STATUS = 0x32 |
| REG | ✅ PASS | 默认值 + R/W 全部正确 |
| OLED | ✅ PASS | 填充/文字/弹球动画显示正常 |
| LM75A | ✅ PASS | 0x48 地址检测到, 22.25~23.00°C |
| HAL RX | ✅ PASS | 收到 E77 ABP 包, SNR 2.8/-1.5/-0.5 dB |
| HAL TX | ✅ PASS | SX1278 收到 13 包, RSSI -68 dBm |
| W5500 UDP | ✅ PASS | PHY 链路建立, 5 条消息双向回显正确 |
