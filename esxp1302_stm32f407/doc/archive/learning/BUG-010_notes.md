# BUG-010 瀛︿範绗旇锛歛ckr 涓轰粈涔堜綆锛熸€庝箞鎵惧埌鐨勶紵

> 瀵瑰簲 bugfix: [BUG-010](../bugfix/BUG-010_push_data_ack_low_out_of_sync.md)  
> 瀵瑰簲浠ｇ爜: `main/packet_forwarder/lora_pkt_fwd.c`  
> 鍐欐硶锛氱粨鍚堝疄闄呬唬鐮佽鍙峰拰涓插彛鏃ュ織锛岃繕鍘熷綋鏃跺彂鐢熶簡浠€涔?
---

## 绗浂姝ワ細鍏堢湅鎳備覆鍙ｈ緭鍑哄湪璇翠粈涔?
缃戝叧杩愯鏃朵覆鍙ｄ細鎸佺画鎵撳嵃鏃ュ織锛屽叧閿殑涓€琛屾槸 30 绉掔粺璁★細

```
##### 2026-02-21 10:00:30 GMT #####
### [UPSTREAM] ###
# RF packets received by concentrator: 3
# CRC_OK: 100.00%, CRC_FAIL: 0.00%, NO_CRC: 0.00%
# RF packets forwarded: 3 (171 bytes)
# PUSH_DATA datagrams sent: 2 (2154 bytes)
# PUSH_DATA acknowledged: 22.22%        鈫?杩欒鏄棶棰樻墍鍦?### [DOWNSTREAM] ###
# PULL_DATA sent: 3 (100.00% acknowledged)
```

**ackr锛坅cknowledged ratio锛夊氨鏄渶鍚庨偅琛岀殑鐧惧垎姣?*锛岃绠楁柟寮忔槸锛?
$$\text{ackr} = \frac{\text{鏀跺埌 PUSH\_ACK 娆℃暟}}{\text{鍙戝嚭 PUSH\_DATA 娆℃暟}} \times 100\%$$

鏀?LoRa 鍖呫€丆RC 妫€鏌ャ€佷笂鎶?JSON 杩欎簺閮芥槸姝ｅ父鐨勶紝
闂涓撻棬鍑哄湪"鍙戝嚭鍘讳箣鍚庢湁娌℃湁鏀跺埌 NS 鐨勭‘璁ゅ洖鍖?杩欎竴姝ャ€?
---

## 绗竴姝ワ細鍏堟悶娓呮 PUSH_DATA 鈫?PUSH_ACK 鐨勫畬鏁存祦绋?
浠ｇ爜閲岃礋璐ｈ繖浠朵簨鐨勬槸 `thread_up` 鍑芥暟銆?瀹冩槸涓€涓棤闄愬惊鐜紝澶ф姣忛殧 2 绉掓墽琛屼竴杞紝娴佺▼濡備笅锛?
```
鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?thread_up 涓€杞殑鏃堕棿绾匡紙绾?2 绉掞級                          鈹?鈹?                                                        鈹?鈹? 0ms      鏀?LoRa 鍖咃紝缁?JSON锛屽～鍏?buff_up              鈹?鈹?          鐢熸垚闅忔満 token锛堟瘮濡?0x1C7F锛夛紝鍐欏叆鍖呭ご         鈹?鈹?          鈫?                                            鈹?鈹?          drain锛氭竻绌?sock_up 閲屽彲鑳芥畫鐣欑殑鏃?ACK           鈹?鈹?          鈫?                                            鈹?鈹?          send(sock_up, buff_up, ...)   鈫?鍙戝嚭 PUSH_DATA 鈹?鈹?          鈫?                                            鈹?鈹?          绗?1 娆?recv()锛岀瓑鏈€澶?250ms                   鈹?鈹?          鈫?瓒呮椂鎴栨敹鍒?                                 鈹?鈹?          绗?2 娆?recv()锛岀瓑鏈€澶?250ms                   鈹?鈹?          鈫?                                            鈹?鈹?          妫€鏌ユ敹鍒扮殑鍖咃細token 瀵逛笉瀵癸紵鐗堟湰瀵逛笉瀵癸紵         鈹?鈹?          瀵逛簡 鈫?meas_up_ack_rcv++锛屾墦鍗?PUSH_ACK in Xms" 鈹?鈹?          涓嶅 鈫?鎵撳嵃"out-of-sync"璀﹀憡                   鈹?鈹?                                                        鈹?鈹?~500ms   杩欒疆缁撴潫锛岀户缁瓑涓嬩竴鎵?LoRa 鍖?                  鈹?鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?```

瀵瑰簲浠ｇ爜鍦?lora_pkt_fwd.c 绗?2633鈥?683 琛岋細

```c
// 绗?2633 琛岋細drain 鏃?ACK
{
    uint8_t _tmp[4];
    while (recv(sock_up, (void *)_tmp, sizeof _tmp, MSG_DONTWAIT) > 0) {}
}

// 绗?2641 琛岋細鍙戝嚭 PUSH_DATA
send(sock_up, (void *)buff_up, buff_index, 0);

// 绗?2651 琛岋細绛夊緟 ACK锛屽惊鐜袱娆★紝姣忔鏈€澶氱瓑 push_timeout_half
for (i=0; i<2; ++i) {
    j = recv(sock_up, (void *)buff_ack, sizeof buff_ack, 0);
    if (j == -1) {
        if (errno == EAGAIN) { /* 瓒呮椂锛岀户缁笅涓€娆″惊鐜?*/
            continue;
        } else {
            MSG("WARNING: [up] recv error: %s\n", strerror(errno));
            break;
        }
    } else if ((buff_ack[1] != token_h) || (buff_ack[2] != token_l)) {
        MSG("WARNING: [up] ignored out-of sync ACK packet\n");  // token 涓嶅尮閰?        continue;
    } else {
        MSG("INFO: [up] PUSH_ACK received in %i ms\n", ...);
        meas_up_ack_rcv += 1;  // 杩欎釜璁℃暟鍣ㄥ氨鏄?ackr 鐨勫垎瀛?        break;
    }
}
```

**`push_timeout_half` 鎺у埗姣忔 `recv()` 鏈€澶氱瓑澶氫箙**锛屽畠鍦ㄧ 256 琛屽畾涔夛細

```c
static struct timeval push_timeout_half = {0, (PUSH_TIMEOUT_MS * 500)};
```

---

## 浜ゅ弶楠岃瘉锛氬厛纭闂鍦ㄥ摢涓€渚?
閬囧埌 ackr 浣庯紝鏈€瀹规槗鐘殑閿欒鏄洿鎺ユ敼浠ｇ爜鈥斺€斾絾鍦ㄦ敼涔嬪墠锛岃鍏堢煡閬?*闂鍑哄湪鍝噷**銆?
鐢ㄦ湰鍦?Mock 鏈嶅姟鍣ㄦ浛浠ｇ湡瀹?NS锛?
```bash
python3 scripts/lora_ns_mock.py 1700
```

杩欎釜鑴氭湰鍋氱殑浜嬫瀬鍏剁畝鍗曪細鏀跺埌 PUSH_DATA 灏?*绔嬪埢**鍥?PUSH_ACK锛屽苟鎵撳嵃涓€琛屾棩蹇椼€?
瀵规瘮涓や晶鏃ュ織锛?
```
[Mock 鏈嶅姟鍣╙  PUSH_DATA from ('192.168.1.50', 52313) token=0x1C7F -> PUSH_ACK sent 鉁?[缃戝叧涓插彛]     PUSH_DATA acknowledged: 53.00%   鈫?53% 娌℃敹鍒?```

**Mock 璇村畠鍙戝嚭浜嗭紝缃戝叧璇村畠娌℃敹鍒般€?* 缁撹锛氶棶棰樺湪缃戝叧渚э紝涓嶆槸 NS 闂锛屼篃涓嶆槸"ACK 娌″彂鍑?銆?闂鍦ㄧ綉鍏?鍙戝嚭 PUSH_DATA 涔嬪悗鑳藉惁鍦ㄦ椂闂寸獥鍙ｅ唴鏀跺埌鍥炲寘"杩欎竴姝ャ€?
---

