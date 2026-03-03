# test_pkt_fwd NS 涓婅瀵规帴娴嬭瘯澶囧繕褰?
**鏃ユ湡锛?* 2026-02-21  
**娴嬭瘯鐩殑锛?* 楠岃瘉缃戝叧閫氳繃 UDP锛圫emtech 鍗忚锛変笌 NS 鐨勫畬鏁村鎺ユ祦绋嬶紝
鎺掓煡骞朵慨澶?`PUSH_DATA acknowledged`锛坅ckr锛変綆鑷?22鈥?0% 鐨勬牴鏈師鍥犮€?
---

## 涓€銆佹祴璇曢厤缃?
### 纭欢

| 瑙掕壊 | 璁惧 |
|------|------|
| 缃戝叧 | ESP32-S3 + SX1302锛圗SXP1302 鏉匡級 |
| 鍙戝寘绔?| 鍘熷 LoRa 鍙戝寘浠紙SX1278锛岄潪 LoRaWAN 鑺傜偣锛?|
| NS | 鏈湴 UDP Mock锛坄scripts/lora_ns_mock.py`锛? ChirpStack |

### 缃戠粶鐜

| 鍙傛暟 | 鍊?|
|------|----|
| 璋冭瘯闃舵涓婄綉鏂瑰紡 | 鎵嬫満鐑偣锛?G锛夆啋 鍚庢敼涓虹ǔ瀹?WiFi 璺敱鍣?|
| 鎵嬫満鐑偣 UDP RTT | ~270鈥?00 ms锛堥珮寤惰繜锛屾尝鍔ㄥぇ锛?|
| 绋冲畾 WiFi UDP RTT | < 5 ms锛堝眬鍩熺綉 Mock锛?|

> 鈿狅笍 **鎵嬫満鐑偣娉ㄦ剰浜嬮」**  
> - UDP RTT 鍏稿瀷鍊?300鈥?00 ms锛岃繙楂樹簬鏈夌嚎鎴栦紒涓?WiFi锛? 50 ms锛? 
> - 杩愯惀鍟?CGNAT 涓嶄繚璇?UDP 鍖呬笉涔卞簭/涓嶄涪鍖? 
> - NAT 琛?UDP 瓒呮椂閫氬父绾?30 s锛岄暱鏃堕棿鏃犲寘浼氬鑷存槧灏勫け鏁? 
> - 鐢熶骇鐜搴斾娇鐢ㄦ湁绾?绋冲畾 WiFi锛屾墜鏈虹儹鐐逛粎閫傚悎璋冭瘯

### 棰戠巼閰嶇疆锛圕N490锛屽瓨 NVS锛?
| 鍙傛暟 | 鍊?|
|------|----|
| Radio 0 涓績棰戠巼 | 470.600 MHz |
| Radio 1 涓績棰戠巼 | 471.400 MHz |
| 娴嬭瘯淇￠亾 | 2锛?70.700 MHz锛孯adio 0 +100 kHz锛?|

---

## 浜屻€侀棶棰樼幇璞★紙淇鍓嶏級

```
INFO: [up] PUSH_ACK received in 0 ms       鈫?绔嬪埢鏀跺埌锛堝疄涓轰笂杞仐鐣欑殑杩囨湡 ACK锛?WARNING: [up] ignored out-of sync ACK packet
INFO: [down] PULL_ACK received in 272 ms
PUSH_DATA acknowledged: 22.22%             鈫?鏋佷綆
PULL_DATA sent: 5 (100.00% acknowledged)   鈫?PULL_ACK 姝ｅ父
```

**鍏抽敭浜ゅ弶楠岃瘉**锛氬皢 NS 鍒囨崲涓烘湰鍦?Mock 鑴氭湰锛孧ock 鏃ュ織鏄剧ず 100% 宸插彂鍑?PUSH_ACK锛?浣嗙綉鍏充晶 ackr 浠嶅彧鏈?53鈥?0%锛岃鏄?ACK 纭疄鍙戝嚭锛屾槸缃戝叧渚ф敹鍖呭嚭浜嗛棶棰樸€?
---

## 涓夈€佹牴鏈師鍥犲垎鏋?
### 鍘熷洜 1锛歁QTT TLS 闃诲锛堟渶涓ラ噸锛宎ckr 鈫?22鈥?0%锛?
`ENABLE_MQTT` 鏈叧闂椂锛宍mqtt_task` 鍦ㄦ棤 Broker 鐜涓?TLS 鎻℃墜瓒呮椂闀胯揪 10 绉掞紝
鏈熼棿 `thread_up` 鏃犳硶鍦?`recv()` 绐楀彛鍐呰繍琛屻€?
### 鍘熷洜 2锛歚tv_usec` 婧㈠嚭锛堥潤榛?bug锛宎ckr 鈫?0%锛?
```c
// PUSH_TIMEOUT_MS = 2000 鏃讹細
tv_usec = 2000 脳 500 = 1,000,000  鈫?瓒呰繃鍚堟硶涓婇檺 999,999
// lwIP 灏嗗叾澶勭悊涓?0锛宺ecv() 绔嬪嵆杩斿洖 EAGAIN
```

