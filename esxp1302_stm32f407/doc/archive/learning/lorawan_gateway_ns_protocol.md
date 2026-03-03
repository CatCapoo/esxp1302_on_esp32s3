# LoRaWAN 缃戝叧涓?Network Server 鍗忚瑙勫畾

> 鍐欎綔鑳屾櫙锛?026-02-22 璋冭瘯 ESXP1302 鎺ュ叆 ChirpStack锛?> 娣卞叆鐞嗚В浜?Semtech UDP 鍖呰浆鍙戝崗璁€丟ateway Bridge 鐨勪綔鐢ㄥ拰 MQTT 璇濋瑙勮寖銆?
---

## 涓€銆丩oRaWAN 缃戠粶鏋舵瀯鎬昏

LoRaWAN 閲囩敤鏄熷舰鎷撴墤锛岀敱鍥涘眰缁勬垚锛?
```
鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?  LoRa RF    鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?  UDP/IP   鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?  MQTT   鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?End Node 鈹?鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€ 鈹?Gateway  鈹?鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€ 鈹?Gateway Bridge  鈹?鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€ 鈹?Network Server 鈹?鈹傦紙鑺傜偣锛? 鈹?             鈹傦紙缃戝叧锛? 鈹?1700/1680  鈹傦紙鍗忚杞崲锛?    鈹?         鈹傦紙NS锛孋hirpStack锛夆攤
鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?             鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?            鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?         鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?```

姣忓眰鑱岃矗锛?
| 灞?| 鑱岃矗 |
|----|------|
| **End Node** | 浼犳劅鍣?鎵ц鍣紝閫氳繃 LoRa 鍙戦€佹暟鎹紝瀹炵幇 ClassA/B/C |
| **Gateway** | 绾墿鐞嗗眰杞彂锛屾敹 LoRa 鍖呪啋灏?UDP锛屼笉瑙ｅ瘑銆佷笉鍒ゆ柇鐪熷亣 |
| **Gateway Bridge** | 灏?Semtech UDP 鍗忚杞崲涓?MQTT锛屼袱绔兘鍙互閰嶇疆 |
| **Network Server** | 瑙ｅ瘑銆佸幓閲嶃€丄DR銆佷笅琛岃皟搴︺€佽澶囬壌鏉?|
| **Application Server** | 澶勭悊涓氬姟鏁版嵁锛孨S 閫氳繃 gRPC/HTTP 瀵规帴 |

---

## 浜屻€丼emtech UDP 鍖呰浆鍙戝崗璁紙Semtech Packet Forwarder Protocol锛?
鏈崗璁槸缃戝叧鍜?Gateway Bridge/NS 涔嬮棿鐨勬爣鍑嗗崗璁紝
鐢?Semtech 璁捐骞跺紑婧愶紙basic_pkt_fwd銆乴ora_pkt_fwd锛夛紝
LoRaWAN 鐢熸€佷腑鍑犱箮鎵€鏈夌綉鍏抽兘鏀寔銆?
### 2.1 UDP 绔彛绾﹀畾

| 鏂瑰悜 | 绔彛 | 璇存槑 |
|------|------|------|
| 缃戝叧 鈫?NS锛堝彂閫佹柟锛?| 闅忔満锛堟湰鍦扮鍙ｏ級| 鍔ㄦ€佸垎閰?|
| NS 鈫?缃戝叧锛堟帴鏀舵柟锛屼笂琛岀鍙ｏ級 | **1700**锛堥粯璁わ級| 鎺ユ敹 PUSH_DATA |
| NS 鈫?缃戝叧锛堟帴鏀舵柟锛屼笅琛岀鍙ｏ級 | **1700**锛堥粯璁わ級| 鎺ユ敹 PULL_RESP |
| 缃戝叧鏈湴锛堟帴鏀?PULL_RESP锛墊 **1680**锛堥粯璁わ級| 閮ㄥ垎瀹炵幇鐢ㄤ笉鍚岀鍙?|

> 鏈」鐩厤缃細serv_port_up=1680锛宻erv_port_down=1680锛堣 global_conf.json锛?
### 2.2 浜旂娑堟伅绫诲瀷

#### PUSH_DATA锛堢綉鍏?鈫?NS锛屼笂琛屾暟鎹級

