# LoRaWAN 鑺傜偣璋冭瘯瀛︿範绗旇锛欵77-400M22S OTAA 鍏ㄩ摼璺?
> 鍐欎綔鑳屾櫙锛?026-02-27 浣跨敤鎴愰兘浜夸桨鐗?E77-400M22S锛圕N470锛夐€氳繃 AT 鎸囦护鎺ュ叆
> ESXP1302 缃戝叧 + ChirpStack v4锛岀粡鍘嗗杞?JOIN FAILED 鍚庢渶缁堝畬鏁存墦閫氥€?> 瀹屾暣杩囩▼璁板綍瑙?[test_notes/test_e77_lorawan_node_validation_memo.md](../test_notes/test_e77_lorawan_node_validation_memo.md)

---

## 涓€銆丆N470 chanmask 鐨勫弻閲嶄綔鐢?
杩欐槸鏈璋冭瘯鏈€鏍稿績鐨?Learning銆?
CN470 鐨?chanmask 涓嶅彧鏄喅瀹氳妭鐐?*涓婅棰戠巼**锛岃繕鍚屾椂鍐冲畾鑺傜偣濡備綍**璁＄畻 RX1 涓嬭绐楀彛棰戠巼**銆?
### 1.1 涓婅棰戠巼锛堝鏄撴兂鍒帮級

chanmask 鍛婅瘔鑺傜偣鍝簺淇￠亾鍙敤锛岃妭鐐瑰湪杩欎簺淇￠亾涓婅疆杞彂閫?JoinRequest / 鏁版嵁甯с€?濡傛灉 chanmask 涓庣綉鍏冲疄闄呯洃鍚俊閬撲笉瀵归綈锛岀綉鍏虫敹涓嶅埌涓婅鍖呫€?
### 1.2 RX1 涓嬭棰戠巼锛堝鏄撳拷鐣ワ級

CN470 鐨?RX1 涓嬭棰戠巼鐢变笂琛屼俊閬撶殑**缁濆缂栧彿**鍐冲畾锛?
$$f_{RX1} = 500.3 + (ch_{uplink} \bmod 48) \times 0.2 \text{ MHz}$$

鑺傜偣蹇呴』鐭ラ亾鑷繁鍙戝嚭鐨勯偅鍖?JoinRequest 璧扮殑鏄?CN470 绗嚑鍙蜂俊閬擄紝
鎵嶈兘鍦ㄦ纭殑 RX1 棰戠巼涓婄瓑寰?JoinAccept銆?
濡傛灉 chanmask 鍛婅瘔鑺傜偣"浣犲湪 CH0~CH7"锛屼絾瀹為檯涓婄綉鍏冲湪 CH80~CH87锛?
| 鎯呭舰 | 鑺傜偣璁や负鑷繁鍦?| 鑺傜偣璁＄畻 RX1 | ChirpStack 瀹為檯鍙?JoinAccept |
|------|-------------|------------|---------------------------|
| chanmask 閿欙紙00FF...锛?| CH0~CH7锛堝唴閮ㄥ簭 0~7锛?| 500.3~501.7 MHz | 506.7~508.1 MHz |
| chanmask 瀵癸紙...00FF锛?| CH80~CH87 | **506.7~508.1 MHz** 鉁?| 506.7~508.1 MHz 鉁?|

6.4 MHz 鐨勯鐜囧樊璺濓紝JoinAccept 姘歌繙鏀朵笉鍒帮紝涓€鐩?JOIN FAILED銆?
### 1.3 鏈伐绋嬬殑姝ｇ‘ chanmask

```
--chanmask 0000:0000:0000:0000:0000:00FF
```

**chanmask 缁撴瀯锛?* 6 娈?脳 16bit锛宮ask0=CH0~15锛宮ask1=CH16~31锛?..锛宮ask5=CH80~95

`00FF` in mask5 = bit0~7 of the 6th segment = **CH80~CH87**

> 鈿狅笍 CN470 涓?EU868/US915 鐨勯噸瑕佸尯鍒細EU868 鐨?RX1 鏄浐瀹氶鐜囧亸绉伙紝US915 涔熸槸
> 鍥哄畾鍏紡锛屼笉渚濊禆鑺傜偣"鐭ラ亾鑷繁鍦ㄥ摢涓俊閬?銆侰N470 鐨?RX1 鍩轰簬缁濆淇￠亾鍙凤紝
> 鍥犳 chanmask 鐨勬纭€х洿鎺ュ奖鍝嶄笅琛岃兘鍚︽敹鍒般€?
---