## 闂 1锛歁QTT 瀹㈡埛绔殑 TLS 鎻℃墜鍚冩帀浜嗘墍鏈夋椂闂?
### 妗堝彂鐜板満锛堜慨澶嶅墠鐨勪覆鍙ｆ棩蹇楋級

```
E (15234) MQTT_CLIENT: Error transport connect
E (15234) esp-tls: Failed to open new connection
E (15234) TRANSPORT_SSL: Failed to open a new connection
MQTT_EVENT_DISCONNECTED
... 锛?0 绉掑悗閲嶈瘯锛?E (25891) MQTT_CLIENT: Error transport connect
```

杩?10 绉掗噷锛岀綉鍏冲嚑涔庢病鏈夋墦鍗颁换浣?PUSH_ACK 鐩稿叧鐨勬棩蹇椼€?
### 闂鍦ㄥ摢閲岋細FreeRTOS 浠诲姟鍜?MQTT 浠诲姟鐨勫叧绯?
ESP32 杩愯 FreeRTOS銆傜▼搴忎笉鏄?涓€琛屼竴琛岄『搴忔墽琛?鐨勶紝
鑰屾槸鍚屾椂璺戝涓?浠诲姟"锛圱ask锛夛紝姣忎釜浠诲姟鏈夌嫭绔嬬殑鎵ц娴併€?FreeRTOS 鎸夌収浼樺厛绾у拰鏃堕棿鐗囪疆娴佽鍚勪换鍔℃墽琛屻€?
WiFi 杩炰笂鍚庯紝绗?4125 琛屽惎鍔ㄤ簡涓や釜浠诲姟锛?
```c
// 绗?4125 琛岋細
#if ENABLE_MQTT
    xTaskCreatePinnedToCore(
        (TaskFunction_t) mqtt_task,  // 浠诲姟鍑芥暟
        "mqtt",                      // 浠诲姟鍚?        1*4096,                      // 鏍堝ぇ灏忥紙瀛楄妭锛?        NULL,
        6,                           // 浼樺厛绾?6
        &mqtt_handle,
        0                            // 鍥哄畾鍦?CPU 鏍?0
    );
#endif

// 绗?4128 琛岋細
xTaskCreatePinnedToCore(
    (TaskFunction_t) pkt_fwd_task,   // 鍖呭惈 thread_up 鐨勪换鍔?    "pkt_fwd",
    1*4096,
    NULL,
    6,                               // 浼樺厛绾т篃鏄?6
    &pkt_fwd_handle,
    0                                // 涔熷浐瀹氬湪 CPU 鏍?0 锛?);
```

**鍏抽敭鐐癸細涓や釜浠诲姟閮藉浐瀹氬湪 CPU 鏍?0锛屼紭鍏堢骇鐩稿悓銆?*
FreeRTOS 瀵瑰悓浼樺厛绾т换鍔″仛鏃堕棿鐗囪疆鎹紝浣嗗鏋滀竴涓换鍔￠暱鏃堕棿涓嶄富鍔ㄨ鍑?CPU
锛堟瘮濡傚崱鍦?TLS 鎻℃墜閲岋級锛屽彟涓€涓换鍔″氨寰椾笉鍒板厖鍒嗘墽琛屾満浼氥€?
`mqtt_task` 閲岃皟鐢ㄤ簡 MQTT 瀹㈡埛绔繛鎺?`mqtt://192.168.1.202`锛堢 187鈥?88 琛岀殑閰嶇疆锛夈€?杩欏彴鏈哄櫒娌℃湁 MQTT Broker锛孴CP 杩炴帴鍜?TLS 鎻℃墜浼?*瓒呮椂绛夊緟闀胯揪 10 绉?*銆?鍦ㄨ繖 10 绉掗噷锛宍pkt_fwd_task`锛堝寘鍚?`thread_up`锛夎兘鎶㈠埌鐨?CPU 鏃堕棿鏋佸皯锛?瀵艰嚧 `recv()` 鐨?500ms 绐楀彛鍐呮牴鏈病鏈夋満浼氳繍琛岋紝ACK 鍒颁簡涔熸病浜鸿銆?
### 淇锛氭妸 MQTT 瀹屽叏缂栬瘧鎺?
绗?187 琛岋細

```c
// 淇鍓嶏細
#define ENABLE_MQTT  1

// 淇鍚庯細
#define ENABLE_MQTT  0   /* set to 1 to enable MQTT, 0 to disable */
```

C 璇█鐨勬潯浠剁紪璇戯紙`#if` / `#endif`锛夛細
濡傛灉 `ENABLE_MQTT` 涓?0锛屼袱涓?`#if ENABLE_MQTT` 鈥?`#endif` 涔嬮棿鐨勬墍鏈変唬鐮?**瀹屽叏涓嶄細琚紪璇戣繘鍥轰欢**锛屽氨鍍忚繖浜涗唬鐮佷笉瀛樺湪涓€鏍枫€?`xTaskCreatePinnedToCore(mqtt_task, ...)` 閭ｈ涔熷氨涓嶄細杩愯銆?
### 淇鍚庢晥鏋?
ackr 浠?22鈥?0% 鎻愬崌鍒?53鈥?0%銆侻QTT 闃诲纭疄鏄富瑕佸共鎵版簮锛屼絾杩樻湁鍏朵粬闂銆?
---

## 闂 2锛歵v_usec 濉簡涓€涓秴鍑哄悎娉曡寖鍥寸殑鍊硷紝lwIP 鎶婂畠褰?0 澶勭悊

### 鑳屾櫙锛歚struct timeval` 鏄粈涔堬紝鎬庝箞鐢?
`recv()` 涓嶆槸姘歌繙绛変笅鍘荤殑銆傚彲浠ラ鍏堢粰 socket 璁剧疆涓€涓秴鏃跺€硷紝
瓒呰繃鏃堕棿娌℃敹鍒版暟鎹紝`recv()` 灏辫繑鍥為敊璇紝閿欒鐮佹槸 `EAGAIN`锛堟剰鎬濇槸"涓嬫鍐嶈瘯"锛夈€?
璁剧疆瓒呮椂鐢ㄧ殑鏄?`setsockopt()`锛岃秴鏃舵椂闂寸敤 `struct timeval` 琛ㄧず锛?
```c
// <sys/time.h> 閲屽畾涔夌殑缁撴瀯浣擄細
struct timeval {
    long tv_sec;   // 绉掓暟锛堟暣鏁伴儴鍒嗭級
    long tv_usec;  // 寰鏁帮紙灏忔暟閮ㄥ垎锛?                   // 1 绉?= 1,000,000 寰
                   // 鈿狅笍 鍚堟硶鑼冨洿锛? ~ 999,999锛堜笉鑳?>= 1,000,000锛?};

// 渚嬶細璁剧疆 250ms 瓒呮椂
struct timeval t;
t.tv_sec  = 0;
t.tv_usec = 250000;  // 250,000 寰 = 0.25 绉?= 250ms

// 渚嬶細璁剧疆 1.5 绉掕秴鏃?t.tv_sec  = 1;       // 1 绉?t.tv_usec = 500000;  // + 0.5 绉?
// 鐒跺悗搴旂敤鍒?socket锛?setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof t);
// 鍙傛暟 1: socket 鏂囦欢鎻忚堪绗?// 鍙傛暟 2: SOL_SOCKET = 杩欐槸 socket 绾у埆鐨勯€夐」锛堜笉鏄?TCP 绾у埆锛?// 鍙傛暟 3: SO_RCVTIMEO = 璁剧疆鎺ユ敹瓒呮椂锛圧eceive Timeout锛?// 鍙傛暟 4: 鎸囧悜 timeval 缁撴瀯浣撶殑鎸囬拡
// 鍙傛暟 5: 缁撴瀯浣撳ぇ灏?```

