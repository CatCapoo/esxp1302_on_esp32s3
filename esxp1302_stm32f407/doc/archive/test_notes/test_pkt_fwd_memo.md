# test_pkt_fwd 娴嬭瘯澶囧繕褰?
**鏃ユ湡锛?* 2026-02-20  
**娴嬭瘯鐩殑锛?* 楠岃瘉瀹屾暣鐨?Packet Forwarder 娴佹按绾匡紙SX1302 LoRa 鏀跺寘 鈫?ESP32-S3 缁勫抚 鈫?WiFi UDP 涓婃姤 NS锛?
---

## 涓€銆佹祴璇曢厤缃?
### 纭欢

| 瑙掕壊 | 璁惧 |
|------|------|
| 缃戝叧 | ESP32-S3 + SX1302锛圗SXP1302 鏉匡級 |
| 鍙戝寘绔?| 鍘熷 LoRa 鍙戝寘浠紙闈?LoRaWAN 鑺傜偣锛?|

### 棰戠巼璁″垝锛圕N490锛?
| 鍙傛暟 | 鍊?|
|------|----|
| Radio 0 涓績棰戠巼 | 480.400 MHz |
| Radio 1 涓績棰戠巼 | 481.200 MHz |
| 淇￠亾甯﹀ | 125 kHz |
| 淇￠亾 0鈥? | Radio 0锛?80.1 / 480.3 / 480.5 / 480.7 MHz |
| 淇￠亾 4鈥? | Radio 1锛?80.9 / 481.1 / 481.3 / 481.5 MHz |
| LoRa std 淇￠亾 | Radio 1锛?81.0 MHz锛?50 kHz BW锛孲F7 Explicit |
| FSK 淇￠亾 | Radio 1锛?81.5 MHz锛?25 kHz BW锛?0 kbps |

### 鍙戝寘绔弬鏁帮紙涓庢湰娆℃祴璇曞搴旓級

| 鍙傛暟 | 鍊?| 璇存槑 |
|------|----|------|
| 棰戠巼 | 480.500 MHz | 淇￠亾 2锛孯adio 0 +100 kHz |
| BW | 125 kHz | |
| SF | 12 | |
| CR | 4/5 | |
| SyncWord | 0x34 | LoRaWAN 鍏叡缃戠粶 |
| CRC | 寮€鍚?| pkt_fwd 榛樿鍙浆鍙?CRC 姝ｇ‘鐨勫寘 |
| Preamble | 鈮?8 | |
| Payload | `hello world` | |

> **娉ㄦ剰 CRC**锛歱kt_fwd 浠庨厤缃枃浠惰鍙?`forward_crc_valid = true`銆乣forward_crc_error = false`銆?> `forward_crc_disabled = false`锛屽洜姝ゅ彂鍖呯**蹇呴』寮€鍚?CRC**锛屽惁鍒欏寘浼氳闈欓粯涓㈠純銆?
---

## 浜屻€乸kt_fwd CLI 鐢ㄦ硶

缃戝叧鍚姩鍚庤繘鍏?ESP-IDF 鎺у埗鍙帮紝鍛戒护鍚嶄负 `pkt_fwd`銆?**浠讳綍甯﹀弬鏁扮殑璋冪敤鍧囦細灏嗛厤缃啓鍏?NVS锛岀劧鍚庣珛鍗抽噸鍚澶囥€?*

### 2.1 甯姪

```
pkt_fwd -h
```

### 2.2 閰嶇疆 WiFi 骞惰繛鎺?
```
pkt_fwd -u <SSID> -p <瀵嗙爜>
```

绀轰緥锛?```
pkt_fwd -u MyRouterSSID -p MyPassword123
```

鎵ц鍚庤澶囦繚瀛橀厤缃埌 NVS 骞堕噸鍚紝閲嶅惎鍚庤嚜鍔ㄤ互 Station 妯″紡杩炴帴 WiFi銆?
### 2.3 閰嶇疆 NS锛堢綉缁滄湇鍔″櫒锛夊湴鍧€

```
pkt_fwd --host <NS_IP鎴栧煙鍚? --port <绔彛鍙?
```

绀轰緥锛?```
pkt_fwd --host 192.168.1.10 --port 1700
```

榛樿閰嶇疆鏂囦欢锛坄global_conf.cn490.json`锛変腑 NS 鍦板潃涓?`192.168.1.202:1680`锛岃嫢鏈€氳繃姝ゅ懡浠よ鐩栦笖 NVS 涓棤鍊硷紝鍒欎娇鐢ㄩ厤缃枃浠剁殑缂虹渷鍊笺€?
### 2.4 閰嶇疆缃戝叧 ID

```
pkt_fwd --gwid <16浣嶅崄鍏繘鍒跺瓧绗︿覆>
```

绀轰緥锛?```
pkt_fwd --gwid AA555A0000001234
```

鏈厤缃椂缃戝叧ID浠庨厤缃枃浠惰鍙栵紝榛樿涓?`AA555A00000021FB`銆?
### 2.5 鍚屾椂閰嶇疆澶氫釜鍙傛暟

鍚勫弬鏁板彲鍦ㄤ竴鏉″懡浠や腑缁勫悎锛?
```
pkt_fwd -u MySSID -p MyPass --host 10.0.0.1 --port 1700 --gwid AA555A0000001234
```