## 浜屻€丱TAA JOIN FAILED 鍒嗗眰璇婃柇娉?
JOIN 澶辫触鍙兘鍦ㄤ笁涓嫭绔嬩綅缃柇閾撅紝闇€瑕佷粠搴曞眰鍚戜笂閫愬眰鎺掗櫎锛?
```
鑺傜偣鍙戦€?JoinRequest
        鈹?        鈻?鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?灞?1锛氱墿鐞嗗眰锛圧F 棰戠巼锛?              鈹?鈹?璇婃柇锛氱綉鍏?Monitor 鏈夋棤 JSON up锛?    鈹?鈹?      gateway-bridge event=up 璁℃暟锛? 鈹?鈹?鏃?鈫?chanmask 閿欙紝涓婅棰戠巼涓嶅        鈹?鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?        鈹?鏈?JSON up
        鈻?鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?灞?2锛氱綉缁滄湇鍔″櫒灞?                   鈹?鈹?璇婃柇锛歝hirpstack 鏃ュ織鏈夋棤 join 浜嬩欢锛?鈹?鈹?      Device Events 鏈夋棤 join锛?      鈹?鈹?鏃?鈫?AppKey 涓嶅尮閰?/ DevEUI 鏈敞鍐?  鈹?鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?        鈹?鏈?join锛岀湅鍒?JoinAccept 涓嬪彂
        鈻?鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?灞?3锛氫笅琛屾帴鏀跺眰锛圧X1 棰戠巼锛?        鈹?鈹?璇婃柇锛氱綉鍏虫湁鏃?JSON down锛?           鈹?鈹?      涓嬭棰戠巼鏄惁涓?RX1 鍏紡涓€鑷达紵   鈹?鈹?鑺傜偣浠?JOIN FAILED 鈫?RX1 棰戠巼鍋忕Щ   鈹?鈹?鈫?chanmask 瀵艰嚧鑺傜偣 RX1 璁＄畻閿欒     鈹?鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?        鈹?鑺傜偣鏀跺埌 JoinAccept
        鈻?  +EVT:JOINED 鉁?```

**鍏抽敭锛?* gateway-bridge 鏃ュ織鏄?NS 渚х殑绗竴涓洃鎺х偣銆傚嚭闂鏃跺厛鐪嬪畠锛?姣旂洿鎺ョ湅 chirpstack 鏃ュ織瀹氫綅鏇村揩銆?
---

## 涓夈€丒77-400M22S AT 鎺ュ彛瑕佺偣

### 3.1 pyserial 鎵撳紑涓插彛瑙﹀彂纭欢澶嶄綅

pyserial `serial.Serial()` 榛樿浼氭搷浣?DTR/RTS 寮曡剼锛屽緢澶?USB-UART 鑺墖灏?杩欎袱鏍圭嚎鎺ュ埌妯″潡鐨?NRST/BOOT锛屽鑷存ā鍧楀浣嶃€傛墦寮€涓插彛鍚庣珛鍗冲彂 AT 蹇呯劧澶辫触銆?
**蹇呴』璁剧疆锛?*
```python
serial.Serial(port, baud,
    dsrdtr=False,   # 涓嶈嚜鍔ㄦ搷浣?DSR/DTR
    rtscts=False,   # 涓嶇‖浠舵祦鎺?    xonxoff=False,  # 涓嶈蒋浠舵祦鎺?)
time.sleep(1.5)  # 绛夊緟涓婄數/澶嶄綅瀹屾垚
ser.reset_input_buffer()
```

### 3.2 鍏ョ綉鍓嶄笉鑳借缃?AT+CCLASS

E77 鎵嬪唽锛欳lass A 鏄叆缃戦粯璁ゆā寮忥紝鍏ョ綉鍓?`AT+CCLASS=X` 杩斿洖 `AT_NO_NETWORK_JOINED`銆?鍙湁鍦?`+EVT:JOINED` 涔嬪悗鎵嶈兘鍒囨崲 Class B/C銆?
### 3.3 妯″潡 echo on锛氬搷搴斾腑鍖呭惈鍛戒护鍥炴樉

妯″潡榛樿寮€鍚?echo锛屽彂閫?`AT+REGION=2` 鍚庝細鍏堟敹鍒?`AT+REGION=2` 鍐嶆敹鍒?`OK`銆?瑙ｆ瀽鍝嶅簲鏃堕渶瑕佽繃婊ゆ帀涓庡彂閫佸懡浠ょ浉鍚岀殑琛岋紝鍚﹀垯浼氳鍒ゃ€?
```python
echo_str = cmd.strip().upper()
lines = [l for l in raw_lines if l.upper() != echo_str]
```

### 3.4 `AT+CJOIN=1:0` 鍙傛暟鍚箟

```
AT+CJOIN=<JOIN_MODE>:<AUTO_JOIN>
         1=OTAA      0=涓嶈嚜鍔ㄩ噸杩?         0=ABP       1=涓婄數鑷姩鍏ョ綉
```

OTAA 鎵嬪姩娴嬭瘯鐢?`AT+CJOIN=1:0`锛孉BP 鐢?`AT+CJOIN=0:0`銆?
### 3.5 `AT+SEND` 鏍煎紡

