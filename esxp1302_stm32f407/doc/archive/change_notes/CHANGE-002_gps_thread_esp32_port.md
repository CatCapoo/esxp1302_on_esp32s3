# CHANGE-002：完成 GPS 线程 ESP32 移植，实现时间同步与坐标上报

- **日期**：2026-02-28  
- **分支**：`bringup/gps`  
- **涉及文件**：`main/packet_forwarder/lora_pkt_fwd.c`、`main/board_config.h`

---

## 背景

原工程从 Linux 参考实现（lora_pkt_fwd）移植而来，GPS 线程相关代码整体存在于源文件中，  
但从未在 ESP32 上运行过，属于 TODO 状态的死代码。具体表现为：

- `thread_gps` / `thread_valid` 被 `#if 0` 包裹，从未启动
- 主循环中有一段临时调试代码直接读取 GPS UART，仅打印不解析，占用了串口数据
- `thread_gps` 内部使用 POSIX `read()` 系统调用，无法在 ESP-IDF 上操作 UART
- 时间同步仅依赖 u-blox UBX 私有协议帧，而硬件 ATGM336H 仅输出标准 NMEA

此次变更将以上四个问题全部修复，完成 GPS 功能的 ESP32 移植。

---

## 问题分析

### 问题一：GPS 线程从未启动

线程启动代码沿用了 Linux `pthread_create`，并被 `#if 0` 包裹：

```c
#if 0
    pthread_create(&thrid_gps, NULL, (void * (*)(void *))thread_gps, NULL);
    pthread_create(&thrid_valid, NULL, (void * (*)(void *))thread_valid, NULL);
#endif
```

没有 `thread_gps`，NMEA 数据永远不会被解析，坐标和时间变量永远不会被更新。

### 问题二：主循环抢占 UART 数据

主循环的统计周期内有一段临时调试代码，每次统计间隔都会调用 `uart_read_bytes`  
将 GPS 缓冲区中的数据全部读走，仅做打印，完全不解析：

```c
// 主循环每 5 秒
ESP_ERROR_CHECK(uart_get_buffered_data_len(gps_tty_fd, &length));
length = uart_read_bytes(gps_tty_fd, data, min, 100);
// → 数据全被吃掉，thread_gps 拿不到任何字节
```

这也解释了为什么日志中可以看到正常的 NMEA 输出（有卫星、有定位），  
但程序却报告坐标不可用——数据被主循环消耗，`thread_gps` 从未运行更无从解析。

同时这段代码每次读 900+ 字节的整块数据，多条 NMEA 语句混合打印，  
造成日志中出现帧错位的乱码现象：

```
$GPGSV,2,1,07,01,73,05ZDA,052329.000   ← 两条语句粘连
$GPGSV,2,1,07,01,73,=SOC_BootLoader   ← 完全乱码
```

### 问题三：`thread_gps` 使用了错误的 UART API

`thread_gps` 内部使用 POSIX `read()` 系统调用：

```c
ssize_t nb_char = read(gps_tty_fd, serial_buff + wr_idx, LGW_GPS_MIN_MSG_SIZE);
```

但 `gps_tty_fd` 实际上是 ESP-IDF 的 `uart_port_t`（整数端口号），  
不是 Linux 文件描述符，`read()` 在 ESP-IDF 环境中无法操作 UART。

### 问题四：时间同步仅依赖 UBX 私有协议

`gps_process_sync()` 只在收到 `UBX_NAV_TIMEGPS` 消息时触发：

```c
} else if (latest_msg == UBX_NAV_TIMEGPS) {
    gps_process_sync();  // ← ATGM336H 从不发出此帧
}
// NMEA_RMC 分支只调用 gps_process_coords()，不同步时间
```

ATGM336H 是国产模块，默认只输出标准 NMEA 协议（GPS+北斗双模），  
不输出 u-blox UBX 私有协议，因此 `UBX_NAV_TIMEGPS` 永远不会出现。  
实际上 `$GNRMC` 语句本身就包含完整的日期时间，`lgw_parse_nmea()` 解析后  
已将时间写入内部变量（`gps_time_ok = true`），只差调用 `gps_process_sync()` 触发同步。

---

## 修改内容

### 1. 用 FreeRTOS 任务替换 pthread，正式启动 GPS 线程

