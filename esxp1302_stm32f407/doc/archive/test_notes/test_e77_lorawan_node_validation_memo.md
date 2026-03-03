# E77-400M22S LoRaWAN 鑺傜偣瀹屾暣鍔熻兘楠岃瘉澶囧繕褰?
**鏃ユ湡锛?* 2026-02-27  
**娴嬭瘯鐩殑锛?* 浣跨敤鎴愰兘浜夸桨鐗?E77-400M22S 鑺傜偣楠岃瘉 ESXP1302 缃戝叧瀹屾暣 LoRaWAN
鍔熻兘閾捐矾锛屽寘鎷?OTAA 鍏ョ綉銆佷笂琛屾暟鎹€佷笅琛?ACK銆丄DR 鑷€傚簲閫熺巼銆?
---

## 涓€銆佹祴璇曢厤缃?
### 纭欢

| 瑙掕壊 | 璁惧 |
|------|------|
| 缃戝叧 | ESXP1302锛圗SP32-S3 + SX1302 + SX1250锛?|
| LoRaWAN 鑺傜偣 | 鎴愰兘浜夸桨鐗?E77-400M22S锛圕N470锛孲TM32WLE5锛?|
| 鑺傜偣杩炴帴鏂瑰紡 | UART 9600 8N1锛宍/dev/ttyUSB0` |

### 杞欢/鏈嶅姟

| 缁勪欢 | 鐗堟湰/閰嶇疆 |
|------|---------|
| 缃戝叧鍥轰欢 | 鏈伐绋?ESXP1302 |
| ChirpStack | v4锛孌ocker 閮ㄧ讲锛宍~/chirpstack-docker/` |
| ChirpStack region | `cn470_10` |
| Gateway Bridge | UDP 绔彛 `1680`锛堟敞鎰忥細闈為粯璁?1700锛?|
| 鎺у埗鑴氭湰 | `scripts/e77_node_ctrl.py` |

### 缃戝叧 RF 閰嶇疆锛坈n490.json锛屽凡楠岃瘉锛?
| 鍙傛暟 | 鍊?|
|------|----|
| Radio 0 涓績棰戠巼 | 486.6 MHz |
| Radio 1 涓績棰戠巼 | 487.4 MHz |
| 涓婅淇￠亾 | CH80~CH87锛圕N470 鏍囧噯缂栧彿锛?|
| 涓婅棰戠巼鑼冨洿 | 486.3 ~ 487.7 MHz |
| 涓嬭 RX1 鑼冨洿 | 506.7 ~ 508.1 MHz |
| 涓嬭 RX2 鍥哄畾棰戠巼 | 505.3 MHz |
| Gateway EUI | `AA555A00000021FB` |

### E77 鑺傜偣鍏抽敭鍙傛暟锛堟祴璇曠敤锛屽凡鏇挎崲锛?
| 鍙傛暟 | 鍊?|
|------|----|
| DevEUI | 閫氳繃 `AT+CDEVEUI=?` 鏌ヨ鑾峰彇 |
| AppKey | 鍦?ChirpStack Device 璇︽儏椤电敓鎴?|
| chanmask | `0000:0000:0000:0000:0000:00FF`锛圕H80~CH87锛?|
| Region | `2`锛圕N470锛?|
| 涓插彛娉㈢壒鐜?| 9600 bps |

---

## 浜屻€佹帶鍒惰剼鏈敤娉?
鑴氭湰璺緞锛歚scripts/e77_node_ctrl.py`锛屼緷璧栵細`pip3 install pyserial`

### 2.1 瀛愬懡浠や竴瑙?
| 瀛愬懡浠?| 璇存槑 |
|--------|------|
| `otaa` | OTAA 鍏ョ綉骞跺懆鏈熷彂鍖?|
| `abp` | ABP 鏈湴鍏ョ綉骞跺懆鏈熷彂鍖?|
| `query` | 鏌ヨ妯″潡褰撳墠鎵€鏈夊弬鏁帮紙涓嶅叆缃戯級 |
| `send` | 浠呭彂涓€鍖咃紙宸插叆缃戝墠鎻愶級 |
| `restore` | 鎭㈠鍑哄巶閰嶇疆 |

### 2.2 蹇€熸祴璇曞懡浠わ紙瀹為檯鍙敤锛?
浠ヤ笅鍛戒护宸叉牴鎹疄闄呰澶囧拰 ChirpStack 閰嶇疆濉叆鍙傛暟锛屽彲鐩存帴澶嶅埗鎵ц锛?
| 鍙傛暟鏉ユ簮 | 鍊?|
|---------|-----|
| **DevEUI** | 浠庢ā鍧楁煡璇細`AT+CDEVEUI=?` |
| **AppKey** | 鑷畾涔夛紝闇€鍦?ChirpStack Device 璇︽儏椤佃缃负鍚屼竴鍊?|
| **AppEUI** | 鍥哄畾 `0000000000000000`锛堣嚜寤烘祴璇曪級 |
| **chanmask** | 鏈伐绋嬬綉鍏冲浐瀹?`0000:0000:0000:0000:0000:00FF`锛圕H80~CH87锛?|

> **鈿狅笍 閲嶈锛?* 棣栨娴嬭瘯鎴栧彂鐜?AT 鍛戒护琚嫆缁濇椂锛屽姟蹇呭厛鎵ц涓嬮潰鐨?**鈶?娓呴櫎鏃ч厤缃?* 鍛戒护娓呯┖妯″潡鎸佷箙鍖栭厤缃紝鍐嶈繘琛屽叾浠栨祴璇曘€?
#### 鈶?鏌ヨ妯″潡褰撳墠鐘舵€?```bash
python3 scripts/e77_node_ctrl.py query --port /dev/ttyUSB0
```

**杈撳嚭绀轰緥锛?*
```
DevEUI: AABBCCDD11223344
AppEUI: 0000000000000000
Region: 2:CN470
```

#### 鈶?OTAA 鍏ョ綉 + 鍛ㄦ湡鍙戝寘锛?0绉掗棿闅旓級

**鍓嶇疆鏉′欢锛?* 鍦?ChirpStack 涓垱寤鸿澶囷紝璁剧疆 AppKey = `00112233445566778899AABBCCDDEEFF`

```bash
python3 scripts/e77_node_ctrl.py otaa \
    --port /dev/ttyUSB0 \
    --region 2 \
    --deveui AABBCCDD11223344 \
    --appeui 0000000000000000 \
    --appkey 00112233445566778899AABBCCDDEEFF \
    --chanmask 0000:0000:0000:0000:0000:00FF \
    --adr 1 \
    --interval 30 \
    --payload DEADBEEF01020304
