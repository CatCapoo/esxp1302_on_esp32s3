# 04 — W5500 以太网驱动集成

## 概述

本文记录将 WIZnet W5500 硬件 TCP/IP 芯片通过 SPI2 接入 STM32F407ZGTx 的完整实现过程，
包括驱动移植、引脚适配、网络配置，以及与原 ESP32 WiFi 方案的差异分析。

> **背景**：原 ESP32 网关使用内置 WiFi + lwIP 提供 UDP 功能。F407 版本改用外置 W5500，
> 通过 WIZnet ioLibrary 提供相似的 socket 接口，为 pkt_fwd 移植做铺垫。

---

## 文件结构

```
esxp1302_stm32f407/
├── W5500/
│   ├── Inc/
│   │   ├── w5500.h          W5500 寄存器宏定义、底层 R/W 接口声明
│   │   ├── wizchip_conf.h   WIZCHIP 配置层：类型、回调注册、网络 API 声明
│   │   └── socket.h         Berkeley-like socket API 声明
│   └── Src/
│       ├── w5500.c          底层寄存器 R/W（通过 WIZCHIP 函数指针）
│       ├── wizchip_conf.c   配置层 + STM32 平台实现函数
│       └── socket.c         socket() / sendto() / recvfrom() 实现
└── test/
    └── test_w5500_udp.c     UDP bringup 测试
```