### 浠ｇ爜閲岀殑瀹為檯璁＄畻锛堝嚭浜嗕粈涔堥棶棰橈級

绗?109 琛屽畾涔夎秴鏃跺畯锛岀 256 琛岀敤瀹冨垵濮嬪寲缁撴瀯浣擄細

```c
// 绗?109 琛岋紙褰撴椂鐨勯敊璇€硷級锛?#define PUSH_TIMEOUT_MS  2000

// 绗?256 琛岋細
static struct timeval push_timeout_half = {0, (PUSH_TIMEOUT_MS * 500)};
//                             tv_sec=0 鈫?  鈫?tv_usec = 2000 脳 500 = 1,000,000
//                                          鈫?姝ｅソ绛変簬 1 绉掞紝瓒呭嚭鍚堟硶涓婇檺 999,999锛?```

涓轰粈涔堜箻浠?500锛熶唬鐮佽璁℃槸"姣忔绛変竴鍗婄殑瓒呮椂锛屽惊鐜袱娆?锛?
```
push_timeout_half.tv_usec = PUSH_TIMEOUT_MS 脳 500
                           = 2000 脳 500 = 1,000,000  鈫?闈炴硶锛?
璁捐鎰忓浘锛氭€昏秴鏃?2000ms锛屽垎涓ゆ锛屾瘡娆?1000ms
姝ｇ‘鍐欐硶锛氬簲璇ョ敤 tv_sec=1, tv_usec=0
浣嗕唬鐮佸彧鐢ㄤ簡 tv_usec 瀛楁锛?,000,000 灏辨孩鍑哄悎娉曡寖鍥翠簡
```

### lwIP 閬囧埌闈炴硶鍊兼€庝箞澶勭悊

ESP32 鐨勭綉缁滃簱 lwIP 瀵?`setsockopt()` 閲岀殑 `struct timeval` 鍋氬悎娉曟€ф鏌ワ細

```
濡傛灉 tv_usec >= 1,000,000 鈫?鏁翠釜 timeval 褰撲綔鏃犳晥锛岃秴鏃惰涓?0锛堢珛鍒昏繑鍥烇級
```

**鏁堟灉锛歚recv()` 鐨勮秴鏃跺彉鎴愪簡 0 姣鈥斺€旇皟鐢ㄥ悗绔嬪埢杩斿洖 `EAGAIN`锛屽畬鍏ㄤ笉绛夊緟銆?*

### 瀹為檯鍙戠敓浜嗕粈涔堬紙浠ｇ爜鎵ц璺緞锛?
```c
// push_timeout_half = {tv_sec=0, tv_usec=1,000,000}
// lwIP 澶勭悊涓猴細瓒呮椂 = 0ms

// 绗?1777 琛岋紙socket 鍒濆鍖栨椂锛夛細
setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &push_timeout_half, sizeof push_timeout_half);
// 鈫?瀹為檯璁剧疆浜?0ms 瓒呮椂

// 绗?2651 琛岋紙绛?ACK 寰幆锛夛細
for (i=0; i<2; ++i) {
    j = recv(sock_up, (void *)buff_ack, sizeof buff_ack, 0);
    // 鈫?瓒呮椂=0ms锛岀珛鍒昏繑鍥?-1
    if (errno == EAGAIN) continue;  // 鈫?姣忔閮借蛋杩欓噷
}
// 涓ゆ寰幆閮界珛鍒昏秴鏃讹紝meas_up_ack_rcv 浠庝笉澧炲姞锛宎ckr 姘歌繙鏄?0%
```

杩欐槸鍏稿瀷鐨?闈欓粯 bug"鈥斺€旂▼搴忔病鏈夋姤閿欙紝鏃ュ織鍙槸澶氫簡寰堝 EAGAIN锛?琛屼负鍗村畬鍏ㄩ敊浜嗐€?
### 杩樻湁涓€涓鐩栧叧绯昏娉ㄦ剰

浠ｇ爜鏈変袱灞傝秴鏃惰缃紝绗?1197鈥?200 琛屼細鍦ㄥ惎鍔ㄦ椂鐢?JSON 閰嶇疆鏂囦欢**瑕嗙洊** `#define` 鐨勫€硷細

```c
// 绗?1197 琛岋紙parse_gateway_configuration 鍑芥暟閲岋級锛?val = json_object_get_value(conf_obj, "push_timeout_ms");
if (val != NULL) {
    push_timeout_half.tv_usec = 500 * (long int)json_value_get_number(val);
    //  鈫?鐢?JSON 閲岀殑鍊肩洿鎺ヨ鐩栨帀绗?256 琛?#define 绠楀嚭鏉ョ殑鍒濆鍊?    MSG("INFO: upstream PUSH_DATA time-out is configured to %u ms\n",
        (unsigned)(push_timeout_half.tv_usec / 500));
}
```

鎵€浠?`push_timeout_half` 鐨勫疄闄呭€硷細
1. 绋嬪簭鍚姩鏃舵寜 `#define PUSH_TIMEOUT_MS` 绠楀嚭鍒濆鍊硷紙绗?256 琛岋級
2. **鐒跺悗**璋冪敤 `parse_gateway_configuration()`锛屽鏋?JSON 閲屾湁 `"push_timeout_ms"` 瀛楁锛屽氨鐢ㄥ畠**瑕嗙洊**

褰撴椂 `global_conf.cn490.json` 閲屾槸 `"push_timeout_ms": 100`锛堝師濮嬮厤缃殑鍊硷級锛?```
tv_usec = 500 脳 100 = 50,000 寰 = 50ms/娆★紝鎬诲叡鏈€澶氱瓑 100ms
```
50ms 瀵逛簬浠讳綍鐪熷疄缃戠粶閮藉お鐭紙灞€鍩熺綉 RTT 绾?1鈥?ms锛屾墜鏈虹儹鐐圭害 300ms锛夈€?
### 淇

```c
// 绗?109 琛岋紝淇鍚庯細
#define PUSH_TIMEOUT_MS  500  // tv_usec = 500脳500 = 250,000 鉁?鍚堟硶锛?50ms/娆?#define PULL_TIMEOUT_MS  400  // tv_usec = 400脳1000 = 400,000 鉁?鍚堟硶锛?00ms
```

鍚屾淇敼 `global_conf.cn490.json`锛堝惁鍒欏惎鍔ㄦ椂浼氳 JSON 閲岀殑鏃у€艰鐩栨帀浠ｇ爜鐨勪慨姝ｏ級锛?
```json
"push_timeout_ms": 500
```

淇敼鍚庨噸鏂扮敓鎴?`global_json.h`锛堣繖涓枃浠舵槸 JSON 鐨勪簩杩涘埗宓屽叆鐗堬紝鍥轰欢浠庡畠璇诲彇閰嶇疆锛夈€?
---

## 闂 3锛氭竻绌烘棫 ACK 鐨?drain 寰幆鏈変竴涓綆姒傜巼闄烽槺