缃戝叧鏀跺埌 LoRa 鍖呭悗锛屾墦鍖呮垚 JSON 閫氳繃 UDP 鍙戠粰 NS锛?
```
瀛楄妭0锛氬崗璁増鏈紙0x02锛?瀛楄妭1-2锛氶殢鏈?token锛?瀛楄妭锛岀敤浜庡尮閰?ACK锛?瀛楄妭3锛氭秷鎭被鍨?0x00 = PUSH_DATA
瀛楄妭4-11锛氱綉鍏?EUI锛?瀛楄妭锛屽ぇ绔級
瀛楄妭12+锛欽SON 瀛楃涓诧紙rxpk 鏁扮粍锛?```

JSON 绀轰緥锛?```json
{
  "rxpk": [{
    "tmst": 12345678,     // 缃戝叧鏈湴鏃堕棿鎴筹紙寰锛?2浣嶅惊鐜級
    "freq": 486.7,        // 鎺ユ敹棰戠巼锛圡Hz锛?    "chan": 2,            // 淇￠亾缂栧彿锛圛F0~IF7锛?    "rfch": 0,            // radio 缂栧彿锛?鎴?锛?    "stat": 1,            // CRC 鐘舵€侊紙1=OK, -1=FAIL, 0=NO_CRC锛?    "modu": "LORA",       // 璋冨埗鏂瑰紡
    "datr": "SF12BW125",  // 鏁版嵁鐜囷紙SFxBWyyy锛?    "codr": "4/5",        // 缂栫爜鐜?    "rssi": -73,          // RSSI锛坉Bm锛?    "lsnr": 4.0,          // SNR锛坉B锛?    "size": 11,           // payload 瀛楄妭鏁?    "data": "aGVsbG8="   // Base64 缂栫爜鐨?payload
  }]
}
```

#### PUSH_ACK锛圢S 鈫?缃戝叧锛岀‘璁ゆ敹鍒颁笂琛岋級

```
瀛楄妭0锛?x02
瀛楄妭1-2锛氫笌 PUSH_DATA 鐩稿悓鐨?token
瀛楄妭3锛?x01 = PUSH_ACK
```

**ackr锛坅cknowledged ratio锛?* 灏辨槸杩欎釜 ACK 鐨勫洖澶嶇巼锛?`lora_pkt_fwd.c` 缁熻 30 绉掑唴鍙戝嚭 PUSH_DATA 娆℃暟鍜屾敹鍒?PUSH_ACK 娆℃暟涔嬫瘮銆?
#### PULL_DATA锛堢綉鍏?鈫?NS锛屼繚鎸佽繛鎺?/ 鎷夊彇涓嬭锛?
缃戝叧姣忛殧 ~10 绉掑彂涓€娆★紝鐢ㄩ€旓細
1. 鍛婅瘔 NS 缃戝叧鐨?UDP 鍑哄彛 IP:Port锛圢AT 绌块€忥級
2. 璁?NS 鐭ラ亾缃戝叧鍦ㄧ嚎

```
瀛楄妭0锛?x02
瀛楄妭1-2锛歵oken
瀛楄妭3锛?x02 = PULL_DATA
瀛楄妭4-11锛氱綉鍏?EUI
```

#### PULL_ACK锛圢S 鈫?缃戝叧锛岀‘璁ゆ敹鍒?PULL_DATA锛?
```
瀛楄妭0锛?x02
瀛楄妭1-2锛氫笌 PULL_DATA 鐩稿悓 token
瀛楄妭3锛?x04 = PULL_ACK
```

#### PULL_RESP锛圢S 鈫?缃戝叧锛屼笅琛屽彂鍖呮寚浠わ級

NS 瑕佸彂涓嬭甯ф椂锛岄€氳繃涔嬪墠 PULL_DATA 纭鐨勫湴鍧€锛屽彂 PULL_RESP 缁欑綉鍏筹細