驱动来源：[WIZnet ioLibrary_Driver](https://github.com/Wiznet/ioLibrary_Driver)
（从参考项目 `f407_W5500` 复制后适配）

---

## SPI2 + GPIO 配置（CubeMX）

### SPI2 参数

| 参数 | 值 |
|------|----|
| 模式 | 全双工主机 |
| 数据位 | 8 bit |
| 时钟极性/相位 | CPOL=0, CPHA=0（Mode 0） |
| NSS | Software（手动控制） |
| 预分频 | 8（APB1 42 MHz ÷ 8 = **5.25 MHz**） |
| MISO | PC2 |
| MOSI | PC3 |
| SCK | PB10 |

### SPI2 速率选型说明

W5500 最高支持 80 MHz SPI 时钟。STM32F407 SPI2/SPI3 均挂在 APB1（42 MHz），
可选分频系数如下：

| 预分频 | SPI 时钟 | 适用场景 |
|--------|---------|----------|
| ÷2 | 21 MHz | 高吞吐，需严格 PCB 阻抗控制 |
| ÷4 | 10.5 MHz | 性能与稳定性平衡，建议 PCB 等长走线 |
| **÷8（当前）** | **5.25 MHz** | **保守稳定，PCB 布线无特殊要求** |
| ÷16 | 2.625 MHz | 调试用 |

**当前选择 ÷8（5.25 MHz）的原因：**
- 初版 PCB 未做阻抗控制，走线较短但未等长处理，保守配置可规避信号完整性风险
- W5500 内置硬件 TCP/IP 栈，CPU 通过 SPI 读写寄存器和 socket 缓冲区，5.25 MHz 带宽已足够百兆以太网的实际吞吐需求
- 如后续需要提升吞吐，可在 CubeMX 中将预分频调整至 ÷4，PCB 上为 CLK/MOSI 串联 22–33 Ω 阻尼电阻后可进一步提升至 ÷2

> **注意**：SPI 外设参数（预分频等）统一在 CubeMX 中调整，不直接修改 `spi.c` 生成代码。

### GPIO 引脚

| 引脚 | 方向 | 功能 | 宏定义（main.h） |
|------|------|------|-----------------|
| PA2 | 输出 | W5500 硬件复位（低有效） | `W5500_RES_Pin / W5500_RES_GPIO_Port` |
| PA3 | 输出 | W5500 片选（低有效） | `W5500_NSS_Pin / W5500_NSS_GPIO_Port` |

> SX1302 使用 SPI3（PC10/PC11/PC12），两组 SPI 互不干扰。

---

## 适配改动记录

原参考项目 `f407_W5500` 在 `wizchip_conf.c` 中使用了自定义 `gpio.h` 中的宏
（`W5500_RES_High/Low()`、`W5500_NSS_High/Low()`），依赖特定引脚（PC0=RST, PC1=NSS）。
本项目引脚不同（PA2/PA3），做了以下改动：

### 1. 头文件包含替换

```c
// 原（参考项目，依赖其私有 gpio.h + 宏）
#include "gpio.h"
#include "spi.h"
#include "string.h"
#include "stdio.h"

// 改为（直接包含 STM32 HAL + main.h，避免宏依赖）
#include "main.h"          // 提供 W5500_RES_Pin/Port, W5500_NSS_Pin/Port
#include "spi.h"           // 提供 hspi2 句柄
#include <string.h>
#include <stdio.h>
```

### 2. 硬件复位函数

```c
// 原（依赖 gpio.h 宏）
void W5500_RESET(void) {
    W5500_RES_Low();
    HAL_Delay(50);
    W5500_RES_High();
    HAL_Delay(50);
}

// 改为（直接调用 HAL，引脚来自 main.h）
void W5500_RESET(void) {
    HAL_GPIO_WritePin(W5500_RES_GPIO_Port, W5500_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(W5500_RES_GPIO_Port, W5500_RES_Pin, GPIO_PIN_SET);
    HAL_Delay(50);
}
```

### 3. 片选函数

SPI_CS_Select/Deselect 在原参考代码中已经使用 `HAL_GPIO_WritePin` + 
`W5500_NSS_GPIO_Port/Pin` 宏，无需修改（只要 main.h 中已定义这些宏即可）：

```c
void SPI_CS_Select(void) {
    HAL_GPIO_WritePin(W5500_NSS_GPIO_Port, W5500_NSS_Pin, GPIO_PIN_RESET);
}
void SPI_CS_Deselect(void) {
    HAL_GPIO_WritePin(W5500_NSS_GPIO_Port, W5500_NSS_Pin, GPIO_PIN_SET);
}
```

### 4. 网络配置去除外部依赖

```c
// 原（依赖 extern uint8_t Board_ip[4], Gateway_ip[4]）
extern uint8_t Board_ip[4];
extern uint8_t Gateway_ip[4];
memcpy(gWIZNETINFO.ip, Board_ip, sizeof(Board_ip));
memcpy(gWIZNETINFO.gw, Gateway_ip, sizeof(Gateway_ip));

// 改为（嵌入式静态配置，无外部依赖）
wiz_NetInfo gWIZNETINFO = {
    .mac  = {0x00, 0x83, 0x68, 0x88, 0x56, 0x72},
    .ip   = {192, 168,  10,  15},
    .sn   = {255, 255, 255,   0},
    .gw   = {192, 168,  10,   1},
    .dns  = {  0,   0,   0,   0},
    .dhcp = NETINFO_STATIC
};

void NetworkParameterConfiguration(void) {
    ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO);
    ctlnetwork(CN_GET_NETINFO, (void*)&gWIZNETINFO);
    printf("W5500 IP: %d.%d.%d.%d\r\n", ...);
}
```

### 5. PHY 链路等待增加超时和诊断输出

```c
// 原（无限等待，无任何输出，调试困难）
do {
    if (ctlwizchip(CW_GET_PHYLINK, &tmp) == -1) while(1);
} while (tmp == PHY_LINK_OFF);

// 改为（5秒超时 + 分阶段输出）
printf("[W5500] Waiting for PHY link...\r\n");
uint32_t t0 = HAL_GetTick();
do {
    if (ctlwizchip(CW_GET_PHYLINK, &tmp) == -1) {
        printf("[W5500] ERROR: CW_GET_PHYLINK failed (SPI error?)\r\n");
        while(1);
    }
    if ((HAL_GetTick() - t0) > 5000) {
        printf("[W5500] ERROR: PHY link timeout - check Ethernet cable!\r\n");
        while(1);
    }
} while (tmp == PHY_LINK_OFF);
printf("[W5500] PHY link UP\r\n");
```

---

## WIZnet ioLibrary 架构

```
应用层 (test_w5500_udp.c)
       │  socket() / sendto() / recvfrom()
       ▼
socket.c  ─────────────────────────────────── 高层 socket API
       │  getSn_SR() / getSn_RX_RSR() / ...
       ▼
wizchip_conf.c  ────────────────────────────── 配置 + 回调分发
       │  WIZCHIP.IF.SPI._write_byte() 等函数指针
       ▼
w5500.c  ──────────────────────────────────── W5500 寄存器 R/W
       │  WIZCHIP_WRITE() / WIZCHIP_READ()
       ▼
SPI_WriteByte() / SPI_ReadByte()  ──────────── 平台 SPI 实现
       │
       ▼
HAL_SPI_TransmitReceive(&hspi2, ...)  ─────── STM32 HAL SPI2
```

### 回调注册机制

WIZnet ioLibrary 通过函数指针实现平台无关性：

```c
reg_wizchip_cris_cbfunc(SPI_CrisEnter, SPI_CrisExit);     // 临界区
reg_wizchip_cs_cbfunc(SPI_CS_Select, SPI_CS_Deselect);    // 片选
reg_wizchip_spi_cbfunc(SPI_ReadByte, SPI_WriteByte);       // SPI R/W
```

所有 STM32 平台实现均在 `wizchip_conf.c` 末尾：

| 函数 | 实现 |
|------|------|
| `SPI_CrisEnter()` | `__set_PRIMASK(1)` 关全局中断 |
| `SPI_CrisExit()` | `__set_PRIMASK(0)` 开全局中断 |
| `SPI_CS_Select()` | `HAL_GPIO_WritePin(PA3, RESET)` |
| `SPI_CS_Deselect()` | `HAL_GPIO_WritePin(PA3, SET)` |
| `SPI_WriteByte(u8)` | `HAL_SPI_TransmitReceive(&hspi2, ...)` |
| `SPI_ReadByte()` | `HAL_SPI_TransmitReceive(&hspi2, 0xFF→data)` |

---

## CMakeLists.txt 集成

```cmake
# W5500 驱动源文件
set(W5500_SOURCES
    W5500/Src/w5500.c
    W5500/Src/wizchip_conf.c
    W5500/Src/socket.c
)

target_sources(... PRIVATE ${W5500_SOURCES} ...)

target_include_directories(... PRIVATE
    ...
    W5500/Inc       # ← 新增
)
```

---

## 初始化流程

```
W5500_ChipInit()
    │
    ├─ W5500_RESET()
    │      PA2: HIGH→LOW(50ms)→HIGH(50ms)
    │
    ├─ reg_wizchip_cris_cbfunc(Enter, Exit)
    ├─ reg_wizchip_cs_cbfunc(Select, Deselect)
    ├─ reg_wizchip_spi_cbfunc(Read, Write)
    │
    ├─ ChipParametersConfiguration()
    │      ├─ CW_INIT_WIZCHIP: 8个socket各分配2KB RX/TX buffer
    │      └─ 等待PHY链路建立（超时5秒）
    │
    └─ NetworkParameterConfiguration()
           ├─ CN_SET_NETINFO: 写 IP/MAC/SN/GW 到 W5500 寄存器
           └─ CN_GET_NETINFO: 回读验证、打印
```

---

## 与 ESP32 WiFi 的架构对比

| 对比项 | ESP32 WiFi + lwIP | STM32 W5500 |
|--------|------------------|-------------|
| 协议栈位置 | 软件（lwIP in ESP32） | 硬件（W5500 芯片内） |
| 初始化方式 | 异步事件驱动 | 同步阻塞 |
| socket API | POSIX（`AF_INET`, `SOCK_DGRAM`） | WIZnet ioLibrary |
| `socket()` 参数 | `(AF_INET, SOCK_DGRAM, 0)` | `(sn, Sn_MR_UDP, port, flag)` |
| `sendto()` 参数 | `(fd, buf, len, 0, &addr, addrlen)` | `(sn, buf, len, ip[4], port)` |
| `recvfrom()` 参数 | `(fd, buf, len, 0, &addr, &addrlen)` | `(sn, buf, len, ip[4], &port)` |
| IP 配置 | DHCP / NVS 读取 | 静态结构体 |
| CPU 占用 | 协议栈在独立 LwIP task | 仅 SPI 通信开销 |

---

## pkt_fwd 移植注意事项

esp32 版 pkt_fwd 使用 POSIX socket API，移植到 W5500 时需要一个适配层。

### 需要适配的接口

```c
// ESP32 pkt_fwd 用法：
int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
bind(sock, ...);
sendto(sock, buf, len, 0, (struct sockaddr*)&dest, sizeof(dest));
recvfrom(sock, buf, len, 0, (struct sockaddr*)&src, &srclen);
```

### W5500 对应 API

```c
// W5500 ioLibrary：
int8_t sn = socket(0, Sn_MR_UDP, local_port, 0);  // 0=socket号
sendto(sn, buf, len, dest_ip, dest_port);
recvfrom(sn, buf, len, src_ip, &src_port);
```

### 建议方案

在 `loragw_net.c` 中封装一个薄适配层，对上层 pkt_fwd 呈现类 POSIX 接口，
内部映射到 WIZnet API：

```c
// loragw_net.h（对 pkt_fwd 暴露）
typedef struct { uint8_t sn; uint16_t port; } lgw_socket_t;
int  lgw_net_open_udp(lgw_socket_t *s, uint16_t local_port);
int  lgw_net_sendto(lgw_socket_t *s, const void *buf, int len,
                    const char *ip_str, uint16_t port);
int  lgw_net_recvfrom(lgw_socket_t *s, void *buf, int len,
                      char *src_ip, uint16_t *src_port);
void lgw_net_close(lgw_socket_t *s);
```
