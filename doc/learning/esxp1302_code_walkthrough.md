# ESXP1302 LoRaWAN 网关代码完整解析

> 日期：2026-02-22  
> 项目：esxp1302_on_esp32s3  
> 平台：ESP32-S3 + SX1302/SX1303 LoRa 集中器  
> 框架：ESP-IDF v5.4.3 + FreeRTOS  
> HAL：Semtech sx1302_hal v2.1.0  
> 协议：Semtech Packet Forwarder Protocol v2 (UDP)

---

## 目录

- [第一部分：app\_main() 入口及启动流程](#第一部分app_main-入口及启动流程)
  - [1.1 OLED 初始化与按钮检测](#11-oled-初始化与按钮检测)
  - [1.2 NVS 配置系统（深入）](#12-nvs-配置系统深入)
  - [1.3 WiFi 模式决策逻辑](#13-wifi-模式决策逻辑)
  - [1.4 串口控制台 REPL（深入）](#14-串口控制台-repl深入)
- [第二部分：WiFi 连接与事件驱动架构](#第二部分wifi-连接与事件驱动架构)
  - [2.1 wifi\_init\_sta() 详解](#21-wifi_init_sta-详解)
  - [2.2 ESP 事件循环机制（深入）](#22-esp-事件循环机制深入)
  - [2.3 wifi\_sta\_event\_handler 三个状态](#23-wifi_sta_event_handler-三个状态)
- [第三部分：pkt\_fwd\_main() 核心初始化](#第三部分pkt_fwd_main-核心初始化)
  - [3.1 互斥量创建](#31-互斥量创建)
  - [3.2 JSON 配置加载与频率补丁](#32-json-配置加载与频率补丁)
  - [3.3 三个 parse\_\* 函数](#33-三个-parse_-函数)
  - [3.4 UDP Socket 创建与连接](#34-udp-socket-创建与连接)
  - [3.5 SX1302 复位与启动](#35-sx1302-复位与启动)
  - [3.6 工作线程创建](#36-工作线程创建)
  - [3.7 主循环：统计收集](#37-主循环统计收集)
- [第四部分：三大核心线程详解](#第四部分三大核心线程详解)
  - [4.1 thread\_up — 上行线程](#41-thread_up--上行线程)
  - [4.2 thread\_down — 下行线程](#42-thread_down--下行线程)
  - [4.3 thread\_jit — JIT 定时发射线程](#43-thread_jit--jit-定时发射线程)
  - [4.4 三线程协作完整数据流](#44-三线程协作完整数据流)
- [附录A：温度传感器问题详解（踩坑记录）](#附录a温度传感器问题详解踩坑记录)
- [附录B：互斥量使用汇总](#附录b互斥量使用汇总)
- [附录C：关键常量速查表](#附录c关键常量速查表)
- [附录D：硬件引脚映射](#附录d硬件引脚映射)

---

## 第一部分：app_main() 入口及启动流程

**文件**: `main/packet_forwarder/lora_pkt_fwd.c` 约第4336行

### 1.1 OLED 初始化与按钮检测

`app_main()` 是 ESP-IDF 的唯一入口函数，等同于 Linux 的 `main()`。启动顺序如下：

```c
void app_main(void)
{
    // ① 打印版本号
    printf("\n\n*** ESXP1302 Gateway. Version: %s ***\n\n\n", EXSP1302_VERSION);

    // ② OLED 初始化并显示启动画面
    oled_init();
    oled_cls();
    oled_show_str(0, 0, "ESXP1302 GATEWAY", 2);

    // ③ 配置两个用户按钮的 GPIO（IO0 和 IO6）
    // 按钮是低电平有效（BUTTON_PRESSED=0），所以开启上拉
    if(USER_BUTTON_1 != GPIO_NUM_NC){
        gpio_set_direction(USER_BUTTON_1, GPIO_MODE_INPUT);
        gpio_pullup_en(USER_BUTTON_1);
    }
    // USER_BUTTON_2 同理

    // ④ 从 NVS 读取配置（见 1.2 节详解）
    read_config_from_nvs();

    // ⑤ 判断 WiFi 模式（见 1.3 节）
    // ⑥ 启动 HTTP 服务和 REPL 控制台（见 1.4 节）
}
```

### 1.2 NVS 配置系统（深入）

**文件**: `main/packet_forwarder/web_config.c` + `web_config.h`

NVS（Non-Volatile Storage）是 ESP-IDF 提供的键值对存储系统，数据保存在 Flash 的专用分区中，掉电不丢失。

#### 数据结构

```c
// web_config.h
typedef enum {
    WIFI_SSID = 0, WIFI_PASSWORD, NS_HOST, NS_PORT, GW_ID,
    WIFI_MODE, FREQ_REGION, FREQ_RADIO0, FREQ_RADIO1, NTP_SERVER,
    CONFIG_NUM,      // = 10，总共10个配置项
    CONFIG_ERR = 255
} tag_e;

typedef struct {
    tag_e tag;       // 枚举索引
    char name[16];   // NVS 中的键名
    char *val;       // 值（动态分配的字符串）
    int len;         // 字符串长度
} config_s;
```

配置表初始化：

```c
config_s config[CONFIG_NUM] = {
    { WIFI_SSID,     "wifi_ssid",   NULL, 0 },
    { WIFI_PASSWORD,  "wifi_pswd",   NULL, 0 },
    { NS_HOST,        "ns_host",     NULL, 0 },
    { NS_PORT,        "ns_port",     NULL, 0 },
    { GW_ID,          "gw_id",       NULL, 0 },
    { WIFI_MODE,      "wifi_mode",   NULL, 0 },
    { FREQ_REGION,    "freq_region", NULL, 0 },
    { FREQ_RADIO0,    "freq_radio0", NULL, 0 },
    { FREQ_RADIO1,    "freq_radio1", NULL, 0 },
    { NTP_SERVER,     "ntp_server",  NULL, 0 },
};
```

#### 初始化流程

```c
esp_err_t init_config_storage(void)
{
    esp_err_t err = nvs_flash_init();
    // 如果 NVS 分区损坏（版本不匹配或满了），先擦除再初始化
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if(err == ESP_OK)
        nvs_ready = true;
    return err;
}
```

#### 读取流程（两步调用模式）

```c
esp_err_t read_config(void)
{
    nvs_handle_t my_handle;
    nvs_open("nvs", NVS_READONLY, &my_handle);  // 打开 namespace "nvs"

    for(int i = 0; i < CONFIG_NUM; i++){
        // 第一步：传 NULL 获取值的长度
        err = nvs_get_str(my_handle, config[i].name, NULL, &len);

        // 第二步：根据长度 malloc，再传入真正的缓冲区
        p = malloc(len);
        err = nvs_get_str(my_handle, config[i].name, p, &len);

        config[i].val = p;
        config[i].len = len - 1;  // 不算 \0
    }
    nvs_close(my_handle);
}
```

**为什么需要两步调用？**  
因为 NVS 中存储的字符串长度不固定，必须先查询长度再分配内存，避免浪费或溢出。这是 ESP-IDF NVS API 的标准用法。

#### 写入流程

```c
int save_config(void)
{
    nvs_handle_t my_handle;
    nvs_open("nvs", NVS_READWRITE, &my_handle);

    for(int i = 0; i < CONFIG_NUM; i++) {
        if(config[i].val != NULL)
            nvs_set_str(my_handle, config[i].name, config[i].val);
    }
    nvs_commit(my_handle);  // 关键！不 commit 数据不会写入 Flash
    nvs_close(my_handle);
}
```

**`nvs_commit()` 的必要性：** `nvs_set_str()` 只是把数据写入 RAM 缓冲区，必须调用 `nvs_commit()` 才能真正持久化到 Flash。

#### 配置来源优先级

NVS 中的值会覆盖 JSON 中的默认值（在 `pkt_fwd_main()` 中实现）：

```
JSON 默认值 (global_cn_conf) → NVS 覆盖 (如果有值)
                                ↑
                        Web 页面 / CLI 修改 → save_config() → NVS
```

### 1.3 WiFi 模式决策逻辑

WiFi 模式判断的优先级链：

```c
if (USER_BUTTON_1 按下)         → Soft-AP 模式（强制）
else if (USER_BUTTON_2 按下)     → Station 模式（强制）
else if (NVS 中 wifi_mode 未设置) → Soft-AP 模式（首次启动默认）
else if (NVS 中 wifi_mode == "soft_ap") → Soft-AP 模式
else                              → Station 模式
```

**关键设计：开机前按住 IO0 按钮可以强制进入 Soft-AP 配置模式**，这是"救砖"手段——即使 WiFi 密码配错了，也能通过按钮进入 AP 模式重新配置。

#### Soft-AP 模式启动

```c
if(soft_ap_mode == true){
    // 先把 wifi_mode 改为 station 并保存到 NVS
    // 这样下次重启就不会再进 AP 模式
    config_wifi_mode(WIFI_MODE_STATION);

    // 设置 10 分钟后自动重启（防止用户忘了配置就离开）
    reboot_delay_s = 60 * 10;
    reboot_flag = true;
    start_reboot_timer_ms(reboot_delay_s * 1000);

    wifi_init_soft_ap();
    // OLED 显示: IP=192.168.4.1, Soft AP mode, SSID=esp32, PSWD=esp32wifi
}
```

#### Station 模式启动

```c
else {
    // 先把 wifi_mode 改为 soft_ap 并保存
    // 这样如果连不上 WiFi，下次重启就会进 AP 模式
    config_wifi_mode(WIFI_MODE_SOFT_AP);

    // 设置 5 分钟后自动重启
    reboot_delay_s = 60 * 5;
    reboot_flag = true;
    start_reboot_timer_ms(reboot_delay_s * 1000);

    wifi_init_sta();  // → 详见第二部分
}
```

**模式切换的自保护机制：**

| 动作 | NVS wifi_mode 写入值 | 为什么 |
|------|------|------|
| 进入 Soft-AP | `"station"` | 下次重启自动回 Station（不卡在配置模式） |
| 进入 Station | `"soft_ap"` | 如果 WiFi 连不上，下次重启自动进 Soft-AP（可重新配置） |
| Station 连接成功 | `"station"` | WiFi 确认可用，保持 Station 模式 |

### 1.4 串口控制台 REPL（深入）

REPL（Read-Eval-Print Loop）是通过串口（USB-UART）实现的命令行交互界面。

#### 初始化

```c
// 配置 REPL
esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
repl_config.task_stack_size = 4096 * 2;  // 8KB 栈
repl_config.prompt = "ESXP1302_GW>";     // 命令提示符

// 注册自定义命令
usage();           // 打印帮助信息
register_config(); // 注册 "pkt_fwd" 命令

// 创建 UART REPL 并启动
esp_console_repl_t *repl = NULL;
esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
ESP_ERROR_CHECK(esp_console_start_repl(repl));
```

#### 命令注册（argtable3）

```c
static struct {
    struct arg_str *wifi_ssid;   // --ssid "304"
    struct arg_str *wifi_pswd;   // --pswd "password"
    struct arg_str *udp_host;    // --host "192.168.71.108"
    struct arg_int *udp_port;    // --port 1700
    struct arg_str *gw_id;       // --gwid "AA555A00000021FB"
    struct arg_end *end;
} net_conf_args;

void register_config(void)
{
    // 定义每个参数
    net_conf_args.wifi_ssid  = arg_str0(NULL, "ssid", "<SSID>", "SSID of AP");
    net_conf_args.wifi_pswd  = arg_str0(NULL, "pswd", "<Password>", "Password of AP");
    net_conf_args.udp_host   = arg_str0(NULL, "host", "<UDP Host>", "UDP Host");
    net_conf_args.udp_port   = arg_int0(NULL, "port", "<UDP Port>", "UDP Port");
    net_conf_args.gw_id      = arg_str0(NULL, "gwid", "<gateway id>", "Gateway Id");
    net_conf_args.end = arg_end(2);

    // 注册到 esp_console
    const esp_console_cmd_t hal_conf_cmd = {
        .command = "pkt_fwd",
        .help = "ESP32 packet forwarder based on sx1302_hal",
        .func = &do_net_config_cmd,     // 回调函数
        .argtable = &net_conf_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&hal_conf_cmd));
}
```

**用法示例：**
```
ESXP1302_GW> pkt_fwd --ssid "MyWiFi" --pswd "12345678" --host "192.168.1.100" --port 1700
```

#### 命令处理函数

```c
static int do_net_config_cmd(int argc, char **argv)
{
    // argtable3 解析参数
    int nerrors = arg_parse(argc, argv, (void **)&net_conf_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, net_conf_args.end, argv[0]);
        return 1;
    }

    // 逐个检查并更新 config[]
    if (net_conf_args.wifi_ssid->count > 0) {
        // 更新 config[WIFI_SSID]
    }
    // ... 其他参数同理

    save_config();  // 保存到 NVS

    // 取消重启定时器（说明用户通过 CLI 在线修改了配置）
    reboot_flag = false;
    // 标记需要重启以应用新配置
    printf("Config saved. Please reboot to apply.\n");
    return 0;
}
```

**`reboot_flag = false` 的意义：** 在 Station 模式下，系统默认设了 5 分钟重启定时器（怕连不上 WiFi 卡死）。如果用户通过 CLI 成功修改了配置（比如改了 WiFi 密码），说明串口是通的，用户有控制权，就取消自动重启。

---

## 第二部分：WiFi 连接与事件驱动架构

### 2.1 wifi_init_sta() 详解

```c
void wifi_init_sta(void)
{
    // ① 创建 FreeRTOS 事件组（用于同步等待）
    s_wifi_event_group = xEventGroupCreate();

    // ② 初始化 TCP/IP 协议栈
    ESP_ERROR_CHECK(esp_netif_init());

    // ③ 创建默认事件循环（全局单例）
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // ④ 创建默认的 STA 网络接口
    esp_netif_create_default_wifi_sta();

    // ⑤ WiFi 初始化
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // ⑥ 注册事件处理函数
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,            // 事件基
        ESP_EVENT_ANY_ID,      // 订阅所有 WiFi 事件
        &wifi_sta_event_handler,
        NULL,
        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,              // IP 事件基
        IP_EVENT_STA_GOT_IP,   // 只订阅"获取到IP"事件
        &wifi_sta_event_handler,
        NULL,
        &instance_got_ip));

    // ⑦ 配置并启动 WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ...,       // 从 config[WIFI_SSID] 来
            .password = ...,   // 从 config[WIFI_PASSWORD] 来
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());  // 触发 WIFI_EVENT_STA_START
}
```

**注意：`esp_wifi_start()` 是非阻塞的。** 它只是告诉 WiFi 驱动"开始吧"，然后立即返回。真正的连接过程是异步的，通过事件回调通知结果。

### 2.2 ESP 事件循环机制（深入）

ESP-IDF 的事件循环是一个**发布-订阅**模式的消息系统：

```
                    ┌─────────────────────────────┐
                    │      默认事件循环            │
                    │   (运行在独立的系统任务中)    │
                    └──────────┬──────────────────┘
                               │
            ┌──────────────────┼──────────────────┐
            │                  │                  │
     WIFI_EVENT           IP_EVENT          自定义事件
     ├─ STA_START         ├─ GOT_IP          ...
     ├─ STA_DISCONNECTED  ├─ LOST_IP
     ├─ STA_CONNECTED     └─ ...
     └─ ...
```

**核心概念：**
- `event_base`（事件基）：分类，如 `WIFI_EVENT`、`IP_EVENT`
- `event_id`：具体事件，如 `WIFI_EVENT_STA_START`
- `event_data`：事件携带的数据（如 `ip_event_got_ip_t` 包含 IP 地址）
- 处理函数运行在**事件循环任务**的上下文中，不是中断上下文

**订阅 vs 发布：**
```c
// 订阅（应用层做的）
esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &handler, ...);

// 发布（WiFi 驱动内部做的）
esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_START, NULL, 0, 0);
```

### 2.3 wifi_sta_event_handler 三个状态

```c
static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data)
```

#### 状态1：STA_START → 发起连接

```
esp_wifi_start() ─→ [WIFI_EVENT_STA_START] ─→ esp_wifi_connect()
```

WiFi 驱动初始化完成后自动触发此事件，处理函数里调用 `esp_wifi_connect()` 发起实际的 AP 连接。

#### 状态2：DISCONNECTED → 重试或重启

```
连接失败/断开 ─→ [WIFI_EVENT_STA_DISCONNECTED] ─→ retry < 5 ? 重试 : 放弃
```

```c
if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < WIFI_MAXIMUM_RETRY) {     // WIFI_MAXIMUM_RETRY = 5
        esp_wifi_connect();                      // 重试
        s_retry_num++;
    } else {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        // 5 次失败后不再重试，5分钟后自动重启进 Soft-AP 模式
    }
}
```

#### 状态3：GOT_IP → 启动 pkt_fwd 任务

```
DHCP 获取到 IP ─→ [IP_EVENT_STA_GOT_IP] ─→ 创建 pkt_fwd_task
```

```c
if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    snprintf(self_ip, sizeof(self_ip), IPSTR, IP2STR(&event->ip_info.ip));

    wifi_ready = true;

    // WiFi 连接成功，保存 wifi_mode 为 station
    config_wifi_mode(WIFI_MODE_STATION);

    // 取消重启定时器
    reboot_flag = false;

    // 创建核心任务！16KB 栈，优先级 6
    xTaskCreate(pkt_fwd_task, "pkt_fwd", 4096*4, NULL, 6, &pkt_fwd_handle);

    // OLED 显示 IP 地址
    oled_show_one_line(0, 4, self_ip, 2);
}
```

**关键点：**
- `pkt_fwd_task` 是在 WiFi 连接成功后才创建的，确保网络就绪
- `config_wifi_mode(WIFI_MODE_STATION)` 把 NVS 中的模式改回 Station，确认 WiFi 可用
- `reboot_flag = false` 取消 5 分钟自动重启定时器

---

## 第三部分：pkt_fwd_main() 核心初始化

**文件**: `lora_pkt_fwd.c` 约第1590行  
`pkt_fwd_task()` 是一个 FreeRTOS 任务包装，内部直接调用 `pkt_fwd_main()`。

### 3.1 互斥量创建

```c
mx_concent = xSemaphoreCreateMutex();  // SPI 总线互斥（最重要）
mx_xcorr   = xSemaphoreCreateMutex();  // XTAL 校正值互斥
mx_timeref = xSemaphoreCreateMutex();  // GPS 时间参考互斥
mx_meas_up = xSemaphoreCreateMutex();  // 上行统计互斥
mx_meas_dw = xSemaphoreCreateMutex();  // 下行统计互斥
mx_meas_gps = xSemaphoreCreateMutex(); // GPS 统计互斥
mx_stat_rep = xSemaphoreCreateMutex(); // 状态报告互斥
```

`mx_concent` 是最核心的互斥量——SX1302 只有一条 SPI 总线，多个线程同时访问会冲突，所以所有 `lgw_*()` 调用都必须在 `mx_concent` 保护下执行。

### 3.2 JSON 配置加载与频率补丁

```c
// 根据 NVS 中的 freq_region 选择对应的 JSON 配置
if(strncmp(config[FREQ_REGION].val, "eu868", 5) == 0){
    conf_array = malloc(sizeof(global_eu_conf));
    memcpy(conf_array, global_eu_conf, sizeof(global_eu_conf));
} else if(strncmp(config[FREQ_REGION].val, "us915", 5) == 0){
    conf_array = malloc(sizeof(global_us_conf));
    memcpy(conf_array, global_us_conf, sizeof(global_us_conf));
} else {
    conf_array = malloc(sizeof(global_cn_conf));  // 默认 cn470
    memcpy(conf_array, global_cn_conf, sizeof(global_cn_conf));
}
```

JSON 是编译时嵌入到固件中的（通过 `global_json.h` 中的字符数组），不是从文件系统加载的。

**频率补丁：直接在 JSON 字符串中替换频率值**

```c
// 用 strstr 找到 JSON 中第一个和第二个 "freq" 字段
char *radio0_index = strstr(conf_array, "\"freq\"");
char *radio1_index = strstr(radio0_index + 8, "\"freq\"");

// 用 NVS 中的值覆盖 JSON 中的频率（必须刚好 9 字符，如 "486600000"）
if(config[FREQ_RADIO0].val != NULL && config[FREQ_RADIO0].len == 9)
    strncpy(radio0_index + 8, config[FREQ_RADIO0].val, 9);
```

**为什么这么"粗暴"？** 因为 JSON 已经编译到固件里了，不能改结构，只能在解析前原地替换频率数字。限制是频率值必须恰好 9 位数字（如 `486600000`），否则长度不匹配就不替换。

### 3.3 三个 parse_* 函数

```c
parse_SX130x_configuration(conf_array);   // 解析射频硬件配置
parse_gateway_configuration(conf_array);   // 解析网关网络配置
parse_debug_configuration(conf_array);     // 解析调试配置
```

#### parse_SX130x_configuration

从 JSON 的 `"SX130x_conf"` 对象中解析：
- `com_type`: SPI 或 USB
- `com_path`: 设备路径
- `lorawan_public`: 是否使用公共 LoRaWAN syncword
- `clksrc`: 时钟源
- `antenna_gain`: 天线增益
- `radio_0` / `radio_1`: 射频链路配置（频率、类型、RSSI 温度补偿系数）
- `chan_multiSF_0` ~ `chan_multiSF_7`: 8 个 multi-SF 信道
- `chan_Lora_std`: 标准 LoRa 信道
- `chan_FSK`: FSK 信道
- TX gain LUT: 发射功率查找表

#### parse_gateway_configuration

从 JSON 的 `"gateway_conf"` 对象中解析：
- `server_address`, `serv_port_up`, `serv_port_down`: NS 地址和端口
- `keepalive_interval`: PULL_DATA 发送间隔（默认 5 秒）
- `stat_interval`: 统计报告间隔（默认 30 秒）
- `forward_crc_valid/error/nocrc`: 包过滤开关
- `gps_tty_path`, `fake_gps`: GPS 配置
- `beacon_*`: Beacon 配置

### 3.4 UDP Socket 创建与连接

```c
// NVS 的值覆盖 JSON 的默认值
if(udp_host[0] == '\0')
    strncpy(udp_host, serv_addr, sizeof udp_host);
if(udp_port == 0)
    udp_port = atoi(serv_port_up);

// 创建两个独立的 UDP socket
sock_up   = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);  // 上行专用
sock_down = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);  // 下行专用

// DNS 解析并连接
dns_loopup(udp_host, ip);
dest_addr.sin_addr.s_addr = inet_addr(ip);
dest_addr.sin_port = htons(udp_port);

// 两个 socket 都 connect 到同一个 NS 地址
connect(sock_up,   (struct sockaddr *)&dest_addr, sizeof(dest_addr));
connect(sock_down, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

// 设置上行 socket 的接收超时 = 250ms（half of PUSH_TIMEOUT_MS=500）
setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &push_timeout_half, ...);
```

**为什么用两个 socket？** `thread_up` 和 `thread_down` 运行在不同的线程中，使用独立的 socket 避免互斥锁开销，提高并发性能。

**为什么对 UDP socket 调用 `connect()`？** UDP 的 `connect()` 不会建立连接，只是绑定了远端地址，之后可以用 `send()`/`recv()` 代替 `sendto()`/`recvfrom()`，代码更简洁。

### 3.5 SX1302 复位与启动

```c
// SPI 模式下先硬件复位 SX1302
if (com_type == LGW_COM_SPI)
    lgw_reset();

// 启动集中器
i = lgw_start();
if (i == LGW_HAL_SUCCESS)
    MSG("INFO: [main] concentrator started, packet can now be received\n");
else
    exit(EXIT_FAILURE);
```

`lgw_start()` 内部做了大量工作：
1. 打开 SPI 和 I2C 端口
2. 搜索 I2C 温度传感器（0x39, 0x3B, 0x38）← **见附录A**
3. 校准 SX1250 射频前端
4. 配置所有 IF 信道和解调参数
5. 启动 SX1302 数字基带
6. 验证芯片版本号（chip version 0x10 = v1.0）

### 3.6 工作线程创建

```c
// LED 指示灯守护线程
xTaskCreatePinnedToCore(vDaemonLedIndication, "led_flash", 4096, NULL, 1, NULL, tskNO_AFFINITY);

// JIT 队列初始化（两条 RF 链路各一个队列）
jit_queue_init(&jit_queue[0]);
jit_queue_init(&jit_queue[1]);

// 上行线程（16KB 栈，优先级 6）
xTaskCreatePinnedToCore(thread_up,   "thread_up",   4096*4, NULL, 6, &pThreadUp, tskNO_AFFINITY);

// 下行线程（8KB 栈，优先级 6）
xTaskCreatePinnedToCore(thread_down, "thread_down",  4096*2, NULL, 6, NULL, tskNO_AFFINITY);

// JIT 线程（8KB 栈，优先级 6）
xTaskCreatePinnedToCore(thread_jit,  "thread_jit",   4096*2, NULL, 6, NULL, tskNO_AFFINITY);
```

`tskNO_AFFINITY` 表示线程可以在 ESP32-S3 的任意一个 CPU 核心上运行。

### 3.7 主循环：统计收集

```c
while (!exit_sig && !quit_sig) {
    // 每 5 秒刷新一次 OLED 时间显示
    while(time_count < stat_interval) {
        vTaskDelay(1000 * TIME_REFRESH / portTICK_PERIOD_MS);
        time_count += TIME_REFRESH;
        // 更新 OLED 时间
    }

    // 每 stat_interval (30s) 收集一次统计
    // 1. 加锁读取 + 清零上行统计
    xSemaphoreTake(mx_meas_up, portMAX_DELAY);
    cp_nb_rx_rcv = meas_nb_rx_rcv; meas_nb_rx_rcv = 0;
    // ... 其他字段 ...
    xSemaphoreGive(mx_meas_up);

    // 2. 加锁读取 + 清零下行统计
    xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
    // ...
    xSemaphoreGive(mx_meas_dw);

    // 3. 读取温度（加 SPI 互斥锁）
    xSemaphoreTake(mx_concent, portMAX_DELAY);
    i = lgw_get_temperature(&temperature);
    xSemaphoreGive(mx_concent);

    // 4. 打印统计报告到串口
    printf("##### %s #####\n", stat_timestamp);
    printf("# RF packets received: %u\n", cp_nb_rx_rcv);
    // ...

    // 5. 组装 JSON 状态报告（给 thread_up 发送）
    xSemaphoreTake(mx_stat_rep, portMAX_DELAY);
    snprintf(status_report, STATUS_SIZE,
        "\"stat\":{\"time\":\"%s\",\"rxnb\":%u,\"rxok\":%u,...,\"temp\":%.1f}",
        ...);
    report_ready = true;  // 通知 thread_up 下次发包时带上状态
    xSemaphoreGive(mx_stat_rep);
}
```

---

## 第四部分：三大核心线程详解

### 线程协作全景图

```
SX1302 硬件                thread_up              Network Server (NS)
   │                           │                         │
   │ 有包到达                   │                         │
   │──lgw_receive()────────────►│                         │
   │                           │ 组装JSON                 │
   │                           │──PUSH_DATA──────────────►│
   │                           │◄─PUSH_ACK────────────────│
   │                           │                         │
   │                           │        thread_down       │
   │                           │◄───────────────────────  │
   │                           │        PULL_DATA         │
   │                           │─────────────────────────►│
   │                           │◄─────────────────────────│
   │                           │        PULL_ACK          │
   │                           │◄─────────────────────────│
   │                           │        PULL_RESP(下行包)  │
   │                      jit_queue                       │
   │◄──lgw_send()──────────────│                         │
   │   thread_jit 发射          │                         │
```

### 4.1 thread_up — 上行线程

**文件**: `lora_pkt_fwd.c` 约第2145行

#### 主循环结构

```c
void thread_up(void)
{
    // 预填充固定报文头（协议版本 + 类型 + 网关MAC）
    buff_up[0] = PROTOCOL_VERSION;   // = 2
    buff_up[3] = PKT_PUSH_DATA;      // = 0
    *(unsigned int *)(buff_up + 4) = net_mac_h;  // 网关ID高32位
    *(unsigned int *)(buff_up + 8) = net_mac_l;  // 网关ID低32位

    while (!exit_sig && !quit_sig) {

        // ① 从 SX1302 取包（加 SPI 互斥锁）
        xSemaphoreTake(mx_concent, portMAX_DELAY);
        nb_pkt = lgw_receive(NB_PKT_MAX, rxpkt);  // 最多取24个包
        xSemaphoreGive(mx_concent);

        // ② 检查是否有状态报告要发
        send_report = report_ready;

        // ③ 没有包也没有报告 → 睡10ms继续
        if ((nb_pkt == 0) && (send_report == false)) {
            vTaskDelay(FETCH_SLEEP_MS / portTICK_PERIOD_MS); // 10ms
            continue;
        }

        // ④ 有包则闪烁上行LED
        if (nb_pkt > 0)
            vUplinkFlash(10);

        // ⑤ 逐包处理，组装JSON
        // ⑥ 发送UDP，等待ACK
    }
}
```

#### PUSH_DATA UDP 报文结构

```
字节偏移  长度   内容
────────  ────   ──────────────────────────────────────
0         1      协议版本 = 0x02
1-2       2      随机 token（用于匹配 ACK）
3         1      报文类型 = 0x00 (PUSH_DATA)
4-7       4      网关 MAC 高32位（网络字节序）
8-11      4      网关 MAC 低32位（网络字节序）
12+       N      JSON 字符串
```

#### 包过滤逻辑

```c
switch(p->status) {
    case STAT_CRC_OK:
        meas_nb_rx_ok += 1;
        if (!fwd_valid_pkt) continue;  // JSON 配置 forward_crc_valid=false 则丢弃
        break;
    case STAT_CRC_BAD:
        meas_nb_rx_bad += 1;
        if (!fwd_error_pkt) continue;  // 默认丢弃CRC错误包
        break;
    case STAT_NO_CRC:
        meas_nb_rx_nocrc += 1;
        if (!fwd_nocrc_pkt) continue;  // 默认丢弃无CRC包
        break;
}
```

#### JSON rxpk 字段详解

| 字段 | 来源 | 含义 |
|---|---|---|
| `tmst` | `p->count_us` | SX1302 内部自由计数器（µs），下行对齐用 |
| `chan` | `p->if_chain` | 接收信道号（0-7=multiSF, 8=std, 9=FSK） |
| `rfch` | `p->rf_chain` | 射频链路号（0或1） |
| `freq` | `p->freq_hz/1e6` | 接收频率（MHz） |
| `mid` | `p->modem_id` | 解调器ID |
| `stat` | `p->status` | CRC状态：1=OK, -1=错误, 0=无CRC |
| `modu` | `p->modulation` | 调制方式：LORA 或 FSK |
| `datr` | `p->datarate+bandwidth` | 数据速率：如 "SF12BW125" |
| `codr` | `p->coderate` | 编码率：4/5, 4/6, 4/7, 4/8 |
| `rssis` | `p->rssis` | 信号 RSSI（dBm），已温度补偿 |
| `lsnr` | `p->snr` | LoRa SNR（dB） |
| `foff` | `p->freq_offset` | 频率偏移（Hz），反映终端晶振误差 |
| `rssi` | `p->rssic` | 信道 RSSI（dBm） |
| `size` | `p->size` | 载荷字节数 |
| `data` | `p->payload` | Base64 编码的载荷 |

#### 清空旧 ACK + 发送 + 等待机制

```c
// ★ 发送前先清空 socket 缓冲区里的旧 ACK
uint8_t _tmp[4];
while (recv(sock_up, (void *)_tmp, sizeof _tmp, MSG_DONTWAIT) > 0) {}

// 发送 PUSH_DATA
send(sock_up, (void *)buff_up, buff_index, 0);

// 等待 PUSH_ACK（最多两轮，每轮 250ms）
for (i=0; i<2; ++i) {
    j = recv(sock_up, (void *)buff_ack, sizeof buff_ack, 0);
    // sock_up 已设 SO_RCVTIMEO = 250ms

    if (j == -1 && errno == EAGAIN)
        continue;  // 超时，再等一轮

    // 验证：协议版本、报文类型、token 匹配
    if (buff_ack[1]==token_h && buff_ack[2]==token_l) {
        MSG("INFO: [up] PUSH_ACK received in %i ms\n", ...);
        meas_up_ack_rcv += 1;
        vBackhaulFlash(10);  // 闪烁回传LED
        break;
    }
}
```

**为什么发送前要清空旧 ACK？**  
上一轮 PUSH_DATA 如果超时了但 ACK 其实已经在路上，会在这轮发送前抵达 socket 缓冲区。不清空的话，这个旧 ACK 会被误认为是新的 PUSH_ACK，导致 `ackr`（ACK 率）统计虚高。

**为什么等两轮？**  
网络抖动可能导致 ACK 比预期晚到，两轮共 500ms 的等待窗口增加了收到 ACK 的概率。

### 4.2 thread_down — 下行线程

**文件**: `lora_pkt_fwd.c` 约第2770行

#### 主循环结构

```c
void thread_down(void)
{
    // 预填充 PULL_DATA 报文头
    buff_req[0] = PROTOCOL_VERSION;
    buff_req[3] = PKT_PULL_DATA;  // = 0x02
    *(unsigned int *)(buff_req + 4) = net_mac_h;
    *(unsigned int *)(buff_req + 8) = net_mac_l;

    while (!exit_sig && !quit_sig) {

        // ① autoquit 检测：如果连续 N 次 PULL_DATA 都没收到 ACK，退出
        if ((autoquit_threshold > 0) && (autoquit_cnt >= autoquit_threshold))
            exit_sig = true;

        // ② 发送 PULL_DATA
        token_h = rand(); token_l = rand();
        buff_req[1] = token_h; buff_req[2] = token_l;
        send(sock_down, buff_req, sizeof buff_req, 0);

        // ③ 在 keepalive_time(10s) 时间窗口内持续监听
        while (difftimespec(recv_time, send_time) < keepalive_time) {
            msg_len = recv(sock_down, buff_down, sizeof buff_down, 0);
            // sock_down 设 SO_RCVTIMEO = 400ms
            // 每次超时就继续循环，直到 keepalive_time 到期

            if (msg_len == -1) continue;  // 超时

            // ④ 处理 PULL_ACK
            if (buff_down[3] == PKT_PULL_ACK) { ... }

            // ⑤ 处理 PULL_RESP（下行包）
            if (buff_down[3] == PKT_PULL_RESP) { ... }
        }
    }
}
```

#### PULL_DATA / PULL_ACK 时序

```
thread_down                              NS
    │                                     │
    │──PULL_DATA(token=0xA1B2)───────────►│
    │◄─PULL_ACK(token=0xA1B2)────────────│  (实测约143ms)
    │                                     │
    │  ...监听最多 10 秒...                │
    │                                     │
    │  (如果有下行包):                      │
    │◄─PULL_RESP(txpk={...})─────────────│
    │  解析JSON → jit_enqueue()           │
    │──TX_ACK(token)─────────────────────►│
    │                                     │
    │──PULL_DATA(token=0xC3D4)───────────►│ (10秒后再来一次)
```

#### PULL_RESP 解析 — 三种下行模式

| 模式 | JSON 字段 | LoRaWAN Class | 说明 |
|---|---|---|---|
| 立即发射 | `"imme": true` | Class C | 不需要时间戳，立刻发 |
| 时间戳发射 | `"tmst": 3784146` | Class A | 在指定 SX1302 计数器值时发 |
| GPS时间发射 | `"tmms": 1234567890` | Class B | 在指定 GPS 毫秒时刻发 |

**Class A（最常见）：** NS 把上行包的 `tmst` 加上 RX1Delay（1秒）作为下行的 `tmst`。

#### PULL_RESP 处理流程

```c
// 解析 JSON → 填充 txpkt 结构体
// 包含: freq, rfch, powe, modu, datr, codr, ipol, size, data

// 检查频率范围
if (txpkt.freq_hz < tx_freq_min[...] || txpkt.freq_hz > tx_freq_max[...])
    jit_result = JIT_ERROR_TX_FREQ;

// 检查发射功率（查表找最接近的支持值）
get_tx_gain_lut_index(txpkt.rf_chain, txpkt.rf_power, &tx_lut_idx);

// 放入 JIT 队列（不立即发射）
jit_enqueue(&jit_queue[txpkt.rf_chain], current_concentrator_time, &txpkt, downlink_type);

// 回复 TX_ACK 给 NS
send_tx_ack(buff_down[1], buff_down[2], jit_result, warning_value);
```

### 4.3 thread_jit — JIT 定时发射线程

**文件**: `lora_pkt_fwd.c` 约第3456行

JIT = Just In Time（恰好及时）。下行包不能想发就发，必须在**精确的时刻**发射，否则终端节点的接收窗口已关闭。

#### 主循环结构

```c
void thread_jit(void)
{
    while (!exit_sig && !quit_sig) {
        vTaskDelay(10 / portTICK_PERIOD_MS);  // 每 10ms 检查一次

        for (i = 0; i < LGW_RF_CHAIN_NB; i++) {  // 遍历两条 RF 链路

            // ① 读取当前 SX1302 计数器值
            xSemaphoreTake(mx_concent, portMAX_DELAY);
            lgw_get_instcnt(&current_concentrator_time);
            xSemaphoreGive(mx_concent);

            // ② 查看队列头部是否到期
            jit_result = jit_peek(&jit_queue[i], current_concentrator_time, &pkt_index);

            if (jit_result == JIT_ERROR_OK && pkt_index > -1) {

                // ③ 取出包
                jit_dequeue(&jit_queue[i], pkt_index, &pkt, &pkt_type);

                // ④ Beacon 频率补偿（晶振校正）
                if (pkt_type == JIT_PKT_TYPE_BEACON) {
                    xSemaphoreTake(mx_xcorr, portMAX_DELAY);
                    pkt.freq_hz = (unsigned int)(xtal_correct * (double)pkt.freq_hz);
                    xSemaphoreGive(mx_xcorr);
                }

                // ⑤ 检查 TX 状态
                lgw_status(pkt.rf_chain, TX_STATUS, &tx_status);
                if (tx_status == TX_EMITTING) continue;  // 正在发射，跳过

                // ⑥ 发射！
                xSemaphoreTake(mx_concent, portMAX_DELAY);
                result = lgw_send(&pkt);
                xSemaphoreGive(mx_concent);

                if (result == LGW_HAL_SUCCESS) {
                    meas_nb_tx_ok += 1;
                    vDownlinkFlash(10);  // 闪烁下行LED
                } else {
                    meas_nb_tx_fail += 1;
                }
            }
        }
    }
}
```

#### JIT 队列时间线示例

```
上行包到达: tmst = 3,784,146 µs
                │
                │  NS 收到上行包，计算下行时刻
                │  RX1: tmst + 1,000,000 = 4,784,146 µs
                │
thread_down: jit_enqueue(count_us=4,784,146)
                │
                │  thread_jit 每 10ms 检查
                │
current_time ≈ 4,783,146 µs  → jit_peek(): "还差约1ms，时机已到"
                │
                ▼
            lgw_send(&pkt)  → SPI 写入 SX1302
                │
current_time = 4,784,146 µs
                │
                ▼
            SX1302 硬件在精确时刻发射射频信号
            终端节点的 RX1 窗口正好打开 → 成功接收
```

### 4.4 三线程协作完整数据流

以一次完整的 Class A 上下行交互为例：

```
时间轴 ──────────────────────────────────────────────────────────►

[SX1302 硬件]
  t=3,784,146µs: 终端发来上行包，SX1302 接收完成

[thread_up] (每10ms轮询)
  t≈3,784,156µs: lgw_receive() 取出包
                 CRC_OK → 通过过滤
                 组装JSON: {"rxpk":[{"tmst":3784146,...}]}
                 send(sock_up, PUSH_DATA, ...)
                 ↓蓝灯闪
                 recv() 等待 ACK...
                 收到 PUSH_ACK → ↓绿灯闪

[NS 服务器]
  收到 PUSH_DATA
  计算 RX1: tmst = 3784146 + 1000000 = 4784146
  发送 PULL_RESP: {"txpk":{"tmst":4784146,"freq":505.3,...}}

[thread_down] (一直在 recv 等待)
  收到 PULL_RESP
  解析JSON → txpkt.count_us = 4784146
  jit_enqueue(&jit_queue[0], ..., CLASS_A)
  send_tx_ack(JIT_ERROR_OK) → ↓绿灯闪

[thread_jit] (每10ms检查)
  t≈4,783,156µs: jit_peek() → 时机已到！
                 jit_dequeue() → 取出 txpkt
                 lgw_status() → TX_FREE
                 lgw_send(&txpkt)
                 ↓红灯闪

[SX1302 硬件]
  t=4,784,146µs: 精确时刻发射下行射频信号
  终端节点 RX1 窗口打开 → 成功接收下行包
```

#### LED 三色指示

```c
vUplinkFlash(10);   // thread_up: 上行有包到达  → 蓝灯闪
vBackhaulFlash(10); // ACK 收到（上行/下行）    → 绿灯闪
vDownlinkFlash(10); // thread_jit: 发射成功     → 红灯闪
```

---

## 附录A：温度传感器问题详解（踩坑记录）

### 问题现象

启动日志中出现以下警告和错误：

```
WARNING: failed to configure temperature sensor on port 0x39
WARNING: failed to configure temperature sensor on port 0x3B
WARNING: failed to configure temperature sensor on port 0x38
WARNING: no temperature sensor found.
```

运行时每次 `lgw_receive()` 都打印：

```
ERROR: failed to read I2C device 0x38 (err=-1)
ERROR: failed to get current temperature
```

### 根因分析

#### 谁在读温度？

HAL 库中有两个地方读取温度：

1. **`lgw_receive()` 内部**：每次从 SX1302 取包时，调用 `lgw_get_temperature()` 获取当前温度，用于 RSSI 温度补偿。**这就是每次收包都打印错误的原因。**

2. **主循环统计**：每 30 秒调用一次 `lgw_get_temperature()`，用于状态报告和 OLED 显示。

#### 温度用在哪？

```c
// loragw_hal.c 中 lgw_receive() 的处理
res = lgw_get_temperature(&current_temperature);
// ...
// 用温度做 RSSI 补偿
rssi_offset = sx1302_rssi_get_temperature_offset(&rssi_tcomp, current_temperature);
// offset = a*T^4 + b*T^3 + c*T^2 + d*T + e
```

这是对 SX1250 射频前端接收到的 RSSI 值进行温度补偿。不同温度下 LNA（低噪声放大器）增益会漂移，需要用温度系数多项式来修正。

#### 温度传感器是哪个？

HAL 期望的是 Semtech CoreCell 参考设计上的**外部 I2C 温度传感器** STTS751（ST 半导体），地址 0x39/0x3B/0x38。这颗芯片放置在 SX1302 模块附近，测量的是 RF 芯片周围的环境温度。

```c
// loragw_stts751.h
static const uint8_t I2C_PORT_TEMP_SENSOR[] = {0x39, 0x3B, 0x38};
```

#### 为什么我的板子没有？

我使用的是现成的 SX1302 模组，模组内部有 TCXO（温度补偿晶振）但**没有焊接 STTS751 温度传感器**——这颗芯片是 Semtech 参考设计的一部分，并非 SX1302 的必需组件。

### TCXO 与温度补偿的区别

| 项目 | TCXO | RSSI 温度补偿（代码中的） |
|---|---|---|
| 补偿什么 | 26MHz 参考时钟频率漂移 | RSSI 读数随温度的偏移 |
| 硬件 | 模组内置的温补晶振 | 外部 I2C 温度传感器(STTS751) |
| 是否需要代码参与 | 否（纯硬件自动补偿） | 是（需要读取温度计算补偿量） |
| 影响 | 频率精度 | RSSI 精度 |

### 实际影响

**功能不受影响。** `lgw_get_temperature()` 失败时，HAL 使用默认温度值（上次成功读取的值或初始值 25°C）计算补偿。对于室内固定网关，温度变化范围不大，RSSI 误差在可接受范围内。

### 为什么不能用 ESP32-S3 内置温度传感器代替？

**不行。** ESP32-S3 内置 tsens 测量的是 MCU 内核温度，在 CPU 负载下比环境温度高 10~30°C。用 MCU 温度去补偿 RF 前端的 RSSI，反而会引入更大的系统误差。

### 如果要彻底解决

在 SX1302 模组附近的 I2C 总线（SDA=GPIO4, SCL=GPIO5）上焊接一颗兼容的温度传感器芯片：
- **STTS751**（ST 半导体，HAL 原生支持）
- **MCP9808**（Microchip，地址兼容）
- **SE97B**（NXP，地址兼容）

SO8 封装，几毛钱，放在模组旁边即可。

---

## 附录B：互斥量使用汇总

```
mx_concent (SPI 总线保护 — 最重要):
    thread_up    → lgw_receive()
    thread_jit   → lgw_get_instcnt() / lgw_status() / lgw_send()
    thread_down  → lgw_get_instcnt()（用于 jit_enqueue 获取当前时间）
    pkt_fwd_main → lgw_get_instcnt() / lgw_get_trigcnt() / lgw_get_temperature()

mx_meas_up (上行统计):
    thread_up    → 写入 meas_nb_rx_* / meas_up_*
    pkt_fwd_main → 读取并清零

mx_meas_dw (下行统计):
    thread_down  → 写入 meas_dw_* / meas_nb_tx_requested
    thread_jit   → 写入 meas_nb_tx_ok / meas_nb_tx_fail / meas_nb_beacon_sent
    pkt_fwd_main → 读取并清零

mx_stat_rep (状态报告):
    pkt_fwd_main → 写入 status_report, 设 report_ready=true
    thread_up    → 读取 status_report, 清 report_ready=false

mx_xcorr (晶振校正):
    thread_valid → 写入 xtal_correct（GPS模式下，当前代码中禁用）
    thread_jit   → 读取 xtal_correct（用于 Beacon 频率补偿）

mx_timeref (GPS 时间参考):
    thread_gps   → 写入 time_reference_gps（当前禁用）
    thread_up    → 读取 local_ref（用于 UTC 时间戳转换）
    thread_down  → 读取（用于 Beacon 调度和 GPS 时间发射）

mx_meas_gps (GPS 坐标):
    thread_gps   → 写入 gps_coord_valid / meas_gps_coord
    pkt_fwd_main → 读取用于统计显示
```

---

## 附录C：关键常量速查表

| 常量 | 值 | 含义 |
|---|---|---|
| `PROTOCOL_VERSION` | 2 | Semtech 协议 v1.6 |
| `NB_PKT_MAX` | 24 | 每次 lgw_receive 最多取的包数 |
| `FETCH_SLEEP_MS` | 10 | 无包时的轮询间隔(ms) |
| `PUSH_TIMEOUT_MS` | 500 | PUSH_ACK 总等待时间(ms) |
| `PULL_TIMEOUT_MS` | 400 | PULL_ACK 单次 recv 超时(ms) |
| `DEFAULT_KEEPALIVE` | 5 | PULL_DATA 发送间隔(s) |
| `DEFAULT_STAT` | 30 | 统计报告间隔(s) |
| `WIFI_MAXIMUM_RETRY` | 5 | WiFi 连接最大重试次数 |
| `TX_BUFF_SIZE` | ~13230 | 上行 UDP 缓冲区大小 |
| `PKT_PUSH_DATA` | 0 | 上行数据包类型 |
| `PKT_PUSH_ACK` | 1 | 上行确认包类型 |
| `PKT_PULL_DATA` | 2 | 下行拉取请求类型 |
| `PKT_PULL_RESP` | 3 | 下行数据响应类型 |
| `PKT_PULL_ACK` | 4 | 下行拉取确认类型 |
| `PKT_TX_ACK` | 5 | 发射确认包类型 |

---

## 附录D：硬件引脚映射

文件: `main/board_config.h`

| 功能 | GPIO | 说明 |
|---|---|---|
| SPI MISO | 13 | SX1302 数据输出 |
| SPI MOSI | 11 | SX1302 数据输入 |
| SPI CLK | 12 | SPI 时钟 |
| SPI CS | 14 | SX1302 片选 |
| SX1302 RESET | 2 | 硬件复位（低有效） |
| I2C SDA | 4 | OLED / 温度传感器 |
| I2C SCL | 5 | OLED / 温度传感器 |
| 心跳 LED | 1 | 启动完成后熄灭 |
| 蓝色 LED | 33 | 上行包指示 |
| 绿色 LED | 7 | 回传通信指示 |
| 红色 LED | 27 | 下行发射指示 |
| 用户按钮1 | 0 | IO0，强制进 Soft-AP |
| 用户按钮2 | 6 | IO6，预留 |

---

## 附录E：运行时日志解读参考

以下是一段实际运行日志的关键部分注释：

```
*** ESXP1302 Gateway. Version: 1.0.6 ***        ← 固件版本

wifi_ssid: 304                                    ← NVS 读取的配置
ns_host: 192.168.71.108
ns_port: 1700
gw_id: AA555A00000021FB
freq_region: cn470
freq_radio0: 486600000                            ← Radio0 中心频率 486.6MHz
freq_radio1: 487400000                            ← Radio1 中心频率 487.4MHz

INFO: concentrator started                        ← SX1302 启动成功

WARNING: no temperature sensor found.             ← 正常，没有外部 STTS751

INFO: Received pkt from mote: 6F6C6C65 (fcnt=28535) ← 收到终端上行包
JSON up: {"rxpk":[{..., "data":"aGVsbG8gd29ybGQ="}]} ← Base64 = "hello world"
INFO: [up] PUSH_ACK received in 2 ms             ← NS 确认收到

INFO: [down] PULL_ACK received in 143 ms          ← NS 存活确认

ERROR: failed to get current temperature          ← 温度读取失败（见附录A）
### Concentrator temperature unknown ###           ← 温度不可用，不影响功能

##### 2026-02-22 01:23:45 UTC #####               ← 30秒统计报告
# RF packets received: 3
# CRC_OK: 100.00%
# PUSH_DATA acknowledged: 100.00%                ← 所有上行包 NS 都确认了
# PULL_DATA sent: 3 (100.00% acknowledged)        ← 与 NS 通信正常
```