### 鍘熷洜 3锛歞rain 寰幆 setsockopt 澶辫触椋庨櫓

鏃т唬鐮佺敤鍙?`setsockopt` 鍒囨崲瓒呮椂鏉?drain stale ACK锛?鑻ョ浜屾 `setsockopt` 澶辫触锛坙wIP 鍦ㄥ唴瀛樺帇鍔涗笅鍋跺彂锛夛紝
`sock_up` 瓒呮椂姘歌繙鍗″湪 1 ms锛屾鍚庢瘡杞潎绔嬪嵆瓒呮椂銆?
### 鍘熷洜 4锛歐iFi Modem Sleep AP 缂撳啿寤惰繜锛堟渶鍏抽敭锛宎ckr 鈫?53鈥?0%锛?
ESP32 榛樿 `WIFI_PS_MIN_MODEM`锛學iFi 鏃犵嚎鐢靛懆鏈熸€т紤鐪狅紙闂撮殧 100鈥?00 ms锛夛紝
AP 鍦ㄦ鏈熼棿缂撳啿鍙戝線缃戝叧鐨勫崟鎾?UDP 鍖呫€?PUSH_ACK 鍒拌揪鏃舵伆濂藉浜庣潯鐪犵獥鍙ｏ紝AP 缂撳啿鍚庡欢杩熼噴鏀撅紝瓒呭嚭 `recv()` 250 ms 绐楀彛銆?
---

## 鍥涖€佷慨澶嶅唴瀹?
| 淇敼 | 浣嶇疆 | 鏁堟灉 |
|------|------|------|
| `ENABLE_MQTT 0` | `lora_pkt_fwd.c` 瀹忓畾涔?| 娑堥櫎 10 s TLS 闃诲 |
| `PUSH_TIMEOUT_MS 500`锛?50ms/娆★紝500ms鎬伙級 | `lora_pkt_fwd.c` 瀹忓畾涔?| 淇 tv_usec 婧㈠嚭锛涗粛鏈夎冻澶熶綑閲?|
| `PULL_TIMEOUT_MS 400`锛?00ms锛?| `lora_pkt_fwd.c` 瀹忓畾涔?| 鍚屼笂 |
| drain 鏀圭敤 `MSG_DONTWAIT` | `thread_up` 鍙戦€佸墠 | 褰诲簳娑堥櫎 setsockopt 澶辫触椋庨櫓 |
| `esp_wifi_set_ps(WIFI_PS_NONE)` | `wifi_init_sta()` | 娑堥櫎 AP 缂撳啿寤惰繜锛屾渶澶у崟娆℃彁鍗?|
| `global_conf.cn490.json push_timeout_ms: 500` | JSON 閰嶇疆鏂囦欢 | 涓庝唬鐮侀粯璁ゅ€煎榻?|
| 閲嶆柊鐢熸垚 `global_json.h` | 鑷姩鐢熸垚鏂囦欢 | 宓屽叆鏂伴厤缃€煎埌鍥轰欢 |
| 鏂板 `MSG("WARNING: [up] recv error: ...")` | `thread_up` recv 鍒嗘敮 | 渚夸簬鎺掓煡闈炶秴鏃?recv 閿欒 |

---

## 浜斻€佷慨澶嶅悗鐜拌薄

### 绋冲畾 WiFi + 鏈湴 Mock锛堝眬鍩熺綉锛?
```
INFO: [up] PUSH_ACK received in 3 ms
INFO: [down] PULL_ACK received in 2 ms
PUSH_DATA acknowledged: 100.00%
PULL_DATA sent: 3 (100.00% acknowledged)
```