```

**棰勬湡缁撴灉锛?*
```
[19:41:02] [OK  ] 鉁?OTAA 鍏ョ綉鎴愬姛锛?[19:41:03] [INFO] 鈹€鈹€鈹€ 绗?1 鍖?鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
[19:41:04] [OK  ] TX 纭: +EVT:SEND_CONFIRMED
[19:41:05] [DOWN] 涓嬭鏀跺埌: +EVT:RX_1, PORT 0, DR 5, RSSI -74, SNR 9
[19:41:35] [INFO] 鈹€鈹€鈹€ 绗?2 鍖?鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
...锛堟瘡 30 绉掑彂涓€鍖咃紝鏃犻檺寰幆锛?```

#### 鈶?OTAA 鍏ョ綉 + Confirmed uplink锛堥獙璇佸弻鍚戦€氫俊锛?0绉掗棿闅旓級

```bash
python3 scripts/e77_node_ctrl.py otaa \
    --port /dev/ttyUSB0 \
    --region 2 \
    --deveui AABBCCDD11223344 \
    --appeui 0000000000000000 \
    --appkey 00112233445566778899AABBCCDDEEFF \
    --chanmask 0000:0000:0000:0000:0000:00FF \
    --adr 1 \
    --interval 10 \
    --ack 1 \
    --payload DEADBEEF01020304
```

> `--ack 1` = Confirmed uplink锛圡AC layer ACK锛夛紝姣忓寘閮界瓑寰呬笅琛?ACK銆? 
> 鐢ㄤ簬楠岃瘉鍙屽悜閾捐矾姝ｅ父銆?
#### 鈶?OTAA 鍏ョ綉 + 鍙戝浐瀹氭鏁帮紙5鍖呭悗閫€鍑猴級

```bash
python3 scripts/e77_node_ctrl.py otaa \
    --port /dev/ttyUSB0 \
    --region 2 \
    --deveui AABBCCDD11223344 \
    --appeui 0000000000000000 \
    --appkey 00112233455566778899AABBCCDDEEFF \
    --chanmask 0000:0000:0000:0000:0000:00FF \
    --interval 20 \
    --count 5 \
    --ack 1
```

**缁撴灉锛?* 鍙?5 鍖呭悗鑴氭湰鑷姩閫€鍑恒€傜敤浜庡揩閫熼獙璇侀摼璺姸鎬併€?
#### 鈶?ABP 鍏ョ綉锛堜笉闇€瑕?Join 娴佺▼锛岀洿鎺ヤ娇鐢ㄩ潤鎬佸瘑閽ワ級

**鍓嶇疆鏉′欢锛?* 鍦?ChirpStack Device 鈫?**Activation** 鏍囩椤垫墜鍔ㄥ～鍐欎互涓嬪€硷細
- DevAddr: `26011234`
- NwkSEncKey: `00112233445566778899AABBCCDDEEFF`
- AppSKey: `FFEEDDCCBBAA99887766554433221100`

```bash
python3 scripts/e77_node_ctrl.py abp \
    --port /dev/ttyUSB0 \
    --region 2 \
    --devaddr 26011234 \
    --nwkskey 00112233445566778899AABBCCDDEEFF \
    --appskey FFEEDDCCBBAA99887766554433221100 \
    --chanmask 0000:0000:0000:0000:0000:00FF \
    --adr 0 \
    --dr 2 \
    --interval 60
```

> ABP 璺宠繃 Join 娴佺▼锛岀洿鎺ュ叆缃戙€傜敤浜庡姣旀祴璇曟垨缃戠粶璋冭瘯銆?
#### 鈶?鎭㈠鍑哄巶閰嶇疆锛堟竻闄ゆ棫璁剧疆锛?
```bash
python3 scripts/e77_node_ctrl.py restore --port /dev/ttyUSB0
```

**鐢ㄩ€旓細** 娴嬭瘯鍓嶆竻闄ゆ棫閰嶇疆锛岄伩鍏嶅巻鍙插弬鏁板共鎵帮紙濡?chanmask銆丄ppKey 绛夛級銆?
#### 鈶?verbose 妯″紡锛堣皟璇曚覆鍙ｉ€氫俊锛?
```bash
python3 scripts/e77_node_ctrl.py query --port /dev/ttyUSB0 --verbose
```

**鏁堟灉锛?* 鎵撳嵃鎵€鏈変覆鍙ｆ敹鍙戠殑鍘熷瀛楃涓诧紝鐢ㄤ簬璇婃柇閫氫俊闂銆?
#### 鈶?浠呭彂涓€鍖咃紙宸插叆缃戝悗锛?
```bash
python3 scripts/e77_node_ctrl.py send \
    --port /dev/ttyUSB0 \
    --payload CAFEBABE \
    --port-fwd 10 \
    --ack 0
