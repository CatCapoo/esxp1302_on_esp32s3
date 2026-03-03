# CHANGE-002锛氬畬鎴?GPS 绾跨▼ ESP32 绉绘锛屽疄鐜版椂闂村悓姝ヤ笌鍧愭爣涓婃姤

- **鏃ユ湡**锛?026-02-28  
- **鍒嗘敮**锛歚bringup/gps`  
- **娑夊強鏂囦欢**锛歚main/packet_forwarder/lora_pkt_fwd.c`銆乣main/board_config.h`

---

## 鑳屾櫙

鍘熷伐绋嬩粠 Linux 鍙傝€冨疄鐜帮紙lora_pkt_fwd锛夌Щ妞嶈€屾潵锛孏PS 绾跨▼鐩稿叧浠ｇ爜鏁翠綋瀛樺湪浜庢簮鏂囦欢涓紝  
浣嗕粠鏈湪 ESP32 涓婅繍琛岃繃锛屽睘浜?TODO 鐘舵€佺殑姝讳唬鐮併€傚叿浣撹〃鐜颁负锛?
- `thread_gps` / `thread_valid` 琚?`#if 0` 鍖呰９锛屼粠鏈惎鍔?- 涓诲惊鐜腑鏈変竴娈典复鏃惰皟璇曚唬鐮佺洿鎺ヨ鍙?GPS UART锛屼粎鎵撳嵃涓嶈В鏋愶紝鍗犵敤浜嗕覆鍙ｆ暟鎹?- `thread_gps` 鍐呴儴浣跨敤 POSIX `read()` 绯荤粺璋冪敤锛屾棤娉曞湪 ESP-IDF 涓婃搷浣?UART
- 鏃堕棿鍚屾浠呬緷璧?u-blox UBX 绉佹湁鍗忚甯э紝鑰岀‖浠?ATGM336H 浠呰緭鍑烘爣鍑?NMEA

姝ゆ鍙樻洿灏嗕互涓婂洓涓棶棰樺叏閮ㄤ慨澶嶏紝瀹屾垚 GPS 鍔熻兘鐨?ESP32 绉绘銆?
---

## 闂鍒嗘瀽

### 闂涓€锛欸PS 绾跨▼浠庢湭鍚姩

绾跨▼鍚姩浠ｇ爜娌跨敤浜?Linux `pthread_create`锛屽苟琚?`#if 0` 鍖呰９锛?
```c
#if 0
    pthread_create(&thrid_gps, NULL, (void * (*)(void *))thread_gps, NULL);
    pthread_create(&thrid_valid, NULL, (void * (*)(void *))thread_valid, NULL);
#endif
```

娌℃湁 `thread_gps`锛孨MEA 鏁版嵁姘歌繙涓嶄細琚В鏋愶紝鍧愭爣鍜屾椂闂村彉閲忔案杩滀笉浼氳鏇存柊銆?
### 闂浜岋細涓诲惊鐜姠鍗?UART 鏁版嵁

涓诲惊鐜殑缁熻鍛ㄦ湡鍐呮湁涓€娈典复鏃惰皟璇曚唬鐮侊紝姣忔缁熻闂撮殧閮戒細璋冪敤 `uart_read_bytes`  
灏?GPS 缂撳啿鍖轰腑鐨勬暟鎹叏閮ㄨ璧帮紝浠呭仛鎵撳嵃锛屽畬鍏ㄤ笉瑙ｆ瀽锛?
```c
// 涓诲惊鐜瘡 5 绉?ESP_ERROR_CHECK(uart_get_buffered_data_len(gps_tty_fd, &length));
length = uart_read_bytes(gps_tty_fd, data, min, 100);
// 鈫?鏁版嵁鍏ㄨ鍚冩帀锛宼hread_gps 鎷夸笉鍒颁换浣曞瓧鑺?```

杩欎篃瑙ｉ噴浜嗕负浠€涔堟棩蹇椾腑鍙互鐪嬪埌姝ｅ父鐨?NMEA 杈撳嚭锛堟湁鍗槦銆佹湁瀹氫綅锛夛紝  
浣嗙▼搴忓嵈鎶ュ憡鍧愭爣涓嶅彲鐢ㄢ€斺€旀暟鎹涓诲惊鐜秷鑰楋紝`thread_gps` 浠庢湭杩愯鏇存棤浠庤В鏋愩€?
鍚屾椂杩欐浠ｇ爜姣忔璇?900+ 瀛楄妭鐨勬暣鍧楁暟鎹紝澶氭潯 NMEA 璇彞娣峰悎鎵撳嵃锛? 
閫犳垚鏃ュ織涓嚭鐜板抚閿欎綅鐨勪贡鐮佺幇璞★細