```json
{
  "txpk": {
    "imme": false,       // 鏄惁绔嬪嵆鍙戯紙false = 鐢?tmst 绮剧‘瀹氭椂锛?    "tmst": 12346678,    // 鍙戝皠鏃堕棿鎴筹紙涓婅 tmst + 1000000 for RX1锛?    "freq": 507.1,       // 涓嬭棰戠巼锛圡Hz锛?    "rfch": 0,           // 浣跨敤鍝釜 radio 鍙戝皠
    "powe": 14,          // 鍙戝皠鍔熺巼锛坉Bm锛?    "modu": "LORA",
    "datr": "SF12BW125",
    "codr": "4/5",
    "ipol": true,        // 涓嬭鏋佸寲鍙嶈浆锛圠oRaWAN 瑙勫畾涓嬭蹇呴』 ipol=true锛?    "size": 17,
    "data": "YAFCAQABAAAAc..."
  }
}
```

> **鏋佸寲鍙嶈浆锛坕pol锛?* 鏄?LoRaWAN 鐨勪竴涓噸瑕佺粏鑺傦細
> 鑺傜偣涓婅 ipol=false锛岀綉鍏充笅琛?ipol=true锛?> 杩欐牱涓や釜鑺傜偣鐩镐簰涔嬮棿鏀朵笉鍒板鏂圭殑涓婅鍖咃紝鍙湁缃戝叧锛坕pol=true锛夋墠鑳芥敹鍒拌妭鐐逛笂琛屻€?
---

### 2.3 杩炴帴鐘舵€佺淮鎶?
Semtech UDP 鏄棤杩炴帴鐨勶紝缃戝叧闈犱互涓嬫満鍒剁淮鎶?鍦ㄧ嚎"鐘舵€侊細

```
缃戝叧                         NS (Gateway Bridge)
 鈹傗攢鈹€鈹€鈹€鈹€ PUSH_DATA锛坰tats锛夆攢鈹€鈹€鈹€鈫掆攤  姣?0绉掍竴娆★紝鍚綉鍏崇粺璁℃暟鎹? 鈹傗啇鈹€鈹€鈹€鈹€ PUSH_ACK 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? 鈹?                            鈹? 鈹傗攢鈹€鈹€鈹€鈹€ PULL_DATA 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈫掆攤  姣?0绉掍竴娆? 鈹傗啇鈹€鈹€鈹€鈹€ PULL_ACK 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?```

NS 閫氳繃杩炵画鏀跺埌 PUSH_DATA 鎴?PULL_DATA 鏉ュ垽鏂綉鍏虫槸鍚﹀湪绾匡紝
瓒呰繃涓€瀹氭椂闂存病鏀跺埌灏辨爣璁颁负绂荤嚎銆?
---

## 涓夈€丟ateway Bridge 鐨勪綔鐢?
### 3.1 涓轰粈涔堥渶瑕?Gateway Bridge锛?
ChirpStack NS 鍘熺敓浣跨敤 **MQTT** 涓庣綉鍏抽€氫俊锛?鑰?99% 鐨?LoRa 缃戝叧纭欢瀹炵幇鐨勬槸 Semtech UDP 鍗忚锛屼簩鑰呬笉鍏煎銆?
Gateway Bridge 浣滀负閫傞厤鍣紝鎶?UDP 鍗忚杞崲鎴?MQTT锛?
```
缃戝叧锛圲DP锛夆攢鈹€鈫?Gateway Bridge 鈹€鈹€鈫?MQTT Broker 鈹€鈹€鈫?ChirpStack NS
                鈫?涔熷彲鍙嶅悜锛?                NS 閫氳繃 MQTT 鍙戝懡浠?鈫?Bridge 杞垚 UDP PULL_RESP 鈫?缃戝叧
```

### 3.2 MQTT Topic 瑙勮寖

Gateway Bridge 鍙戝竷鍜岃闃呯殑 topic 鏍煎紡锛?
```
{topic_prefix}/gateway/{gateway_eui}/event/{event_type}
{topic_prefix}/gateway/{gateway_eui}/state/{state_type}
{topic_prefix}/gateway/{gateway_eui}/command/#
```

| 瀛楁 | 鍚箟 | 绀轰緥 |
|------|------|------|
| `topic_prefix` | 棰戠巼璁″垝鏍囪瘑绗?| `cn470_10` |
| `gateway_eui` | 缃戝叧 EUI锛?*灏忓啓鍗佸叚杩涘埗** | `aa555a00000021fb` |
| `event_type` | 浜嬩欢绫诲瀷 | `up`锛堜笂琛岋級銆乣stats`锛堢粺璁★級銆乣ack` |
| `state_type` | 鐘舵€佺被鍨?| `conn`锛堣繛鎺ョ姸鎬侊級|

