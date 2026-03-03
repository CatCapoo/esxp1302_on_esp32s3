# 05 — W5500 以太网 + UDP 通信测试

## 概述

本测试验证 **W5500 硬件 TCP/IP 芯片**通过 SPI2 接入 STM32F407，实现以太网 UDP
双向通信，为后续移植 Packet Forwarder（pkt_fwd）做铺垫。

此前网关使用 ESP32 WiFi（lwIP），现替换为 W5500 有线以太网，通过 WIZnet ioLibrary
驱动提供 Berkeley-like socket API。

源文件: `test/test_w5500_udp.c`  
测试脚本: `scripts/udp_test.py`

---

## 硬件连接

### W5500 模块 ↔ STM32F407ZGTx

| W5500 引脚 | STM32 引脚 | 功能 |
|-----------|-----------|------|
| SCLK | PB10 | SPI2 时钟 |
| MOSI | PC3 | SPI2 MOSI |
| MISO | PC2 | SPI2 MISO |
| CSN | PA3 | 片选（软件 NSS） |
| RST | PA2 | 硬件复位 |
| 3.3V | 3.3V | 电源 |
| GND | GND | 地 |

> W5500 模块自带 LDO，通常也可接 5V 电源（视模块型号）。

### 以太网连接

```
W5500 RJ45  ────────────────────  PC 以太网口
                 直连网线
```

PC 以太网口 IP 需与 W5500 在同一子网：

| 设备 | IP | 子网掩码 | 备注 |
|------|----|---------|------|
| PC 以太网 | 192.168.10.1 | 255.255.255.0 | 静态设置 |
| STM32 W5500 | 192.168.10.15 | 255.255.255.0 | 固件硬编码 |

---

## 网络配置

配置定义在 `W5500/Src/wizchip_conf.c`：

```c
wiz_NetInfo gWIZNETINFO = {
    .mac  = {0x00, 0x83, 0x68, 0x88, 0x56, 0x72},
    .ip   = {192, 168,  10,  15},
    .sn   = {255, 255, 255,   0},
    .gw   = {192, 168,  10,   1},
    .dns  = {  0,   0,   0,   0},
    .dhcp = NETINFO_STATIC
};
```

修改 IP/网关只需改此结构体并重新编译。

---

## 测试程序逻辑（test_w5500_udp.c）

```
W5500_ChipInit()
    ├── W5500_RESET()           复位芯片（PA2 拉低 50ms 再拉高）
    ├── reg_wizchip_cris_cbfunc()  注册临界区回调（__set_PRIMASK）
    ├── reg_wizchip_cs_cbfunc()    注册 CS 回调（PA3 HAL_GPIO_WritePin）
    ├── reg_wizchip_spi_cbfunc()   注册 SPI 字节收发（SPI2 HAL_SPI_TransmitReceive）
    ├── ChipParametersConfiguration()
    │       ├── CW_INIT_WIZCHIP   初始化 8 路 socket 各 2KB buffer
    │       └── CW_GET_PHYLINK    等待网线插入 PHY 链路建立（超时 5 秒报错）
    └── NetworkParameterConfiguration()
            ├── CN_SET_NETINFO    写入 IP/MAC/GW 到 W5500 寄存器
            └── CN_GET_NETINFO    回读验证并打印

打印网络信息（MAC / IP / SN / GW / DHCP模式）

socket(0, Sn_MR_UDP, 5005, 0)    打开 socket 0，UDP 模式，监听 5005 端口

sendto()   发送 hello 包到 PC（192.168.10.1:5005）

while(1):
    getSn_RX_RSR()    轮询是否有数据
    recvfrom()        接收数据 + 来源 IP:Port
    printf()          打印接收内容
    sendto()          回显（加 "Echo #N: " 前缀）
    osDelay(10)       让出 FreeRTOS 时间片
```

---

## 开启测试

### 第一步：选择测试

`CMakeLists.txt` 的编译定义段确保只启用 `TEST_W5500_UDP`：

```cmake
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # TEST_LORAGW_SPI
    # TEST_LORAGW_SPI_SX1250
    # TEST_LORAGW_REG
    # TEST_LORAGW_I2C_OLED
    # TEST_LORAGW_I2C_LM75A
    TEST_W5500_UDP          # ← 只取消注释这一行
)
```

### 第二步：编译

```powershell
cd esxp1302_stm32f407
cmake --preset Debug
cmake --build --preset Debug -j8
```

### 第三步：烧录

```powershell
STM32_Programmer_CLI -c port=SWD -w build/esxp1302_stm32f407.elf -v -rst
```

### 第四步：运行 PC 测试脚本

```powershell
# 在 esxp1302_stm32f407 目录下
python scripts/udp_test.py

# 或手动指定参数
python scripts/udp_test.py --port 5005 --target 192.168.10.15
```

**脚本行为：**
1. 绑定本机 UDP 端口 5005（监听所有网卡）
2. 等待 STM32 上电后发来的 hello 包（超时 5 秒）
3. 发送 5 条编号测试消息
4. 等待并打印每条回显
5. 进入交互模式（持续监听，Ctrl+C 退出）

---

## 预期输出

### 串口（USART1, 115200 baud）