```

**鍓嶇疆鏉′欢锛?* 鑺傜偣宸叉垚鍔?OTAA 鎴?ABP 鍏ョ綉銆? 
**鏁堟灉锛?* 鍙戦€佷竴涓?FPort=10銆乸ayload=CAFEBABE 鐨?unconfirmed 涓婅锛岀劧鍚庨€€鍑恒€?
#### 鈶?娓呴櫎鏃ч厤缃紙寮€濮嬫柊娴嬭瘯鍓嶅繀鎵ц锛?
```bash
python3 scripts/e77_node_ctrl.py restore --port /dev/ttyUSB0
```

**鐢ㄩ€旓細** 娓呴櫎妯″潡涓墍鏈夋棫鐨勬寔涔呭寲閰嶇疆锛圖evEUI銆丄ppKey銆丄ppEUI銆侀娈点€佷俊閬撴帺鐮佺瓑锛夛紝
骞惰Е鍙戠‖浠堕噸鍚€傚紑濮嬩换浣曟柊鐨勬祴璇曞墠蹇呴』鎵ц姝ゅ懡浠わ紝閬垮厤鍘嗗彶鍙傛暟骞叉壈銆?
**绛夊緟鏃堕棿锛?* 鍛戒护鎵ц鍚庣瓑寰?2~3 绉掕妯″潡瀹屾垚閲嶅惎锛屽啀鎵ц鍚庣画鍛戒护銆?
**鐥囩姸锛?* 濡傛灉閬囧埌 `AT+REGION=X` 杩斿洖 `AT_PARAM_ERROR` 鎴?`AT+CDEVEUI/CAPPEUI/CAPPKEY` 杩斿洖 `AT_ERROR`锛?璇存槑妯″潡鏈夋棫閰嶇疆鍐茬獊锛岀珛鍗虫墽琛屾鍛戒护娓呴櫎銆?
---

## 涓夈€佹祴璇曠粨鏋?
### 3.1 OTAA 鍏ョ綉

**缁撹锛氣渽 鎴愬姛**

鍏ョ綉鑰楁椂绾?5~8 绉掞紝涓插彛鏃ュ織绀轰緥锛?```
[19:41:01.234] [INFO] 鉁?璁剧疆棰戞: AT+REGION=2
[19:41:01.389] [INFO] 鉁?璁剧疆 DevEUI: AT+CDEVEUI=XXXXXXXXXXXX
[19:41:01.549] [INFO] 鉁?璁剧疆 AppEUI: AT+CAPPEUI=0000000000000000
[19:41:01.704] [INFO] 鉁?璁剧疆 AppKey: AT+CAPPKEY=XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
[19:41:01.859] [INFO] 鉁?鍙戝皠鍔熺巼: AT+CTXP=0
[19:41:02.017] [INFO] 鉁?ADR: AT+CADR=1
[19:41:02.176] [INFO] 鉁?鎵嬪姩鎺╃爜浣胯兘: AT+CMANUALMASK=1
[19:41:02.334] [INFO] 鉁?淇￠亾鎺╃爜: AT+CFREQBANDMASK=0000:0000:0000:0000:0000:00FF
[19:41:02.334] [INFO] 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
[19:41:02.335] [INFO] 瑙﹀彂 OTAA 鍏ョ綉 (region=CN470) ...
[19:41:12.001] [OK  ] 鉁?OTAA 鍏ョ綉鎴愬姛锛?```

缃戝叧 Monitor 鍚屾鍑虹幇 JoinRequest 涓婅鍜?JoinAccept 涓嬭锛?```
JSON up:  freq:487.100000  stat:1  data:"AAAAAA...RDM..."   鈫?JoinRequest
PULL_RESP received  鈫?ChirpStack 鎺ㄩ€佷笅琛?JSON down: freq:507.5 size:33                               鈫?JoinAccept
```

### 3.2 涓婅鏁版嵁

**缁撹锛氣渽 鎴愬姛锛?7+ 鍖呰繛缁彂閫侀浂涓㈠寘**

鍏稿瀷鍗曞寘鏃ュ織锛?```
[19:41:13.002] [INFO] 鈹€鈹€鈹€ 绗?1 鍖?鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
[19:41:13.003] [INFO] 鍙戦€佷笂琛?鈫?port=2 ack=1 payload=DEADBEEF01020304
[19:41:14.891] [OK  ] TX 纭: OK+SENT:01
[19:41:14.892] [OK  ] TX 纭: +EVT:SEND_CONFIRMED
[19:41:15.201] [DOWN] 涓嬭鏀跺埌: +EVT:RX_1, PORT 0, DR 5, RSSI -74, SNR 9
```

| 瀛楁 | 鍚箟 |
|------|------|
| `OK+SENT:01` | 鍙戦€佸畬鎴愶紝閲嶄紶 1 娆★紙Confirmed uplink 姝ｅ父琛屼负锛?|
| `+EVT:SEND_CONFIRMED` | 鏀跺埌 ChirpStack 涓嬪彂鐨?ACK锛屽弻鍚戦摼璺甯?|
| `+EVT:RX_1, PORT 0` | 涓嬭鍦?RX1 绐楀彛鏀跺埌锛孭ORT=0 琛ㄧず MAC 灞?ACK |
| `DR 5` | 涓嬭鏁版嵁閫熺巼 DR5锛圫F7锛夛紝ADR 宸插皢閫熺巼浼樺寲鍒版渶蹇?|
| `RSSI -74` | 鎺ユ敹淇″彿寮哄害 -74 dBm锛堝鍐呮祴璇曪紝鑹ソ锛?|
| `SNR 9` | 淇″櫔姣?9 dB锛堜紭绉€锛孲F7 鏈€浣庡彲鐢?-7.5 dB锛?|

ChirpStack Device Events 鎴潰鍙杩炵画 `uplink` 鍜?`ack` 浜嬩欢锛屽抚璁℃暟閫掑鏃犺烦鍙枫€?
### 3.3 涓嬭鏁版嵁锛堟墜鍔ㄥ叆闃燂級

**缁撹锛氣渽 鎴愬姛**

鍦?ChirpStack GUI 鈫?Device 鈫?Queue 鍏ラ槦 FPort=2 Payload=`AABBCC`锛?绛夊緟鑺傜偣涓嬩竴娆′笂琛岃Е鍙?RX1锛岃妭鐐逛覆鍙ｅ嚭鐜帮細
```
[DOWN] 涓嬭鏀跺埌: +EVT:RX_1, PORT 2, DR 5, RSSI -72, SNR 9
```

### 3.4 ADR 鑷€傚簲閫熺巼

**缁撹锛氣渽 鎴愬姛锛孲F12 鈫?SF7 鑷姩璋冩暣**

缃戝叧杩戣窛绂绘斁缃紙绾?1~2 绫筹級锛孋hirpStack ADR 绠楁硶閫氳繃 LinkADRReq MAC 鍛戒护鍦?鍓?3~4 鍖呭唴瀹屾垚璋冮€燂細
- 鍏ョ綉鍒濆锛欴R0锛圫F12BW125锛?- 绗?3 鍖呭悗锛欴R5锛圫F7BW125锛?- 绋冲畾鍚?RSSI锛?72 ~ -74 dBm锛孲NR锛? dB

### 3.5 缁煎悎淇″彿璐ㄩ噺

| 鎸囨爣 | 鍏稿瀷鍊?| 璇勪环 |
|------|--------|------|
| RSSI | -72 ~ -74 dBm | 鑹ソ锛堝鍐呰繎璺濈锛屽ぉ绾跨洿杩炲瀷锛?|
| SNR | 9 dB | 浼樼锛圫F7 瀹归檺鑼冨洿鍐呮湁澶ч噺浣欓噺锛?|
| 棰戠巼鍋忓樊 foff | ~300~500 Hz | 姝ｅ父锛屾櫠鎸宸寖鍥?|
| 涓㈠寘鐜囷紙57鍖呮牱鏈級 | 0% | 绋冲畾 |

---

## 鍥涖€侀亣鍒扮殑闂涓庡鐜?
### 闂 1锛氶娆¤繍琛岀珛鍗宠繑鍥?`AT_ERROR`

**鐜拌薄锛?*
```
[INFO] 鎵撳紑涓插彛 /dev/ttyUSB0 @ 9600 bps
[WARN] 鎸囦护澶辫触: AT  鍝嶅簲: ['AT_ERROR']
[WARN] AT 娴嬭瘯绗?1/3 娆″け璐?```

**鏍规湰鍘熷洜锛歱yserial 鎵撳紑涓插彛鏃堕粯璁ゆ媺鍔?DTR/RTS 鐢靛钩锛岃Е鍙戞ā鍧楃‖浠跺浣?*

E77-400M22S 鐨?NRST 寮曡剼杩炴帴鍒?USB-UART 杞崲鑺墖鐨?DTR/RTS锛堝父瑙佹帴娉曪級锛?pyserial 鍦?`serial.Serial()` 鏋勯€犳椂浼氬皢杩欎袱鏍圭嚎鎷夐珮/鎷変綆锛屼娇妯″潡杩涘叆澶嶄綅鐘舵€併€?澶嶄綅瀹屾垚锛堢害 1 绉掞級鍓嶅彂閫?AT 蹇呯劧杩斿洖 `AT_ERROR`銆?
**澶嶇幇姝ラ锛?*
1. 浠ｇ爜涓?`serial.Serial(port, baud, timeout=0.3)` 鈥?涓嶈 dsrdtr/rtscts
2. 绔嬪埢鍙戦€?`AT` 鎸囦护

**淇锛?*
```python
self.ser = serial.Serial(
    port=port, baudrate=baud,
    dsrdtr=False,   # 绂佹鑷姩鎿嶄綔 DSR/DTR
    rtscts=False,   # 绂佹纭欢娴佹帶
    xonxoff=False,  # 绂佹杞欢娴佹帶
)
time.sleep(1.5)                     # 绛夊緟妯″潡涓婄數灏辩华
self.ser.reset_input_buffer()       # 涓㈠純鍚姩娑堟伅
```
鍚屾椂 `at_test()` 澧炲姞 3 娆￠噸璇曪紝鍏煎鍏朵粬瑙﹀彂澶嶄綅鐨勫満鏅€?
---

### 闂 2锛歚AT+CCLASS=A` 杩斿洖 `AT_NO_NETWORK_JOINED`

