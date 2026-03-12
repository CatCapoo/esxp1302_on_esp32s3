# Bug 汇总与修复记录

本文档汇总移植过程中遇到的所有 Bug，按发现时间顺序编号。

---

### B01 — `_write` / `_read` 等 syscall 缺失

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | 链接报错 `undefined reference to '_write'`, `_read`, `_close` 等 |
| 原因 | newlib 需要底层 I/O 桩函数，ESP32 对应函数不存在于 STM32 环境 |
| 修复 | 启用 `syscalls.c` 或在 CMakeLists.txt 中添加 `--specs=nosys.specs` |

### B02 — 浮点 printf 不输出

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 / 运行期 |
| 现象 | `printf("%.1f", value)` 不输出任何字符，整数 `%d` 正常 |
| 原因 | newlib-nano 默认不包含浮点格式化 |
| 修复 | CMakeLists.txt 添加 `target_link_options(... PRIVATE -u_printf_float)` |

### B03 — `struct timespec` 未定义

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | `error: unknown type name 'struct timespec'` |
| 原因 | ESP32 代码使用 POSIX `clock_gettime()`，STM32 无此支持 |
| 修复 | 用 `HAL_GetTick()` / TIM2 替代，删除 `<time.h>` 依赖 |

### B04 — `pthread.h` 未找到

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | `fatal error: pthread.h: No such file or directory` |
| 原因 | ESP32 使用 POSIX 线程 API，STM32 bare-metal 无此支持 |
| 修复 | 移除 `pthread_mutex_*` 调用，如需互斥使用 FreeRTOS `osMutex*` |

### B05 — `esp_log.h` / `esp_err.h` 等 ESP-IDF 头文件

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | 大量 ESP-IDF 特定头文件找不到 |
| 修复 | 创建 `config.h` 中定义替代宏 (`ESP_OK` → `0`, `ESP_LOGI` → `printf`) |

### B06 — `jit_queue` / `jit_result` 未定义

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | HAL TX 路径引用 JIT queue 类型但未包含 |
| 修复 | 添加头文件包含或在 config.h 中声明前置类型 |

---

### B07 — SX1250 GET_STATUS 返回错误值

| 项目 | 内容 |
|------|------|
| 阶段 | TEST_LORAGW_SPI_SX1250 |
| 现象 | `GET_STATUS` 返回 `0x00` 或 `0xA2` 而非期望的 `0x32` |
| 原因 | SPI bridge 的 opcode 高位被 `& 0x7F` 掩码截断，`0xC0` → `0x40` |
| 修复 | 删除 `sx1302_reg_read/write` 中对 ≥0x80 opcode 的掩码操作 |
| 影响 | **关键 Bug**，不修复则所有 SX1250 通信失败 |

### B08 — SPI 事务中 CS 时序不正确

| 项目 | 内容 |
|------|------|
| 阶段 | TEST_LORAGW_SPI |
| 现象 | 偶发的数据错误 |
| 原因 | ESP32 SPI 驱动在事务前后自动管理 CS，STM32 HAL SPI 不会 |
| 修复 | 每次 SPI 事务手动拉低/拉高 NSS (PB12) |

---

### B09 — LM75A 温度传感器地址错误

| 项目 | 内容 |
|------|------|
| 阶段 | TEST_LORAGW_I2C_LM75A / HAL RX |
| 现象 | I2C 扫描找不到温度传感器，HAL 启动报温度传感器错误 |
| 原因 | 原始代码适配 STTS751 (地址 0x39/0x3B/0x38/0x3A)，板载实际是 LM75A (0x48) |
| 修复 | `loragw_stts751.c` 中将地址列表改为 `{0x48, 0x49, 0x4A, 0x4B}` |

### B10 — I2C 地址未左移

| 项目 | 内容 |
|------|------|
| 阶段 | TEST_LORAGW_I2C_OLED |
| 现象 | I2C 通信失败，NACK |
| 原因 | STM32 HAL I2C 函数要求 7-bit 地址左移 1 位 |
| 修复 | `loragw_i2c.c` 中传地址时 `dev_addr << 1` |

---

### B11 — 固件数组 `const` 限定符不匹配

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | Warning: assignment discards 'const' qualifier |
| 原因 | 固件 `.var` 文件生成的数组是 `const`，但指针声明为非 `const` |
| 修复 | 将指针改为 `const uint8_t *` |

