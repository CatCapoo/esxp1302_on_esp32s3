# BUG-010锛歅USH_DATA acknowledged 浣庯紙ackr 22鈥?0%锛?
- **鏃ユ湡**锛?026-02-21  
- **鏂囦欢**锛歚main/packet_forwarder/lora_pkt_fwd.c`銆乣main/packet_forwarder/global_conf.json/global_conf.cn490.json`  
- **涓ラ噸绾у埆**锛氬姛鑳芥€ч敊璇紙NS 鏃犳硶姝ｅ父纭涓婅鍖咃紝缃戝叧缁熻 ackr 鏋佷綆锛?- **commit**锛歚55c1e82` fix: improve ackr by fixing WiFi PS, drain loop, and timeouts

---

## 鐜拌薄

缃戝叧杩愯鍚?ackr锛圥USH_DATA acknowledged锛夐暱鏈熷崱鍦?22鈥?0%锛?鍗充娇缃戠粶鏈嶅姟鍣ㄦ棩蹇楁樉绀?100% 鏀跺埌骞跺洖澶嶄簡 PUSH_ACK銆?
```
INFO: [up] PUSH_ACK received in 0 ms      鈫?绔嬪埢鏀跺埌锛堝疄涓轰笂杞仐鐣欑殑杩囨湡 ACK锛?WARNING: [up] ignored out-of sync ACK packet
PUSH_DATA acknowledged: 22.22%            鈫?鏋佷綆
PULL_DATA sent: 5 (100.00% acknowledged)  鈫?PULL_ACK 姝ｅ父
```

---

## 鏍规湰鍘熷洜锛堝洓涓嫭绔嬮棶棰橈級

### 闂 1锛歁QTT TLS 杩炴帴闃诲锛堟渶涓ラ噸锛?
`ENABLE_MQTT` 鏈叧闂紝`mqtt_task` 鍚姩鍚庡皾璇曞悜 `mqtt://192.168.1.202` 寤虹珛 TLS 杩炴帴銆?鏃?Broker 鏃?TLS 鎻℃墜瓒呮椂闀胯揪 **10 绉?*锛屾湡闂?FreeRTOS 璋冨害鍣ㄨ鍗犵敤锛?`thread_up` 鏃犳硶鍦?`recv()` 绐楀彛鍐呰繍琛岋紝ackr 璺岃嚦 22鈥?0%銆?
**淇锛?*

```c
#define ENABLE_MQTT  0   /* set to 1 to enable MQTT, 0 to disable */
```

鎵€鏈?MQTT 鐩稿叧浠ｇ爜鐢?`#if ENABLE_MQTT ... #endif` 鍖呰９锛屼笉鍚敤鏃跺畬鍏ㄤ笉缂栬瘧銆?
---

### 闂 2锛歚struct timeval.tv_usec` 婧㈠嚭锛堥潤榛?bug锛?
`push_timeout_half` 鐨勮绠楁柟寮忎负锛?
```c
static struct timeval push_timeout_half = {0, (PUSH_TIMEOUT_MS * 500)};
```

`tv_usec` 鍚堟硶鑼冨洿鏄?`[0, 999999]`銆傚綋 `PUSH_TIMEOUT_MS = 2000` 鏃讹細

```
tv_usec = 2000 脳 500 = 1,000,000  鈫?婧㈠嚭锛?```

ESP32 lwIP 灏嗘棤鏁堢殑 `tv_usec = 1,000,000` 澶勭悊涓?`0`锛?`recv()` 姣忔绔嬪嵆杩斿洖 `EAGAIN`锛屾案杩滅瓑涓嶅埌 ACK銆?
**淇锛?*

```c
// 鍘熷€硷紙婧㈠嚭锛夛細
#define PUSH_TIMEOUT_MS  2000   // tv_usec = 1,000,000 鈫?婧㈠嚭
#define PULL_TIMEOUT_MS  1000   // tv_usec = 1,000,000 鈫?婧㈠嚭

// 淇鍚庯紙瀹夊叏锛夛細
#define PUSH_TIMEOUT_MS  500    // tv_usec = 250,000 (250ms/娆★紝鍏?500ms)
#define PULL_TIMEOUT_MS  400    // tv_usec = 400,000 (400ms)
```

鍚屾淇 `global_conf.cn490.json`锛?
```json
// 鍘熷€硷紙100ms 瀵逛换浣曠湡瀹炵綉缁滈兘澶煭锛夛細
"push_timeout_ms": 100

// 淇鍚庯細
"push_timeout_ms": 500
```

骞堕噸鏂扮敓鎴?`main/global_json.h`銆?
---

### 闂 3锛歞rain 寰幆 `setsockopt` 澶辫触椋庨櫓