**鐜拌薄锛?*
```
[WARN] 鎸囦护澶辫触: AT+CCLASS=A  鍝嶅簲: ['AT_NO_NETWORK_JOINED']
```
閰嶇疆姝ラ琚腑鏂紝鍚庣画鍛戒护鏈墽琛屻€?
**鏍规湰鍘熷洜锛欵77 鎵嬪唽瑙勫畾 Class A 鏄叆缃戦粯璁ゆā寮忥紝鍦ㄦ湭鍏ョ綉鐘舵€佷笅鏃犳硶璁剧疆 CCLASS**

> 鎵嬪唽鍘熸枃锛欳lass A 绫诲瀷閫傜敤浜庝笂琛屾暟鎹姤鏂囷紝E77 榛樿涓?Class A锛岃澶囧湪鍏ョ綉鍓嶄笉鏀寔鏇存敼 Class 绫诲瀷銆?
**澶嶇幇姝ラ锛?*
1. 鍦?`config_otaa()` 涓坊鍔?`steps.append(("AT+CCLASS=A", "璁剧疆绫诲瀷"))`
2. 鏈叆缃戞椂鍙戦€佽鍛戒护

**淇锛?*  
鐩存帴浠?`config_otaa()` 鍜?`config_abp()` 涓垹闄?`AT+CCLASS=A` 閰嶇疆姝ラ锛?鍏ョ綉鍚庡闇€鍒囨崲 Class C 鎵嶉渶鍐嶆墽琛屻€?
---

### 闂 3锛歄TAA JOIN FAILED锛堢涓€闃舵锛夆€斺€斾笂琛岄鐜囧畬鍏ㄤ笉瀵?
**鐜拌薄锛?*
```
[19:xx:xx] [ERROR] 鉁?OTAA 鍏ョ綉澶辫触锛佸搷搴? ['+EVT:JOIN FAILED']
```
缃戝叧 Monitor 鏃犱换浣?`JSON up` 鏃ュ織锛坓ateway-bridge 鏃ュ織 `event=up` 璁℃暟涓洪浂锛夈€?
**鏍规湰鍘熷洜锛歝hanmask `00FF:0000:0000:0000:0000:0000` 瀵瑰簲 CH0~CH7锛?70.3~471.7 MHz锛夛紝
鑰岀綉鍏冲疄闄呯洃鍚?CH80~CH87锛?86.3~487.7 MHz锛夛紝鐩稿樊 16 MHz**