### B12 — `LGW_REG_SUCCESS` 等宏重定义

| 项目 | 内容 |
|------|------|
| 阶段 | 编译期 |
| 现象 | Warning: macro redefined |
| 原因 | 多个头文件中重复定义了返回码宏 |
| 修复 | 使用 `#ifndef` 包裹或统一在一个头文件定义 |

---

### B13 — E77 串口识别错误

| 项目 | 内容 |
|------|------|
| 阶段 | HAL RX 测试准备 |
| 现象 | 向 COM3 发 AT 命令无反应 |
| 原因 | COM3 是蓝牙虚拟串口，E77 实际在 COM11 |
| 修复 | 用 `e77_probe.py` 探测或在设备管理器中确认 |

### B14 — RadioLib SyncWord 不匹配

| 项目 | 内容 |
|------|------|
| 阶段 | HAL TX 测试 |
| 现象 | SX1302 发射成功但 SX1278 收不到 |
| 原因 | SX1278 默认 SyncWord 0x12 (private)，SX1302 使用 0x34 (public) |
| 修复 | RadioLib 端 `radio.begin()` 参数设置 SyncWord = 0x34 |

### B15 — RadioLib CRC/IQ 设置不匹配

| 项目 | 内容 |
|------|------|
| 阶段 | HAL TX 测试 |
| 现象 | 偶尔收到包但 CRC 校验失败 |
| 原因 | SX1302 TX 设置 `no_crc=true, invert_pol=false`，接收端未对齐 |
| 修复 | RadioLib 端 `setCRC(false)`, `setInvertIQ(false)` |

---

### B16 — stat 时间戳格式导致网关永不上线

| 项目 | 内容 |
|------|------|
| 阶段 | 集成测试 |
| 现象 | 网关 UDP PULL_ACK 100% 正常，ChirpStack 中网关始终离线（never seen） |
| 定位 | `docker logs chirpstack-gateway-bridge` 中每 30 秒出现一条 error：`parsing time "\"00:00:36\"" as "\"2006-01-02 15:04:05 MST\""` |
| 根因 | `lora_pkt_fwd.c` 的 stat JSON `time` 字段格式为 `"00:00:36"`（设备运行时长），ChirpStack Gateway Bridge 要求 `"YYYY-MM-DD HH:MM:SS TZD"` 格式，解析失败后直接丢弃整个 PUSH_DATA 报文 |
| 修复 | `snprintf(stat_timestamp, ..., "2000-01-01 %02lu:%02lu:%02lu GMT", h, m, s)`；缓冲区 24→32 字节 |
| 影响 | 修复后 Gateway Bridge 立即发布 `event/stats` 到 MQTT，网关上线 |
| 文件 | `packet_forwarder/lora_pkt_fwd.c` |
| 详见 | [testing/08_chirpstack_gateway_online_test.md](../testing/08_chirpstack_gateway_online_test.md) |

### B17 — 链接脚本未保护配置扇区