```
$GPGSV,2,1,07,01,73,05ZDA,052329.000   鈫?涓ゆ潯璇彞绮樿繛
$GPGSV,2,1,07,01,73,=SOC_BootLoader   鈫?瀹屽叏涔辩爜
```

### 闂涓夛細`thread_gps` 浣跨敤浜嗛敊璇殑 UART API

`thread_gps` 鍐呴儴浣跨敤 POSIX `read()` 绯荤粺璋冪敤锛?
```c
ssize_t nb_char = read(gps_tty_fd, serial_buff + wr_idx, LGW_GPS_MIN_MSG_SIZE);
```

浣?`gps_tty_fd` 瀹為檯涓婃槸 ESP-IDF 鐨?`uart_port_t`锛堟暣鏁扮鍙ｅ彿锛夛紝  
涓嶆槸 Linux 鏂囦欢鎻忚堪绗︼紝`read()` 鍦?ESP-IDF 鐜涓棤娉曟搷浣?UART銆?
### 闂鍥涳細鏃堕棿鍚屾浠呬緷璧?UBX 绉佹湁鍗忚

`gps_process_sync()` 鍙湪鏀跺埌 `UBX_NAV_TIMEGPS` 娑堟伅鏃惰Е鍙戯細

```c
} else if (latest_msg == UBX_NAV_TIMEGPS) {
    gps_process_sync();  // 鈫?ATGM336H 浠庝笉鍙戝嚭姝ゅ抚
}
// NMEA_RMC 鍒嗘敮鍙皟鐢?gps_process_coords()锛屼笉鍚屾鏃堕棿
```

ATGM336H 鏄浗浜фā鍧楋紝榛樿鍙緭鍑烘爣鍑?NMEA 鍗忚锛圙PS+鍖楁枟鍙屾ā锛夛紝  
涓嶈緭鍑?u-blox UBX 绉佹湁鍗忚锛屽洜姝?`UBX_NAV_TIMEGPS` 姘歌繙涓嶄細鍑虹幇銆? 
瀹為檯涓?`$GNRMC` 璇彞鏈韩灏卞寘鍚畬鏁寸殑鏃ユ湡鏃堕棿锛宍lgw_parse_nmea()` 瑙ｆ瀽鍚? 
宸插皢鏃堕棿鍐欏叆鍐呴儴鍙橀噺锛坄gps_time_ok = true`锛夛紝鍙樊璋冪敤 `gps_process_sync()` 瑙﹀彂鍚屾銆?
---

## 淇敼鍐呭

### 1. 鐢?FreeRTOS 浠诲姟鏇挎崲 pthread锛屾寮忓惎鍔?GPS 绾跨▼

```c
// 鏃э細#if 0 鍖呰９鐨?pthread 浠ｇ爜
// 鏂帮細
#if GPS_ENABLE
    if (gps_enabled == true) {
        xTaskCreatePinnedToCore((TaskFunction_t)thread_gps,  "thread_gps",
                                4096*2, NULL, 6, NULL, tskNO_AFFINITY);
        xTaskCreatePinnedToCore((TaskFunction_t)thread_valid, "thread_valid",
                                4096*2, NULL, 6, NULL, tskNO_AFFINITY);
    }
#endif
```

### 2. 绂佺敤涓诲惊鐜腑鐨?GPS UART 璇诲彇

鐢?`#if 0` 绂佺敤涓诲惊鐜噷鐨勮皟璇曡鍙栧潡锛岃 `thread_gps` 鐙崰涓插彛锛?
```c
#if 0  /* Disabled: thread_gps now handles UART reading and NMEA parsing */
    if (gps_enabled) {
        uart_read_bytes(...);   // 杩欐浠ｇ爜涓嶅啀鎵ц
        ...
    }
#endif
```

### 3. 鐢?`uart_read_bytes()` 鏇挎崲 `read()`

```c
// 鏃?ssize_t nb_char = read(gps_tty_fd, serial_buff + wr_idx, LGW_GPS_MIN_MSG_SIZE);

// 鏂?int nb_char = uart_read_bytes(gps_tty_fd, (uint8_t *)(serial_buff + wr_idx),
                              LGW_GPS_MIN_MSG_SIZE, pdMS_TO_TICKS(1000));
```