### 涓轰粈涔堥渶瑕?drain锛堟竻绌烘棫 ACK锛?
璁炬兂杩欎釜鍦烘櫙锛?
```
杞 1锛?  t=0ms    鍙戝嚭 PUSH_DATA锛坱oken = 0x1C7F锛?  t=250ms  绗?1 娆?recv() 瓒呮椂锛屾病鏀跺埌锛堢綉缁滄姈鍔級
  t=500ms  绗?2 娆?recv() 瓒呮椂锛屾病鏀跺埌
           ackr 鍒嗗瓙涓嶅鍔狅紝杩欒疆澶辫触

杞 2锛?  t=550ms  鍑嗗鍙戞柊 PUSH_DATA锛坱oken 鎹㈡垚 0x4D21锛?           鈫?姝ゅ埢 socket 缂撳啿鍖洪噷鏈変粈涔堬紵
  t=580ms  涓婁竴杞殑 PUSH_ACK锛坱oken=0x1C7F锛夊濮楁潵杩燂紝杩涘叆 sock_up 缂撳啿鍖?  t=581ms  鍙戝嚭鏂?PUSH_DATA锛坱oken = 0x4D21锛?  t=582ms  recv() 璇诲埌缂撳啿鍖洪噷鐨勬棫 ACK锛坱oken = 0x1C7F锛?           浠ｇ爜妫€鏌ワ細0x1C7F 鈮?0x4D21
           鈫?MSG("WARNING: [up] ignored out-of sync ACK packet\n")
           鈫?杩欒疆涔熷け璐ワ紒

杞 3锛?  t=600ms  NS 鐪熸鍥炲簲 0x4D21 鐨?ACK 鍒版潵
           浣?recv() 宸茬粡鍦ㄨ疆娆?2 閲屾秷鑰楁帀浜嗕袱娆℃満浼氾紙涓€娆¤鏃CK锛屼竴娆¤秴鏃讹級
           鈫?杩欎釜 ACK 娌′汉璇伙紝鍙堢暀鍦ㄧ紦鍐插尯绛変笅涓€杞?..鎭舵€у惊鐜?```

鎵€浠ユ瘡娆″彂鏂?PUSH_DATA **涔嬪墠**锛屽繀椤诲厛鎶?socket 缂撳啿鍖洪噷绉帇鐨勬棫 ACK 娓呯┖銆?
### 鏃т唬鐮佺殑鍋氭硶锛堜慨澶嶅墠锛?
```c
// 鏃т唬鐮侊紙鏈夐闄╋級锛?struct timeval drain_tv = {0, 1000}; // 1ms 瓒呮椂锛堝嚑涔庣瓑浜?绔嬪埢杩斿洖"锛?uint8_t _tmp[4];

// 鈶?鎶?sock_up 鐨勬帴鏀惰秴鏃舵敼涓?1ms
setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &drain_tv, sizeof drain_tv);

// 鈶?鍙嶅 recv() 鐩村埌缂撳啿鍖虹┖浜嗭紙杩斿洖 -1 琛ㄧず EAGAIN锛屾病涓滆タ浜嗭級
while (recv(sock_up, (void *)_tmp, sizeof _tmp, 0) > 0) {}

// 鈶?鎶婅秴鏃舵仮澶嶅洖 250ms锛屽悗闈㈢湡姝ｇ瓑 ACK 鐨?recv() 鎵嶈兘鏈夋椂闂寸瓑
setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, (void *)&push_timeout_half,
           sizeof push_timeout_half);
```

閫昏緫涓婃病闂锛屼絾绗?鈶?姝ョ殑 `setsockopt()` 娌℃湁妫€鏌ヨ繑鍥炲€笺€?**`setsockopt()` 鍦?ESP32 lwIP 閲岋紝鍦ㄥ唴瀛樺帇鍔涘ぇ鏃跺彲鑳借繑鍥為敊璇紙-1锛夈€?*

濡傛灉绗?鈶?姝ユ倓鎮勫け璐ヤ簡锛?
```
淇敼澶辫触 鈫?sock_up 鐨勮秴鏃舵案杩滃仠鍦?1ms
鈫?鍚庨潰鎵€鏈?recv() 绛?1ms 灏辨斁寮?鈫?ACK 涓嶇浠€涔堟椂鍊欏埌閮芥敹涓嶅埌
鈫?ackr 璺屽埌鎺ヨ繎 0%
鈫?閲嶅惎鎵嶈兘鎭㈠
```

杩欑 bug 涓嶆槸姣忔閮藉嚭鐜帮紝鍙湁鍦ㄥ唴瀛樺帇鍔涘ぇ鐨勭壒瀹氭椂鍒绘墠鍋跺彂锛屽緢闅剧ǔ瀹氬鐜般€?
### 淇锛氱敤 `MSG_DONTWAIT` 鏍囧織锛屽畬鍏ㄤ笉纰?`SO_RCVTIMEO`

淇鍚庣殑绗?2637鈥?639 琛岋細

```c
// 鏂颁唬鐮侊細
uint8_t _tmp[4];
while (recv(sock_up, (void *)_tmp, sizeof _tmp, MSG_DONTWAIT) > 0) {}
```

`recv()` 鐨勫畬鏁村嚱鏁扮鍚嶏細

```c
ssize_t recv(int sockfd,   // socket 鏂囦欢鎻忚堪绗?             void *buf,    // 鎺ユ敹缂撳啿鍖?             size_t len,   // 鏈€澶氳澶氬皯瀛楄妭
             int flags);   // 鏍囧織浣嶏紙杩欓噷鏄叧閿級
```

`MSG_DONTWAIT` 鏄竴涓爣蹇椾綅锛屽惈涔夋槸锛?"杩?*涓€娆?* `recv()` 璋冪敤锛屼笉绠?socket 鏈夋病鏈夎缃秴鏃讹紝
濡傛灉缂撳啿鍖洪噷娌℃湁鏁版嵁锛岀珛鍒昏繑鍥?`EAGAIN`锛屼笉瑕佺瓑銆?

瀹冨彧褰卞搷杩欎竴娆¤皟鐢紝**涓嶄慨鏀?socket 鐨?`SO_RCVTIMEO` 璁剧疆**銆?鎵€浠ュ悗闈㈢湡姝ｇ瓑 ACK 鐨?`recv()`锛堢 2655 琛岋紝娌℃湁 `MSG_DONTWAIT` 鏍囧織锛?杩樻槸鎸夌収涔嬪墠 `setsockopt` 璁剧疆鐨?250ms 瓒呮椂鏉ョ瓑銆?
鐢ㄨ繖绉嶆柟寮忥紝鏁翠釜 drain 杩囩▼瀹屽叏涓嶉渶瑕佺 `SO_RCVTIMEO`锛?涔熷氨涓嶅瓨鍦?鎭㈠澶辫触"鐨勯闄┿€?
---

## 闂 4锛歐iFi 鍦ㄦ墦鐩癸紝璺敱鍣ㄦ浛瀹冩墸鎶间簡 ACK 鍖?
### 杩欐槸 ackr 浠?70% 鎻愬埌 86鈥?00% 鐨勫叧閿慨澶?
瑙ｅ喅鍓嶉潰涓変釜闂鍚庯紝ackr 杩樻槸鍙湁 60鈥?0%銆?Mock 鏃ュ織鏄剧ず ACK 100% 鍙戝嚭浜嗭紝缃戝叧灏辨槸鏀朵笉鍒帮紝璇存槑 ACK 鍦?璺敱鍣ㄥ埌缃戝叧"杩欐娑堝け浜嗐€?杩欒窡浠ｇ爜閫昏緫鏃犲叧锛屾槸纭欢/椹卞姩灞傜殑琛屼负銆?
### WiFi Modem Sleep 鏄粈涔?
ESP32 WiFi 鏈夊嚑绉嶇渷鐢垫ā寮忥紝榛樿鏄?`WIFI_PS_MIN_MODEM`锛堟渶灏忚皟鍒惰В璋冨櫒鐫＄湢锛夈€?
鍦ㄨ繖涓ā寮忎笅锛孍SP32 WiFi 鏃犵嚎鐢典細鍛ㄦ湡鎬у叧闂紝鍙湪璺敱鍣ㄥ彂閫?Beacon 淇℃爣甯х殑鏃跺€欓啋鏉ャ€?Beacon 鐨勯棿闅旂敱璺敱鍣ㄩ厤缃紝鍏稿瀷鍊兼槸 100ms锛圖TIM=1锛夊埌 300ms锛圖TIM=3锛夈€?
**褰?ESP32 鏃犵嚎鐢典紤鐪犳椂锛岃矾鐢卞櫒鍙戠粰瀹冪殑鍗曟挱鍖呮€庝箞鍔烇紵**
璺敱鍣ㄤ笉浼氫涪寮冿紝浼?*缂撳瓨**杩欎簺鍖咃紝绛?ESP32 鍦ㄤ笅涓€涓?Beacon 鍛ㄦ湡閱掓潵鍚庡啀鍙戦€併€?
### 鏃堕棿绾匡細ACK 鏄€庝箞琚崱浣忕殑