```c
// 旧：#if 0 包裹的 pthread 代码
// 新：
#if GPS_ENABLE
    if (gps_enabled == true) {
        xTaskCreatePinnedToCore((TaskFunction_t)thread_gps,  "thread_gps",
                                4096*2, NULL, 6, NULL, tskNO_AFFINITY);
        xTaskCreatePinnedToCore((TaskFunction_t)thread_valid, "thread_valid",
                                4096*2, NULL, 6, NULL, tskNO_AFFINITY);
    }
#endif
```

### 2. 禁用主循环中的 GPS UART 读取

用 `#if 0` 禁用主循环里的调试读取块，让 `thread_gps` 独占串口：

```c
#if 0  /* Disabled: thread_gps now handles UART reading and NMEA parsing */
    if (gps_enabled) {
        uart_read_bytes(...);   // 这段代码不再执行
        ...
    }
#endif
```

### 3. 用 `uart_read_bytes()` 替换 `read()`

```c
// 旧
ssize_t nb_char = read(gps_tty_fd, serial_buff + wr_idx, LGW_GPS_MIN_MSG_SIZE);

// 新
int nb_char = uart_read_bytes(gps_tty_fd, (uint8_t *)(serial_buff + wr_idx),
                              LGW_GPS_MIN_MSG_SIZE, pdMS_TO_TICKS(1000));
```

`uart_read_bytes` 带超时参数，避免死等，也符合 FreeRTOS 任务调度习惯。

### 4. 在 NMEA_RMC 分支触发时间同步

```c
} else if (latest_msg == NMEA_RMC) {
    gps_process_coords();
    gps_process_sync();  // ← 新增：ATGM336H 无 UBX，从 RMC 同步时间
}
```

`lgw_parse_nmea()` 解析 `$GNRMC` 后已更新 `gps_time_ok` 和时间变量，  
此时调用 `gps_process_sync()` 可以将 GPS 时间与 SX1302 硬件计数器做关联。

### 5. 将 GPS 编译开关和日志等级移入 `board_config.h`

```c
// board_config.h 新增
#define GPS_ENABLE       1   // 0 = 禁用所有 GPS 代码
#define GPS_LOG_VERBOSE  0   // 0=静默  1=关键事件  2=完整 NMEA 转储
```

原 `lora_pkt_fwd.c` 中的 `#define GPS_LOG_VERBOSE 1` 移除，  
GPS 初始化、线程启动、函数定义均用 `#if GPS_ENABLE` 包裹。

### 6. 改善 GPS 日志

| 日志等级 | 输出内容 |
|----------|----------|
| `GPS_LOG_VERBOSE 0` | 无 GPS 调试输出，`could not get GPS time` 静默 |
| `GPS_LOG_VERBOSE 1` | 每次同步成功打印 UTC 时间；每 5 秒打印 fix 状态+坐标 |
| `GPS_LOG_VERBOSE 2` | （预留）完整 NMEA 帧转储 |

---

## 验证结果

| 验证项 | 结果 |
|--------|------|
| 线程启动 | ✅ `thread_gps spawned` / `thread_valid spawned` |
| GPS 坐标 | ✅ `GPS coordinates: latitude 31.31109, longitude 121.37719, altitude 45 m` |
| 时间参考有效 | ✅ `Valid time reference (age: 0 sec)` |
| JSON 上报含坐标 | ✅ `"lati":31.31109,"long":121.37719,"alti":45` |
| 编译无错误 | ✅ `Project build complete` |

---

## 硬件接线备注

| 信号 | GPIO |
|------|------|
| GPS TX（模块发） | GPIO 20（ESP32 RX） |
| GPS RX（模块收） | GPIO 19（ESP32 TX） |
| GPS VCC | 3.3 V |
| GPS GND | GND |

- 波特率：9600（ATGM336H 默认）
- 输出语句：`$GNGGA`、`$GNRMC`、`$GNZDA`、`$GPGSV`、`$BDGSV` 等（GPS+北斗双模）

---

## 相关文档

- 学习笔记：[gps_nmea_and_lorawan_sync.md](../learning/gps_nmea_and_lorawan_sync.md)
- 对应 commit：`68eb6a4 fix: complete ESP32 port of GPS sync thread (ATGM336H, NMEA-only)`