### 绋冲畾 WiFi + 杩滅 NS锛堝疄娴嬶級

```
INFO: [up] PUSH_ACK received in 47鈥?4 ms
PUSH_DATA acknowledged: 86鈥?00%
```

鍓╀綑 0鈥?4% 鎹熷け涓烘棤绾?UDP 鐗╃悊涓㈠寘锛屾湇鍔″櫒渚ф湭鍙戝嚭 ACK锛屽睘姝ｅ父鑼冨洿銆?
---

## 鍏€乤ckr 鎻愬崌杩囩▼

| 闃舵 | ackr | 涓诲洜 |
|------|------|------|
| 淇鍓?| 22鈥?0% | MQTT TLS 闃诲 |
| 绂佺敤 MQTT 鍚?| 53鈥?0% | tv_usec 婧㈠嚭 + WiFi PS |
| 淇瓒呮椂鍊煎悗 | 60鈥?0% | WiFi PS |
| 绂佺敤 WiFi PS 鍚?| **86鈥?00%** | 鈥?|

---

## 涓冦€佹湰鍦拌皟璇曞伐鍏凤細UDP NS Mock 鑴氭湰

褰撴棤娉曡繛鎺ヨ繙绔?NS 鏃讹紝鍙敤鏈湴 Mock 鑴氭湰蹇€熼獙璇佷笂琛岄摼璺€?
### 7.1 鑴氭湰浣嶇疆

```
scripts/lora_ns_mock.py
```

### 7.2 鍔熻兘

- 鐩戝惉 UDP锛屽洖搴?`PUSH_DATA` 鈫?`PUSH_ACK`锛坱oken 鍖归厤锛?- 鍥炲簲 `PULL_DATA` 鈫?`PULL_ACK`锛坱oken 鍖归厤锛?- 鎵撳嵃鏀跺埌鐨?uplink JSON payload锛堝惈 rxpk锛?- 鎵撳嵃姣忔潯鏀跺彂璁板綍锛屼究浜庝笌缃戝叧鏃ュ織鍋氫氦鍙夊姣?
### 7.3 浣跨敤鏂规硶

**姝ラ 1锛?* 鍦ㄤ笌缃戝叧鍚屼竴灞€鍩熺綉鐨勪富鏈轰笂杩愯锛?
```bash
python3 scripts/lora_ns_mock.py 1700
```

**姝ラ 2锛?* 鏌ユ湰鏈?IP锛堜緥濡?`192.168.1.100`锛夛紝鍦ㄧ綉鍏?CLI 閰嶇疆锛?
```
pkt_fwd --host 192.168.1.100 --port 1700
```

璁惧鑷姩淇濆瓨 NVS 骞堕噸鍚€?
**姝ラ 3锛?* 瀵规瘮 Mock 鏃ュ織涓庣綉鍏虫棩蹇楋細

- Mock 鏄剧ず PUSH_ACK 宸插彂鍑猴紝缃戝叧渚т篃鏀跺埌 鈫?姝ｅ父
- Mock 鏄剧ず PUSH_ACK 宸插彂鍑猴紝缃戝叧渚ц秴鏃舵湭鏀跺埌 鈫?WiFi/缃戠粶灞傞棶棰?
### 7.4 棰勬湡杈撳嚭绀轰緥

```
[NS Mock] Listening on UDP 0.0.0.0:1700
[NS Mock] PULL_DATA  from ('192.168.1.50', 52314) token=0xA3B2 -> PULL_ACK sent
[NS Mock] PUSH_DATA  from ('192.168.1.50', 52313) token=0x1C7F -> PUSH_ACK sent
[NS Mock]   rxpk[0]: freq=470.7 sf=SF12BW125 rssi=-70 data=aGVsbG8gd29ybGQ=
```

### 7.5 娉ㄦ剰浜嬮」

- 鑴氭湰闇€涓庣綉鍏冲湪**鍚屼竴灞€鍩熺綉**锛堟垨鏈夎矾鐢卞彲杈撅級
- 鏈満闃茬伀澧欓渶鏀捐 UDP 1700 绔彛
- Mock 涓嶅疄鐜颁笅琛岋紝浠呯敤浜庨獙璇佷笂琛岄摼璺?
---