| 项目 | 内容 |
|------|------|
| 阶段 | 工程设计 |
| 现象 | 当前无影响；潜在风险：固件增长超过 896KB 时链接器会把代码放入 Sector 11（`0x080E0000`），覆盖 Flash 配置数据 |
| 根因 | `STM32F407ZGTx_FLASH_cmake.ld` 中 `FLASH LENGTH = 1024K`，覆盖了配置存储区 |
| 修复 | `FLASH LENGTH = 896K`（Sectors 0-10），Sector 11 不再属于可链接区域；固件超限时链接器报错而非静默覆盖 |
| 文件 | `STM32F407ZGTx_FLASH_cmake.ld` |
| 详见 | [impl/06_freq_plan_flash_config_v4.md](../impl/06_freq_plan_flash_config_v4.md#part-4链接脚本-flash-区域保护) |

---

### B18 — `rfconf.tx_enable` 未赋值导致 JoinAccept（所有下行）TX 全失败

| 项目 | 内容 |
|------|------|
| 阶段 | E2E 测试（OTAA 入网） |
| 现象 | E77 节点反复 `+EVT:JOIN FAILED`；网关统计 `RF packets sent: 1 / TX errors: 1`；`gateway-bridge` 日志有 `event=up`（收包正常）和 `event=ack`（NS 已下发 JoinAccept），但节点从未收到 |
| 定位 | 三层诊断：Layer 1（gateway-bridge `event=up`）✅；Layer 2（ChirpStack NS join 事件）✅；Layer 3（节点 `+EVT:JOINED`）❌ → 下行 TX 本身失败 |
| 根因 | `lora_pkt_fwd.c` 的 Radio 配置循环中，从 JSON 解析出 `conf_is_tx_enable[i]` 之后，**只更新了本地数组，从未赋值给 `rfconf.tx_enable`**，导致 `lgw_rxrf_setconf()` 调用时 `rfconf.tx_enable = 0`（默认零值），SX1302 HAL 将该 RF 链路标记为 TX 禁用。随后每次下行，`lgw_send()` 返回 `LGW_HAL_ERROR`，pkt_fwd 日志输出 `ERROR: SELECTED RF_CHAIN IS DISABLED FOR TX ON SELECTED BOARD` |
| 修复 | 在 Radio 配置解析块末尾补一行：`rfconf.tx_enable = conf_is_tx_enable[i];` |
| 代码位置 | `packet_forwarder/lora_pkt_fwd.c`，`parse_radio_configuration()` 内，`lgw_rxrf_setconf(i, rfconf)` 之前 |
| 修复前后对比 | 修复前：`gateway-bridge` 始终无 `event=ack`（或 ack 后 TX error）；修复后：JoinAccept 成功发送，节点 `+EVT:JOINED`，RSSI/SNR 均正常 |
| 影响范围 | 所有下行帧（JoinAccept、Confirmed ACK、ADR 命令、应用下行）均受影响；网关在此 Bug 下实际**完全无下行能力** |
| 潜伏时间 | 此 Bug 自移植初期就存在，但之前测试（HAL TX 测试、ABP 测试）均未触发下行路径，因此未被发现 |

**修复 diff（核心）：**

```c
/* 修复前（lora_pkt_fwd.c，radio 配置循环末尾）*/
rfconf.freq_hz     = (uint32_t)json_object_get_number(conf_obj, "freq");
rfconf.rssi_offset = (float)json_object_get_number(conf_obj, "rssi_offset");
rfconf.rssi_tcomp  = ...;
rfconf.type        = ...;
// ← 缺少 rfconf.tx_enable = conf_is_tx_enable[i];
if (lgw_rxrf_setconf(i, rfconf) != LGW_HAL_SUCCESS) { ... }

/* 修复后 */
rfconf.tx_enable   = conf_is_tx_enable[i];   // ← 补加这一行
if (lgw_rxrf_setconf(i, rfconf) != LGW_HAL_SUCCESS) { ... }
```

---

### B19 — E77 入网状态下 AT 参数命令报 `AT_PARAM_ERROR` / `AT_ERROR`

| 项目 | 内容 |
|------|------|
| 阶段 | E2E 测试（测试脚本） |
| 现象 | `e77_node_ctrl.py otaa` 执行时，`AT+REGION=2`、`AT+CDEVEUI=…`、`AT+CAPPEUI=…`、`AT+CAPPKEY=…` 均失败（`AT_PARAM_ERROR` 或 `AT_ERROR`），但 `AT+CMANUALMASK`、`AT+CFREQBANDMASK`、`AT+CTXP`、`AT+CADR` 成功；最终 OTAA 入网仍可成功（之前的参数未变） |
| 根因 | E77-400M22S 固件规定：设备处于已入网（Joined）状态时，`REGION`、`CDEVEUI`、`CAPPEUI`、`CAPPKEY` 等入网凭据类命令被锁定，拒绝修改（返回错误）。脚本在重复调用 `otaa` 子命令时，模块仍保持上一次的入网状态，导致参数写入失败 |
| 验证 | `AT+REGION=?` 返回 `2:CN470` —— 证明值本身已正确，仅因已入网而拒绝 SET |
| 修复 | 在 `config_otaa()` 和 `config_abp()` 函数开头调用 `self.restore()`，先恢复出厂状态（清除入网状态），再依次写入参数；`restore()` 内部已等待 2 秒重启完成，额外加 0.5 秒确保稳定 |
| 文件 | `scripts/e77_node_ctrl.py` |
| 副作用 | `restore()` 会将 DevEUI 恢复为出厂值（`0080E11506A99424`），但随即被 `AT+CDEVEUI=AABBCCDD11223344` 覆盖，无影响 |
| 修复验证 | 修复后连续三次运行 `otaa` 子命令，每次全部 AT 指令均返回 `OK`，无警告 |

---

### B20 — OLED Row 6 时间永久空白（多根因复合）

| 项目 | 内容 |
|------|------|
| 阶段 | SNTP 功能集成后（2026-03-08） |
| 现象 | 复位后 OLED 第 6 行始终空白，不显示 `Up HH:MM:SS` 也不显示 UTC 时间；串口 SNTP 同步成功、`stat_timestamp` 字符串正确；**间歇性**（约 30–60% 复位触发） |
| 根因 | 4 个独立根因复合，需同时修复（见下） |
| 严重度 | 高（OLED 时间显示完全失效） |
| 详见 | [impl/08_oled_display_bug.md](../impl/08_oled_display_bug.md) |

**根因 1 — 主循环入口前 5 秒空白窗口**

| 项目 | 内容 |
|------|------|
| 原因 | 启动屏 `oled_show_one_line(0, 5, ...)` 调用 `oled_refresh()` 将全零 Row 6 推送到屏幕；主循环有 `vTaskDelay(5s)` 才首次更新 Row 6 |
| 修复 | 在主循环入口前立即写一次 Row 6 运行时长字符串 |
| 文件 | `packet_forwarder/lora_pkt_fwd.c` |

**根因 2 — I2C 超时裕量不足 → HAL_TIMEOUT**

| 项目 | 内容 |
|------|------|
| 原因 | 1025 字节 OLED 刷新 ≈ 92ms，超时设为 100ms（仅 8ms 裕量）；FreeRTOS `thread_up`（AboveNormal）抢占期间消耗超时裕量 |
| 修复 | 动态超时 `= I2C_TIMEOUT_MS + size/5`；1025 字节 → 305ms |
| 文件 | `libloragw/loragw_i2c.c` |

**根因 3 — STM32F4 I2C Errata ES0182：BUSY 标志锁死**

| 项目 | 内容 |
|------|------|
| 原因 | `lgw_start()` 向 4 个 LM75A 地址（0x48–0x4B）发 I2C 探测，板上无传感器，全部 NACK；依 Errata ES0182，特定 NACK 时序使 `I2C_SR2.BUSY` 位永久置 1；此后所有 `HAL_I2C_*` 调用立即返回 `HAL_BUSY`，不发出任何 I2C 信号 |
| 修复 | 检测到 `HAL_BUSY`/`HAL_TIMEOUT` 时执行 `HAL_I2C_DeInit()` + `HAL_I2C_Init()` + 重试（ST 官方推荐 workaround） |
| 文件 | `libloragw/loragw_i2c.c` |

**根因 4 — I2C2 总线无 FreeRTOS Mutex**

| 项目 | 内容 |
|------|------|
| 原因 | `thread_up`（AboveNormal）调用链 `lgw_receive()` → `lgw_get_temperature()` → `lgw_i2c_read()`；主任务 `oled_refresh()` → `lgw_i2c_write_buf()`；两者共用 `hi2c2`；STM32 HAL `__HAL_LOCK()` 非原子，FreeRTOS 抢占下不安全 |
| 修复 | 添加 `SemaphoreHandle_t s_i2c_mtx = xSemaphoreCreateMutex()`，所有 I2C 调用前后 `xSemaphoreTake/Give(portMAX_DELAY)` |
| 文件 | `libloragw/loragw_i2c.c` |

**根因 5（辅助）— oled_refresh() void 返回沉默失败**

| 项目 | 内容 |
|------|------|
| 原因 | `void oled_refresh()` 丢弃所有 I2C 返回值，失败完全不可见，导致上述根因长时间未被发现 |
| 修复 | `int err = 0; err |= ...` 累积模式；失败时打印 `[OLED] ERR: refresh failed (#N)`（最多 10 次） |
| 文件 | `libloragw/loragw_oled.c` |

---

### B21 — DMA 已配置但实际未使用，残留文件干扰工程

| 项目 | 内容 |
|------|------|
| 日期 | 2026-03-12 |
| 现象 | CubeMX 生成 `Core/Src/dma.c` 和 `Core/Inc/dma.h`，但 SPI2/SPI3 均以阻塞（Blocking）模式调用 `HAL_SPI_TransmitReceive()`，无任何代码实际调用 DMA 传输函数；`dma.c` 仅做 MX_DMA_Init() 空初始化 |
| 根因 | 历史上曾在 CubeMX 中为 SPI2 配置了 DMA（TX/RX），后来改用轮询模式但未在 CubeMX 中同步移除 DMA 配置；生成的文件保留但无实际用途 |
| 修复 | 1. 在 CubeMX `.ioc` 中移除 SPI2 的 DMA Request 配置；2. 重新生成代码（不再生成 `dma.c`/`dma.h`）；3. 删除残留的 `Core/Src/dma.c` 和 `Core/Inc/dma.h`；4. 从 `cmake/stm32cubemx/CMakeLists.txt` 的 `MX_Application_Src` 中移除 `dma.c` 条目（HAL 层 `stm32f4xx_hal_dma.c` 保留，供 HAL 内部使用）|
| 文件 | `Core/Src/dma.c`（已删除）、`Core/Inc/dma.h`（已删除）、`cmake/stm32cubemx/CMakeLists.txt` |
| 验证 | `cmake --build --preset Debug -j8` 编译通过，无 DMA 相关警告 |

---

### B22 — `board_config.h` 头部注释引脚描述与实际不符

| 项目 | 内容 |
|------|------|
| 日期 | 2026-03-12 |
| 现象 | `board_config.h` 文件顶部注释中，SX1302 NSS 引脚描述与实际 CubeMX 配置不一致，误导性地指向错误引脚编号 |
| 修复 | 注释修正为与 `main.h` CubeMX 宏定义一致：`NSS PD2 = GPIO_PIN_2`，`RESET PA8 = GPIO_PIN_8` |
| 文件 | `libloragw/board_config.h` |
| 影响 | 仅注释，不影响运行，但影响代码可读性和维护 |

---

### B23 — CubeMX 重新生成后 SPI3 时钟分频变化

| 项目 | 内容 |
|------|------|
| 日期 | 2026-03-12 |
| 现象 | CubeMX 重新生成 `spi.c` 后，SPI3（SX1302）`BaudRatePrescaler` 从原 `SPI_BAUDRATEPRESCALER_32`（≈1.3 MHz）变为 `SPI_BAUDRATEPRESCALER_8`（≈5.25 MHz）；SPI2（W5500）维持 `SPI_BAUDRATEPRESCALER_8`（≈5.25 MHz）|
| 根因 | CubeMX `.ioc` 中 SPI3 的 Prescaler 参数被修改（可能是 CubeMX 版本升级或配置导入时的默认值变化）|
| 影响评估 | SX1302 最高支持 10 MHz SPI 时钟；5.25 MHz 在规格范围内，实测验证通过后可接受；若通信出现异常可在 CubeMX 中回调至 ÷32 |
| 验证 | 重新生成后完整编译通过；user code 区域（`freertos.c`、`stm32f4xx_it.c`、`main.c`）均未被覆盖 |
| 文件 | `Core/Src/spi.c`（CubeMX 生成，不手动修改）|
| 注意 | **外设参数（Prescaler 等）统一在 CubeMX 中调整，不直接修改 `spi.c` 生成代码** |

---

## 快速参考表

| ID | 简述 | 严重度 |
|----|------|--------|
| B01 | syscall 缺失 | 高 |
| B02 | 浮点 printf 不输出 | 中 |
| B03 | timespec 未定义 | 高 |
| B04 | pthread.h 缺失 | 高 |
| B05 | ESP-IDF 头文件缺失 | 高 |
| B06 | JIT 类型缺失 | 中 |
| B07 | SX1250 opcode 掩码截断 | **致命** |
| B08 | SPI CS 时序错误 | 高 |
| B09 | LM75A 地址错误 | 中 |
| B10 | I2C 地址未左移 | 高 |
| B11 | const 限定符不匹配 | 低 |
| B12 | 宏重定义 | 低 |
| B13 | 串口识别错误 | 低 |
| B14 | SyncWord 不匹配 | 高 |
| B15 | CRC/IQ 设置不匹配 | 高 |
| B16 | stat 时间戳格式错误 | **致命** |
| B17 | 链接脚本未保护配置扇区 | 高 |
| B18 | rfconf.tx_enable 未赋值，下行 TX 全失败 | **致命** |
| B19 | E77 入网后 AT 参数不可写 | 中 |
| B20 | OLED Row 6 永久空白（多根因复合） | 高 |
| B21 | DMA 残留文件（未使用但保留） | 中 |
| B22 | board_config.h 注释引脚描述错误 | 低 |
| B23 | CubeMX 重新生成后 SPI3 分频变化 | 低 |
