# ESXP1302 LoRaWAN 缃戝叧浠ｇ爜瀹屾暣瑙ｆ瀽

> 鏃ユ湡锛?026-02-22  
> 椤圭洰锛歟sxp1302_on_esp32s3  
> 骞冲彴锛欵SP32-S3 + SX1302/SX1303 LoRa 闆嗕腑鍣? 
> 妗嗘灦锛欵SP-IDF v5.4.3 + FreeRTOS  
> HAL锛歋emtech sx1302_hal v2.1.0  
> 鍗忚锛歋emtech Packet Forwarder Protocol v2 (UDP)

---

## 鐩綍

- [绗竴閮ㄥ垎锛歛pp\_main() 鍏ュ彛鍙婂惎鍔ㄦ祦绋媇(#绗竴閮ㄥ垎app_main-鍏ュ彛鍙婂惎鍔ㄦ祦绋?
  - [1.1 OLED 鍒濆鍖栦笌鎸夐挳妫€娴媇(#11-oled-鍒濆鍖栦笌鎸夐挳妫€娴?
  - [1.2 NVS 閰嶇疆绯荤粺锛堟繁鍏ワ級](#12-nvs-閰嶇疆绯荤粺娣卞叆)
  - [1.3 WiFi 妯″紡鍐崇瓥閫昏緫](#13-wifi-妯″紡鍐崇瓥閫昏緫)
  - [1.4 涓插彛鎺у埗鍙?REPL锛堟繁鍏ワ級](#14-涓插彛鎺у埗鍙?repl娣卞叆)
- [绗簩閮ㄥ垎锛歐iFi 杩炴帴涓庝簨浠堕┍鍔ㄦ灦鏋刔(#绗簩閮ㄥ垎wifi-杩炴帴涓庝簨浠堕┍鍔ㄦ灦鏋?
  - [2.1 wifi\_init\_sta() 璇﹁В](#21-wifi_init_sta-璇﹁В)
  - [2.2 ESP 浜嬩欢寰幆鏈哄埗锛堟繁鍏ワ級](#22-esp-浜嬩欢寰幆鏈哄埗娣卞叆)
  - [2.3 wifi\_sta\_event\_handler 涓変釜鐘舵€乚(#23-wifi_sta_event_handler-涓変釜鐘舵€?
- [绗笁閮ㄥ垎锛歱kt\_fwd\_main() 鏍稿績鍒濆鍖朷(#绗笁閮ㄥ垎pkt_fwd_main-鏍稿績鍒濆鍖?
  - [3.1 浜掓枼閲忓垱寤篯(#31-浜掓枼閲忓垱寤?
  - [3.2 JSON 閰嶇疆鍔犺浇涓庨鐜囪ˉ涓乚(#32-json-閰嶇疆鍔犺浇涓庨鐜囪ˉ涓?
  - [3.3 涓変釜 parse\_\* 鍑芥暟](#33-涓変釜-parse_-鍑芥暟)
  - [3.4 UDP Socket 鍒涘缓涓庤繛鎺(#34-udp-socket-鍒涘缓涓庤繛鎺?
  - [3.5 SX1302 澶嶄綅涓庡惎鍔╙(#35-sx1302-澶嶄綅涓庡惎鍔?
  - [3.6 宸ヤ綔绾跨▼鍒涘缓](#36-宸ヤ綔绾跨▼鍒涘缓)
  - [3.7 涓诲惊鐜細缁熻鏀堕泦](#37-涓诲惊鐜粺璁℃敹闆?
- [绗洓閮ㄥ垎锛氫笁澶ф牳蹇冪嚎绋嬭瑙(#绗洓閮ㄥ垎涓夊ぇ鏍稿績绾跨▼璇﹁В)
  - [4.1 thread\_up 鈥?涓婅绾跨▼](#41-thread_up--涓婅绾跨▼)
  - [4.2 thread\_down 鈥?涓嬭绾跨▼](#42-thread_down--涓嬭绾跨▼)
  - [4.3 thread\_jit 鈥?JIT 瀹氭椂鍙戝皠绾跨▼](#43-thread_jit--jit-瀹氭椂鍙戝皠绾跨▼)
  - [4.4 涓夌嚎绋嬪崗浣滃畬鏁存暟鎹祦](#44-涓夌嚎绋嬪崗浣滃畬鏁存暟鎹祦)
- [闄勫綍A锛氭俯搴︿紶鎰熷櫒闂璇﹁В锛堣俯鍧戣褰曪級](#闄勫綍a娓╁害浼犳劅鍣ㄩ棶棰樿瑙ｈ俯鍧戣褰?
- [闄勫綍B锛氫簰鏂ラ噺浣跨敤姹囨€籡(#闄勫綍b浜掓枼閲忎娇鐢ㄦ眹鎬?
- [闄勫綍C锛氬叧閿父閲忛€熸煡琛╙(#闄勫綍c鍏抽敭甯搁噺閫熸煡琛?
- [闄勫綍D锛氱‖浠跺紩鑴氭槧灏刔(#闄勫綍d纭欢寮曡剼鏄犲皠)

---

## 绗竴閮ㄥ垎锛歛pp_main() 鍏ュ彛鍙婂惎鍔ㄦ祦绋?
**鏂囦欢**: `main/packet_forwarder/lora_pkt_fwd.c` 绾︾4336琛?
### 1.1 OLED 鍒濆鍖栦笌鎸夐挳妫€娴?
`app_main()` 鏄?ESP-IDF 鐨勫敮涓€鍏ュ彛鍑芥暟锛岀瓑鍚屼簬 Linux 鐨?`main()`銆傚惎鍔ㄩ『搴忓涓嬶細

```c
void app_main(void)
{
    // 鈶?鎵撳嵃鐗堟湰鍙?    printf("\n\n*** ESXP1302 Gateway. Version: %s ***\n\n\n", EXSP1302_VERSION);

    // 鈶?OLED 鍒濆鍖栧苟鏄剧ず鍚姩鐢婚潰
    oled_init();
    oled_cls();
    oled_show_str(0, 0, "ESXP1302 GATEWAY", 2);

    // 鈶?閰嶇疆涓や釜鐢ㄦ埛鎸夐挳鐨?GPIO锛圛O0 鍜?IO6锛?    // 鎸夐挳鏄綆鐢靛钩鏈夋晥锛圔UTTON_PRESSED=0锛夛紝鎵€浠ュ紑鍚笂鎷?    if(USER_BUTTON_1 != GPIO_NUM_NC){
        gpio_set_direction(USER_BUTTON_1, GPIO_MODE_INPUT);
        gpio_pullup_en(USER_BUTTON_1);
    }
    // USER_BUTTON_2 鍚岀悊

    // 鈶?浠?NVS 璇诲彇閰嶇疆锛堣 1.2 鑺傝瑙ｏ級
    read_config_from_nvs();

    // 鈶?鍒ゆ柇 WiFi 妯″紡锛堣 1.3 鑺傦級
    // 鈶?鍚姩 HTTP 鏈嶅姟鍜?REPL 鎺у埗鍙帮紙瑙?1.4 鑺傦級
}
```

### 1.2 NVS 閰嶇疆绯荤粺锛堟繁鍏ワ級

**鏂囦欢**: `main/packet_forwarder/web_config.c` + `web_config.h`

NVS锛圢on-Volatile Storage锛夋槸 ESP-IDF 鎻愪緵鐨勯敭鍊煎瀛樺偍绯荤粺锛屾暟鎹繚瀛樺湪 Flash 鐨勪笓鐢ㄥ垎鍖轰腑锛屾帀鐢典笉涓㈠け銆?
#### 鏁版嵁缁撴瀯

```c
// web_config.h
typedef enum {
    WIFI_SSID = 0, WIFI_PASSWORD, NS_HOST, NS_PORT, GW_ID,
    WIFI_MODE, FREQ_REGION, FREQ_RADIO0, FREQ_RADIO1, NTP_SERVER,
    CONFIG_NUM,      // = 10锛屾€诲叡10涓厤缃」
    CONFIG_ERR = 255
} tag_e;

typedef struct {
    tag_e tag;       // 鏋氫妇绱㈠紩
    char name[16];   // NVS 涓殑閿悕
    char *val;       // 鍊硷紙鍔ㄦ€佸垎閰嶇殑瀛楃涓诧級
    int len;         // 瀛楃涓查暱搴?} config_s;
```

閰嶇疆琛ㄥ垵濮嬪寲锛?
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

#### 鍒濆鍖栨祦绋?
```c
esp_err_t init_config_storage(void)
{
    esp_err_t err = nvs_flash_init();
    // 濡傛灉 NVS 鍒嗗尯鎹熷潖锛堢増鏈笉鍖归厤鎴栨弧浜嗭級锛屽厛鎿﹂櫎鍐嶅垵濮嬪寲
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if(err == ESP_OK)
        nvs_ready = true;
    return err;
}
```

#### 璇诲彇娴佺▼锛堜袱姝ヨ皟鐢ㄦā寮忥級

```c
esp_err_t read_config(void)
{
    nvs_handle_t my_handle;
    nvs_open("nvs", NVS_READONLY, &my_handle);  // 鎵撳紑 namespace "nvs"

    for(int i = 0; i < CONFIG_NUM; i++){
        // 绗竴姝ワ細浼?NULL 鑾峰彇鍊肩殑闀垮害
        err = nvs_get_str(my_handle, config[i].name, NULL, &len);

        // 绗簩姝ワ細鏍规嵁闀垮害 malloc锛屽啀浼犲叆鐪熸鐨勭紦鍐插尯
        p = malloc(len);
        err = nvs_get_str(my_handle, config[i].name, p, &len);

        config[i].val = p;
        config[i].len = len - 1;  // 涓嶇畻 \0
    }
    nvs_close(my_handle);
}
```

**涓轰粈涔堥渶瑕佷袱姝ヨ皟鐢紵**  
鍥犱负 NVS 涓瓨鍌ㄧ殑瀛楃涓查暱搴︿笉鍥哄畾锛屽繀椤诲厛鏌ヨ闀垮害鍐嶅垎閰嶅唴瀛橈紝閬垮厤娴垂鎴栨孩鍑恒€傝繖鏄?ESP-IDF NVS API 鐨勬爣鍑嗙敤娉曘€?
#### 鍐欏叆娴佺▼

```c
int save_config(void)
{
    nvs_handle_t my_handle;
    nvs_open("nvs", NVS_READWRITE, &my_handle);

    for(int i = 0; i < CONFIG_NUM; i++) {
        if(config[i].val != NULL)
            nvs_set_str(my_handle, config[i].name, config[i].val);
    }
    nvs_commit(my_handle);  // 鍏抽敭锛佷笉 commit 鏁版嵁涓嶄細鍐欏叆 Flash
    nvs_close(my_handle);
}
```

**`nvs_commit()` 鐨勫繀瑕佹€э細** `nvs_set_str()` 鍙槸鎶婃暟鎹啓鍏?RAM 缂撳啿鍖猴紝蹇呴』璋冪敤 `nvs_commit()` 鎵嶈兘鐪熸鎸佷箙鍖栧埌 Flash銆?
#### 閰嶇疆鏉ユ簮浼樺厛绾?
NVS 涓殑鍊间細瑕嗙洊 JSON 涓殑榛樿鍊硷紙鍦?`pkt_fwd_main()` 涓疄鐜帮級锛?
```
JSON 榛樿鍊?(global_cn_conf) 鈫?NVS 瑕嗙洊 (濡傛灉鏈夊€?
                                鈫?                        Web 椤甸潰 / CLI 淇敼 鈫?save_config() 鈫?NVS
```

### 1.3 WiFi 妯″紡鍐崇瓥閫昏緫

WiFi 妯″紡鍒ゆ柇鐨勪紭鍏堢骇閾撅細

```c
if (USER_BUTTON_1 鎸変笅)         鈫?Soft-AP 妯″紡锛堝己鍒讹級
else if (USER_BUTTON_2 鎸変笅)     鈫?Station 妯″紡锛堝己鍒讹級
else if (NVS 涓?wifi_mode 鏈缃? 鈫?Soft-AP 妯″紡锛堥娆″惎鍔ㄩ粯璁わ級
else if (NVS 涓?wifi_mode == "soft_ap") 鈫?Soft-AP 妯″紡
else                              鈫?Station 妯″紡
```

**鍏抽敭璁捐锛氬紑鏈哄墠鎸変綇 IO0 鎸夐挳鍙互寮哄埗杩涘叆 Soft-AP 閰嶇疆妯″紡**锛岃繖鏄?鏁戠爾"鎵嬫鈥斺€斿嵆浣?WiFi 瀵嗙爜閰嶉敊浜嗭紝涔熻兘閫氳繃鎸夐挳杩涘叆 AP 妯″紡閲嶆柊閰嶇疆銆?
#### Soft-AP 妯″紡鍚姩

```c
if(soft_ap_mode == true){
    // 鍏堟妸 wifi_mode 鏀逛负 station 骞朵繚瀛樺埌 NVS
    // 杩欐牱涓嬫閲嶅惎灏变笉浼氬啀杩?AP 妯″紡
    config_wifi_mode(WIFI_MODE_STATION);

    // 璁剧疆 10 鍒嗛挓鍚庤嚜鍔ㄩ噸鍚紙闃叉鐢ㄦ埛蹇樹簡閰嶇疆灏辩寮€锛?    reboot_delay_s = 60 * 10;
    reboot_flag = true;
    start_reboot_timer_ms(reboot_delay_s * 1000);

    wifi_init_soft_ap();
    // OLED 鏄剧ず: IP=192.168.4.1, Soft AP mode, SSID=esp32, PSWD=esp32wifi
}
```

#### Station 妯″紡鍚姩

```c
else {
    // 鍏堟妸 wifi_mode 鏀逛负 soft_ap 骞朵繚瀛?    // 杩欐牱濡傛灉杩炰笉涓?WiFi锛屼笅娆￠噸鍚氨浼氳繘 AP 妯″紡
    config_wifi_mode(WIFI_MODE_SOFT_AP);

    // 璁剧疆 5 鍒嗛挓鍚庤嚜鍔ㄩ噸鍚?    reboot_delay_s = 60 * 5;
    reboot_flag = true;
    start_reboot_timer_ms(reboot_delay_s * 1000);

    wifi_init_sta();  // 鈫?璇﹁绗簩閮ㄥ垎
}
```

**妯″紡鍒囨崲鐨勮嚜淇濇姢鏈哄埗锛?*

| 鍔ㄤ綔 | NVS wifi_mode 鍐欏叆鍊?| 涓轰粈涔?|
|------|------|------|
| 杩涘叆 Soft-AP | `"station"` | 涓嬫閲嶅惎鑷姩鍥?Station锛堜笉鍗″湪閰嶇疆妯″紡锛?|
| 杩涘叆 Station | `"soft_ap"` | 濡傛灉 WiFi 杩炰笉涓婏紝涓嬫閲嶅惎鑷姩杩?Soft-AP锛堝彲閲嶆柊閰嶇疆锛?|
| Station 杩炴帴鎴愬姛 | `"station"` | WiFi 纭鍙敤锛屼繚鎸?Station 妯″紡 |

### 1.4 涓插彛鎺у埗鍙?REPL锛堟繁鍏ワ級

REPL锛圧ead-Eval-Print Loop锛夋槸閫氳繃涓插彛锛圲SB-UART锛夊疄鐜扮殑鍛戒护琛屼氦浜掔晫闈€?
#### 鍒濆鍖?
```c
// 閰嶇疆 REPL
esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
repl_config.task_stack_size = 4096 * 2;  // 8KB 鏍?repl_config.prompt = "ESXP1302_GW>";     // 鍛戒护鎻愮ず绗?
// 娉ㄥ唽鑷畾涔夊懡浠?usage();           // 鎵撳嵃甯姪淇℃伅
register_config(); // 娉ㄥ唽 "pkt_fwd" 鍛戒护

// 鍒涘缓 UART REPL 骞跺惎鍔?esp_console_repl_t *repl = NULL;
esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
ESP_ERROR_CHECK(esp_console_start_repl(repl));
```

#### 鍛戒护娉ㄥ唽锛坅rgtable3锛?
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
    // 瀹氫箟姣忎釜鍙傛暟
    net_conf_args.wifi_ssid  = arg_str0(NULL, "ssid", "<SSID>", "SSID of AP");
    net_conf_args.wifi_pswd  = arg_str0(NULL, "pswd", "<Password>", "Password of AP");
    net_conf_args.udp_host   = arg_str0(NULL, "host", "<UDP Host>", "UDP Host");
    net_conf_args.udp_port   = arg_int0(NULL, "port", "<UDP Port>", "UDP Port");
    net_conf_args.gw_id      = arg_str0(NULL, "gwid", "<gateway id>", "Gateway Id");
    net_conf_args.end = arg_end(2);

    // 娉ㄥ唽鍒?esp_console
    const esp_console_cmd_t hal_conf_cmd = {
        .command = "pkt_fwd",
        .help = "ESP32 packet forwarder based on sx1302_hal",
        .func = &do_net_config_cmd,     // 鍥炶皟鍑芥暟
        .argtable = &net_conf_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&hal_conf_cmd));
}
```

**鐢ㄦ硶绀轰緥锛?*
```
ESXP1302_GW> pkt_fwd --ssid "MyWiFi" --pswd "12345678" --host "192.168.1.100" --port 1700
```

#### 鍛戒护澶勭悊鍑芥暟

```c
static int do_net_config_cmd(int argc, char **argv)
{
    // argtable3 瑙ｆ瀽鍙傛暟
    int nerrors = arg_parse(argc, argv, (void **)&net_conf_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, net_conf_args.end, argv[0]);
        return 1;
    }

    // 閫愪釜妫€鏌ュ苟鏇存柊 config[]
    if (net_conf_args.wifi_ssid->count > 0) {
        // 鏇存柊 config[WIFI_SSID]
    }
    // ... 鍏朵粬鍙傛暟鍚岀悊

    save_config();  // 淇濆瓨鍒?NVS

    // 鍙栨秷閲嶅惎瀹氭椂鍣紙璇存槑鐢ㄦ埛閫氳繃 CLI 鍦ㄧ嚎淇敼浜嗛厤缃級
    reboot_flag = false;
    // 鏍囪闇€瑕侀噸鍚互搴旂敤鏂伴厤缃?    printf("Config saved. Please reboot to apply.\n");
    return 0;
}
```

**`reboot_flag = false` 鐨勬剰涔夛細** 鍦?Station 妯″紡涓嬶紝绯荤粺榛樿璁句簡 5 鍒嗛挓閲嶅惎瀹氭椂鍣紙鎬曡繛涓嶄笂 WiFi 鍗℃锛夈€傚鏋滅敤鎴烽€氳繃 CLI 鎴愬姛淇敼浜嗛厤缃紙姣斿鏀逛簡 WiFi 瀵嗙爜锛夛紝璇存槑涓插彛鏄€氱殑锛岀敤鎴锋湁鎺у埗鏉冿紝灏卞彇娑堣嚜鍔ㄩ噸鍚€?
---

## 绗簩閮ㄥ垎锛歐iFi 杩炴帴涓庝簨浠堕┍鍔ㄦ灦鏋?
### 2.1 wifi_init_sta() 璇﹁В

```c
void wifi_init_sta(void)
{
    // 鈶?鍒涘缓 FreeRTOS 浜嬩欢缁勶紙鐢ㄤ簬鍚屾绛夊緟锛?    s_wifi_event_group = xEventGroupCreate();

    // 鈶?鍒濆鍖?TCP/IP 鍗忚鏍?    ESP_ERROR_CHECK(esp_netif_init());

    // 鈶?鍒涘缓榛樿浜嬩欢寰幆锛堝叏灞€鍗曚緥锛?    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 鈶?鍒涘缓榛樿鐨?STA 缃戠粶鎺ュ彛
    esp_netif_create_default_wifi_sta();

    // 鈶?WiFi 鍒濆鍖?    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 鈶?娉ㄥ唽浜嬩欢澶勭悊鍑芥暟
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,            // 浜嬩欢鍩?        ESP_EVENT_ANY_ID,      // 璁㈤槄鎵€鏈?WiFi 浜嬩欢
        &wifi_sta_event_handler,
        NULL,
        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,              // IP 浜嬩欢鍩?        IP_EVENT_STA_GOT_IP,   // 鍙闃?鑾峰彇鍒癐P"浜嬩欢
        &wifi_sta_event_handler,
        NULL,
        &instance_got_ip));

    // 鈶?閰嶇疆骞跺惎鍔?WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ...,       // 浠?config[WIFI_SSID] 鏉?            .password = ...,   // 浠?config[WIFI_PASSWORD] 鏉?            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());  // 瑙﹀彂 WIFI_EVENT_STA_START
}
```

**娉ㄦ剰锛歚esp_wifi_start()` 鏄潪闃诲鐨勩€?* 瀹冨彧鏄憡璇?WiFi 椹卞姩"寮€濮嬪惂"锛岀劧鍚庣珛鍗宠繑鍥炪€傜湡姝ｇ殑杩炴帴杩囩▼鏄紓姝ョ殑锛岄€氳繃浜嬩欢鍥炶皟閫氱煡缁撴灉銆?
### 2.2 ESP 浜嬩欢寰幆鏈哄埗锛堟繁鍏ワ級

ESP-IDF 鐨勪簨浠跺惊鐜槸涓€涓?*鍙戝竷-璁㈤槄**妯″紡鐨勬秷鎭郴缁燂細

```
                    鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?                    鈹?     榛樿浜嬩欢寰幆            鈹?                    鈹?  (杩愯鍦ㄧ嫭绔嬬殑绯荤粺浠诲姟涓?    鈹?                    鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?                               鈹?            鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?            鈹?                 鈹?                 鈹?     WIFI_EVENT           IP_EVENT          鑷畾涔変簨浠?     鈹溾攢 STA_START         鈹溾攢 GOT_IP          ...
     鈹溾攢 STA_DISCONNECTED  鈹溾攢 LOST_IP
     鈹溾攢 STA_CONNECTED     鈹斺攢 ...
     鈹斺攢 ...
```

**鏍稿績姒傚康锛?*
- `event_base`锛堜簨浠跺熀锛夛細鍒嗙被锛屽 `WIFI_EVENT`銆乣IP_EVENT`
- `event_id`锛氬叿浣撲簨浠讹紝濡?`WIFI_EVENT_STA_START`
- `event_data`锛氫簨浠舵惡甯︾殑鏁版嵁锛堝 `ip_event_got_ip_t` 鍖呭惈 IP 鍦板潃锛?- 澶勭悊鍑芥暟杩愯鍦?*浜嬩欢寰幆浠诲姟**鐨勪笂涓嬫枃涓紝涓嶆槸涓柇涓婁笅鏂?
**璁㈤槄 vs 鍙戝竷锛?*
```c
// 璁㈤槄锛堝簲鐢ㄥ眰鍋氱殑锛?esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &handler, ...);

// 鍙戝竷锛圵iFi 椹卞姩鍐呴儴鍋氱殑锛?esp_event_post(WIFI_EVENT, WIFI_EVENT_STA_START, NULL, 0, 0);
```

### 2.3 wifi_sta_event_handler 涓変釜鐘舵€?
```c
static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data)
```

#### 鐘舵€?锛歋TA_START 鈫?鍙戣捣杩炴帴

```
esp_wifi_start() 鈹€鈫?[WIFI_EVENT_STA_START] 鈹€鈫?esp_wifi_connect()
```

WiFi 椹卞姩鍒濆鍖栧畬鎴愬悗鑷姩瑙﹀彂姝や簨浠讹紝澶勭悊鍑芥暟閲岃皟鐢?`esp_wifi_connect()` 鍙戣捣瀹為檯鐨?AP 杩炴帴銆?
#### 鐘舵€?锛欴ISCONNECTED 鈫?閲嶈瘯鎴栭噸鍚?
```
杩炴帴澶辫触/鏂紑 鈹€鈫?[WIFI_EVENT_STA_DISCONNECTED] 鈹€鈫?retry < 5 ? 閲嶈瘯 : 鏀惧純
```

```c
if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < WIFI_MAXIMUM_RETRY) {     // WIFI_MAXIMUM_RETRY = 5
        esp_wifi_connect();                      // 閲嶈瘯
        s_retry_num++;
    } else {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        // 5 娆″け璐ュ悗涓嶅啀閲嶈瘯锛?鍒嗛挓鍚庤嚜鍔ㄩ噸鍚繘 Soft-AP 妯″紡
    }
}
```

#### 鐘舵€?锛欸OT_IP 鈫?鍚姩 pkt_fwd 浠诲姟

```
DHCP 鑾峰彇鍒?IP 鈹€鈫?[IP_EVENT_STA_GOT_IP] 鈹€鈫?鍒涘缓 pkt_fwd_task
```

```c
if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    snprintf(self_ip, sizeof(self_ip), IPSTR, IP2STR(&event->ip_info.ip));

    wifi_ready = true;

    // WiFi 杩炴帴鎴愬姛锛屼繚瀛?wifi_mode 涓?station
    config_wifi_mode(WIFI_MODE_STATION);

    // 鍙栨秷閲嶅惎瀹氭椂鍣?    reboot_flag = false;

    // 鍒涘缓鏍稿績浠诲姟锛?6KB 鏍堬紝浼樺厛绾?6
    xTaskCreate(pkt_fwd_task, "pkt_fwd", 4096*4, NULL, 6, &pkt_fwd_handle);

    // OLED 鏄剧ず IP 鍦板潃
    oled_show_one_line(0, 4, self_ip, 2);
}
```

**鍏抽敭鐐癸細**
- `pkt_fwd_task` 鏄湪 WiFi 杩炴帴鎴愬姛鍚庢墠鍒涘缓鐨勶紝纭繚缃戠粶灏辩华
- `config_wifi_mode(WIFI_MODE_STATION)` 鎶?NVS 涓殑妯″紡鏀瑰洖 Station锛岀‘璁?WiFi 鍙敤
- `reboot_flag = false` 鍙栨秷 5 鍒嗛挓鑷姩閲嶅惎瀹氭椂鍣?
---

## 绗笁閮ㄥ垎锛歱kt_fwd_main() 鏍稿績鍒濆鍖?
**鏂囦欢**: `lora_pkt_fwd.c` 绾︾1590琛? 
`pkt_fwd_task()` 鏄竴涓?FreeRTOS 浠诲姟鍖呰锛屽唴閮ㄧ洿鎺ヨ皟鐢?`pkt_fwd_main()`銆?
### 3.1 浜掓枼閲忓垱寤?
```c
mx_concent = xSemaphoreCreateMutex();  // SPI 鎬荤嚎浜掓枼锛堟渶閲嶈锛?mx_xcorr   = xSemaphoreCreateMutex();  // XTAL 鏍℃鍊间簰鏂?mx_timeref = xSemaphoreCreateMutex();  // GPS 鏃堕棿鍙傝€冧簰鏂?mx_meas_up = xSemaphoreCreateMutex();  // 涓婅缁熻浜掓枼
mx_meas_dw = xSemaphoreCreateMutex();  // 涓嬭缁熻浜掓枼
mx_meas_gps = xSemaphoreCreateMutex(); // GPS 缁熻浜掓枼
mx_stat_rep = xSemaphoreCreateMutex(); // 鐘舵€佹姤鍛婁簰鏂?```

`mx_concent` 鏄渶鏍稿績鐨勪簰鏂ラ噺鈥斺€擲X1302 鍙湁涓€鏉?SPI 鎬荤嚎锛屽涓嚎绋嬪悓鏃惰闂細鍐茬獊锛屾墍浠ユ墍鏈?`lgw_*()` 璋冪敤閮藉繀椤诲湪 `mx_concent` 淇濇姢涓嬫墽琛屻€?
### 3.2 JSON 閰嶇疆鍔犺浇涓庨鐜囪ˉ涓?
```c
// 鏍规嵁 NVS 涓殑 freq_region 閫夋嫨瀵瑰簲鐨?JSON 閰嶇疆
if(strncmp(config[FREQ_REGION].val, "eu868", 5) == 0){
    conf_array = malloc(sizeof(global_eu_conf));
    memcpy(conf_array, global_eu_conf, sizeof(global_eu_conf));
} else if(strncmp(config[FREQ_REGION].val, "us915", 5) == 0){
    conf_array = malloc(sizeof(global_us_conf));
    memcpy(conf_array, global_us_conf, sizeof(global_us_conf));
} else {
    conf_array = malloc(sizeof(global_cn_conf));  // 榛樿 cn470
    memcpy(conf_array, global_cn_conf, sizeof(global_cn_conf));
}
```

JSON 鏄紪璇戞椂宓屽叆鍒板浐浠朵腑鐨勶紙閫氳繃 `global_json.h` 涓殑瀛楃鏁扮粍锛夛紝涓嶆槸浠庢枃浠剁郴缁熷姞杞界殑銆?
**棰戠巼琛ヤ竵锛氱洿鎺ュ湪 JSON 瀛楃涓蹭腑鏇挎崲棰戠巼鍊?*

```c
// 鐢?strstr 鎵惧埌 JSON 涓涓€涓拰绗簩涓?"freq" 瀛楁
char *radio0_index = strstr(conf_array, "\"freq\"");
char *radio1_index = strstr(radio0_index + 8, "\"freq\"");

// 鐢?NVS 涓殑鍊艰鐩?JSON 涓殑棰戠巼锛堝繀椤诲垰濂?9 瀛楃锛屽 "486600000"锛?if(config[FREQ_RADIO0].val != NULL && config[FREQ_RADIO0].len == 9)
    strncpy(radio0_index + 8, config[FREQ_RADIO0].val, 9);
```

**涓轰粈涔堣繖涔?绮楁毚"锛?* 鍥犱负 JSON 宸茬粡缂栬瘧鍒板浐浠堕噷浜嗭紝涓嶈兘鏀圭粨鏋勶紝鍙兘鍦ㄨВ鏋愬墠鍘熷湴鏇挎崲棰戠巼鏁板瓧銆傞檺鍒舵槸棰戠巼鍊煎繀椤绘伆濂?9 浣嶆暟瀛楋紙濡?`486600000`锛夛紝鍚﹀垯闀垮害涓嶅尮閰嶅氨涓嶆浛鎹€?
### 3.3 涓変釜 parse_* 鍑芥暟

```c
parse_SX130x_configuration(conf_array);   // 瑙ｆ瀽灏勯纭欢閰嶇疆
parse_gateway_configuration(conf_array);   // 瑙ｆ瀽缃戝叧缃戠粶閰嶇疆
parse_debug_configuration(conf_array);     // 瑙ｆ瀽璋冭瘯閰嶇疆
```

#### parse_SX130x_configuration

浠?JSON 鐨?`"SX130x_conf"` 瀵硅薄涓В鏋愶細
- `com_type`: SPI 鎴?USB
- `com_path`: 璁惧璺緞
- `lorawan_public`: 鏄惁浣跨敤鍏叡 LoRaWAN syncword
- `clksrc`: 鏃堕挓婧?- `antenna_gain`: 澶╃嚎澧炵泭
- `radio_0` / `radio_1`: 灏勯閾捐矾閰嶇疆锛堥鐜囥€佺被鍨嬨€丷SSI 娓╁害琛ュ伩绯绘暟锛?- `chan_multiSF_0` ~ `chan_multiSF_7`: 8 涓?multi-SF 淇￠亾
- `chan_Lora_std`: 鏍囧噯 LoRa 淇￠亾
- `chan_FSK`: FSK 淇￠亾
- TX gain LUT: 鍙戝皠鍔熺巼鏌ユ壘琛?
#### parse_gateway_configuration

浠?JSON 鐨?`"gateway_conf"` 瀵硅薄涓В鏋愶細
- `server_address`, `serv_port_up`, `serv_port_down`: NS 鍦板潃鍜岀鍙?- `keepalive_interval`: PULL_DATA 鍙戦€侀棿闅旓紙榛樿 5 绉掞級
- `stat_interval`: 缁熻鎶ュ憡闂撮殧锛堥粯璁?30 绉掞級
- `forward_crc_valid/error/nocrc`: 鍖呰繃婊ゅ紑鍏?- `gps_tty_path`, `fake_gps`: GPS 閰嶇疆
- `beacon_*`: Beacon 閰嶇疆

### 3.4 UDP Socket 鍒涘缓涓庤繛鎺?
```c
// NVS 鐨勫€艰鐩?JSON 鐨勯粯璁ゅ€?if(udp_host[0] == '\0')
    strncpy(udp_host, serv_addr, sizeof udp_host);
if(udp_port == 0)
    udp_port = atoi(serv_port_up);

// 鍒涘缓涓や釜鐙珛鐨?UDP socket
sock_up   = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);  // 涓婅涓撶敤
sock_down = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);  // 涓嬭涓撶敤

// DNS 瑙ｆ瀽骞惰繛鎺?dns_loopup(udp_host, ip);
dest_addr.sin_addr.s_addr = inet_addr(ip);
dest_addr.sin_port = htons(udp_port);

// 涓や釜 socket 閮?connect 鍒板悓涓€涓?NS 鍦板潃
connect(sock_up,   (struct sockaddr *)&dest_addr, sizeof(dest_addr));
connect(sock_down, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

// 璁剧疆涓婅 socket 鐨勬帴鏀惰秴鏃?= 250ms锛坔alf of PUSH_TIMEOUT_MS=500锛?setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &push_timeout_half, ...);
```

**涓轰粈涔堢敤涓や釜 socket锛?* `thread_up` 鍜?`thread_down` 杩愯鍦ㄤ笉鍚岀殑绾跨▼涓紝浣跨敤鐙珛鐨?socket 閬垮厤浜掓枼閿佸紑閿€锛屾彁楂樺苟鍙戞€ц兘銆?
**涓轰粈涔堝 UDP socket 璋冪敤 `connect()`锛?* UDP 鐨?`connect()` 涓嶄細寤虹珛杩炴帴锛屽彧鏄粦瀹氫簡杩滅鍦板潃锛屼箣鍚庡彲浠ョ敤 `send()`/`recv()` 浠ｆ浛 `sendto()`/`recvfrom()`锛屼唬鐮佹洿绠€娲併€?
### 3.5 SX1302 澶嶄綅涓庡惎鍔?
```c
// SPI 妯″紡涓嬪厛纭欢澶嶄綅 SX1302
if (com_type == LGW_COM_SPI)
    lgw_reset();

// 鍚姩闆嗕腑鍣?i = lgw_start();
if (i == LGW_HAL_SUCCESS)
    MSG("INFO: [main] concentrator started, packet can now be received\n");
else
    exit(EXIT_FAILURE);
```

`lgw_start()` 鍐呴儴鍋氫簡澶ч噺宸ヤ綔锛?1. 鎵撳紑 SPI 鍜?I2C 绔彛
2. 鎼滅储 I2C 娓╁害浼犳劅鍣紙0x39, 0x3B, 0x38锛夆啇 **瑙侀檮褰旳**
3. 鏍″噯 SX1250 灏勯鍓嶇
4. 閰嶇疆鎵€鏈?IF 淇￠亾鍜岃В璋冨弬鏁?5. 鍚姩 SX1302 鏁板瓧鍩哄甫
6. 楠岃瘉鑺墖鐗堟湰鍙凤紙chip version 0x10 = v1.0锛?
### 3.6 宸ヤ綔绾跨▼鍒涘缓

```c
// LED 鎸囩ず鐏畧鎶ょ嚎绋?xTaskCreatePinnedToCore(vDaemonLedIndication, "led_flash", 4096, NULL, 1, NULL, tskNO_AFFINITY);

// JIT 闃熷垪鍒濆鍖栵紙涓ゆ潯 RF 閾捐矾鍚勪竴涓槦鍒楋級
jit_queue_init(&jit_queue[0]);
jit_queue_init(&jit_queue[1]);

// 涓婅绾跨▼锛?6KB 鏍堬紝浼樺厛绾?6锛?xTaskCreatePinnedToCore(thread_up,   "thread_up",   4096*4, NULL, 6, &pThreadUp, tskNO_AFFINITY);

// 涓嬭绾跨▼锛?KB 鏍堬紝浼樺厛绾?6锛?xTaskCreatePinnedToCore(thread_down, "thread_down",  4096*2, NULL, 6, NULL, tskNO_AFFINITY);

// JIT 绾跨▼锛?KB 鏍堬紝浼樺厛绾?6锛?xTaskCreatePinnedToCore(thread_jit,  "thread_jit",   4096*2, NULL, 6, NULL, tskNO_AFFINITY);
```

`tskNO_AFFINITY` 琛ㄧず绾跨▼鍙互鍦?ESP32-S3 鐨勪换鎰忎竴涓?CPU 鏍稿績涓婅繍琛屻€?
### 3.7 涓诲惊鐜細缁熻鏀堕泦

```c
while (!exit_sig && !quit_sig) {
    // 姣?5 绉掑埛鏂颁竴娆?OLED 鏃堕棿鏄剧ず
    while(time_count < stat_interval) {
        vTaskDelay(1000 * TIME_REFRESH / portTICK_PERIOD_MS);
        time_count += TIME_REFRESH;
        // 鏇存柊 OLED 鏃堕棿
    }

    // 姣?stat_interval (30s) 鏀堕泦涓€娆＄粺璁?    // 1. 鍔犻攣璇诲彇 + 娓呴浂涓婅缁熻
    xSemaphoreTake(mx_meas_up, portMAX_DELAY);
    cp_nb_rx_rcv = meas_nb_rx_rcv; meas_nb_rx_rcv = 0;
    // ... 鍏朵粬瀛楁 ...
    xSemaphoreGive(mx_meas_up);

    // 2. 鍔犻攣璇诲彇 + 娓呴浂涓嬭缁熻
    xSemaphoreTake(mx_meas_dw, portMAX_DELAY);
    // ...
    xSemaphoreGive(mx_meas_dw);

    // 3. 璇诲彇娓╁害锛堝姞 SPI 浜掓枼閿侊級
    xSemaphoreTake(mx_concent, portMAX_DELAY);
    i = lgw_get_temperature(&temperature);
    xSemaphoreGive(mx_concent);

    // 4. 鎵撳嵃缁熻鎶ュ憡鍒颁覆鍙?    printf("##### %s #####\n", stat_timestamp);
    printf("# RF packets received: %u\n", cp_nb_rx_rcv);
    // ...

    // 5. 缁勮 JSON 鐘舵€佹姤鍛婏紙缁?thread_up 鍙戦€侊級
    xSemaphoreTake(mx_stat_rep, portMAX_DELAY);
    snprintf(status_report, STATUS_SIZE,
        "\"stat\":{\"time\":\"%s\",\"rxnb\":%u,\"rxok\":%u,...,\"temp\":%.1f}",
        ...);
    report_ready = true;  // 閫氱煡 thread_up 涓嬫鍙戝寘鏃跺甫涓婄姸鎬?    xSemaphoreGive(mx_stat_rep);
}
```

---

## 绗洓閮ㄥ垎锛氫笁澶ф牳蹇冪嚎绋嬭瑙?
### 绾跨▼鍗忎綔鍏ㄦ櫙鍥?
```
SX1302 纭欢                thread_up              Network Server (NS)
   鈹?                          鈹?                        鈹?   鈹?鏈夊寘鍒拌揪                   鈹?                        鈹?   鈹傗攢鈹€lgw_receive()鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻衡攤                         鈹?   鈹?                          鈹?缁勮JSON                 鈹?   鈹?                          鈹傗攢鈹€PUSH_DATA鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻衡攤
   鈹?                          鈹傗梽鈹€PUSH_ACK鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?   鈹?                          鈹?                        鈹?   鈹?                          鈹?       thread_down       鈹?   鈹?                          鈹傗梽鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€  鈹?   鈹?                          鈹?       PULL_DATA         鈹?   鈹?                          鈹傗攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻衡攤
   鈹?                          鈹傗梽鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?   鈹?                          鈹?       PULL_ACK          鈹?   鈹?                          鈹傗梽鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?   鈹?                          鈹?       PULL_RESP(涓嬭鍖?  鈹?   鈹?                     jit_queue                       鈹?   鈹傗梽鈹€鈹€lgw_send()鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?                        鈹?   鈹?  thread_jit 鍙戝皠          鈹?                        鈹?```

### 4.1 thread_up 鈥?涓婅绾跨▼

**鏂囦欢**: `lora_pkt_fwd.c` 绾︾2145琛?
#### 涓诲惊鐜粨鏋?
```c
void thread_up(void)
{
    // 棰勫～鍏呭浐瀹氭姤鏂囧ご锛堝崗璁増鏈?+ 绫诲瀷 + 缃戝叧MAC锛?    buff_up[0] = PROTOCOL_VERSION;   // = 2
    buff_up[3] = PKT_PUSH_DATA;      // = 0
    *(unsigned int *)(buff_up + 4) = net_mac_h;  // 缃戝叧ID楂?2浣?    *(unsigned int *)(buff_up + 8) = net_mac_l;  // 缃戝叧ID浣?2浣?
    while (!exit_sig && !quit_sig) {

        // 鈶?浠?SX1302 鍙栧寘锛堝姞 SPI 浜掓枼閿侊級
        xSemaphoreTake(mx_concent, portMAX_DELAY);
        nb_pkt = lgw_receive(NB_PKT_MAX, rxpkt);  // 鏈€澶氬彇24涓寘
        xSemaphoreGive(mx_concent);

        // 鈶?妫€鏌ユ槸鍚︽湁鐘舵€佹姤鍛婅鍙?        send_report = report_ready;

        // 鈶?娌℃湁鍖呬篃娌℃湁鎶ュ憡 鈫?鐫?0ms缁х画
        if ((nb_pkt == 0) && (send_report == false)) {
            vTaskDelay(FETCH_SLEEP_MS / portTICK_PERIOD_MS); // 10ms
            continue;
        }

        // 鈶?鏈夊寘鍒欓棯鐑佷笂琛孡ED
        if (nb_pkt > 0)
            vUplinkFlash(10);

        // 鈶?閫愬寘澶勭悊锛岀粍瑁匤SON
        // 鈶?鍙戦€乁DP锛岀瓑寰匒CK
    }
}
```

#### PUSH_DATA UDP 鎶ユ枃缁撴瀯

```
瀛楄妭鍋忕Щ  闀垮害   鍐呭
鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€  鈹€鈹€鈹€鈹€   鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
0         1      鍗忚鐗堟湰 = 0x02
1-2       2      闅忔満 token锛堢敤浜庡尮閰?ACK锛?3         1      鎶ユ枃绫诲瀷 = 0x00 (PUSH_DATA)
4-7       4      缃戝叧 MAC 楂?2浣嶏紙缃戠粶瀛楄妭搴忥級
8-11      4      缃戝叧 MAC 浣?2浣嶏紙缃戠粶瀛楄妭搴忥級
12+       N      JSON 瀛楃涓?```

#### 鍖呰繃婊ら€昏緫

```c
switch(p->status) {
    case STAT_CRC_OK:
        meas_nb_rx_ok += 1;
        if (!fwd_valid_pkt) continue;  // JSON 閰嶇疆 forward_crc_valid=false 鍒欎涪寮?        break;
    case STAT_CRC_BAD:
        meas_nb_rx_bad += 1;
        if (!fwd_error_pkt) continue;  // 榛樿涓㈠純CRC閿欒鍖?        break;
    case STAT_NO_CRC:
        meas_nb_rx_nocrc += 1;
        if (!fwd_nocrc_pkt) continue;  // 榛樿涓㈠純鏃燙RC鍖?        break;
}
```

#### JSON rxpk 瀛楁璇﹁В

| 瀛楁 | 鏉ユ簮 | 鍚箟 |
|---|---|---|
| `tmst` | `p->count_us` | SX1302 鍐呴儴鑷敱璁℃暟鍣紙碌s锛夛紝涓嬭瀵归綈鐢?|
| `chan` | `p->if_chain` | 鎺ユ敹淇￠亾鍙凤紙0-7=multiSF, 8=std, 9=FSK锛?|
| `rfch` | `p->rf_chain` | 灏勯閾捐矾鍙凤紙0鎴?锛?|
| `freq` | `p->freq_hz/1e6` | 鎺ユ敹棰戠巼锛圡Hz锛?|
| `mid` | `p->modem_id` | 瑙ｈ皟鍣↖D |
| `stat` | `p->status` | CRC鐘舵€侊細1=OK, -1=閿欒, 0=鏃燙RC |
| `modu` | `p->modulation` | 璋冨埗鏂瑰紡锛歀ORA 鎴?FSK |
| `datr` | `p->datarate+bandwidth` | 鏁版嵁閫熺巼锛氬 "SF12BW125" |
| `codr` | `p->coderate` | 缂栫爜鐜囷細4/5, 4/6, 4/7, 4/8 |
| `rssis` | `p->rssis` | 淇″彿 RSSI锛坉Bm锛夛紝宸叉俯搴﹁ˉ鍋?|
| `lsnr` | `p->snr` | LoRa SNR锛坉B锛?|
| `foff` | `p->freq_offset` | 棰戠巼鍋忕Щ锛圚z锛夛紝鍙嶆槧缁堢鏅舵尟璇樊 |
| `rssi` | `p->rssic` | 淇￠亾 RSSI锛坉Bm锛?|
| `size` | `p->size` | 杞借嵎瀛楄妭鏁?|
| `data` | `p->payload` | Base64 缂栫爜鐨勮浇鑽?|

#### 娓呯┖鏃?ACK + 鍙戦€?+ 绛夊緟鏈哄埗

```c
// 鈽?鍙戦€佸墠鍏堟竻绌?socket 缂撳啿鍖洪噷鐨勬棫 ACK
uint8_t _tmp[4];
while (recv(sock_up, (void *)_tmp, sizeof _tmp, MSG_DONTWAIT) > 0) {}

// 鍙戦€?PUSH_DATA
send(sock_up, (void *)buff_up, buff_index, 0);

// 绛夊緟 PUSH_ACK锛堟渶澶氫袱杞紝姣忚疆 250ms锛?for (i=0; i<2; ++i) {
    j = recv(sock_up, (void *)buff_ack, sizeof buff_ack, 0);
    // sock_up 宸茶 SO_RCVTIMEO = 250ms

    if (j == -1 && errno == EAGAIN)
        continue;  // 瓒呮椂锛屽啀绛変竴杞?
    // 楠岃瘉锛氬崗璁増鏈€佹姤鏂囩被鍨嬨€乼oken 鍖归厤
    if (buff_ack[1]==token_h && buff_ack[2]==token_l) {
        MSG("INFO: [up] PUSH_ACK received in %i ms\n", ...);
        meas_up_ack_rcv += 1;
        vBackhaulFlash(10);  // 闂儊鍥炰紶LED
        break;
    }
}
```

**涓轰粈涔堝彂閫佸墠瑕佹竻绌烘棫 ACK锛?*  
涓婁竴杞?PUSH_DATA 濡傛灉瓒呮椂浜嗕絾 ACK 鍏跺疄宸茬粡鍦ㄨ矾涓婏紝浼氬湪杩欒疆鍙戦€佸墠鎶佃揪 socket 缂撳啿鍖恒€備笉娓呯┖鐨勮瘽锛岃繖涓棫 ACK 浼氳璇涓烘槸鏂扮殑 PUSH_ACK锛屽鑷?`ackr`锛圓CK 鐜囷級缁熻铏氶珮銆?
**涓轰粈涔堢瓑涓よ疆锛?*  
缃戠粶鎶栧姩鍙兘瀵艰嚧 ACK 姣旈鏈熸櫄鍒帮紝涓よ疆鍏?500ms 鐨勭瓑寰呯獥鍙ｅ鍔犱簡鏀跺埌 ACK 鐨勬鐜囥€?
### 4.2 thread_down 鈥?涓嬭绾跨▼

**鏂囦欢**: `lora_pkt_fwd.c` 绾︾2770琛?
#### 涓诲惊鐜粨鏋?
```c
void thread_down(void)
{
    // 棰勫～鍏?PULL_DATA 鎶ユ枃澶?    buff_req[0] = PROTOCOL_VERSION;
    buff_req[3] = PKT_PULL_DATA;  // = 0x02
    *(unsigned int *)(buff_req + 4) = net_mac_h;
    *(unsigned int *)(buff_req + 8) = net_mac_l;

    while (!exit_sig && !quit_sig) {

        // 鈶?autoquit 妫€娴嬶細濡傛灉杩炵画 N 娆?PULL_DATA 閮芥病鏀跺埌 ACK锛岄€€鍑?        if ((autoquit_threshold > 0) && (autoquit_cnt >= autoquit_threshold))
            exit_sig = true;

        // 鈶?鍙戦€?PULL_DATA
        token_h = rand(); token_l = rand();
        buff_req[1] = token_h; buff_req[2] = token_l;
        send(sock_down, buff_req, sizeof buff_req, 0);

        // 鈶?鍦?keepalive_time(10s) 鏃堕棿绐楀彛鍐呮寔缁洃鍚?        while (difftimespec(recv_time, send_time) < keepalive_time) {
            msg_len = recv(sock_down, buff_down, sizeof buff_down, 0);
            // sock_down 璁?SO_RCVTIMEO = 400ms
            // 姣忔瓒呮椂灏辩户缁惊鐜紝鐩村埌 keepalive_time 鍒版湡

            if (msg_len == -1) continue;  // 瓒呮椂

            // 鈶?澶勭悊 PULL_ACK
            if (buff_down[3] == PKT_PULL_ACK) { ... }

            // 鈶?澶勭悊 PULL_RESP锛堜笅琛屽寘锛?            if (buff_down[3] == PKT_PULL_RESP) { ... }
        }
    }
}
```

#### PULL_DATA / PULL_ACK 鏃跺簭

```
thread_down                              NS
    鈹?                                    鈹?    鈹傗攢鈹€PULL_DATA(token=0xA1B2)鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻衡攤
    鈹傗梽鈹€PULL_ACK(token=0xA1B2)鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? (瀹炴祴绾?43ms)
    鈹?                                    鈹?    鈹? ...鐩戝惉鏈€澶?10 绉?..                鈹?    鈹?                                    鈹?    鈹? (濡傛灉鏈変笅琛屽寘):                      鈹?    鈹傗梽鈹€PULL_RESP(txpk={...})鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?    鈹? 瑙ｆ瀽JSON 鈫?jit_enqueue()           鈹?    鈹傗攢鈹€TX_ACK(token)鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻衡攤
    鈹?                                    鈹?    鈹傗攢鈹€PULL_DATA(token=0xC3D4)鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻衡攤 (10绉掑悗鍐嶆潵涓€娆?
```

#### PULL_RESP 瑙ｆ瀽 鈥?涓夌涓嬭妯″紡

| 妯″紡 | JSON 瀛楁 | LoRaWAN Class | 璇存槑 |
|---|---|---|---|
| 绔嬪嵆鍙戝皠 | `"imme": true` | Class C | 涓嶉渶瑕佹椂闂存埑锛岀珛鍒诲彂 |
| 鏃堕棿鎴冲彂灏?| `"tmst": 3784146` | Class A | 鍦ㄦ寚瀹?SX1302 璁℃暟鍣ㄥ€兼椂鍙?|
| GPS鏃堕棿鍙戝皠 | `"tmms": 1234567890` | Class B | 鍦ㄦ寚瀹?GPS 姣鏃跺埢鍙?|

**Class A锛堟渶甯歌锛夛細** NS 鎶婁笂琛屽寘鐨?`tmst` 鍔犱笂 RX1Delay锛?绉掞級浣滀负涓嬭鐨?`tmst`銆?
#### PULL_RESP 澶勭悊娴佺▼

```c
// 瑙ｆ瀽 JSON 鈫?濉厖 txpkt 缁撴瀯浣?// 鍖呭惈: freq, rfch, powe, modu, datr, codr, ipol, size, data

// 妫€鏌ラ鐜囪寖鍥?if (txpkt.freq_hz < tx_freq_min[...] || txpkt.freq_hz > tx_freq_max[...])
    jit_result = JIT_ERROR_TX_FREQ;

// 妫€鏌ュ彂灏勫姛鐜囷紙鏌ヨ〃鎵炬渶鎺ヨ繎鐨勬敮鎸佸€硷級
get_tx_gain_lut_index(txpkt.rf_chain, txpkt.rf_power, &tx_lut_idx);

// 鏀惧叆 JIT 闃熷垪锛堜笉绔嬪嵆鍙戝皠锛?jit_enqueue(&jit_queue[txpkt.rf_chain], current_concentrator_time, &txpkt, downlink_type);

// 鍥炲 TX_ACK 缁?NS
send_tx_ack(buff_down[1], buff_down[2], jit_result, warning_value);
```

### 4.3 thread_jit 鈥?JIT 瀹氭椂鍙戝皠绾跨▼

**鏂囦欢**: `lora_pkt_fwd.c` 绾︾3456琛?
JIT = Just In Time锛堟伆濂藉強鏃讹級銆備笅琛屽寘涓嶈兘鎯冲彂灏卞彂锛屽繀椤诲湪**绮剧‘鐨勬椂鍒?*鍙戝皠锛屽惁鍒欑粓绔妭鐐圭殑鎺ユ敹绐楀彛宸插叧闂€?
#### 涓诲惊鐜粨鏋?
```c
void thread_jit(void)
{
    while (!exit_sig && !quit_sig) {
        vTaskDelay(10 / portTICK_PERIOD_MS);  // 姣?10ms 妫€鏌ヤ竴娆?
        for (i = 0; i < LGW_RF_CHAIN_NB; i++) {  // 閬嶅巻涓ゆ潯 RF 閾捐矾

            // 鈶?璇诲彇褰撳墠 SX1302 璁℃暟鍣ㄥ€?            xSemaphoreTake(mx_concent, portMAX_DELAY);
            lgw_get_instcnt(&current_concentrator_time);
            xSemaphoreGive(mx_concent);

            // 鈶?鏌ョ湅闃熷垪澶撮儴鏄惁鍒版湡
            jit_result = jit_peek(&jit_queue[i], current_concentrator_time, &pkt_index);

            if (jit_result == JIT_ERROR_OK && pkt_index > -1) {

                // 鈶?鍙栧嚭鍖?                jit_dequeue(&jit_queue[i], pkt_index, &pkt, &pkt_type);

                // 鈶?Beacon 棰戠巼琛ュ伩锛堟櫠鎸牎姝ｏ級
                if (pkt_type == JIT_PKT_TYPE_BEACON) {
                    xSemaphoreTake(mx_xcorr, portMAX_DELAY);
                    pkt.freq_hz = (unsigned int)(xtal_correct * (double)pkt.freq_hz);
                    xSemaphoreGive(mx_xcorr);
                }

                // 鈶?妫€鏌?TX 鐘舵€?                lgw_status(pkt.rf_chain, TX_STATUS, &tx_status);
                if (tx_status == TX_EMITTING) continue;  // 姝ｅ湪鍙戝皠锛岃烦杩?
                // 鈶?鍙戝皠锛?                xSemaphoreTake(mx_concent, portMAX_DELAY);
                result = lgw_send(&pkt);
                xSemaphoreGive(mx_concent);

                if (result == LGW_HAL_SUCCESS) {
                    meas_nb_tx_ok += 1;
                    vDownlinkFlash(10);  // 闂儊涓嬭LED
                } else {
                    meas_nb_tx_fail += 1;
                }
            }
        }
    }
}
```

#### JIT 闃熷垪鏃堕棿绾跨ず渚?
```
涓婅鍖呭埌杈? tmst = 3,784,146 碌s
                鈹?                鈹? NS 鏀跺埌涓婅鍖咃紝璁＄畻涓嬭鏃跺埢
                鈹? RX1: tmst + 1,000,000 = 4,784,146 碌s
                鈹?thread_down: jit_enqueue(count_us=4,784,146)
                鈹?                鈹? thread_jit 姣?10ms 妫€鏌?                鈹?current_time 鈮?4,783,146 碌s  鈫?jit_peek(): "杩樺樊绾?ms锛屾椂鏈哄凡鍒?
                鈹?                鈻?            lgw_send(&pkt)  鈫?SPI 鍐欏叆 SX1302
                鈹?current_time = 4,784,146 碌s
                鈹?                鈻?            SX1302 纭欢鍦ㄧ簿纭椂鍒诲彂灏勫皠棰戜俊鍙?            缁堢鑺傜偣鐨?RX1 绐楀彛姝ｅソ鎵撳紑 鈫?鎴愬姛鎺ユ敹
```

### 4.4 涓夌嚎绋嬪崗浣滃畬鏁存暟鎹祦

浠ヤ竴娆″畬鏁寸殑 Class A 涓婁笅琛屼氦浜掍负渚嬶細

```
鏃堕棿杞?鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻?
[SX1302 纭欢]
  t=3,784,146碌s: 缁堢鍙戞潵涓婅鍖咃紝SX1302 鎺ユ敹瀹屾垚

[thread_up] (姣?0ms杞)
  t鈮?,784,156碌s: lgw_receive() 鍙栧嚭鍖?                 CRC_OK 鈫?閫氳繃杩囨护
                 缁勮JSON: {"rxpk":[{"tmst":3784146,...}]}
                 send(sock_up, PUSH_DATA, ...)
                 鈫撹摑鐏棯
                 recv() 绛夊緟 ACK...
                 鏀跺埌 PUSH_ACK 鈫?鈫撶豢鐏棯

[NS 鏈嶅姟鍣╙
  鏀跺埌 PUSH_DATA
  璁＄畻 RX1: tmst = 3784146 + 1000000 = 4784146
  鍙戦€?PULL_RESP: {"txpk":{"tmst":4784146,"freq":505.3,...}}

[thread_down] (涓€鐩村湪 recv 绛夊緟)
  鏀跺埌 PULL_RESP
  瑙ｆ瀽JSON 鈫?txpkt.count_us = 4784146
  jit_enqueue(&jit_queue[0], ..., CLASS_A)
  send_tx_ack(JIT_ERROR_OK) 鈫?鈫撶豢鐏棯

[thread_jit] (姣?0ms妫€鏌?
  t鈮?,783,156碌s: jit_peek() 鈫?鏃舵満宸插埌锛?                 jit_dequeue() 鈫?鍙栧嚭 txpkt
                 lgw_status() 鈫?TX_FREE
                 lgw_send(&txpkt)
                 鈫撶孩鐏棯

[SX1302 纭欢]
  t=4,784,146碌s: 绮剧‘鏃跺埢鍙戝皠涓嬭灏勯淇″彿
  缁堢鑺傜偣 RX1 绐楀彛鎵撳紑 鈫?鎴愬姛鎺ユ敹涓嬭鍖?```

#### LED 涓夎壊鎸囩ず

```c
vUplinkFlash(10);   // thread_up: 涓婅鏈夊寘鍒拌揪  鈫?钃濈伅闂?vBackhaulFlash(10); // ACK 鏀跺埌锛堜笂琛?涓嬭锛?   鈫?缁跨伅闂?vDownlinkFlash(10); // thread_jit: 鍙戝皠鎴愬姛     鈫?绾㈢伅闂?```

---

## 闄勫綍A锛氭俯搴︿紶鎰熷櫒闂璇﹁В锛堣俯鍧戣褰曪級

### 闂鐜拌薄

鍚姩鏃ュ織涓嚭鐜颁互涓嬭鍛婂拰閿欒锛?
```
WARNING: failed to configure temperature sensor on port 0x39
WARNING: failed to configure temperature sensor on port 0x3B
WARNING: failed to configure temperature sensor on port 0x38
WARNING: no temperature sensor found.
```

杩愯鏃舵瘡娆?`lgw_receive()` 閮芥墦鍗帮細

```
ERROR: failed to read I2C device 0x38 (err=-1)
ERROR: failed to get current temperature
```

### 鏍瑰洜鍒嗘瀽

#### 璋佸湪璇绘俯搴︼紵

HAL 搴撲腑鏈変袱涓湴鏂硅鍙栨俯搴︼細

1. **`lgw_receive()` 鍐呴儴**锛氭瘡娆′粠 SX1302 鍙栧寘鏃讹紝璋冪敤 `lgw_get_temperature()` 鑾峰彇褰撳墠娓╁害锛岀敤浜?RSSI 娓╁害琛ュ伩銆?*杩欏氨鏄瘡娆℃敹鍖呴兘鎵撳嵃閿欒鐨勫師鍥犮€?*

2. **涓诲惊鐜粺璁?*锛氭瘡 30 绉掕皟鐢ㄤ竴娆?`lgw_get_temperature()`锛岀敤浜庣姸鎬佹姤鍛婂拰 OLED 鏄剧ず銆?
#### 娓╁害鐢ㄥ湪鍝紵

```c
// loragw_hal.c 涓?lgw_receive() 鐨勫鐞?res = lgw_get_temperature(&current_temperature);
// ...
// 鐢ㄦ俯搴﹀仛 RSSI 琛ュ伩
rssi_offset = sx1302_rssi_get_temperature_offset(&rssi_tcomp, current_temperature);
// offset = a*T^4 + b*T^3 + c*T^2 + d*T + e
```

杩欐槸瀵?SX1250 灏勯鍓嶇鎺ユ敹鍒扮殑 RSSI 鍊艰繘琛屾俯搴﹁ˉ鍋裤€備笉鍚屾俯搴︿笅 LNA锛堜綆鍣０鏀惧ぇ鍣級澧炵泭浼氭紓绉伙紝闇€瑕佺敤娓╁害绯绘暟澶氶」寮忔潵淇銆?
#### 娓╁害浼犳劅鍣ㄦ槸鍝釜锛?
HAL 鏈熸湜鐨勬槸 Semtech CoreCell 鍙傝€冭璁′笂鐨?*澶栭儴 I2C 娓╁害浼犳劅鍣?* STTS751锛圫T 鍗婂浣擄級锛屽湴鍧€ 0x39/0x3B/0x38銆傝繖棰楄姱鐗囨斁缃湪 SX1302 妯″潡闄勮繎锛屾祴閲忕殑鏄?RF 鑺墖鍛ㄥ洿鐨勭幆澧冩俯搴︺€?
```c
// loragw_stts751.h
static const uint8_t I2C_PORT_TEMP_SENSOR[] = {0x39, 0x3B, 0x38};
```

#### 涓轰粈涔堟垜鐨勬澘瀛愭病鏈夛紵

鎴戜娇鐢ㄧ殑鏄幇鎴愮殑 SX1302 妯＄粍锛屾ā缁勫唴閮ㄦ湁 TCXO锛堟俯搴﹁ˉ鍋挎櫠鎸級浣?*娌℃湁鐒婃帴 STTS751 娓╁害浼犳劅鍣?*鈥斺€旇繖棰楄姱鐗囨槸 Semtech 鍙傝€冭璁＄殑涓€閮ㄥ垎锛屽苟闈?SX1302 鐨勫繀闇€缁勪欢銆?
### TCXO 涓庢俯搴﹁ˉ鍋跨殑鍖哄埆

| 椤圭洰 | TCXO | RSSI 娓╁害琛ュ伩锛堜唬鐮佷腑鐨勶級 |
|---|---|---|
| 琛ュ伩浠€涔?| 26MHz 鍙傝€冩椂閽熼鐜囨紓绉?| RSSI 璇绘暟闅忔俯搴︾殑鍋忕Щ |
| 纭欢 | 妯＄粍鍐呯疆鐨勬俯琛ユ櫠鎸?| 澶栭儴 I2C 娓╁害浼犳劅鍣?STTS751) |
| 鏄惁闇€瑕佷唬鐮佸弬涓?| 鍚︼紙绾‖浠惰嚜鍔ㄨˉ鍋匡級 | 鏄紙闇€瑕佽鍙栨俯搴﹁绠楄ˉ鍋块噺锛?|
| 褰卞搷 | 棰戠巼绮惧害 | RSSI 绮惧害 |

### 瀹為檯褰卞搷

**鍔熻兘涓嶅彈褰卞搷銆?* `lgw_get_temperature()` 澶辫触鏃讹紝HAL 浣跨敤榛樿娓╁害鍊硷紙涓婃鎴愬姛璇诲彇鐨勫€兼垨鍒濆鍊?25掳C锛夎绠楄ˉ鍋裤€傚浜庡鍐呭浐瀹氱綉鍏筹紝娓╁害鍙樺寲鑼冨洿涓嶅ぇ锛孯SSI 璇樊鍦ㄥ彲鎺ュ彈鑼冨洿鍐呫€?
### 涓轰粈涔堜笉鑳界敤 ESP32-S3 鍐呯疆娓╁害浼犳劅鍣ㄤ唬鏇匡紵

**涓嶈銆?* ESP32-S3 鍐呯疆 tsens 娴嬮噺鐨勬槸 MCU 鍐呮牳娓╁害锛屽湪 CPU 璐熻浇涓嬫瘮鐜娓╁害楂?10~30掳C銆傜敤 MCU 娓╁害鍘昏ˉ鍋?RF 鍓嶇鐨?RSSI锛屽弽鑰屼細寮曞叆鏇村ぇ鐨勭郴缁熻宸€?
### 濡傛灉瑕佸交搴曡В鍐?
鍦?SX1302 妯＄粍闄勮繎鐨?I2C 鎬荤嚎锛圫DA=GPIO4, SCL=GPIO5锛変笂鐒婃帴涓€棰楀吋瀹圭殑娓╁害浼犳劅鍣ㄨ姱鐗囷細
- **STTS751**锛圫T 鍗婂浣擄紝HAL 鍘熺敓鏀寔锛?- **MCP9808**锛圡icrochip锛屽湴鍧€鍏煎锛?- **SE97B**锛圢XP锛屽湴鍧€鍏煎锛?
SO8 灏佽锛屽嚑姣涢挶锛屾斁鍦ㄦā缁勬梺杈瑰嵆鍙€?
---

## 闄勫綍B锛氫簰鏂ラ噺浣跨敤姹囨€?
```
mx_concent (SPI 鎬荤嚎淇濇姢 鈥?鏈€閲嶈):
    thread_up    鈫?lgw_receive()
    thread_jit   鈫?lgw_get_instcnt() / lgw_status() / lgw_send()
    thread_down  鈫?lgw_get_instcnt()锛堢敤浜?jit_enqueue 鑾峰彇褰撳墠鏃堕棿锛?    pkt_fwd_main 鈫?lgw_get_instcnt() / lgw_get_trigcnt() / lgw_get_temperature()

mx_meas_up (涓婅缁熻):
    thread_up    鈫?鍐欏叆 meas_nb_rx_* / meas_up_*
    pkt_fwd_main 鈫?璇诲彇骞舵竻闆?
mx_meas_dw (涓嬭缁熻):
    thread_down  鈫?鍐欏叆 meas_dw_* / meas_nb_tx_requested
    thread_jit   鈫?鍐欏叆 meas_nb_tx_ok / meas_nb_tx_fail / meas_nb_beacon_sent
    pkt_fwd_main 鈫?璇诲彇骞舵竻闆?
mx_stat_rep (鐘舵€佹姤鍛?:
    pkt_fwd_main 鈫?鍐欏叆 status_report, 璁?report_ready=true
    thread_up    鈫?璇诲彇 status_report, 娓?report_ready=false

mx_xcorr (鏅舵尟鏍℃):
    thread_valid 鈫?鍐欏叆 xtal_correct锛圙PS妯″紡涓嬶紝褰撳墠浠ｇ爜涓鐢級
    thread_jit   鈫?璇诲彇 xtal_correct锛堢敤浜?Beacon 棰戠巼琛ュ伩锛?
mx_timeref (GPS 鏃堕棿鍙傝€?:
    thread_gps   鈫?鍐欏叆 time_reference_gps锛堝綋鍓嶇鐢級
    thread_up    鈫?璇诲彇 local_ref锛堢敤浜?UTC 鏃堕棿鎴宠浆鎹級
    thread_down  鈫?璇诲彇锛堢敤浜?Beacon 璋冨害鍜?GPS 鏃堕棿鍙戝皠锛?
mx_meas_gps (GPS 鍧愭爣):
    thread_gps   鈫?鍐欏叆 gps_coord_valid / meas_gps_coord
    pkt_fwd_main 鈫?璇诲彇鐢ㄤ簬缁熻鏄剧ず
```

---

## 闄勫綍C锛氬叧閿父閲忛€熸煡琛?
| 甯搁噺 | 鍊?| 鍚箟 |
|---|---|---|
| `PROTOCOL_VERSION` | 2 | Semtech 鍗忚 v1.6 |
| `NB_PKT_MAX` | 24 | 姣忔 lgw_receive 鏈€澶氬彇鐨勫寘鏁?|
| `FETCH_SLEEP_MS` | 10 | 鏃犲寘鏃剁殑杞闂撮殧(ms) |
| `PUSH_TIMEOUT_MS` | 500 | PUSH_ACK 鎬荤瓑寰呮椂闂?ms) |
| `PULL_TIMEOUT_MS` | 400 | PULL_ACK 鍗曟 recv 瓒呮椂(ms) |
| `DEFAULT_KEEPALIVE` | 5 | PULL_DATA 鍙戦€侀棿闅?s) |
| `DEFAULT_STAT` | 30 | 缁熻鎶ュ憡闂撮殧(s) |
| `WIFI_MAXIMUM_RETRY` | 5 | WiFi 杩炴帴鏈€澶ч噸璇曟鏁?|
| `TX_BUFF_SIZE` | ~13230 | 涓婅 UDP 缂撳啿鍖哄ぇ灏?|
| `PKT_PUSH_DATA` | 0 | 涓婅鏁版嵁鍖呯被鍨?|
| `PKT_PUSH_ACK` | 1 | 涓婅纭鍖呯被鍨?|
| `PKT_PULL_DATA` | 2 | 涓嬭鎷夊彇璇锋眰绫诲瀷 |
| `PKT_PULL_RESP` | 3 | 涓嬭鏁版嵁鍝嶅簲绫诲瀷 |
| `PKT_PULL_ACK` | 4 | 涓嬭鎷夊彇纭绫诲瀷 |
| `PKT_TX_ACK` | 5 | 鍙戝皠纭鍖呯被鍨?|

---

## 闄勫綍D锛氱‖浠跺紩鑴氭槧灏?
鏂囦欢: `main/board_config.h`

| 鍔熻兘 | GPIO | 璇存槑 |
|---|---|---|
| SPI MISO | 13 | SX1302 鏁版嵁杈撳嚭 |
| SPI MOSI | 11 | SX1302 鏁版嵁杈撳叆 |
| SPI CLK | 12 | SPI 鏃堕挓 |
| SPI CS | 14 | SX1302 鐗囬€?|
| SX1302 RESET | 2 | 纭欢澶嶄綅锛堜綆鏈夋晥锛?|
| I2C SDA | 4 | OLED / 娓╁害浼犳劅鍣?|
| I2C SCL | 5 | OLED / 娓╁害浼犳劅鍣?|
| 蹇冭烦 LED | 1 | 鍚姩瀹屾垚鍚庣唲鐏?|
| 钃濊壊 LED | 33 | 涓婅鍖呮寚绀?|
| 缁胯壊 LED | 7 | 鍥炰紶閫氫俊鎸囩ず |
| 绾㈣壊 LED | 27 | 涓嬭鍙戝皠鎸囩ず |
| 鐢ㄦ埛鎸夐挳1 | 0 | IO0锛屽己鍒惰繘 Soft-AP |
| 鐢ㄦ埛鎸夐挳2 | 6 | IO6锛岄鐣?|

---

## 闄勫綍E锛氳繍琛屾椂鏃ュ織瑙ｈ鍙傝€?
浠ヤ笅鏄竴娈靛疄闄呰繍琛屾棩蹇楃殑鍏抽敭閮ㄥ垎娉ㄩ噴锛?
```
*** ESXP1302 Gateway. Version: 1.0.6 ***        鈫?鍥轰欢鐗堟湰

wifi_ssid: 304                                    鈫?NVS 璇诲彇鐨勯厤缃?ns_host: 192.168.71.108
ns_port: 1700
gw_id: AA555A00000021FB
freq_region: cn470
freq_radio0: 486600000                            鈫?Radio0 涓績棰戠巼 486.6MHz
freq_radio1: 487400000                            鈫?Radio1 涓績棰戠巼 487.4MHz

INFO: concentrator started                        鈫?SX1302 鍚姩鎴愬姛

WARNING: no temperature sensor found.             鈫?姝ｅ父锛屾病鏈夊閮?STTS751

INFO: Received pkt from mote: 6F6C6C65 (fcnt=28535) 鈫?鏀跺埌缁堢涓婅鍖?JSON up: {"rxpk":[{..., "data":"aGVsbG8gd29ybGQ="}]} 鈫?Base64 = "hello world"
INFO: [up] PUSH_ACK received in 2 ms             鈫?NS 纭鏀跺埌

INFO: [down] PULL_ACK received in 143 ms          鈫?NS 瀛樻椿纭

ERROR: failed to get current temperature          鈫?娓╁害璇诲彇澶辫触锛堣闄勫綍A锛?### Concentrator temperature unknown ###           鈫?娓╁害涓嶅彲鐢紝涓嶅奖鍝嶅姛鑳?
##### 2026-02-22 01:23:45 UTC #####               鈫?30绉掔粺璁℃姤鍛?# RF packets received: 3
# CRC_OK: 100.00%
# PUSH_DATA acknowledged: 100.00%                鈫?鎵€鏈変笂琛屽寘 NS 閮界‘璁や簡
# PULL_DATA sent: 3 (100.00% acknowledged)        鈫?涓?NS 閫氫俊姝ｅ父
```