```
========================================
 TEST: W5500 Ethernet + UDP
========================================
[W5500] Initializing...
[W5500] Waiting for PHY link...
[W5500] PHY link UP
W5500 IP: 192.168.10.15
[W5500] Init OK
[W5500] MAC : 00:83:68:88:56:72
[W5500] IP  : 192.168.10.15
[W5500] SN  : 255.255.255.0
[W5500] GW  : 192.168.10.1
[W5500] DHCP: Static
[W5500] Opening UDP socket 0 on port 5005
[W5500] Sent hello (27 bytes) to 192.168.10.1:5005
[W5500] Entering echo loop...
[W5500] #1 Recv 23 bytes from 192.168.10.1:5005 => Test message #1 from PC
[W5500] #2 Recv 23 bytes from 192.168.10.1:5005 => Test message #2 from PC
[W5500] #3 Recv 23 bytes from 192.168.10.1:5005 => Test message #3 from PC
[W5500] #4 Recv 23 bytes from 192.168.10.1:5005 => Test message #4 from PC
[W5500] #5 Recv 23 bytes from 192.168.10.1:5005 => Test message #5 from PC
```

### PC 脚本输出

```
[UDP Test] Listening on 0.0.0.0:5005
[UDP Test] Target: 192.168.10.15:5005

[UDP Test] Waiting for STM32 hello packet...
[UDP Test] Got from ('192.168.10.15', 5005): Hello from STM32F407 W5500!

[UDP Test] Sending: Test message #1 from PC
[UDP Test] Reply from ('192.168.10.15', 5005): Echo #1: Test message #1 from PC
[UDP Test] Sending: Test message #2 from PC
[UDP Test] Reply from ('192.168.10.15', 5005): Echo #2: Test message #2 from PC
[UDP Test] Sending: Test message #3 from PC
[UDP Test] Reply from ('192.168.10.15', 5005): Echo #3: Test message #3 from PC
[UDP Test] Sending: Test message #4 from PC
[UDP Test] Reply from ('192.168.10.15', 5005): Echo #4: Test message #4 from PC
[UDP Test] Sending: Test message #5 from PC
[UDP Test] Reply from ('192.168.10.15', 5005): Echo #5: Test message #5 from PC

[UDP Test] Done. Sent 5 messages.
[UDP Test] Entering interactive mode (Ctrl+C to exit)...
```

---

## 故障排查

### 串口无任何输出

- **新固件没有烧录**：检查 STM32_Programmer_CLI 是否返回 `Download verified successfully`
- **ST-Link 连接错误**：确认 `ST-LINK error (DEV_CONNECT_ERR)` 不是因为 gdb-server 占用 ST-Link；需要先停止 gdb-server

### 串口仍打印旧测试内容

- CMake 缓存未更新：彻底清理重编译
  ```powershell
  Remove-Item -Recurse -Force build
  cmake --preset Debug
  cmake --build --preset Debug -j8
  ```

### `[W5500] Waiting for PHY link...` 后停在这里

- **网线未连接**：插好直连网线后复位板子
- **5 秒超时**：之后会打印 `ERROR: PHY link timeout - check Ethernet cable!`

### `[W5500] ERROR: CW_INIT_WIZCHIP failed`

- SPI2 通信失败，检查：
  - W5500 接线（PB10/PC2/PC3/PA3/PA2）
  - W5500 供电（3.3V）
  - SPI2 时钟极性/相位（Mode 0）

### PC 脚本 `No hello received (timeout)`

1. 确认 PC 以太网 IP 设置为 `192.168.10.1/24`（静态，非 DHCP）
2. 检查 Windows 防火墙是否放行 UDP 5005 入站：
   ```powershell
   # 以管理员身份运行
   netsh advfirewall firewall add rule name="UDP 5005 W5500" protocol=UDP dir=in localport=5005 action=allow
   ```
3. 脚本需要先于 STM32 复位运行，否则错过 hello 包

### 路径错误

脚本必须在 `esxp1302_stm32f407` 目录下运行：
```powershell
cd esxp1302_stm32f407
python scripts/udp_test.py   # ✅ 正确
```

---

## 测试结果

| 项目 | 状态 | 详情 |
|------|------|------|
| W5500 SPI 初始化 | ✅ PASS | CW_INIT_WIZCHIP 返回 0 |
| PHY 链路建立 | ✅ PASS | 检测到以太网直连网线 |
| 网络配置写入 | ✅ PASS | IP/MAC/GW 回读正确 |
| UDP socket 创建 | ✅ PASS | socket(0, Sn_MR_UDP, 5005, 0) = 0 |
| STM32 → PC hello | ✅ PASS | 27 字节，PC 收到 |
| PC → STM32 消息1~5 | ✅ PASS | 23 字节，全部收到并正确回显 |

**通过日期：** 2026-03-03

---

## 与 ESP32 WiFi 的对比

| 特性 | ESP32 WiFi | STM32 W5500 |
|------|-----------|------------|
| 协议栈 | lwIP（软件） | 硬件 TCP/IP |
| 初始化 | 事件驱动（异步） | 同步阻塞 |
| IP 配置 | DHCP / NVS 读取 | 静态（结构体） |
| Socket API | POSIX（AF_INET） | WIZnet ioLibrary |
| 连接方式 | 无线 | 有线以太网 |
| 延迟 | ~1ms（局域网） | <1ms（直连） |
| 稳定性 | 受干扰影响 | 有线更稳定 |