```
鍋囪璺敱鍣?DTIM Beacon 闂撮殧 = 200ms

t=0ms    缃戝叧鍙戝嚭 PUSH_DATA
t=0ms    ESP32 WiFi 鎭板ソ杩涘叆浼戠湢锛堝垰鍒氬彂瀹屽寘灏变紤鐪犱簡锛?t=2ms    NS 鏀跺埌锛岀珛鍒诲洖 PUSH_ACK锛堝眬鍩熺綉 RTT < 5ms锛?t=2ms    PUSH_ACK 鍒拌揪璺敱鍣?
         鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?         鈹? ESP32 WiFi 鏃犵嚎鐢垫鍦ㄤ紤鐪狅紙绾?198ms锛?         鈹?         鈹? 璺敱鍣細杩欎釜鍖呯洰鏍囨槸 ESP32锛屽畠鍦ㄧ潯锛屽厛缂撳瓨鐫€   鈹?         鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?
t=200ms  ESP32 WiFi 閱掓潵锛屾帴鏀跺埌璺敱鍣ㄧ殑 Beacon
t=200ms  璺敱鍣細"浣犻啋浜嗭紝鎶婁箣鍓嶇紦瀛樼殑鍖呯粰浣?
t=200ms  PUSH_ACK 缁堜簬鍒拌揪 ESP32 recv() 缂撳啿鍖?
姝ゆ椂 thread_up 鐨?recv() 绐楀彛鐘舵€侊細
  绗?1 娆?recv()锛歵=0 鈫?t=250ms锛?50ms 瓒呮椂锛?  t=200ms 鏀跺埌锛岃窛瓒呮椂杩樺墿 50ms锛岃兘鏀跺埌 鉁?
  浣嗗鏋?DTIM 闂撮殧鏄?300ms锛?  t=300ms 鎵嶅埌锛岃秴杩?250ms 绐楀彛 鈫?recv() 宸茬粡瓒呮椂閫€鍑?鉁?```

杩欒В閲婁簡涓轰粈涔?ackr 涓嶆槸 0%锛堜笉鏄畬鍏ㄦ敹涓嶅埌锛夛紝鑰屾槸 60鈥?0%锛?澶у鏁版儏鍐?ACK 鍦ㄧ獥鍙ｅ唴鍒拌揪锛屼絾褰?WiFi 鐫＄湢鍛ㄦ湡鍒氬ソ璁?ACK 寤惰繜鍒扮獥鍙ｆ湯灏炬椂灏变涪鎺変簡銆?
### 淇锛氱 4168鈥?172 琛?
```c
// wifi_init_sta() 鍑芥暟閲岋細
ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
ESP_ERROR_CHECK(esp_wifi_start());

// 鈫?鏂板杩欎竴琛岋紝绱ф帴鍦?esp_wifi_start() 涔嬪悗锛?ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
// WIFI_PS_NONE = 瀹屽叏绂佺敤鐪佺數妯″紡
// WiFi 鏃犵嚎鐢靛缁堜繚鎸佸紑鍚紝璺敱鍣ㄥ彂鏉ョ殑鍖呯珛鍒婚€佽揪锛屼笉鍐嶇紦瀛?```

鍏充簬 `ESP_ERROR_CHECK()`锛氳繖鏄?ESP-IDF 鎻愪緵鐨勫畯锛屽睍寮€鍚庡ぇ姒傛槸锛?
```c
// 绛変环浜庯細
esp_err_t err = esp_wifi_set_ps(WIFI_PS_NONE);
if (err != ESP_OK) {
    ESP_LOGE(..., "esp_wifi_set_ps failed: %s", esp_err_to_name(err));
    abort();  // 鎵撳嵃閿欒骞堕噸鍚?}
```

鐢ㄥ畠鍖呰９鍏抽敭鍒濆鍖栨搷浣滐紝纭繚璁剧疆澶辫触鏃朵笉浼氶潤榛樺湴缁х画杩愯锛堥伩鍏嶄骇鐢熼毦浠ヨ拷鏌ョ殑鍚庣画闂锛夈€?
### 淇鍚庡疄娴嬫暟鎹?
```
淇鍓嶏紙鏈?WiFi PS锛夛細
  INFO: [up] PUSH_ACK received in 2 ms
  INFO: [up] PUSH_ACK received in 237 ms   鈫?蹇秴鏃朵簡
  锛堟煇浜涜疆娆￠潤榛樿秴鏃讹紝娌℃湁鎵撳嵃锛?  PUSH_DATA acknowledged: 70%

淇鍚庯紙WIFI_PS_NONE锛夛細
  INFO: [up] PUSH_ACK received in 2 ms
  INFO: [up] PUSH_ACK received in 3 ms
  INFO: [up] PUSH_ACK received in 47 ms    鈫?杩滅 NS锛屽欢杩熸甯?  INFO: [up] PUSH_ACK received in 94 ms
  PUSH_DATA acknowledged: 100.00%
```

---

## 鍏ㄧ▼ ackr 鍙樺寲鍥為【

```
鍒濆鐘舵€侊細22鈥?0%
  鈫?鍏虫帀 MQTT锛圗NABLE_MQTT=0锛屾秷闄?10s TLS 闃诲锛?  53鈥?0%
  鈫?淇瓒呮椂鍊硷紙PUSH_TIMEOUT_MS=500锛屾秷闄?tv_usec 婧㈠嚭锛?    鍚屾鏇存柊 JSON + 閲嶆柊鐢熸垚 global_json.h
  60鈥?0%
  鈫?绂佺敤 WiFi Modem Sleep锛圵IFI_PS_NONE锛屾秷闄?AP 缂撳啿寤惰繜锛?  86鈥?00% 鉁?
锛坉rain 鏀?MSG_DONTWAIT 鏄槻寰℃€т慨澶嶏紝姝ｅ父鎯呭喌鐪嬩笉鍑哄樊鍒紝
  娑堥櫎浜嗗唴瀛樺帇鍔涗笅鍋跺彂鐨?瓒呮椂姘镐箙鍗?1ms"鏋佺鎯呭喌锛?```

---

## 璋冭瘯鎬濊矾鎬荤粨

**1. 浜ゅ弶楠岃瘉鎵炬柇鐐癸紝涓嶈鐚?*
璁╀袱绔兘鏈夋棩蹇楋細Mock 鍙戜簡 + 缃戝叧娌℃敹鍒?鈫?闂鍦ㄤ紶杈?鎺ユ敹灞傘€?杩欎釜鏂规硶鑳借繀閫熸妸"鍙兘鑼冨洿"缂╁埌鏌愪竴渚э紝鐪佸幓澶ч噺鐚滄祴鏃堕棿銆?
**2. 闈欓粯 bug锛氭敼浜嗗弬鏁版病鏁堟灉鏃讹紝鍏堥獙璇佸弬鏁版槸鍚︾湡鐨勭敓鏁?*
`tv_usec = 1,000,000` 娌℃湁浠讳綍鎶ラ敊銆?閬囧埌"鏀逛簡鍙傛暟瀹屽叏娌℃晥鏋?鐨勬儏鍐碉紝鍦ㄥ叧閿綅缃墦 log 楠岃瘉锛?```c
MSG("DBG: push_timeout_half.tv_usec = %ld\n", push_timeout_half.tv_usec);
```

**3. 鍋跺彂 bug锛氭敼璁捐姣斿姞閲嶈瘯鏇存湁鏁?*
setsockopt 澶辫触鏄綆姒傜巼浜嬩欢锛岀敤 `MSG_DONTWAIT` 浠庢牴鏈笂娑堥櫎閭ｄ釜姝ラ锛?璁?bug 娌℃湁鏈轰細鍑虹幇锛屾瘮浜嬪悗澶勭悊鏇村彲闈犮€?
**4. 纭欢鐗规€т篃鏄?bug 鏉ユ簮**
WiFi Modem Sleep 鏄┍鍔ㄨ涓猴紝浠ｇ爜閫昏緫瀹屽叏姝ｇ‘涔熶細瀵艰嚧鏁版嵁寤惰繜銆?浠ュ悗閬囧埌"浠ｇ爜娌￠棶棰樹絾鏁版嵁杩樻槸涓㈠け/寤惰繜"锛岃鑰冭檻锛?- 纭欢灞傛湁娌℃湁鐪佺數/缂撳啿鏈哄埗锛?- 椹卞姩灞傛湁娌℃湁 DMA銆佷腑鏂欢杩燂紵
- OS 灞傛湁娌℃湁浠诲姟璋冨害褰卞搷锛?
---

## 鎵╁睍鐭ヨ瘑

---

### Q1锛歚struct timeval` 鐨?`tv_sec` 鍜?`tv_usec` 鎬庝箞缁勫悎锛?
**绛旓細瀵癸紝`{1, 500000}` 灏辨槸 1.5 绉掋€?* 瑙勫垯寰堢畝鍗曪細