**瀹屾暣 topic 绀轰緥锛?*

```
cn470_10/gateway/aa555a00000021fb/event/up
cn470_10/gateway/aa555a00000021fb/event/stats
cn470_10/gateway/aa555a00000021fb/state/conn
cn470_10/gateway/aa555a00000021fb/command/#
```

### 3.3 topic_prefix 涓庨鐜囪鍒掔殑鍏崇郴

`topic_prefix` **蹇呴』**涓?NS 涓?region 閰嶇疆鏂囦欢鐨?`topic_prefix` 瀛楁瀹屽叏涓€鑷淬€?
杩欐槸 ChirpStack 鐭ラ亾"杩欎釜缃戝叧灞炰簬鍝釜棰戠巼璁″垝"鐨勫敮涓€渚濇嵁锛?
```
gateway-bridge 鐨?topic_prefix="cn470_10"
        鈫?蹇呴』瀹屽叏涓€鑷达紙鍖呮嫭涓嬪垝绾裤€佹暟瀛楋級
region_cn470_10.toml 鐨?topic_prefix="cn470_10"
```

濡傛灉涓嶄竴鑷达紙鍝€曞彧宸竴涓瓧绗?`cn470` vs `cn470_10`锛夛紝
NS 灏辨敹涓嶅埌杩欎釜缃戝叧鐨勪换浣曟秷鎭紝GUI 姘歌繙鏄剧ず绂荤嚎銆?
---

## 鍥涖€丩oRaWAN 缃戝叧鐨?閫忔槑杞彂"鍘熷垯

**缃戝叧涓嶅仛浠讳綍涓氬姟澶勭悊**锛屽彧璐熻矗鐗╃悊灞傛敹鍙戯紝杩欐槸 LoRaWAN 鏋舵瀯鐨勬牳蹇冭璁″師鍒欙細

| 缃戝叧鍋氱殑 | 缃戝叧涓嶅仛鐨?|
|---------|----------|
| 鎺ユ敹鎵€鏈?LoRa 甯э紙鏃犺鏄惁娉ㄥ唽锛墊 楠岃瘉 DevAddr |
| 娴嬮噺 RSSI / SNR / 棰戝亸 | 瑙ｅ瘑 FRMPayload |
| 鎵撲笂鎺ユ敹鏃堕棿鎴筹紙tmst锛墊 鍒ゆ柇 MIC 鏄惁鍚堟硶 |
| 杞彂缁?NS锛堟棤杩囨护锛墊 ADR 鍐崇瓥 |
| 鎵ц NS 涓嬭鍛戒护 | 缂撳瓨鏁版嵁 |

杩欐剰鍛崇潃锛?- 涓€涓?LoRa 鍖呭彲浠ュ悓鏃惰澶氫釜缃戝叧鏀跺埌锛孨S 璐熻矗**鍘婚噸**
- 缃戝叧瑕嗙洊鑼冨洿鍐呮墍鏈夎澶囩殑涓婅鍖呴兘浼氳涓婃姤锛孨S 鏍规嵁 DevAddr 鍒ゆ柇鏄笉鏄嚜宸辩殑璁惧
- 瀹夊叏鎬у畬鍏ㄧ敱 NS 鍜岃妭鐐逛箣闂寸殑 AES-128 鍔犲瘑淇濊瘉锛岀綉鍏虫棤娉曚吉閫?
---

## 浜斻€丆lassA / ClassB / ClassC 鐨勪笅琛屾椂搴?
LoRaWAN 瀹氫箟浜嗕笁绉嶈妭鐐瑰伐浣滄ā寮忥紝缃戝叧闇€瑕佺簿纭畾鏃跺彂涓嬭鍖咃細

### ClassA锛堟渶甯哥敤锛?
```
鑺傜偣鍙戜笂琛?   鈹?   鈹溾攢鈹€ 1绉掑悗 鈹€鈹€鈫?RX1 绐楀彛锛堜笅琛岄鐜?涓婅棰戠巼鐨勬槧灏勫€硷紝SF=涓婅SF锛?   鈹?   鈹斺攢鈹€ 2绉掑悗 鈹€鈹€鈫?RX2 绐楀彛锛堝浐瀹氶鐜?05.3MHz锛孲F12锛?```

