# BUG-001：test_network_connection.c 多处编译错误

- **日期**：2026-02-19  
- **文件**：`main/test/test_network_connection.c`  
- **严重级别**：编译失败

---

## 问题描述

编译 `test_network_connection.c` 时报出 7 处错误，程序无法构建。

---

## 错误详情与根本原因

### 错误 1：格式符类型不匹配

```
error: format '%d' expects argument of type 'int',
       but argument 7 has type 'uint32_t' {aka 'long unsigned int'}
```

**位置**：第 164 行  
**代码**：
```c
ESP_LOGI(TAG, "Socket created, sending to %s:%d", udp_host, udp_port);
```
**原因**：`udp_port` 是 `uint32_t`，在 ESP32（Xtensa 32-bit）上为 `long unsigned int`，`%d` 对应 `int`，类型不匹配，`-Werror=format=` 使其变为错误。

---

### 错误 2：xTaskCreate 参数过多

```
error: too many arguments to function 'xTaskCreate'
```

**位置**：第 217 行  
**代码**：
```c
xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL, 0);
```
**原因**：标准 `xTaskCreate` 只有 6 个参数，多传了第 7 个参数 `0`（核心亲和性，应使用 `xTaskCreatePinnedToCore` 才有此参数）。

---

### 错误 3：esp_console REPL API 已废弃

```
error: implicit declaration of function 'esp_console_repl_init'
error: implicit declaration of function 'esp_console_repl_start'
```

**位置**：第 331、333 行  
**原因**：ESP-IDF v5.x 移除了旧版 `esp_console_repl_init()` / `esp_console_repl_start()` API，需改用新 API：
- `esp_console_new_repl_uart()` 
- `esp_console_start_repl()`

---

### 错误 4：事件处理句柄传反（运行时崩溃）

**位置**：第 136 行  
**现象**：WiFi 连接成功后立刻触发 `ESP_ERR_NOT_FOUND` abort，backtrace 指向 `wifi_init_sta`。  
**代码**：
```c
// 注册时
esp_event_handler_instance_register(WIFI_EVENT, ..., &instance_any_id);
esp_event_handler_instance_register(IP_EVENT,   ..., &instance_got_ip);

// 注销时（原始—错误）
esp_event_handler_instance_unregister(IP_EVENT,   ..., instance_any_id); // ← 传反
esp_event_handler_instance_unregister(WIFI_EVENT, ..., instance_got_ip); // ← 传反
```
**原因**：注销时 handle 与 event base 不对应，找不到匹配项，返回 `ESP_ERR_NOT_FOUND`（0x105）。

---

## 修改内容

### 修改 1：格式符改用 PRIu32

```c
// 修改前
ESP_LOGI(TAG, "Socket created, sending to %s:%d", udp_host, udp_port);

// 修改后
ESP_LOGI(TAG, "Socket created, sending to %s:%" PRIu32, udp_host, udp_port);
```

### 修改 2：移除多余参数

```c
// 修改前
xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL, 0);

// 修改后
xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);
```

### 修改 3：更新 REPL API

```c
// 修改前
esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
...
ESP_ERROR_CHECK(esp_console_repl_init(&repl_config));
ESP_ERROR_CHECK(esp_console_repl_start());

// 修改后
esp_console_repl_t *repl = NULL;
esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
...
esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
ESP_ERROR_CHECK(esp_console_start_repl(repl));
```

### 修改 4：交换注销句柄

```c
// 修改前
ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT,   IP_EVENT_STA_GOT_IP, instance_any_id));
ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,    instance_got_ip));

// 修改后
ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT,   IP_EVENT_STA_GOT_IP, instance_got_ip));
ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,    instance_any_id));
```

---

## 验证结果

修复后编译通过，WiFi 连接正常，UDP 发包/收包测试通过：

```
I (11909) wifi station: connected to ap SSID:222 password:0987654321
I (11919) wifi station: Socket created, sending to 10.184.141.238:5005
I (11919) wifi station: Message sent
I (12349) wifi station: Received 37 bytes from 10.184.141.238:
I (12349) wifi station: OK: Message from SX1302 ESP32 PKT-FWD
I (12349) wifi station: Received expected message, reconnecting
```
