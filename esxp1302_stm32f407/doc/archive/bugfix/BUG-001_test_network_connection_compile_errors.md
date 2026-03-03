# BUG-001锛歵est_network_connection.c 澶氬缂栬瘧閿欒

- **鏃ユ湡**锛?026-02-19  
- **鏂囦欢**锛歚main/test/test_network_connection.c`  
- **涓ラ噸绾у埆**锛氱紪璇戝け璐?
---

## 闂鎻忚堪

缂栬瘧 `test_network_connection.c` 鏃舵姤鍑?7 澶勯敊璇紝绋嬪簭鏃犳硶鏋勫缓銆?
---

## 閿欒璇︽儏涓庢牴鏈師鍥?
### 閿欒 1锛氭牸寮忕绫诲瀷涓嶅尮閰?
```
error: format '%d' expects argument of type 'int',
       but argument 7 has type 'uint32_t' {aka 'long unsigned int'}
```

**浣嶇疆**锛氱 164 琛? 
**浠ｇ爜**锛?```c
ESP_LOGI(TAG, "Socket created, sending to %s:%d", udp_host, udp_port);
```
**鍘熷洜**锛歚udp_port` 鏄?`uint32_t`锛屽湪 ESP32锛圶tensa 32-bit锛変笂涓?`long unsigned int`锛宍%d` 瀵瑰簲 `int`锛岀被鍨嬩笉鍖归厤锛宍-Werror=format=` 浣垮叾鍙樹负閿欒銆?
---

### 閿欒 2锛歺TaskCreate 鍙傛暟杩囧

```
error: too many arguments to function 'xTaskCreate'
```

**浣嶇疆**锛氱 217 琛? 
**浠ｇ爜**锛?```c
xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL, 0);
```
**鍘熷洜**锛氭爣鍑?`xTaskCreate` 鍙湁 6 涓弬鏁帮紝澶氫紶浜嗙 7 涓弬鏁?`0`锛堟牳蹇冧翰鍜屾€э紝搴斾娇鐢?`xTaskCreatePinnedToCore` 鎵嶆湁姝ゅ弬鏁帮級銆?
---

### 閿欒 3锛歟sp_console REPL API 宸插簾寮?
```
error: implicit declaration of function 'esp_console_repl_init'
error: implicit declaration of function 'esp_console_repl_start'
```

**浣嶇疆**锛氱 331銆?33 琛? 
**鍘熷洜**锛欵SP-IDF v5.x 绉婚櫎浜嗘棫鐗?`esp_console_repl_init()` / `esp_console_repl_start()` API锛岄渶鏀圭敤鏂?API锛?- `esp_console_new_repl_uart()` 
- `esp_console_start_repl()`

---

### 閿欒 4锛氫簨浠跺鐞嗗彞鏌勪紶鍙嶏紙杩愯鏃跺穿婧冿級

**浣嶇疆**锛氱 136 琛? 
**鐜拌薄**锛歐iFi 杩炴帴鎴愬姛鍚庣珛鍒昏Е鍙?`ESP_ERR_NOT_FOUND` abort锛宐acktrace 鎸囧悜 `wifi_init_sta`銆? 
**浠ｇ爜**锛?```c
// 娉ㄥ唽鏃?esp_event_handler_instance_register(WIFI_EVENT, ..., &instance_any_id);
esp_event_handler_instance_register(IP_EVENT,   ..., &instance_got_ip);

// 娉ㄩ攢鏃讹紙鍘熷鈥旈敊璇級
esp_event_handler_instance_unregister(IP_EVENT,   ..., instance_any_id); // 鈫?浼犲弽
esp_event_handler_instance_unregister(WIFI_EVENT, ..., instance_got_ip); // 鈫?浼犲弽
```
**鍘熷洜**锛氭敞閿€鏃?handle 涓?event base 涓嶅搴旓紝鎵句笉鍒板尮閰嶉」锛岃繑鍥?`ESP_ERR_NOT_FOUND`锛?x105锛夈€?
---

## 淇敼鍐呭

### 淇敼 1锛氭牸寮忕鏀圭敤 PRIu32

```c
// 淇敼鍓?ESP_LOGI(TAG, "Socket created, sending to %s:%d", udp_host, udp_port);

// 淇敼鍚?ESP_LOGI(TAG, "Socket created, sending to %s:%" PRIu32, udp_host, udp_port);
```

### 淇敼 2锛氱Щ闄ゅ浣欏弬鏁?
```c
// 淇敼鍓?xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL, 0);

// 淇敼鍚?xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);
```

### 淇敼 3锛氭洿鏂?REPL API

```c
// 淇敼鍓?esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
...
ESP_ERROR_CHECK(esp_console_repl_init(&repl_config));
ESP_ERROR_CHECK(esp_console_repl_start());

// 淇敼鍚?esp_console_repl_t *repl = NULL;
esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
...
esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
ESP_ERROR_CHECK(esp_console_start_repl(repl));
```

### 淇敼 4锛氫氦鎹㈡敞閿€鍙ユ焺

```c
// 淇敼鍓?ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT,   IP_EVENT_STA_GOT_IP, instance_any_id));
ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,    instance_got_ip));

// 淇敼鍚?ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT,   IP_EVENT_STA_GOT_IP, instance_got_ip));
ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,    instance_any_id));
```

---

## 楠岃瘉缁撴灉

淇鍚庣紪璇戦€氳繃锛學iFi 杩炴帴姝ｅ父锛孶DP 鍙戝寘/鏀跺寘娴嬭瘯閫氳繃锛?
```
I (11909) wifi station: connected to ap SSID:222 password:0987654321
I (11919) wifi station: Socket created, sending to 10.184.141.238:5005
I (11919) wifi station: Message sent
I (12349) wifi station: Received 37 bytes from 10.184.141.238:
I (12349) wifi station: OK: Message from SX1302 ESP32 PKT-FWD
I (12349) wifi station: Received expected message, reconnecting
```