$$\text{瀹為檯瓒呮椂} = \text{tv\_sec} \times 1\text{s} + \text{tv\_usec} \times 1\mu\text{s}$$

```c
// 甯歌渚嬪瓙锛?{0, 250000}   // 0 + 250,000渭s = 250ms
{0, 500000}   // 0 + 500,000渭s = 500ms
{1, 0}        // 1s + 0 = 1000ms
{1, 500000}   // 1s + 500,000渭s = 1500ms
{2, 0}        // 2s
```

**鍏抽敭绾︽潫锛歚tv_usec` 蹇呴』鍦?`[0, 999999]` 鑼冨洿鍐呫€?*
`tv_usec` 琛ㄧず涓嶈冻 1 绉掔殑"浣欐暟"閮ㄥ垎锛屽鏋滃啓鎴?`1,000,000` 灏辩浉褰撲簬澶氫簡鏁存暣 1 绉掞紝
鏍囧噯瑙勮寖瑕佹眰杩涗綅鍒?`tv_sec`锛屼笉鍏佽 `tv_usec >= 1,000,000`銆?
Linux 鍜?lwIP 瀵规鐨勫鐞嗕笉鍚岋細
- Linux glibc锛?*闈欓粯杩涗綅**锛坄1,000,000渭s` 鈫?`tv_sec+1, tv_usec=0`锛夛紝涓嶆姤閿?- **lwIP锛圗SP32锛夛細鐩存帴鎷掔粷锛屾妸鏁翠釜 timeval 褰?`{0,0}` 澶勭悊**锛堣秴鏃?0锛岀珛鍒昏繑鍥烇級

杩欏氨鏄?BUG-010 閲?鏀逛簡 `#define` 浣嗗畬鍏ㄦ病鏁堟灉"鐨勬牴鏈師鍥犫€斺€攍wIP 姣?Linux 鏇翠弗鏍笺€?
---

### Q2锛欴TIM Beacon 鏈哄埗鍏蜂綋鎬庝箞宸ヤ綔鐨勶紵

**涓変釜姒傚康瑕佸尯鍒嗘竻妤氾細Beacon銆乀IM銆丏TIM銆?*

**Beacon 甯?*鏄?WiFi AP锛堣矾鐢卞櫒锛夊畾鏈熷箍鎾殑绠＄悊甯э紝鍛婅瘔闄勮繎鎵€鏈夎澶?鎴戝湪杩欓噷锛岃繖鏄垜鐨勫弬鏁?銆?榛樿闂撮殧 `beacon_interval = 100ms`锛堝嵆姣忕 10 娆★級锛岃繖鏄?802.11 鍗忚瑙勫畾鐨勫熀纭€鑺傛媿銆?
**TIM锛圱raffic Indication Map锛?* 鏄?Beacon 甯ч噷鐨勪竴涓瓧娈碉紝AP 鐢ㄥ畠閫氬憡"鍝簺澶勪簬鐪佺數妯″紡鐨?STA 鏈夋暟鎹寘鍦ㄧ瓑寰?銆係TA 鏀跺埌 Beacon锛岀湅鑷繁鐨?AID 鍦?TIM 閲屾湁娌℃湁鏍囪锛屾湁鐨勮瘽鍐嶅彂 PS-Poll 甯ц姹?AP 鎶婄紦瀛樼殑鍖呭彂杩囨潵銆?
**DTIM锛圖elivery TIM锛?* 鏄?TIM 鐨勮秴闆嗭紝姣忛殧 `DTIM Period` 涓?Beacon 鎵嶅嚭鐜颁竴娆°€?DTIM Beacon 姣旀櫘閫?Beacon 鏇撮噸瑕侊細AP 鍦?DTIM Beacon 鏃舵墠浼氬箍鎾紦瀛樼殑**澶氭挱/骞挎挱**鍖咃紱瀵逛簬**鍗曟挱**鍖咃紝STA 鍦ㄤ换鎰?Beacon 閱掓潵鍚庨兘鍙互鐢?PS-Poll 鍙栧洖鏉ャ€?
```
鏃堕棿绾匡紙DTIM Period = 3锛孊eacon 闂撮殧 100ms锛夛細

  t=0ms    Beacon锛堟櫘閫?TIM锛?  t=100ms  Beacon锛堟櫘閫?TIM锛?  t=200ms  Beacon锛堟櫘閫?TIM锛?  t=300ms  Beacon锛圖TIM锛?鈫?AP 閲婃斁骞挎挱鍖咃紝STA 涔熷湪姝ら啋鏉?  t=400ms  Beacon锛堟櫘閫?TIM锛?  ...
```

**ESP32 鐨?`WIFI_PS_MIN_MODEM` 榛樿琛屼负锛?*
1. ESP32 鍙湪姣忎釜 DTIM Beacon 鏃跺敜閱掓棤绾跨數
2. 閱掓潵鍚庢帴鏀?Beacon锛岀湅 TIM 閲屾湁娌℃湁鑷繁鐨勫崟鎾暟鎹?3. 鏈?鈫?鍙?PS-Poll 鈫?AP 鍥炴暟鎹紱娌℃湁 鈫?鏃犵嚎鐢电户缁紤鐪?4. 浼戠湢鏈熼棿 AP 灏嗗彂缁?ESP32 鐨勫崟鎾寘**缂撳瓨鍦ㄥ彂閫侀槦鍒?*閲?
鎵€浠ュ欢杩?= DTIM Period 脳 Beacon 闂撮殧锛堟渶鍧忔儏鍐碉級銆?鎵嬫満鐑偣 DTIM=1 鈫?鏈€澶у欢杩?100ms锛涘鐢ㄨ矾鐢卞櫒 DTIM=3 鈫?鏈€澶у欢杩?300ms銆?杩欏氨鏄?BUG-010 閲?ACK 寤惰繜 0鈥?00ms 鐨勬潵婧愩€?
`WIFI_PS_NONE` 褰诲簳鍏虫帀杩欎釜鏈哄埗锛屾棤绾跨數甯搁┗锛屾敹鍒板寘绔嬪埢澶勭悊锛屾病鏈夌紦瀛樺欢杩熴€?浠ｄ环鏄姛鑰楀鍔狅紙绾?+20鈥?0mA锛夛紝瀵逛簬鏈夌數婧愪緵鐢电殑缃戝叧瀹屽叏鍙互鎺ュ彈銆?
---

### Q3锛歚MSG_DONTWAIT` 鍜?`O_NONBLOCK` 鏈変粈涔堝尯鍒紵

