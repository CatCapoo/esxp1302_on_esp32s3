# BUG-003锛歵est_loragw_hal_rx.c 澶氬缂栬瘧閿欒

- **鏃ユ湡**锛?026-02-19  
- **鏂囦欢**锛歚main/test/test_loragw_hal_rx.c`  
- **涓ラ噸绾у埆**锛氱紪璇戝け璐?
---

## 闂鎻忚堪

缂栬瘧 `test_loragw_hal_rx.c` 鏃舵姤鍑?5 澶勯敊璇紝绋嬪簭鏃犳硶鏋勫缓銆傞敊璇被鍨嬩笌 BUG-001锛坄test_network_connection.c`锛夌浉鍚岋紝鍧囩敱 ESP-IDF v5.x API 鍙樻洿鍙?Xtensa 骞冲彴 `uint32_t` 绫诲瀷瀹藉害寮曡捣銆?
---

## 閿欒璇︽儏涓庢牴鏈師鍥?
### 閿欒 1鈥?锛氭牸寮忕绫诲瀷涓嶅尮閰?
```
error: format '%d' expects argument of type 'int',
       but argument has type 'uint32_t' {aka 'long unsigned int'}
```

| 琛屽彿 | 鍘熶唬鐮?| 鍙橀噺绫诲瀷 |
|------|--------|----------|
| 299 | `printf("'-r' with wrong value: %d; ...", val)` | `uint32_t` |
| 308 | `printf("'-k' with wrong value: %d; ...", val)` | `uint32_t` |
| 346 | `printf("'-m' with wrong value: %d; ...", val)` | `uint32_t` |

**鍘熷洜**锛歚val` 鏄?`uint32_t`锛屽湪 ESP32-S3锛圶tensa 32-bit锛夌紪璇戠幆澧冧腑绛夊悓浜?`long unsigned int`锛宍%d` 瀵瑰簲 `int`锛宍-Werror=format=` 灏嗚鍛婂崌涓洪敊璇€?
**淇**锛?
1. 鍦ㄦ枃浠堕《閮ㄦ坊鍔?`#include <inttypes.h>`銆?2. 灏嗕笁澶?`%d` 鏀逛负 `%" PRIu32 "`锛?
```c
// 淇鍚?printf("'-r' with wrong value: %" PRIu32 "; should be 1255/1257/1250\n", val);
printf("'-k' with wrong value: %" PRIu32 "; should be 0 or 1\n", val);
printf("'-m' with wrong value: %" PRIu32 "; should be 0 or 1\n", val);
```

---

### 閿欒 4鈥?锛歟sp_console REPL API 宸插簾寮?
```
error: implicit declaration of function 'esp_console_repl_init'
error: implicit declaration of function 'esp_console_repl_start'
```

**鍘熷洜**锛欵SP-IDF v5.x 绉婚櫎浜嗘棫鐗?REPL 鍒濆鍖?API锛屾柊 API 鍒嗙浜?UART 璁惧閰嶇疆涓?REPL 鐢熷懡鍛ㄦ湡鎺у埗锛?
| 鏃?API锛坴4.x锛?| 鏂?API锛坴5.x锛?|
|----------------|----------------|
| `esp_console_repl_init(&repl_config, &repl)` | `esp_console_new_repl_uart(&uart_config, &repl_config, &repl)` |
| `esp_console_repl_start(repl)` | `esp_console_start_repl(repl)` |

**淇**锛?
```c
// 淇鍓?esp_console_repl_t *repl = NULL;
esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
repl_config.prompt = "sx1302_hal>";
esp_console_repl_init(&repl_config, &repl);
esp_console_repl_start(repl);

// 淇鍚?esp_console_repl_t *repl = NULL;
esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
repl_config.task_stack_size = 4096 * 16;
repl_config.prompt = "sx1302_hal>";
esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
ESP_ERROR_CHECK(esp_console_start_repl(repl));
```

---

## 楠岃瘉

閲嶆柊缂栬瘧鍚庢棤鎶ラ敊锛岀▼搴忔甯稿惎鍔ㄥ苟杩涘叆 `sx1302_hal>` 浜や簰寮忔帶鍒跺彴銆?