姣忔鍙戦€佹柊 PUSH_DATA 鍓嶏紝鏃т唬鐮佺敤鍙?`setsockopt` 娓呯┖ stale ACK锛?
```c
// 鏃т唬鐮侊紙鏈夐闄╋級锛?struct timeval drain_tv = {0, 1000};
setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &drain_tv, sizeof drain_tv);
while (recv(sock_up, _tmp, sizeof _tmp, 0) > 0) {}
setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &push_timeout_half, sizeof push_timeout_half);
// ^ 鑻ユ娆?setsockopt 澶辫触锛宻ocket 瓒呮椂姘歌繙鍗″湪 1ms锛?```

ESP32 lwIP 鍦ㄥ唴瀛樺帇鍔涗笅鍋跺彂 `setsockopt` 澶辫触锛岀浜屾璋冪敤澶辫触浼氬鑷?`sock_up` 鐨勮秴鏃舵案涔呭仠鍦?1 ms锛屾鍚庢瘡杞?`recv()` 鍧囩珛鍗宠秴鏃躲€?
**淇锛?* 鏀圭敤 `MSG_DONTWAIT` 鏍囧織锛屽畬鍏ㄤ笉鎺ヨЕ `SO_RCVTIMEO`锛?
```c
// 鏂颁唬鐮侊紙瀹夊叏锛夛細
uint8_t _tmp[4];
while (recv(sock_up, (void *)_tmp, sizeof _tmp, MSG_DONTWAIT) > 0) {}
```

---

### 闂 4锛歐iFi Modem Sleep 瀵艰嚧 ACK 寤惰繜鍒拌揪锛堟渶鍏抽敭锛?
ESP32 WiFi 榛樿寮€鍚?`WIFI_PS_MIN_MODEM`锛堟渶灏忚皟鍒惰В璋冨櫒鐫＄湢妯″紡锛夈€?WiFi 鏃犵嚎鐢典細鍛ㄦ湡鎬т紤鐪狅紝AP 鍦ㄦ鏈熼棿缂撳啿鎵€鏈夊彂寰€缃戝叧鐨勫崟鎾寘锛?鐩村埌涓嬩竴涓?DTIM Beacon 鎵嶉噴鏀俱€傚吀鍨嬬紦鍐插欢杩?**100鈥?00 ms**銆?
浜ゅ弶楠岃瘉锛歂S Mock 鏈嶅姟鍣ㄦ棩蹇楁樉绀?100% 宸插彂鍑?PUSH_ACK锛?浣嗙綉鍏?`recv()` 鍦?250 ms 绐楀彛鍐呬粛鏈敹鍒帮紝璇存槑 ACK 琚?AP 缂撳啿鍒扮獥鍙ｄ箣澶栥€?
**淇锛?* 鍦?`wifi_init_sta()` 涓?`esp_wifi_start()` 涔嬪悗绔嬪嵆绂佺敤 Modem Sleep锛?
```c
ESP_ERROR_CHECK(esp_wifi_start());
/* Disable modem sleep so incoming UDP ACKs are not buffered by the AP.
 * Without this, ACKs can be delayed 100-300ms and miss the recv() window. */
ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
```

> **鍔熻€楄鏄?*锛氱綉鍏虫暣鏈猴紙ESP32-S3 + SX1302锛夊姛鑰楃害 400鈥?00 mA锛?> 绂佺敤 Modem Sleep 澧炲姞绾?20鈥?0 mA锛? 10%锛夛紝
> 瀵瑰父渚涚數鐨勫熀纭€璁炬柦璁惧鍙互蹇界暐銆?
---

## 鏁堟灉

| 闃舵 | ackr 鍏稿瀷鍊?| 涓昏鍘熷洜 |
|------|------------|---------|
| 淇鍓?| 22鈥?0% | MQTT TLS 闃诲 |
| 绂佺敤 MQTT 鍚?| 53鈥?0% | 瓒呮椂婧㈠嚭 + WiFi PS |
| 淇瓒呮椂鍊煎悗 | 60鈥?0% | WiFi PS |
| 绂佺敤 WiFi PS 鍚?| **86鈥?00%** | 鈥?|

---

## 楠岃瘉鏂规硶

淇骞剁儳褰曞悗锛岃瀵熶覆鍙ｇ粺璁¤锛?
```
PUSH_DATA acknowledged: 100.00%   鈫?鐩爣
PULL_DATA sent: N (100.00% acknowledged)
```

鍙敤鏈湴 Mock 鑴氭湰鍦ㄥ眬鍩熺綉鍐呭仛浜ゅ弶瀵规瘮楠岃瘉
锛堣瑙?[test_pkt_fwd_ns_uplink_memo.md](../test_notes/test_pkt_fwd_ns_uplink_memo.md)锛夈€?
---

## 鐩稿叧鏂囦欢

- `main/packet_forwarder/lora_pkt_fwd.c`
- `main/packet_forwarder/global_conf.json/global_conf.cn490.json`
- `main/global_json.h`锛堥噸鏂扮敓鎴愶級
- `scripts/lora_ns_mock.py`锛堟湰鍦伴獙璇佸伐鍏凤級
