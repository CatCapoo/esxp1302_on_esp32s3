# BUG-003：test_loragw_hal_rx.c 多处编译错误

- **日期**：2026-02-19  
- **文件**：`main/test/test_loragw_hal_rx.c`  
- **严重级别**：编译失败

---

## 问题描述

编译 `test_loragw_hal_rx.c` 时报出 5 处错误，程序无法构建。错误类型与 BUG-001（`test_network_connection.c`）相同，均由 ESP-IDF v5.x API 变更及 Xtensa 平台 `uint32_t` 类型宽度引起。

---

## 错误详情与根本原因

### 错误 1–3：格式符类型不匹配

```
error: format '%d' expects argument of type 'int',
       but argument has type 'uint32_t' {aka 'long unsigned int'}
```

| 行号 | 原代码 | 变量类型 |
|------|--------|----------|
| 299 | `printf("'-r' with wrong value: %d; ...", val)` | `uint32_t` |
| 308 | `printf("'-k' with wrong value: %d; ...", val)` | `uint32_t` |
| 346 | `printf("'-m' with wrong value: %d; ...", val)` | `uint32_t` |

**原因**：`val` 是 `uint32_t`，在 ESP32-S3（Xtensa 32-bit）编译环境中等同于 `long unsigned int`，`%d` 对应 `int`，`-Werror=format=` 将警告升为错误。

**修复**：

1. 在文件顶部添加 `#include <inttypes.h>`。
2. 将三处 `%d` 改为 `%" PRIu32 "`：

```c
// 修复后
printf("'-r' with wrong value: %" PRIu32 "; should be 1255/1257/1250\n", val);
printf("'-k' with wrong value: %" PRIu32 "; should be 0 or 1\n", val);
printf("'-m' with wrong value: %" PRIu32 "; should be 0 or 1\n", val);
```

---

### 错误 4–5：esp_console REPL API 已废弃

```
error: implicit declaration of function 'esp_console_repl_init'
error: implicit declaration of function 'esp_console_repl_start'
```

**原因**：ESP-IDF v5.x 移除了旧版 REPL 初始化 API，新 API 分离了 UART 设备配置与 REPL 生命周期控制：

| 旧 API（v4.x） | 新 API（v5.x） |
|----------------|----------------|
| `esp_console_repl_init(&repl_config, &repl)` | `esp_console_new_repl_uart(&uart_config, &repl_config, &repl)` |
| `esp_console_repl_start(repl)` | `esp_console_start_repl(repl)` |

**修复**：

```c
// 修复前
esp_console_repl_t *repl = NULL;
esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
repl_config.prompt = "sx1302_hal>";
esp_console_repl_init(&repl_config, &repl);
esp_console_repl_start(repl);

// 修复后
esp_console_repl_t *repl = NULL;
esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
repl_config.task_stack_size = 4096 * 16;
repl_config.prompt = "sx1302_hal>";
esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
ESP_ERROR_CHECK(esp_console_start_repl(repl));
```

---

## 验证

重新编译后无报错，程序正常启动并进入 `sx1302_hal>` 交互式控制台。