### 2.6 Web 閰嶇疆鐣岄潰

杩炴帴鍚屼竴 WiFi 鍚庯紝娴忚鍣ㄨ闂?`http://<璁惧IP>/`锛屽脊鍑?HTTP Basic Auth 璁よ瘉妗嗭細

- **鐢ㄦ埛鍚?*锛歚iot`
- **瀵嗙爜**锛歚lora`

閫氳繃 Web 鐣岄潰鍙互閰嶇疆涓?CLI 鐩稿悓鐨勫弬鏁帮紝淇濆瓨鍚庤澶囪嚜鍔ㄩ噸鍚€?
> **娉ㄦ剰**锛氳嫢娴忚鍣ㄨ繑鍥?**431 Request Header Fields Too Large**锛?> 闇€鍦?`sdkconfig.defaults` 涓鍔?`CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048`
> 骞堕噸鏂扮紪璇戠儳褰曘€傝瑙?[BUG-009](../bugfix/BUG-009_http_431_httpd_max_req_hdr_len_too_small.md)銆?
---

## 涓夈€佹祴璇曟祦绋?
1. 鐑у綍鍥轰欢锛屼笂鐢?2. 鐢?CLI 閰嶇疆 WiFi锛歚pkt_fwd -u <SSID> -p <Password>`
3. 绛夊緟璁惧閲嶅惎骞惰繛鎺?WiFi锛岃瀵熶覆鍙ｈ緭鍑? 
   `INFO: [main] concentrator started, packet can now be received`
4. 鍙戝寘绔互 480.5 MHz SF12 BW125 CRC=ON 鍙戦€佸師濮?LoRa 鍖?5. 涓插彛鐩戣鍖呮帴鏀舵棩蹇?
---

## 鍥涖€侀獙璇佺粨鏋?
SX1278 鍘熷 LoRa 鍙戝寘绔彂閫?`"hello world"`锛圫F12, BW125, 480.5 MHz, CRC ON锛夛紝
缃戝叧鎴愬姛鎺ユ敹骞舵墦鍗?JSON 涓婃姤 UDP锛?
```
INFO: Received pkt from mote: 6F6C6C65 (fcnt=28535)

JSON up: {"rxpk":[{
  "jver":1,
  "tmst":2668949,
  "chan":2,
  "rfch":0,
  "freq":480.500000,
  "mid": 0,
  "stat":1,
  "modu":"LORA",
  "datr":"SF12BW125",
  "codr":"4/5",
  "rssis":-64,
  "lsnr":5.2,
  "foff":780,
  "rssi":-64,
  "size":11,
  "data":"aGVsbG8gd29ybGQ="
}]}
```

`data` 瀛楁 Base64 瑙ｇ爜 = `hello world`锛屼笌鍙戦€佺涓€鑷淬€?
### 淇″彿璐ㄩ噺

| 鎸囨爣 | 鍏稿瀷鍊?|
|------|--------|
| RSSI | -64 dBm |
| SNR | +5.0 ~ +5.5 dB |
| 棰戝亸 foff | ~778 Hz锛堝彂鍖呯涓庣綉鍏虫櫠鎸宸級 |

---

## 浜斻€佸凡鐭ラ潪闃诲鎬у憡璀?
| 鍛婅淇℃伅 | 鍘熷洜 | 褰卞搷 |
|---------|------|------|
| `ERROR: failed to read I2C device 0x38` | 鏃?STTS751 娓╁害浼犳劅鍣ㄧ‖浠?| 鏃狅紝娓╁害涓婃姤涓?0 |
| `MQTT_EVENT_ERROR / MQTT_EVENT_DISCONNECTED` | 鏃?MQTT Broker | 鏃狅紝LoRa 鍖呬粛姝ｅ父閫氳繃 UDP 涓婃姤 |
| `failed to configure temperature sensor` | 鍚屼笂 | 鏃?|

---

## 鍏€佽繘搴︾姸鎬?
- 鉁?CMakeLists.txt INCLUDE_DIRS 淇锛堣 [BUG-006](../bugfix/BUG-006_cmake_include_dirs_multiple_declaration.md)锛?- 鉁?webpage.h 鐢熸垚鏂瑰紡纭锛堣 [BUG-007](../bugfix/BUG-007_webpage_h_not_generated_by_idf_build.md)锛?- 鉁?ESP32-S3 鏃犳晥 GPIO 寮曡剼淇锛堣 [BUG-008](../bugfix/BUG-008_esp32s3_invalid_gpio_pin_defaults.md)锛?- 鉁?HTTP 431 淇锛堣 [BUG-009](../bugfix/BUG-009_http_431_httpd_max_req_hdr_len_too_small.md)锛?- 鉁?WiFi 杩炴帴姝ｅ父锛圫SID: 222锛?- 鉁?SX1302 闆嗕腑鍣ㄥ惎鍔ㄦ垚鍔燂紙`concentrator started`锛?- 鉁?LoRa 鍘熷鍖呮帴鏀堕獙璇侀€氳繃锛圕N490 480.5 MHz SF12 BW125锛?- 鈴?LoRaWAN NS 瀵规帴锛堟棤鍙敤 NS锛屽緟鍚庣画锛?- 鈴?MQTT 瀵规帴锛堟棤 Broker锛屽緟鍚庣画锛?