鍒濆浼犲叆 `--chanmask 00FF:0000:0000:0000:0000:0000`锛圫ubBand1锛夛紝
E77 璁や负涓婅鍦?470.3~471.7 MHz锛岀綉鍏虫牴鏈病鏈夎繖浜涢鐜囩殑鏀跺彂鑳藉姏锛?Join Request 浠庢湭鍒拌揪缃戝叧銆?
**璇婃柇杩囩▼锛?*
1. 缃戝叧 Monitor锛氭棤 `JSON up` 杈撳嚭 鈫?灏勯灞傚畬鍏ㄦ病鏈夋敹鍒板寘
2. 妫€鏌ョ綉鍏冲疄闄?radio 棰戠巼锛歚radio 0 center=486600000, radio 1 center=487400000`
3. 8 涓笂琛屼俊閬擄細486.3~487.7 MHz 鈫?CN470 CH80~CH87
4. 瀵瑰簲 chanmask锛歮ask5 浣?浣?= `0xFF` 鈫?`0000:0000:0000:0000:0000:00FF`

**淇锛?* `--chanmask 0000:0000:0000:0000:0000:00FF`

---

### 闂 4锛歄TAA JOIN FAILED锛堢浜岄樁娈碉級鈥斺€擱X1 涓嬭棰戠巼涓嶅榻?猸?鏈€鏍稿績

> 杩欐槸鏈璋冭瘯涓渶闅愯斀涔熸渶鍏峰涔犱环鍊肩殑闂銆?
**鐜拌薄锛?*  
鏇存 chanmask 鍚庯紝缃戝叧 Monitor 纭涓婅宸叉敹鍒?JoinRequest锛?ChirpStack 涔熸帹閫佷簡 JoinAccept 涓嬭锛屼絾鑺傜偣濮嬬粓 JOIN FAILED锛?```
JSON up:  freq:487.100000  stat:1  鈫?JoinRequest 鏀跺埌 鉁?PULL_RESP received                 鈫?ChirpStack 鎺ㄩ€?JoinAccept 鉁?JSON down: freq:507.5 size:33      鈫?缃戝叧鍙戝嚭涓嬭 鉁?+EVT:JOIN FAILED                   鈫?鑺傜偣灏辨槸鏀朵笉鍒?鉂?```

**鏍规湰鍘熷洜锛欵77 鍐呴儴閫昏緫淇￠亾缂栧彿涓?CN470 缁濆淇￠亾缂栧彿涓嶄竴鑷达紝瀵艰嚧璁＄畻 RX1 閿欒**

E77 鐨?chanmask 浠?mask0 浣庝綅锛坆it0锛夊紑濮嬬嚎鎬х紪鍙峰唴閮ㄤ俊閬擄紝
鍗?mask5 bit0 = 鍐呴儴淇￠亾 80锛屼絾 E77 鍥轰欢鐨?RX1 璁＄畻鍏紡鍙兘鍩轰簬鐨勬槸
**宸插惎鐢ㄤ俊閬撶殑鏈湴搴忓彿**锛?~7锛夛紝鑰屼笉鏄?CN470 缁濆淇￠亾鍙凤紙80~87锛夛細

| E77 鍐呴儴閫昏緫缂栧彿 | CN470 缁濆淇￠亾 | E77 璁＄畻 RX1锛堥敊璇級| 姝ｇ‘ RX1 |
|----------------|--------------|---------------------|---------|
| 0 | CH80 | 500.3 + 0脳0.2 = 500.3 MHz | 506.7 MHz |
| 4 | CH84 | 500.3 + 4脳0.2 = **501.1 MHz** | **507.5 MHz** |

ChirpStack 鎸?CN470 瑙勮寖璁＄畻锛?f_{RX1} = 500.3 + (ch \bmod 48) \times 0.2$

浠?CH84 涓轰緥锛?500.3 + (84 \bmod 48) \times 0.2 = 500.3 + 7.2 = 507.5\ \text{MHz}$

E77 鎸夊唴閮ㄥ簭鍙?4 璁＄畻锛?500.3 + 4 \times 0.2 = 501.1\ \text{MHz}$

涓よ€呯浉宸?**6.4 MHz**锛孞oinAccept 鍦?507.5 MHz 鍙戝嚭锛孍77 鍦?501.1 MHz 绛夊緟锛屽畬鍏ㄩ敊寮€銆?
**璇婃柇杩囩▼锛堝叧閿瘉鎹級锛?*
1. 缃戝叧 Monitor 纭 PULL_RESP 涓嬭棰戠巼锛歚freq:507.5` 鈫?缃戝叧瀹為檯鍙戝埌 507.5 MHz
2. 瀵圭収 RX1 鍏紡鎺ㄧ畻锛?07.5 = 500.3 + 36脳0.2 鈫?ch=84 (mod 48) 鉁?3. 濡?E77 鎸夊唴閮ㄥ簭鍙?4 璁＄畻锛?01.1 MHz锛屽樊鍊?6.4 MHz

**淇锛氱‘淇?chanmask 鏄庣‘鍛婅瘔 E77 鑷繁鍦?CH80~CH87 鐨勭粷瀵逛綅缃?*

```
--chanmask 0000:0000:0000:0000:0000:00FF
```
姝ゆ椂 mask5 bit0~7 琚縺娲伙紝瀵瑰簲鐨勬槸 CN470 CH80~CH87锛?E77 鍐呴儴浼氬皢杩?8 涓俊閬撴槧灏勫埌鍏?RX1 璁＄畻琛ㄧ殑姝ｇ‘琛岋紝RX1 涓?ChirpStack 瀵归綈銆?
> **姝ら棶棰樼殑鏈川**锛欵77 chanmask 涓嶄粎鍐冲畾涓婅鍙戝皠棰戠巼锛岃繕褰卞搷涓嬭 RX1 绐楀彛鐨?> 棰戠巼璁＄畻鍩哄噯銆備袱涓嫭绔嬬殑闂闇€瑕佸悓涓€涓弬鏁帮紙chanmask锛夊悓鏃朵慨姝ｃ€?
---