涓や釜閮借兘璁?`recv()` 鍦ㄦ病鏈夋暟鎹椂绔嬪埢杩斿洖 `EAGAIN`锛岃€屼笉鏄樆濉炵瓑寰呫€?鍖哄埆鍦ㄤ簬**浣滅敤鑼冨洿**锛?
| | `MSG_DONTWAIT` | `O_NONBLOCK`锛坄fcntl` 璁剧疆锛?|
|---|---|---|
| 浣滅敤鑼冨洿 | **浠呰繖涓€娆?* `recv()` 璋冪敤 | **杩欎釜 fd 涔嬪悗鎵€鏈夎皟鐢?* |
| 璁剧疆鏂瑰紡 | 浣滀负 `flags` 鍙傛暟浼犲叆 | `fcntl(fd, F_SETFL, O_NONBLOCK)` |
| 鏄惁淇敼 socket 鐘舵€?| 鍚?| **鏄?*锛屾寔涔呯敓鏁堢洿鍒颁富鍔ㄦ竻闄?|
| 娓呴櫎鏂瑰紡 | 涓嶉渶瑕?| `fcntl(fd, F_SETFL, flags & ~O_NONBLOCK)` |
| 褰卞搷 `send()` 鍚?| 鏈?`MSG_DONTWAIT` 鐗堟湰 | **褰卞搷鎵€鏈?I/O**锛坰end/recv/accept鈥︼級|

```c
// 鏂瑰紡 A锛歁SG_DONTWAIT锛堟湰椤圭洰鐢ㄨ繖涓級
while (recv(sock_up, _tmp, sizeof _tmp, MSG_DONTWAIT) > 0) {}
// 鈫?鍙湁杩欎釜 while 寰幆閲岀殑 recv 鏄潪闃诲鐨?// 鈫?鍚庨潰杩欎釜 recv 涓嶅彈褰卞搷锛岃繕鏄寜 SO_RCVTIMEO 瓒呮椂绛夊緟
j = recv(sock_up, buff_ack, sizeof buff_ack, 0);

// 鏂瑰紡 B锛歄_NONBLOCK
int flags = fcntl(sock_up, F_GETFL, 0);
fcntl(sock_up, F_SETFL, flags | O_NONBLOCK);   // 璁句负闈為樆濉?while (recv(sock_up, _tmp, sizeof _tmp, 0) > 0) {}   // drain
fcntl(sock_up, F_SETFL, flags);                 // 鈫?蹇呴』鎵嬪姩鎭㈠锛?// 鎭㈠澶辫触 鈫?鍚庨潰鎵€鏈?recv 閮藉彉鎴愰潪闃诲 鈫?SO_RCVTIMEO 瀹屽叏澶辨晥
// 杩欏拰鏃т唬鐮佸弻 setsockopt 鏈夊悓鏍风殑"鎭㈠澶辫触"椋庨櫓
```

**缁撹锛氬湪"鍙兂瀵规煇涓€娆¤皟鐢ㄩ潪闃诲"鐨勫満鏅噷锛宍MSG_DONTWAIT` 姣?`O_NONBLOCK` 鏇村畨鍏紝
涓嶉渶瑕?set/restore锛屼篃娌℃湁"蹇樿鎭㈠"鎴?鎭㈠澶辫触"鐨勫彲鑳姐€?*

`O_NONBLOCK` 鏇撮€傚悎"鏁翠釜 socket 鐢熷懡鍛ㄦ湡閮借闈為樆濉?鐨勬灦鏋勶紝
姣斿 event loop锛坋poll/select 椹卞姩鐨勫紓姝?I/O 鏈嶅姟鍣級銆?
---

### Q4锛歚ESP_ERROR_CHECK` 閲岀殑 `abort()` 浼氬仛浠€涔堬紵浼氭墦鍗?backtrace 鍚楋紵

**浼氭墦鍗板緢澶氫俊鎭紝鐒跺悗閲嶅惎銆?* 灞曞紑鐪嬪畯瀹氫箟锛圗SP-IDF `esp_err.h`锛夛細

```c
#define ESP_ERROR_CHECK(x) do {                                 \
    esp_err_t err_rc_ = (x);                                    \
    if (unlikely(err_rc_ != ESP_OK)) {                          \
        _esp_error_check_failed(err_rc_, __FILE__, __LINE__,    \
                                __ASSERT_FUNC, #x);             \
    }                                                           \
} while(0)
```

`_esp_error_check_failed()` 鍑芥暟閲岃皟鐢?`abort()`锛孍SP32 鐨?`abort()` 浼氳Е鍙?FreeRTOS panic handler锛屽疄闄呰緭鍑哄ぇ姒傛槸锛?
```
assert failed: esp_wifi_set_ps wifi_init.c:87 (ESP_OK)

Backtrace: 0x40082b40:0x3ffb1c10 0x40082b73:0x3ffb1c30 0x4008594e:0x3ffb1c50
  0x400d1a52:0x3ffb1ca0 0x400d2bf0:0x3ffb1cc0 ...

ELF file SHA256: ...

Rebooting...
```

**Backtrace 閲屾瘡涓?`0x鍦板潃:0x鏍堝抚` 鏄竴涓嚱鏁拌皟鐢ㄥ眰绾с€?* 鐢?`idf.py monitor` 鍙互鑷姩鎶婂湴鍧€缈昏瘧鎴愬嚱鏁板悕+鏂囦欢+琛屽彿锛堢鍙峰寲锛夛細

```bash
# 鎴栬€呮墜鍔ㄨВ鏋愶細
xtensa-esp32s3-elf-addr2line -pfiaC -e build/ESXP1302-Pkt-Fwd.elf 0x400d1a52
# 杈撳嚭锛歸ifi_init_sta at main/packet_forwarder/lora_pkt_fwd.c:4170
```