姣忔涓婅鍚庢墠鏈変笅琛屾満浼氾紝鏈€鐪佺數銆侼S 蹇呴』鍦?RX1 鎴?RX2 涔嬩竴鍙戦€佷笅琛岋紝鍚﹀垯绛変笅娆′笂琛屻€?
### ClassB

鍦?ClassA 鍩虹涓婏紝鑺傜偣鍛ㄦ湡鎬ф墦寮€"ping slot"鎺ユ敹绐楀彛锛?鐢ㄤ俊鏍囷紙Beacon锛夊悓姝ワ紝鍏佽 NS 鍦ㄥ浐瀹氭椂闅欎富鍔ㄤ笅琛岋紝寤惰繜鍙娴嬨€?
### ClassC

鑺傜偣闄や簡鍙戦€佷笂琛岋紝鍏朵綑鏃堕棿**鎸佺画鐩戝惉**锛孨S 闅忔椂鍙互涓嬭銆?鍔熻€楁渶楂橈紝閫傚悎鏈夌ǔ瀹氱數婧愮殑璁惧锛堝鏅鸿兘鎻掑骇锛夈€?
---

## 鍏€佷笅琛屽彂灏勬椂搴忕殑绮剧‘鎬ц姹?
NS 璋冨害涓嬭鍖呮椂锛屼細鍦?PULL_RESP 鐨?`txpk.tmst` 涓寚瀹氱簿纭彂灏勬椂闂达細

```
tmst_tx = tmst_rx + RX1_delay 脳 1_000_000锛堝井绉掞級
```

鍏朵腑 `tmst_rx` 鏄綉鍏冲湪 PUSH_DATA 涓笂鎶ョ殑鎺ユ敹鏃堕棿鎴炽€?
缃戝叧蹇呴』淇濊瘉鍦?`tmst_tx` 鏃跺埢锛堣宸?< 卤20 渭s锛夊紑濮嬪彂灏勶紝
鍚﹀垯鑺傜偣鐨?RX 绐楀彛鍏抽棴锛屼笅琛屽寘涓㈠け銆?
SX1302 鍐呯疆纭欢瀹氭椂鍣ㄧ敤浜庝繚璇佽繖涓簿搴︼紝杩欎篃鏄负浠€涔堢綉鍏充笉鑳界敤绾蒋浠舵柟妗堢殑鍘熷洜銆?
---

## 涓冦€佸缃戝叧瑕嗙洊涓庡幓閲?
褰撳涓綉鍏抽兘瑕嗙洊鍚屼竴鍖哄煙鏃讹紝鍚屼竴涓笂琛屽寘浼氳澶氫釜缃戝叧鏀跺埌骞朵笂鎶ワ細

```
Node 鈹€鈹€涓婅鈹€鈹€鈫?Gateway A 鈹€鈹€PUSH_DATA鈹€鈹€鈫?NS
           鈹斺攢鈹€鈫?Gateway B 鈹€鈹€PUSH_DATA鈹€鈹€鈫?NS  锛堝悓涓€涓寘锛屼袱浠斤級
```

NS 鐨勫幓閲嶉€昏緫锛圕hirpStack 涓殑 deduplication window锛夛細

1. 鏀跺埌绗竴浠斤紝鍚姩鍘婚噸绐楀彛锛堥粯璁?200ms锛?2. 绐楀彛鍐呮敹鍒扮殑鍏朵粬鍓湰璁板綍 metadata锛圧SSI銆丼NR銆佺綉鍏矱UI绛夛級
3. 绐楀彛缁撴潫鍚庯紝閫夋嫨淇″彿鏈€寮虹殑閭ｄ唤澶勭悊锛屽叾浣欎涪寮?4. 鎶婃墍鏈夌綉鍏崇殑 metadata 姹囨€伙紝渚夸簬缃戠粶浼樺寲

---

## 鍙傝€冭祫鏂?
- LoRa Alliance锛歀oRaWAN Specification v1.0.4
- Semtech锛歔UDP Packet Forwarder Protocol](https://github.com/Lora-net/packet_forwarder/blob/master/PROTOCOL.TXT)
- ChirpStack Gateway Bridge锛歨ttps://www.chirpstack.io/docs/chirpstack-gateway-bridge/