### 闂 5锛欰T 閰嶇疆鍛戒护杩斿洖 `AT_PARAM_ERROR` / `AT_ERROR` 猸?鏂板彂鐜?
**鐜拌薄锛堝疄闄呯‖浠舵祴璇曪級锛?*
```
[20:45:25.677] [WARN] 鎸囦护澶辫触: AT+REGION=2  鍝嶅簲: ['AT_PARAM_ERROR']
[20:45:25.875] [WARN] 鎸囦护澶辫触: AT+CDEVEUI=AABBCCDD11223344  鍝嶅簲: ['AT_ERROR']
[20:45:26.072] [WARN] 鎸囦护澶辫触: AT+CAPPEUI=0000000000000000  鍝嶅簲: ['AT_ERROR']
[20:45:26.287] [WARN] 鎸囦护澶辫触: AT+CAPPKEY=00112233445566778899AABBCCDDEEFF  鍝嶅簲: ['AT_ERROR']
```

浣嗛殢鍚?OTAA 鍏ョ綉浠嶆垚鍔燂紝璇存槑妯″潡浣跨敤鐨勬槸鏃х殑鎸佷箙鍖栭厤缃€岄潪鑴氭湰鏂颁紶鍏ョ殑鍙傛暟銆?
**鏍规湰鍘熷洜锛欵77 妯″潡灏嗛厤缃弬鏁板瓨鍌ㄥ湪 NVS锛堥潪鏄撳け瀛樺偍锛夛紝鍓嶄竴娆℃祴璇曠殑 DevEUI銆丄ppKey銆丄ppEUI銆侀娈电瓑璁剧疆浠嶄繚鐣欏湪鍐呭瓨涓€?鏂扮殑 AT 鍛戒护璇曞浘瑕嗙洊杩欎簺鍊兼椂锛屽鏋滐細**

1. **妯″潡鐘舵€佹満宸插垵濮嬪寲**锛堜箣鍓嶅叆杩囩綉鎴栭厤缃繃锛夛細鎷掔粷鎺ュ彈鏂扮殑 DevEUI/AppEUI/AppKey 璁剧疆锛岃繑鍥?`AT_ERROR`
2. **REGION 鍙傛暟涓庝笂娆′笉鍚?*锛氬彲鑳借Е鍙戝浐浠剁殑鍙傛暟鏍￠獙閫昏緫锛岃繑鍥?`AT_PARAM_ERROR`

鑴氭湰缁х画鎵ц鍚庣画鍛戒护锛坈hanmask銆丄DR銆佸彂閫侊級锛屼絾鑺傜偣浣跨敤鐨勪粛鏄棫鐨?DevEUI/AppKey锛?鍒氬ソ杩欎簺鏃ч厤缃湪 ChirpStack 涓粛鐒舵敞鍐岋紝鎵€浠?OTAA JOIN 鎰忓鎴愬姛銆?
**澶嶇幇姝ラ锛?*
1. 杩炵画鎵ц涓ゆ OTAA 鍛戒护锛屼笉鍏抽棴璁惧鐢垫簮锛屼笉鎵ц restore
2. 绗簩娆¤繍琛屾椂锛屼笂杩?AT 閰嶇疆鍛戒护杩斿洖閿欒

**淇锛?*
鍦ㄤ换浣曟柊鐨?LoRaWAN 娴嬭瘯涔嬪墠锛?*蹇呴』鍏堟墽琛?*锛?```bash
python3 scripts/e77_node_ctrl.py restore --port /dev/ttyUSB0
time.sleep(2)  # 绛夊緟妯″潡閲嶅惎
```

restore 鎵ц鍚庯紝妯″潡鍥炲埌**鍑哄巶榛樿鐘舵€?*锛屾墍鏈?NVS 鍙傛暟锛圖evEUI 纭紪鐮侀櫎澶栵級琚竻绌猴紝
鍚庣画鏂扮殑 AT 閰嶇疆鍛戒护鎵嶈兘琚纭帴鍙椼€?
**瀹為檯楠岃瘉锛堜慨澶嶅悗锛夛細**
```bash
# 绗?1 姝ワ細娓呴櫎鏃ч厤缃?$ python3 scripts/e77_node_ctrl.py restore --port /dev/ttyUSB0
[20:45:44.950] [INFO] 鍑哄巶閰嶇疆宸叉仮澶嶏紝绛夊緟閲嶅惎 ...

# 绛夊緟 2 绉?
# 绗?2 姝ワ細閲嶆柊 OTAA锛堟鏃舵墍鏈?AT 鍛戒护閮借繑鍥?OK锛?$ python3 scripts/e77_node_ctrl.py otaa --port /dev/ttyUSB0 --region 2 ...
[20:45:52.584] [INFO] 鉁?璁剧疆棰戞: AT+REGION=2
[20:45:52.775] [INFO] 鉁?璁剧疆 DevEUI: AT+CDEVEUI=AABBCCDD11223344
[20:45:52.966] [INFO] 鉁?璁剧疆 AppEUI: AT+CAPPEUI=0000000000000000
[20:45:53.174] [INFO] 鉁?璁剧疆 AppKey: AT+CAPPKEY=00112233445566778899AABBCCDDEEFF
[20:46:02.618] [OK  ] 鉁?OTAA 鍏ョ綉鎴愬姛锛?```

---

## 浜斻€佹敞鎰忎簨椤?
### 5.1 chanmask 涓€瀹氳涓庣綉鍏冲瓙棰戞涓ユ牸瀵瑰簲

CN470 鏈?12 涓瓙璁″垝锛岃妭鐐?chanmask 蹇呴』绮剧‘鍖归厤缃戝叧 radio 棰戠巼瀵瑰簲鐨勫瓙璁″垝锛?
| 瀛愯鍒?| radio_0 | 涓婅淇￠亾 | chanmask |
|--------|---------|---------|---------|
| cn470_0 | 470.6 MHz | CH0~CH7 | `00FF:0000:0000:0000:0000:0000` |
| cn470_5 | 480.6 MHz | CH40~CH47 | `0000:0000:0000:FF00:0000:0000`锛堝緟楠岃瘉锛?|
| **cn470_10** | **486.6 MHz** | **CH80~CH87** | **`0000:0000:0000:0000:0000:00FF`** |