```
AT+SEND=<PORT>:<RETRIES>:<ACK>:<PAYLOAD_HEX>
```

- `RETRIES`锛欳onfirmed uplink 鏈敹鍒?ACK 鏃剁殑閲嶄紶娆℃暟
- `ACK=1`锛欳onfirmed uplink锛堝弻鍚戠‘璁わ級锛沗ACK=0`锛歎nconfirmed
- 鍝嶅簲 `OK+SENT:XX`锛歑X 涓哄疄闄呴噸浼犳鏁帮紙棣栨鍙戦€佺畻 0锛?
### 3.6 AT+RESTORE 鐢ㄤ簬娓呴櫎鏃?chanmask

璋冭瘯杩囩▼涓娆′慨鏀?chanmask 鍚庯紝鏃ч厤缃彲鑳芥寔涔呭寲鍦ㄦā鍧?Flash 涓紝
瀵艰嚧琛屼负涓庨鏈熶笉绗︺€傛瘡娆″垏鎹㈤娈佃鍒掑墠寤鸿鍏?`AT+RESTORE` 鎭㈠鍑哄巶銆?
---

## 鍥涖€丆N470 RX1/RX2 閫熸煡

### RX1 棰戠巼鍏紡

$$f_{RX1} = 500.3 + (ch_{uplink} \bmod 48) \times 0.2 \text{ MHz}$$

### 鏈伐绋?CN470_10锛圕H80~CH87锛塕X1 鏄犲皠

| 涓婅淇￠亾 | 涓婅棰戠巼 | ch mod 48 | RX1 涓嬭棰戠巼 |
|---------|---------|-----------|------------|
| CH80 | 486.3 MHz | 32 | 506.7 MHz |
| CH81 | 486.5 MHz | 33 | 506.9 MHz |
| CH82 | 486.7 MHz | 34 | 507.1 MHz |
| CH83 | 486.9 MHz | 35 | 507.3 MHz |
| CH84 | 487.1 MHz | 36 | 507.5 MHz |
| CH85 | 487.3 MHz | 37 | 507.7 MHz |
| CH86 | 487.5 MHz | 38 | 507.9 MHz |
| CH87 | 487.7 MHz | 39 | 508.1 MHz |

### RX2锛堝浐瀹氾級

**505.3 MHz锛孲F12BW125**锛屼笂琛屽悗 2 绉掋€傝妭鐐规湭鏀跺埌 RX1 鍝嶅簲鏃惰嚜鍔ㄥ垏鎹㈠埌 RX2銆?
---

## 浜斻€丄DR 宸ヤ綔鏈哄埗

- **寮€鍚柟娉曪細** `AT+CADR=1`锛孋hirpStack 渚?Device Profile 涔熼』寮€鍚?ADR
- **璋冮€熷師鐞嗭細** ChirpStack 鏀堕泦鍘嗗彶 SNR/RSSI 鏍锋湰锛岃绠楁渶澶у彲鐢?DR锛岄€氳繃 `LinkADRReq` MAC 鍛戒护涓嬪彂鏂?DR 鍜?TXPower
- **鐢熸晥鏃舵満锛?* 閫氬父 3~5 鍖呭悗锛堥渶瑕佽冻澶熺殑 SNR 鏍锋湰锛?- **鏈娴嬭瘯缁撴灉锛?* 鍏ョ綉 SF12锛圖R0锛夆啋 绗?3~4 鍖呭悗 SF7锛圖R5锛夛紝RSSI -74 dBm锛孲NR 9 dB

> ADR 璋冮€熷悗鑺傜偣姣忓寘鑰楁椂浠?~2.5 绉掗檷鍒?~0.1 绉掞紙SF12 vs SF7 on air time锛夛紝
> 瀵圭數姹犱緵鐢佃妭鐐瑰姛鑰楀奖鍝嶆樉钁椼€?
---

## 鍏€佺浉鍏虫枃妗?
| 鏂囨。 | 璇存槑 |
|------|------|
| [lora_cn470_frequency_plan.md](lora_cn470_frequency_plan.md) | CN470 棰戠巼璁″垝瀹屾暣浣撶郴 |
| [chirpstack_v4_usage.md](chirpstack_v4_usage.md) | ChirpStack 閰嶇疆涓庢搷浣?|
| [test_notes/test_e77_lorawan_node_validation_memo.md](../test_notes/test_e77_lorawan_node_validation_memo.md) | 鏈娴嬭瘯瀹屾暣杩囩▼璁板綍锛堝惈澶嶇幇姝ラ鍜屾祴璇曟暟鎹級 |
| `scripts/e77_node_ctrl.py` | E77 鎺у埗鑴氭湰婧愮爜 |

---

*鍐欎綔鏃ユ湡锛?026-02-27*