`uart_read_bytes` 甯﹁秴鏃跺弬鏁帮紝閬垮厤姝荤瓑锛屼篃绗﹀悎 FreeRTOS 浠诲姟璋冨害涔犳儻銆?
### 4. 鍦?NMEA_RMC 鍒嗘敮瑙﹀彂鏃堕棿鍚屾

```c
} else if (latest_msg == NMEA_RMC) {
    gps_process_coords();
    gps_process_sync();  // 鈫?鏂板锛欰TGM336H 鏃?UBX锛屼粠 RMC 鍚屾鏃堕棿
}
```

`lgw_parse_nmea()` 瑙ｆ瀽 `$GNRMC` 鍚庡凡鏇存柊 `gps_time_ok` 鍜屾椂闂村彉閲忥紝  
姝ゆ椂璋冪敤 `gps_process_sync()` 鍙互灏?GPS 鏃堕棿涓?SX1302 纭欢璁℃暟鍣ㄥ仛鍏宠仈銆?
### 5. 灏?GPS 缂栬瘧寮€鍏冲拰鏃ュ織绛夌骇绉诲叆 `board_config.h`

```c
// board_config.h 鏂板
#define GPS_ENABLE       1   // 0 = 绂佺敤鎵€鏈?GPS 浠ｇ爜
#define GPS_LOG_VERBOSE  0   // 0=闈欓粯  1=鍏抽敭浜嬩欢  2=瀹屾暣 NMEA 杞偍
```

鍘?`lora_pkt_fwd.c` 涓殑 `#define GPS_LOG_VERBOSE 1` 绉婚櫎锛? 
GPS 鍒濆鍖栥€佺嚎绋嬪惎鍔ㄣ€佸嚱鏁板畾涔夊潎鐢?`#if GPS_ENABLE` 鍖呰９銆?
### 6. 鏀瑰杽 GPS 鏃ュ織

| 鏃ュ織绛夌骇 | 杈撳嚭鍐呭 |
|----------|----------|
| `GPS_LOG_VERBOSE 0` | 鏃?GPS 璋冭瘯杈撳嚭锛宍could not get GPS time` 闈欓粯 |
| `GPS_LOG_VERBOSE 1` | 姣忔鍚屾鎴愬姛鎵撳嵃 UTC 鏃堕棿锛涙瘡 5 绉掓墦鍗?fix 鐘舵€?鍧愭爣 |
| `GPS_LOG_VERBOSE 2` | 锛堥鐣欙級瀹屾暣 NMEA 甯ц浆鍌?|

---

## 楠岃瘉缁撴灉

| 楠岃瘉椤?| 缁撴灉 |
|--------|------|
| 绾跨▼鍚姩 | 鉁?`thread_gps spawned` / `thread_valid spawned` |
| GPS 鍧愭爣 | 鉁?`GPS coordinates: latitude 31.31109, longitude 121.37719, altitude 45 m` |
| 鏃堕棿鍙傝€冩湁鏁?| 鉁?`Valid time reference (age: 0 sec)` |
| JSON 涓婃姤鍚潗鏍?| 鉁?`"lati":31.31109,"long":121.37719,"alti":45` |
| 缂栬瘧鏃犻敊璇?| 鉁?`Project build complete` |

---

## 纭欢鎺ョ嚎澶囨敞

| 淇″彿 | GPIO |
|------|------|
| GPS TX锛堟ā鍧楀彂锛?| GPIO 20锛圗SP32 RX锛?|
| GPS RX锛堟ā鍧楁敹锛?| GPIO 19锛圗SP32 TX锛?|
| GPS VCC | 3.3 V |
| GPS GND | GND |

- 娉㈢壒鐜囷細9600锛圓TGM336H 榛樿锛?- 杈撳嚭璇彞锛歚$GNGGA`銆乣$GNRMC`銆乣$GNZDA`銆乣$GPGSV`銆乣$BDGSV` 绛夛紙GPS+鍖楁枟鍙屾ā锛?
---

## 鐩稿叧鏂囨。

- 瀛︿範绗旇锛歔gps_nmea_and_lorawan_sync.md](../learning/gps_nmea_and_lorawan_sync.md)
- 瀵瑰簲 commit锛歚68eb6a4 fix: complete ESP32 port of GPS sync thread (ATGM336H, NMEA-only)`