**涓轰粈涔堣鐢?`ESP_ERROR_CHECK` 鑰屼笉鏄?`if (err != ESP_OK) return err`锛?*
鍒濆鍖栭樁娈电殑閿欒閫氬父鏄笉鍙仮澶嶇殑锛堢‖浠朵笉瀛樺湪銆佸唴瀛樹笉瓒崇瓑锛夛紝
缁х画杩愯鍙細浜х敓鏇撮毦瀹氫綅鐨勫悗缁棶棰樸€俙abort()` + 閲嶅惎姣?甯︾潃閿欒璺?鏇村畨鍏ㄣ€?鐢熶骇鐜鏈夋椂鐢?`ESP_ERROR_CHECK_WITHOUT_ABORT`锛屽彧鎵撳嵃 log 涓嶉噸鍚紝
浣嗚皟璇曢樁娈靛缁堝缓璁敤 `ESP_ERROR_CHECK` 浠ヤ究蹇€熷彂鐜伴棶棰樸€?
---

### Q5锛歭wIP 鍜屾爣鍑?BSD socket API 鐨勫叧绯伙紵ESP32 鍜?Linux 涓婃湁鍝簺涓嶅悓锛?
**lwIP锛坙ightweight IP锛夋槸涓€涓紑婧愮殑宓屽叆寮?TCP/IP 鍗忚鏍?*锛?涓撲负娌℃湁鎿嶄綔绯荤粺鎴栬祫婧愭瀬灏戠殑宓屽叆寮忕郴缁熻璁★紙鍐呭瓨鍗犵敤 <100KB锛夈€?ESP32 鐢ㄧ殑鏄?lwIP 绉绘鐗堬紝鐢变箰閼淮鎶ゃ€?
lwIP 瀹炵幇浜?BSD socket API锛坄socket()`銆乣bind()`銆乣recv()`銆乣setsockopt()` 绛夛級锛?鍑芥暟鍚嶅拰鍙傛暟鍜?Linux 瀹屽叏涓€鏍封€斺€旇繖灏辨槸涓轰粈涔堝彲浠ユ妸涓€浠界綉缁滀唬鐮佺◢鍔犱慨鏀瑰悓鏃惰窇鍦?Linux 鍜?ESP32 涓娿€?
**浣嗘湁鍑犱釜宸茬煡鐨勮涓哄樊寮傞渶瑕佹敞鎰忥細**

| 琛屼负 | Linux | ESP32 lwIP |
|---|---|---|
| `tv_usec >= 1,000,000` | 闈欓粯杩涗綅 | **鏁翠釜 timeval 褰?0 澶勭悊** 鈫?鏈?bug 鏉ユ簮 |
| `errno` 鐨勭嚎绋嬮殧绂?| glibc TLS锛屾瘡绾跨▼鐙珛 | FreeRTOS 浠诲姟鏈湴瀛樺偍锛屾晥鏋滅浉鍚?|
| `SO_REUSEPORT` | 鏀寔 | **涓嶆敮鎸?*锛堜粎鏀寔 `SO_REUSEADDR`锛墊
| `MSG_PEEK` | 鏀寔 | 閮ㄥ垎鏀寔锛屾湁宸茬煡 bug |
| `fcntl(F_DUPFD)` | 鏀寔 | **涓嶆敮鎸?* |
| IPv6 | 瀹屾暣鏀寔 | 闇€瑕?menuconfig 寮€鍚紝榛樿鍏?|
| socket 缂撳啿鍖洪粯璁ゅぇ灏?| 鍑犵櫨 KB | 鍑?KB锛屽悶鍚愬瘑闆嗗満鏅渶鎵嬪姩璋冨ぇ |
| `getaddrinfo()` | 璋冪敤 OS DNS | lwIP 鍐呯疆 DNS 瑙ｆ瀽鍣紝鍙?lwIP DNS 缂撳瓨褰卞搷 |

**`errno` 鍦?ESP32 涓婃€庝箞宸ヤ綔锛?*
lwIP socket 鍑芥暟鍑洪敊鏃惰缃?`errno`銆侲SP-IDF 閲?`errno` 鏄畯锛屽睍寮€涓?`(*__errno())`锛?`__errno()` 杩斿洖褰撳墠 FreeRTOS 浠诲姟鐨?TLS锛圱ask Local Storage锛夐噷鐨?errno 妲戒綅鎸囬拡銆?鎵€浠ヤ笉鍚屼换鍔＄殑 `errno` 鐩镐簰鐙珛锛屽拰 Linux 澶氱嚎绋嬭涓轰竴鑷淬€?
**瀹為檯绉绘缁忛獙锛?* 浠ｇ爜浠?Linux 绉诲埌 ESP32锛?0% 鐨?socket 浠ｇ爜涓嶇敤鏀广€?瑕佹敞鎰忕殑鏄細鈶?涓婇潰琛ㄩ噷鐨勫凡鐭ュ樊寮傦紱鈶?`select()`/`poll()` 鐨?fd 涓婇檺锛坙wIP 榛樿 FD_SETSIZE=64锛夛紱鈶?socket 缂撳啿鍖哄ぇ灏忛粯璁ゆ瘮 Linux 灏忓緢澶氾紙褰卞搷鍚炲悙閲忓瘑闆嗗満鏅級銆?
---

### Q6锛欶reeRTOS 鏃堕棿鐗囪疆鎹㈠叿浣撴€庝箞宸ヤ綔锛熷悓浼樺厛绾т换鍔℃€庝箞鍒?CPU 鏃堕棿锛?
**FreeRTOS 璋冨害鍣ㄧ殑涓や釜鏍稿績鏈哄埗锛氭姠鍗犲紡璋冨害 + 鏃堕棿鐗囪疆鎹€?*

**鎶㈠崰寮忥紙Preemptive锛夛細** 楂樹紭鍏堢骇浠诲姟灏辩华鏃讹紝绔嬪埢鎵撴柇浣庝紭鍏堢骇浠诲姟銆?鍙鏈夋洿楂樹紭鍏堢骇鐨勪换鍔′粠闃诲鎬佸彉鎴愬氨缁€侊紙姣斿绛夊緟鐨勪俊鍙烽噺琚噴鏀撅級锛?璋冨害鍣ㄧ珛鍒诲垏鎹紝浣庝紭鍏堢骇浠诲姟琚殏鍋溿€?
**鏃堕棿鐗囷紙Time Slicing锛夛細** 瀵逛簬**鍚屼紭鍏堢骇**浠诲姟锛屾瘡闅斾竴涓?tick 鑷姩杞崲銆?
```
ESP32 鐨?tick 閰嶇疆锛坰dkconfig 閲岋級锛?CONFIG_FREERTOS_HZ = 100   鈫?tick 闂撮殧 = 1/100s = 10ms

鏃堕棿绾匡紙mqtt_task 鍜?pkt_fwd_task锛屽悓浼樺厛绾?6锛岀粦 core 0锛夛細
  0ms     mqtt_task 杩愯
  10ms    tick 鈫?鍒囨崲鍒?pkt_fwd_task
  20ms    tick 鈫?鍒囨崲鍒?mqtt_task
  ...
```

**浣?鏃堕棿鐗?10ms 杞崲"鍙槸鏈€鍧忔儏鍐点€?* 浠诲姟涓€鏃︿富鍔ㄨ繘鍏ラ樆濉炴€侊紝绔嬪埢璁╁嚭 CPU锛?
```c
// 杩欎簺璋冪敤浼氳浠诲姟绔嬪埢鎸傝捣锛屼笉绛?tick锛?recv(sock, buf, len, 0);            // 绛夋暟鎹埌锛屾垨瓒呮椂
vTaskDelay(pdMS_TO_TICKS(100));     // 涓诲姩鐫?100ms
xSemaphoreTake(sem, portMAX_DELAY); // 绛変俊鍙烽噺
xQueueReceive(q, &item, 100);       // 绛夐槦鍒?```

**BUG-010 閲?MQTT 闃诲鐨勬湰璐細**

琛ㄩ潰涓?`mqtt_task` 鍦?TCP connect 閲岄樆濉炵瓑寰咃紝搴旇璁╁嚭 CPU 鎵嶅銆?浣?MQTT 瀹㈡埛绔簱鍦ㄨ繛鎺ュけ璐ュ悗鏈?*蹇欑瓑閲嶈瘯閫昏緫**鈥斺€斾笉鏄函绮归樆濉烇紝鑰屾槸璋冪敤 `vTaskDelay` 鍚庣珛鍒婚噸璇曪紙retry loop锛夛紝
鏈熼棿姣忔閲嶈瘯閮芥墽琛?TLS 鎻℃墜锛宍mbedTLS` 鐨勫姞瑙ｅ瘑璁＄畻姣旇緝鑰?CPU銆?鍔犱笂 10 绉掑懆鏈熷唴澶氭閲嶈瘯锛屽悎璁′笅鏉?`pkt_fwd_task` 鑳芥嬁鍒扮殑 CPU 鏃堕棿寰堝皯銆?
鍗充究 `pkt_fwd_task` 鍦?`recv()` 鐨?250ms 绐楀彛鍐呴樆濉炵瓑寰咃紝`mqtt_task` 鐨?TLS 閲嶈瘯鍗犳弧浜嗚繖娈垫椂闂达紱
绛?`recv()` 瓒呮椂鍥炶皟瑙﹀彂銆乣pkt_fwd_task` 鍙樻垚灏辩华鎬侊紝杩樿绛変笅涓€涓?10ms tick 鎵嶈兘鐪熸杩愯鈥斺€擜CK 绐楀彛宸茬粡閿欒繃銆?
**涓€鍙ヨ瘽鎬荤粨璋冨害琛屼负锛?*
浼樺厛绾у喅瀹?璋佽兘璺?锛岄樆濉炶皟鐢ㄥ喅瀹?璁╀笉璁╄窇"锛屾椂闂寸墖鍐冲畾"璁╁涔?銆?鍚屼紭鍏堢骇浠诲姟涔嬮棿锛岃皝鑳藉鎷?CPU锛岀湅璋侀樆濉炵殑鏃堕棿灏戙€?