chanmask 缁撴瀯锛? 娈?脳 16bit锛宮ask0=CH0~15锛?..锛宮ask5=CH80~95銆? 
**鏈伐绋嬪浐瀹氱敤 `0000:0000:0000:0000:0000:00FF`锛屽垏鍕夸娇鐢ㄩ粯璁ょ殑 `00FF:...`銆?*

### 5.2 Gateway Bridge UDP 绔彛鏄?1680锛屼笉鏄?1700

鏈伐绋?`global_conf.cn490.json` 涓?`serv_port_up/down = 1680`锛?ChirpStack Gateway Bridge 闇€鐩戝惉 **1680** 绔彛銆?榛樿閰嶇疆鏂囦欢鍜屽ぇ澶氭暟鏁欑▼鐢ㄧ殑鏄?1700锛屽垏鎹㈡椂鍔″繀妫€鏌ャ€?
### 5.3 鍏ョ綉鍓嶄笉鑳借缃?AT+CCLASS

E77 鎵嬪唽鏄庣‘锛欳lass 鍙兘鍦ㄥ叆缃戝悗淇敼銆傚叆缃戝墠璋冪敤 `AT+CCLASS=X` 浼氳繑鍥?
`AT_NO_NETWORK_JOINED`锛屽鑷撮厤缃祦绋嬪紓甯镐腑鏂€?
### 5.4 pyserial 鎵撳紑涓插彛浼氳Е鍙?DTR/RTS 澶嶄綅

蹇呴』鏄惧紡浼犲叆 `dsrdtr=False, rtscts=False, xonxoff=False`锛屽苟涓旂瓑寰呰嚦灏?1.5 绉?鍐嶅彂閫?AT 鍛戒护銆傛棤娉曠鐢ㄦ椂锛堟煇浜?USB-UART 鑺墖鍥哄畾鎷夌嚎锛夛紝鍦?`at_test()` 涓?鍔犲叆瓒冲鐨勯噸璇曞拰寤惰繜銆?
### 5.5 E77 妯″潡 echo on 鏃朵細鍥炴樉鍛戒护

妯″潡榛樿寮€鍚?echo锛屽彂 `AT+REGION=2` 鍚庯紝妯″潡浼氬厛鍥炴樉 `AT+REGION=2` 鍐嶅洖
`OK`銆傝剼鏈腑閫氳繃杩囨护鍚屽悕琛屽鐞嗭紝鑻ヨ嚜宸卞啓浠ｇ爜闇€娉ㄦ剰涓嶈灏嗗洖鏄捐鍒や负鍝嶅簲銆?
### 5.6 AT+CJOIN=1:0 瑙﹀彂鍏ョ綉鍚庨渶绛夊緟鏈€闀?30 绉?
LoRaWAN OTAA 瑙勮寖鍏佽鑺傜偣鏈€澶氶噸璇曞娆★紙DR 閫掑噺锛夛紝
JOIN 瓒呮椂璁剧疆涓?30 绉掓槸鍚堢悊鐨勩€傝嫢缃戠粶鏉′欢姝ｅ父浣嗚秴鏃讹紝
浼樺厛妫€鏌?chanmask 鍜?ChirpStack Device AppKey 鏄惁鍖归厤銆?
### 5.7 Confirmed uplink 鐨?`OK+SENT:XX` 鍚箟

`XX` 涓洪噸浼犺鏁帮紝渚嬪 `OK+SENT:01` = 鍙戦€佷簡 1 娆★紙棣栨鍙戦€佺畻 0锛岄噸浼?1 娆＄畻 1锛夈€?杩欐槸 LoRaWAN 鏍囧噯琛屼负锛孋onfirmed uplink 鍦ㄦ敹鍒?ACK 鍓嶆渶澶氶噸璇?retries 娆°€?
### 5.8 姣忔鏂版祴璇曞墠蹇呴』娓呴櫎鏃ч厤缃?
E77 妯″潡鍦?NVS 涓繚瀛?DevEUI銆丄ppKey銆丄ppEUI銆侀娈电瓑鍙傛暟锛屾柇鐢典笉浼氫涪澶便€?濡傛灉娌℃湁鎵ц `restore` 鍛戒护娓呴櫎锛屾柊鐨?AT 閰嶇疆鍛戒护鍙兘琚嫆缁濓紙`AT_ERROR` / `AT_PARAM_ERROR`锛夛紝
骞朵笖鑺傜偣浼氱户缁娇鐢ㄦ棫鐨勫嚟鎹€備负閬垮厤娣锋穯鍜岄殣钄?Bug锛?**姣忔鏇存崲 AppKey 鎴栧垏鎹笉鍚岀殑 ChirpStack 瀹炰緥鏃讹紝鍔″繀鍏堟墽琛屼竴閬?restore**銆?
---

## 鍏€丩earnings

### L1锛歀oRaWAN CN470 chanmask 褰卞搷涓や欢浜?
chanmask 涓嶄粎鍐冲畾鑺傜偣涓婅**鍙戝皠棰戠巼**锛岃繕鐩存帴褰卞搷鑺傜偣瀵?RX1 涓嬭绐楀彛鐨?*棰戠巼璁＄畻鍩哄噯**銆?杩欐槸 CN470 涓?EU868/US915 绛夐娈电殑閲嶈鍖哄埆鈥斺€斿悗鑰呯殑 RX1 鏄浐瀹氱殑鎴栧熀浜庣畝鍗曞亸绉伙紝
鑰?CN470 鐨?RX1 鏄粠缁濆淇￠亾鍙疯绠楃殑銆?鑺傜偣蹇呴』鐭ラ亾鑷繁鐨勪笂琛屼俊閬撳湪 CN470 棰戣氨涓殑**缁濆浣嶇疆**锛屾墠鑳界畻鍑烘纭殑 RX1銆?
### L2锛欽OIN FAILED 鐨勮瘖鏂垎灞傛硶