## 鍏€佷笌 ChirpStack 瀵规帴娉ㄦ剰浜嬮」

1. **娉ㄥ唽缃戝叧 EUI**锛欳hirpStack 鈫?Gateways 鈫?Add锛孍UI 濉?`AA555A00000021FB`
2. **纭 gateway bridge 绔彛**锛歂S 渚ч渶鐩戝惉 UDP 1700锛圫emtech UDP 鍗忚锛?3. **妫€鏌?NS 鏄惁鍦ㄧ嚎**锛?   ```bash
   ss -ulnp | grep 1700
   ```
4. **缁熻姝ｅ父鏍囧織**锛?   ```
   INFO: [up] PUSH_ACK received in XXX ms
   INFO: [down] PULL_ACK received in XXX ms
   PUSH_DATA acknowledged: 100.00%
   ```
5. **缃戝叧绂荤嚎鎺掓煡**锛歅ULL_ACK 鑳芥敹鍒拌鏄?UDP 閫氫簡锛岃嫢 ChirpStack 浠嶆樉绀虹绾匡紝
   妫€鏌?gateway bridge 鈫?ChirpStack gRPC/MQTT 鍐呴儴杞彂鏄惁姝ｅ父

---

## 涔濄€佽繘搴︾姸鎬?
- 鉁?LoRa 鍘熷鍖呮帴鏀堕獙璇侀€氳繃锛圕N490 470.7 MHz SF12 BW125锛?- 鉁?MQTT 绂佺敤锛坄ENABLE_MQTT=0`锛屾棤 Broker 鏃朵笉褰卞搷涓绘祦绋嬶級
- 鉁?`tv_usec` 婧㈠嚭淇锛坄PUSH_TIMEOUT_MS=500`锛宍PULL_TIMEOUT_MS=400`锛?- 鉁?drain 寰幆鏀圭敤 `MSG_DONTWAIT`锛堟秷闄?setsockopt 澶辫触椋庨櫓锛?- 鉁?WiFi Modem Sleep 绂佺敤锛坄WIFI_PS_NONE`锛?- 鉁?`global_conf.cn490.json` 鍚屾鏇存柊锛宍global_json.h` 閲嶆柊鐢熸垚
- 鉁?鏈湴 Mock 浜ゅ弶楠岃瘉锛氱ǔ瀹?WiFi 涓?ackr 杈惧埌 86鈥?00%
- 鈴?ChirpStack 瀵规帴楠岃瘉锛堢綉鍏充笂绾跨‘璁わ級
- 鈴?LoRaWAN 鑺傜偣鍏ョ綉娴嬭瘯锛堜笅琛?Join Accept锛?
---

## 涓€銆佹祴璇曢厤缃?
### 纭欢

| 瑙掕壊 | 璁惧 |
|------|------|
| 缃戝叧 | ESP32-S3 + SX1302锛圗SXP1302 鏉匡級 |
| 鍙戝寘绔?| 鍘熷 LoRa 鍙戝寘浠紙SX1278锛岄潪 LoRaWAN 鑺傜偣锛?|
| NS | ChirpStack锛堣繙绔紝`10.67.9.238:1700`锛?|

### 缃戠粶鐜

| 鍙傛暟 | 鍊?|
|------|----|
| 涓婄綉鏂瑰紡 | **鎵嬫満鐑偣锛?G锛?* |
| 缃戝叧 IP | 10.67.9.11锛圖HCP锛?|
| NS IP | 10.67.9.238:1700 |
| 瀹炴祴 UDP RTT | ~270鈥?00 ms锛堥珮寤惰繜锛屾尝鍔ㄥぇ锛?|

> 鈿狅笍 **鎵嬫満鐑偣娉ㄦ剰浜嬮」**  
> - UDP RTT 鍏稿瀷鍊?300鈥?00 ms锛岃繙楂樹簬鏈夌嚎鎴栦紒涓?WiFi锛? 50 ms锛? 
> - 杩愯惀鍟?CGNAT 涓嶄繚璇?UDP 鍖呬笉涔卞簭/涓嶄涪鍖? 
> - NAT 琛?UDP 瓒呮椂閫氬父绾?30 s锛岄暱鏃堕棿鏃犲寘浼氬鑷存槧灏勫け鏁? 
> - 鐢熶骇鐜搴斾娇鐢ㄦ湁绾?绋冲畾 WiFi锛屾墜鏈虹儹鐐逛粎閫傚悎璋冭瘯

### 棰戠巼閰嶇疆锛圕N490锛屽瓨 NVS锛?
| 鍙傛暟 | 鍊?|
|------|----|
| Radio 0 涓績棰戠巼 | 480.400 MHz |
| Radio 1 涓績棰戠巼 | 481.200 MHz |
| 娴嬭瘯淇￠亾 | 2锛?80.500 MHz锛孯adio 0 +100 kHz锛?|

---

## 浜屻€佹祴璇曠幇璞?
### 2.1 闂鐜拌薄锛堜慨澶嶅墠锛?
```
INFO: [up] PUSH_ACK received in 0 ms          鈫?绔嬪埢鏀跺埌锛屽疄涓轰笂杞仐鐣欑殑杩囨湡 ACK
WARNING: [up] ignored out-of sync ACK packet  鈫?鍙嶅鍑虹幇锛岀害 3 娆?/ 鍒嗛挓
INFO: [down] PULL_ACK received in 272 ms
INFO: [down] PULL_ACK received in 382 ms
PUSH_DATA acknowledged: 22.22%               鈫?缁熻绐楀彛鍐呮瀬浣?```

鍏稿瀷瑙勫緥锛?- 绗?1 杞細`PUSH_ACK received in 0 ms`锛堢灛闂存敹鍒版棫鍖咃紝瀹炰负涓婁竴杞繃鏈?ACK锛?- 绗?2鈥? 杞細`WARNING: out-of sync`锛堢湡姝ｇ殑 ACK 鍦ㄨ秴鏃跺悗鎵嶅埌锛宼oken 宸叉崲锛?- 鍋跺彂 1 杞甯哥‘璁?
### 2.2 淇鍚庣幇璞★紙棰勬湡锛?
```
INFO: [up] PUSH_ACK received in 320 ms        鈫?鍦ㄨ秴鏃剁獥鍙ｅ唴姝ｅ父鏀跺埌
INFO: [down] PULL_ACK received in 280 ms
PUSH_DATA acknowledged: 100.00%              鈫?缁熻绋冲畾
```

---

## 涓夈€佹牴鏈師鍥犲垎鏋?
### 鍘熷洜 1锛歴ocket 璺敱 bug锛堜唬鐮佺己闄凤紝涓庣綉缁滄棤鍏筹級

`sock_up` 鍜?`sock_down` 鍧囨湭璋冪敤 `connect()`銆?`recvfrom(..., &dest_addr, &socklen)` 灏?*鍥炲寘鍙戦€佹柟鍦板潃鍐欏叆鍏ㄥ眬 `dest_addr`**锛?鍚庣画 `sendto(..., &dest_addr, ...)` 鐨勭洰鏍囧湴鍧€鍥犳琚牬鍧忋€?
姝ゅ鏃?`connect()` 鏃?OS 鏃犳硶鎸変簲鍏冪粍璺敱锛孭ULL_ACK 鍙兘琚?`sock_up` 鎶㈠厛璇昏蛋锛?瀵艰嚧 token 涓嶅尮閰嶃€?
**淇锛?* 鍚敤 `connect()`锛屽皢 `sendto/recvfrom` 鍏ㄩ儴鏇挎崲涓?`send/recv`銆?
### 鍘熷洜 2锛欰CK 瓒呮椂绐楀彛杩滃皬浜?NS 瀹為檯寤惰繜

| 鍙傛暟 | 鍘熷€?| 淇鍚?|
|------|------|--------|
| `PUSH_TIMEOUT_MS` | 100 ms | 2000 ms |
| `PULL_TIMEOUT_MS` | 200 ms | 1000 ms |
| `push_timeout_half`锛堝疄闄?per-recv锛?| 50 ms | 1000 ms |
| 鎬荤瓑寰咃紙2 娆″惊鐜級 | **100 ms** | **2000 ms** |
| NS 瀹為檯 RTT | ~300鈥?00 ms | 鈥?|

鍘熶唬鐮佹€荤瓑寰?100 ms 杩滃皬浜?NS RTT 300鈥?00 ms锛?ACK 姣忚疆閮借秴鏃惰涓㈠叆缂撳啿鍖猴紝涓嬩竴杞?`recv()` 璇诲嚭鏃?token 宸叉槸鏂板€?鈫?out-of-sync銆?
### 鍘熷洜 3锛歴tale ACK 姹℃煋锛堜笂杞秴鏃堕仐鐣欙級

鍗充娇鎵╁ぇ浜嗚秴鏃剁獥鍙ｏ紝鑻ユ煇杞洜鍋跺彂涓㈠寘瓒呮椂锛?涓嬩竴杞彂鍑?PUSH_DATA 涔嬪墠缂撳啿鍖轰腑浠嶆湁鏃?ACK 绛夊緟琚銆?鏂拌疆 `recv()` 浼樺厛璇诲埌鏃?ACK 鈫?token 涓嶅尮閰嶃€?
**淇锛?* 姣忔鍙戦€佹柊 PUSH_DATA 涔嬪墠鐢?1 ms 瓒呮椂闈為樆濉?drain 娓呯┖ `sock_up` 缂撳啿鍖恒€?
---

## 鍥涖€佷慨澶嶅唴瀹癸紙`lora_pkt_fwd.c`锛?
| 浣嶇疆 | 淇敼 | 鍘熷洜 |
|------|------|------|
| socket 鍒濆鍖?| 鍚敤 `connect(sock_up/sock_down, &dest_addr, ...)` | 缁戝畾瀵圭锛孫S 绮剧‘璺敱鍥炲寘 |
| `send_tx_ack()` | `sendto 鈫?send` | `connect()` 鍚庢棤闇€閲嶅鎸囧畾瀵圭鍦板潃 |
| `thread_up` 鍙戦€?| `sendto 鈫?send` | 鍚屼笂 |
| `thread_up` 鎺ユ敹 | `recvfrom(&dest_addr) 鈫?recv` | 閬垮厤 dest_addr 琚洖鍖呭湴鍧€瑕嗙洊 |
| `thread_down` 鍙戦€?| `sendto 鈫?send` | 鍚屼笂 |
| `thread_down` 鎺ユ敹 | `recvfrom(&dest_addr) 鈫?recv` | 鍚屼笂 |
| `PUSH_TIMEOUT_MS` | `100 鈫?2000` | 瑕嗙洊鎵嬫満鐑偣寤惰繜 |
| `PULL_TIMEOUT_MS` | `200 鈫?1000` | 鍚屼笂 |
| `thread_up` 鍙戦€佸墠 | 鏂板 1 ms drain 娓呯┖ stale ACK | 闃叉涓婅疆瓒呮椂閬楃暀 ACK 姹℃煋 token 鏍￠獙 |

---

## 浜斻€佹湰鍦拌皟璇曞伐鍏凤細UDP NS Mock 鑴氭湰

褰撴棤娉曡繛鎺ヨ繙绔?NS锛圕hirpStack 涓嶅彲鐢ㄣ€佺綉缁滀笉绋冲畾锛夋椂锛?鍙敤鏈湴 Mock 鑴氭湰妯℃嫙 NS 鍝嶅簲锛屽揩閫熼獙璇佺綉鍏充笂琛岄摼璺€?
### 5.1 鑴氭湰浣嶇疆

```
scripts/lora_ns_mock.py
```

### 5.2 鍔熻兘

- 鐩戝惉 UDP锛屽洖搴?`PUSH_DATA` 鈫?`PUSH_ACK`锛坱oken 鍖归厤锛?- 鍥炲簲 `PULL_DATA` 鈫?`PULL_ACK`锛坱oken 鍖归厤锛?- 鎵撳嵃鏀跺埌鐨?uplink JSON payload锛堝惈 rxpk锛?- 鎵撳嵃缁熻淇℃伅锛堝寘鏁般€丄CK 鐜囩瓑锛?
### 5.3 浣跨敤鏂规硶

**姝ラ 1锛?* 鍦ㄦ湰鏈猴紙涓庣綉鍏冲悓涓€灞€鍩熺綉锛夎繍琛?Mock 鏈嶅姟鍣細

```bash
python3 scripts/lora_ns_mock.py 1700
```

**姝ラ 2锛?* 鏌ュ埌鏈満 IP锛屼緥濡?`10.67.9.100`銆?
**姝ラ 3锛?* 鍦ㄧ綉鍏?CLI 灏?NS 鍦板潃鏀逛负鏈満 IP锛?
```
pkt_fwd --host 10.67.9.100 --port 1700
```

璁惧鑷姩淇濆瓨鍒?NVS 骞堕噸鍚€?
**姝ラ 4锛?* 瑙傚療 Mock 鏈嶅姟鍣ㄨ緭鍑猴紝纭 PUSH_DATA/PULL_DATA 鍧囧凡鍒拌揪骞?ACK銆?
### 5.4 棰勬湡杈撳嚭绀轰緥

```
[NS Mock] Listening on UDP 0.0.0.0:1700
[NS Mock] PULL_DATA  from ('10.67.9.11', 52314) token=0xA3B2 -> PULL_ACK sent
[NS Mock] PUSH_DATA  from ('10.67.9.11', 52313) token=0x1C7F -> PUSH_ACK sent
[NS Mock]   rxpk[0]: freq=480.5 sf=SF12BW125 rssi=-70 data=aGVsbG8gd29ybGQ=
[NS Mock] PUSH_DATA  from ('10.67.9.11', 52313) token=0x4D21 -> PUSH_ACK sent
```

### 5.5 娉ㄦ剰浜嬮」

- 鑴氭湰闇€涓庣綉鍏冲湪**鍚屼竴灞€鍩熺綉**锛堟垨鏈夎矾鐢卞彲杈撅級
- 鏈満闃茬伀澧欓渶鏀捐 UDP 1700 绔彛
- Mock 涓嶅疄鐜颁笅琛岋紝浠呯敤浜庨獙璇佷笂琛岄摼璺?- MQTT 榛樿宸查€氳繃 `ENABLE_MQTT=0` 瀹忕鐢紝鏃犻渶 Broker 鍗冲彲娴嬭瘯

---

## 鍏€佷笌 ChirpStack 瀵规帴娉ㄦ剰浜嬮」

1. **娉ㄥ唽缃戝叧 EUI**锛欳hirpStack 鈫?Gateways 鈫?Add锛孍UI 濉?`AA555A00000021FB`
2. **纭 gateway bridge 绔彛**锛歂S 渚ч渶鐩戝惉 UDP 1700锛圫emtech UDP 鍗忚锛?3. **妫€鏌?NS 鏄惁鍦ㄧ嚎**锛?   ```bash
   # 鍦?NS 鏈嶅姟鍣ㄤ笂纭 gateway bridge 鍦ㄧ洃鍚?   ss -ulnp | grep 1700
   ```
4. **缁熻姝ｅ父鏍囧織**锛氫覆鍙ｅ嚭鐜颁互涓嬪唴瀹硅鏄庝笂琛岄摼璺甯革細
   ```
   INFO: [up] PUSH_ACK received in XXX ms
   INFO: [down] PULL_ACK received in XXX ms
   PUSH_DATA acknowledged: 100.00%
   PULL_DATA sent: N (100.00% acknowledged)
   ```
5. **缃戝叧绂荤嚎鍘熷洜鎺掓煡**锛歅ULL_ACK 鑳芥敹鍒拌鏄?UDP 閫氫簡锛岃嫢 ChirpStack 浠嶆樉绀虹绾匡紝
   妫€鏌?gateway bridge 鈫?ChirpStack gRPC/MQTT 鍐呴儴杞彂鏄惁姝ｅ父

---

## 涓冦€佽繘搴︾姸鎬?
- 鉁?LoRa 鍘熷鍖呮帴鏀堕獙璇侀€氳繃锛圕N490 480.5 MHz SF12 BW125锛?- 鉁?socket 璺敱 bug 淇锛坄connect + send/recv`锛?- 鉁?ACK 瓒呮椂鎵╁ぇ锛?000/1000 ms锛? stale ACK drain
- 鉁?MQTT 绂佺敤锛坄ENABLE_MQTT=0`锛屾棤 Broker 鏃朵笉褰卞搷涓绘祦绋嬶級
- 鉁?鏈湴 UDP Mock 鑴氭湰鍙敤锛坄scripts/lora_ns_mock.py`锛?- 鈴?鍦ㄧǔ瀹氬眬鍩熺綉鐜涓嬮獙璇?100% ACK 鐜?- 鈴?ChirpStack 瀵规帴楠岃瘉锛堢綉鍏充笂绾跨‘璁わ級
- 鈴?LoRaWAN 鑺傜偣鍏ョ綉娴嬭瘯锛堜笅琛?Join Accept锛?