JOIN FAILED 鍙兘鍙戠敓鍦ㄤ笁涓嫭绔嬩綅缃紝闇€瑕佷粠搴曞眰鍚戜笂閫愬眰鎺掗櫎锛?
```
鑺傜偣鍙戦€?JoinRequest
    鈫?1. 缃戝叧鏈夋病鏈夋敹鍒帮紵锛堢湅 Monitor 鐨?JSON up / event=up 璁℃暟锛?   鈫?娌℃湁锛氫笂琛岄鐜囦笉瀵癸紝妫€鏌?chanmask 鍜?gateway radio 棰戠巼
    鈫?2. ChirpStack 鏈夋病鏈夋帹閫?JoinAccept锛燂紙鐪?chirpstack 鏃ュ織 / gateway event=down 璁℃暟锛?   鈫?娌℃湁锛欰ppKey 涓嶅尮閰嶏紝鎴?DevEUI/AppEUI 鏈敞鍐?    鈫?3. 鑺傜偣鏈夋病鏈夋敹鍒?JoinAccept锛燂紙鐪?+EVT:JOINED 鎴?JOIN FAILED锛?   鈫?娌℃湁锛歊X1 棰戠巼鍋忕Щ锛岄€氬父鏄?chanmask 瀵艰嚧鑺傜偣璁＄畻鍑洪敊璇殑 RX1
```

### L3锛歡ateway-bridge 鏃ュ織鏄?NS 渚х殑绗竴涓洃鎺х偣

鍦?ChirpStack 渚ф棤浠讳綍鍙嶅簲鏃讹紝绗竴姝ュ簲璇ョ湅 gateway-bridge 鐨?`event=up`/`event=down` 璁℃暟锛?鑰屼笉鏄洿鎺ョ湅 chirpstack 鏃ュ織銆俫ateway-bridge 鏄?UDP 鈫?MQTT 鐨勭綉鍏筹紝
鑻?`event=up` 涓洪浂锛岃鏄庨棶棰樺湪鐗╃悊灞傦紙RF 棰戠巼锛夛紱
鑻ユ湁 `event=up` 浣?chirpstack 渚ф棤 uplink锛岃鏄庨棶棰樺湪鍗忚灞傦紙AppKey/region锛夈€?
### L4锛欵77 AT 鎸囦护鎺ュ彛鐨勫仴澹€ф妧宸?
| 闂 | 瑙ｆ硶 |
|------|------|
| 涓婄數 AT_ERROR | `dsrdtr=False` + 绛夊緟 1.5s + 閲嶈瘯 3 娆?|
| 鍥炴樉骞叉壈 | 杩囨护鍙戦€佸懡浠ゆ湰韬殑瀛楃涓?|
| CCLASS 鎶ラ敊 | 鍏ョ綉鍓嶄笉璁剧疆锛屽叆缃戞垚鍔熷悗鍐嶅垏鎹?|
| JOIN 瓒呮椂 | 鍏?`AT+RESTORE` 娓呴櫎鏃ч厤缃紝鍐嶉噸鏂伴厤缃?chanmask |

### L5锛欳N470 RX1 棰戠巼鍏紡

$$f_{RX1} = 500.3 + (ch_{uplink} \bmod 48) \times 0.2 \text{ MHz}$$

鏈伐绋嬬綉鍏?CH80~CH87 瀵瑰簲 RX1锛?
$$f_{RX1} = 506.7 \sim 508.1\ \text{MHz}$$

RX2 鍥哄畾锛?05.3 MHz SF12BW125锛堜笂琛屽悗 2 绉掞級銆?
### L6锛欰DR 涓?chanmask 鐨勭浉浜掍緷璧?
寮€鍚?ADR锛坄AT+CADR=1`锛夋椂锛宑hanmask 鎺у埗鑺傜偣涓婅浣跨敤鍝簺淇￠亾锛?ChirpStack ADR 绠楁硶鍩轰簬鍘嗗彶 RSSI/SNR 璁＄畻鏈€浼?DR锛屽苟閫氳繃 `LinkADRReq` MAC 鍛戒护涓嬪彂銆?姝ゆ椂鑺傜偣浼氬湪澶氫釜淇￠亾杞浆鍙戝寘锛岀綉鍏充晶 ADR 闇€瑕佸鍖呮牱鏈墠鑳界敓鏁堬紝閫氬父 3~5 鍖呭悗璋冩暣瀹屾垚銆?娴嬭瘯鏃朵娇鐢ㄧ煭闂撮殧锛坄--interval 10`锛夊彲浠ュ姞閫?ADR 璋冩暣銆?
---

## 涓冦€佹祴璇曡繘搴︾姸鎬?
- 鉁?E77-400M22S AT 鎸囦护鎺у埗鑴氭湰锛坄scripts/e77_node_ctrl.py`锛?- 鉁?瑙ｅ喅 DTR/RTS 澶嶄綅闂
- 鉁?瑙ｅ喅 AT+CCLASS 鍏ョ綉鍓嶆姤閿?- 鉁?瑙ｅ喅 chanmask 涓婅棰戠巼涓嶅锛圕H0~CH7 鈫?CH80~CH87锛?- 鉁?瑙ｅ喅 chanmask 瀵艰嚧 RX1 棰戠巼鍋忕Щ锛?.4 MHz 閿欎綅锛?- 鉁?瑙ｅ喅 NVS 鎸佷箙鍖栭厤缃鑷?AT 鍛戒护琚嫆缁濋棶棰?- 鉁?OTAA 鍏ョ綉鎴愬姛锛圝OIN 鑰楁椂绾?5~8 绉掞級
- 鉁?涓婅鏁版嵁楠岃瘉锛?7+ 鍖咃紝0 涓㈠寘锛?- 鉁?Confirmed uplink / 涓嬭 ACK 楠岃瘉
- 鉁?鎵嬪姩鍏ラ槦涓嬭鏁版嵁锛圥ORT 2锛夐獙璇?- 鉁?ADR 鑷€傚簲楠岃瘉锛圫F12 鈫?SF7 鑷姩璋冮€燂級
- 鈴?ABP 鍏ョ綉楠岃瘉锛堝緟鍚庣画鍒嗘敮锛?- 鈴?Class C 娴嬭瘯锛堝緟鍚庣画鍒嗘敮锛?- 鈴?澶氳妭鐐瑰苟鍙戞祴璇曪紙寰呭悗缁垎鏀級

---

*璁板綍浜猴細cuckooshan锛?026-02-27*
