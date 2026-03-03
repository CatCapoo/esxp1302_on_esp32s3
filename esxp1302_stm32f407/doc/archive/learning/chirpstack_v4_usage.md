# ChirpStack v4 浣跨敤绗旇

> 鍐欎綔鑳屾櫙锛?026-02-22 瀹屾垚 ESXP1302 鎺ュ叆 ChirpStack v4 鐨勫叏閾捐矾璋冭瘯锛?> 璁板綍 ChirpStack 鐨勯厤缃綋绯汇€佸父瑙佹搷浣滃拰璋冭瘯鎶€宸с€?
---

## 涓€銆丆hirpStack v4 鏁翠綋鏋舵瀯

ChirpStack v4 鐩告瘮 v3 鍋氫簡閲嶅ぇ閲嶆瀯锛?
| 瀵规瘮椤?| v3 | v4 |
|--------|----|----|
| 缁勪欢鏁伴噺 | NS + AS + GW Bridge锛?涓嫭绔嬫湇鍔★級| 鍚堝苟涓哄崟涓€ `chirpstack` 杩涚▼ |
| 鏁版嵁搴?| PostgreSQL + Redis | 鍚屼笂 |
| 閰嶇疆鏍煎紡 | 鍚勬湇鍔＄嫭绔?TOML | 鍗曚竴 chirpstack.toml + region_xxx.toml |
| Region 鏀寔 | 缂栬瘧鏃跺喅瀹?| 杩愯鏃堕厤缃紝鍙 region 鍚屾椂杩愯 |
| API | gRPC + REST | 鍚屼笂锛孯EST via chirpstack-rest-api |

### 1.1 Docker 閮ㄧ讲缁撴瀯锛堟湰椤圭洰锛?
```
~/chirpstack-docker/
鈹溾攢鈹€ docker-compose.yml           鈫?鎵€鏈夊鍣ㄥ畾涔?鈹斺攢鈹€ configuration/
    鈹溾攢鈹€ chirpstack/
    鈹?  鈹溾攢鈹€ chirpstack.toml      鈫?NS 涓婚厤缃?    鈹?  鈹溾攢鈹€ region_cn470_10.toml 鈫?CN470_10 鍖哄煙閰嶇疆
    鈹?  鈹斺攢鈹€ region_*.toml        鈫?鍏朵粬鍖哄煙锛堝彲閫夛級
    鈹溾攢鈹€ chirpstack-gateway-bridge/
    鈹?  鈹斺攢鈹€ chirpstack-gateway-bridge.toml
    鈹斺攢鈹€ mosquitto/
        鈹斺攢鈹€ mosquitto.conf
```

### 1.2 鍚勫鍣ㄧ鍙?
| 瀹瑰櫒 | 绔彛 | 鐢ㄩ€?|
|------|------|------|
| chirpstack | **8080** | Web GUI + gRPC API |
| chirpstack-rest-api | **8090** | REST API锛堜唬鐞?gRPC锛墊
| chirpstack-gateway-bridge | **1700/udp** | 鎺ユ敹缃戝叧 UDP 鍖?|
| mosquitto | **1883/tcp** | MQTT Broker |
| redis | 6379 | 鍐呴儴缂撳瓨锛堜笉瀵瑰鏆撮湶锛墊
| postgres | 5432 | 鎸佷箙鍖栨暟鎹簱锛堜笉瀵瑰鏆撮湶锛墊

---

## 浜屻€佸叧閿厤缃枃浠惰瑙?
### 2.1 chirpstack.toml锛圢S 涓婚厤缃級

**鍚敤鍖哄煙锛堝繀椤讳笌 gateway-bridge topic_prefix 涓€鑷达級锛?*

```toml
[network]
  enabled_regions=[
    "cn470_10",
    # 鍙互鍚屾椂鍚敤澶氫釜 region锛屾瘡涓?region 鐙珛澶勭悊瀵瑰簲 topic 鐨勬暟鎹?  ]
```

**API 鍜?Web UI锛?*

```toml
[api]
  bind="0.0.0.0:8080"
  secret="your-secret-key"  # JWT 绛惧悕瀵嗛挜锛屾敼鎺夐粯璁ゅ€?```

**鏁版嵁搴擄細**

```toml
[postgresql]
  dsn="postgres://chirpstack:chirpstack@postgresql/chirpstack?sslmode=disable"

[redis]
  servers=["redis://redis/"]
```

### 2.2 region_cn470_10.toml锛堝尯鍩熼厤缃級

**鏍稿績瀛楁锛?*

```toml
[regions.cn470_10]
  description="CN470 (LoRa Alliance plan, channels 80-87)"

  [regions.cn470_10.gateway]
    force_gws_private=false

  [[regions.cn470_10.network.extra_channels]]
    # CN470_10 鐨?涓笂琛屼俊閬?    frequency=486300000
    min_dr=0
    max_dr=5
    # ... 鍏朵綑7涓被浼?
  [regions.cn470_10.network]
    enabled_uplink_channels=[80, 81, 82, 83, 84, 85, 86, 87]

  [[regions.cn470_10.rx2]]
    frequency=505300000
    dr=0

  [regions.cn470_10.gateway_topic_prefix]
    topic_prefix="cn470_10"    # 蹇呴』涓?gateway-bridge 鐨?topic 鍓嶇紑涓€鑷?```

### 2.3 docker-compose.yml锛坓ateway-bridge 鍏抽敭閰嶇疆锛?
```yaml
chirpstack-gateway-bridge:
  image: chirpstack/chirpstack-gateway-bridge:4
  environment:
    # 杩欎笁琛岀殑鍓嶇紑蹇呴』涓?region TOML 涓殑 topic_prefix 涓€鑷?    - INTEGRATION__MQTT__EVENT_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/event/{{ .EventType }}
    - INTEGRATION__MQTT__STATE_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/state/{{ .StateType }}
    - INTEGRATION__MQTT__COMMAND_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/command/#
  ports:
    - "1700:1700/udp"   # 鎺ユ敹缃戝叧 UDP
```

> 鈿狅笍 **甯歌鍧?*锛歒AML 涓暱瀛楃涓蹭笉鑳芥湁鎹㈣锛屾煇浜涚紪杈戝櫒浼氬湪80鍒楄嚜鍔ㄦ姌琛岋紝
> 瀵艰嚧 template 瀛楃涓茶鎴柇锛宼opic 瑙ｆ瀽澶辫触銆傜敤 Python/sed 淇敼鏇村畨鍏細
> ```bash
> python3 -c "
> content = open('docker-compose.yml').read()
> content = content.replace('cn470/', 'cn470_10/')
> open('docker-compose.yml', 'w').write(content)
> "
> ```

---

## 涓夈€佸湪 ChirpStack 涓敞鍐岃澶囷細瀹屾暣娴佺▼涓庡弬鏁拌鏄?
ChirpStack 娉ㄥ唽涓€鍙版柊璁惧闇€瑕佷緷娆″畬鎴?4 姝ワ紝鍚庨潰鐨勬楠や緷璧栧墠闈㈢殑姝ラ鍒涘缓鐨勫璞★細

```
鈶?娉ㄥ唽缃戝叧锛圙ateway锛?       鈫?鈶?鍒涘缓璁惧閰嶇疆鏂囦欢锛圖evice Profile锛?       鈫?鈶?鍒涘缓搴旂敤锛圓pplication锛?       鈫?鈶?鍦ㄥ簲鐢ㄥ唴娉ㄥ唽璁惧锛圖evice锛夊苟濉啓瀵嗛挜
```

---

### 3.1 娉ㄥ唽缃戝叧锛圙ateway锛?
Web UI 鈫?**Gateways** 鈫?**Add gateway**

#### 瀛楁璇存槑

| 瀛楁 | 绀轰緥鍊?| 璇存槑 |
|------|--------|------|
| **Name** | `ESXP1302-Lab` | 浠呯敤浜庢樉绀猴紝鏃犲姛鑳藉奖鍝?|
| **Gateway ID锛圗UI-64锛?* | `AA555A00000021FB` | **鍏抽敭瀛楁**銆?瀛楄妭 EUI锛屽叏灞€鍞竴鏍囪瘑缃戝叧 |
| Description | 闅忔剰 | 澶囨敞 |
| Tags | 鍙€?| Key-Value 鍏冩暟鎹紝鍙敤浜庤繃婊?|

**Gateway ID 浠庡摢鏉ワ紵**  
鏈伐绋嬪浐浠跺湪鍚姩鏃堕€氳繃 `esp_read_mac()` 璇诲彇 ESP32-S3 WiFi MAC 鍦板潃锛?鎷兼帴鍚庢墦鍗板埌涓插彛 Monitor锛?```
Gateway EUI: AA:55:5A:00:00:00:21:FB
```
鍘绘帀鍐掑彿鍗充负 `AA555A00000021FB`銆備篃鍙湪 `global_conf.cn490.json` 涓殑
`gateway_ID` 瀛楁鏌ュ埌銆?
**娉ㄥ唽鍚庨獙璇侊細**  
鍑犵鍚庡埛鏂伴〉闈紝**Last seen** 搴旀樉绀哄綋鍓嶆椂闂达紙鑰屼笉鏄?"Never"锛夛紱
鐘舵€佹寚绀哄彉涓虹豢鑹?**Online**銆傝嫢浠嶆樉绀虹绾匡紝妫€鏌?gateway-bridge 鐨?MQTT topic 鍓嶇紑鏄惁涓?NS region id 涓€鑷达紙瑙佺浜岃妭锛夈€?
---

### 3.2 鍒涘缓璁惧閰嶇疆鏂囦欢锛圖evice Profile锛?
**Device Profile** 瀹氫箟浜?涓€绫昏妭鐐?鐨?LoRaWAN 鍙傛暟锛屽悓鍨嬪彿鐨勫鍙拌澶囧叡浜悓涓€涓?Profile锛屼笉闇€瑕佹瘡鍙板崟鐙缃€?
Web UI 鈫?**Device profiles** 鈫?**Add device profile**

#### General 閫夐」鍗?
| 瀛楁 | 鎺ㄨ崘鍊?| 璇存槑 |
|------|--------|------|
| **Name** | `E77-CN470-OTAA` | 鍛藉悕寤鸿鍖呭惈鍨嬪彿+棰戞+鍏ョ綉鏂瑰紡锛屼究浜庡尯鍒?|
| **Region** | `CN470_10` | **鍏抽敭瀛楁**銆傚繀椤讳笌缃戝叧 radio 棰戠巼瀵瑰簲鐨勫瓙棰戞涓€鑷达紝鏈伐绋嬪浐瀹氶€?CN470_10 |
| **MAC version** | `LoRaWAN 1.0.3` | **鍏抽敭瀛楁**銆傚繀椤讳笌鑺傜偣鍥轰欢鐗堟湰涓ユ牸鍖归厤銆侲77-400M22S 鍥轰欢涓?LoRaWAN 1.0.3锛岄€夐敊浼氬鑷?MIC 鏍￠獙澶辫触鎴?FCnt 閫昏緫涓嶅吋瀹?|
| **Regional parameters revision** | `A`锛堝嵆 RP002-1.0.1锛?| 涓?MAC version 閰嶅鐨勫尯鍩熷弬鏁扮増鏈€?.0.3 瀵瑰簲閫?A |
| **ADR algorithm** | `Default ADR algorithm (LoRa only)` | ChirpStack 鍐呯疆 ADR 绛栫暐锛氭敹闆嗗巻鍙?SNR 鏍锋湰锛岃嚜鍔ㄤ笅鍙?LinkADRReq 璋冩暣 DR 鍜?TxPower銆傞€?Default 鍗冲彲 |
| **Expected uplink interval** | `3600`锛堢锛?| 棰勬湡鑺傜偣涓婅闂撮殧锛岀敤浜庡垽鏂澶囨槸鍚︽椿璺冿紙瓒呮椂鍚?Device 鐘舵€佸彉涓?inactive锛夈€傛祴璇曟椂鍙～杈冨皬鍊煎 `120` |
| **Device-status request interval** | `0`锛堢鐢級| 鑷姩鍙?DevStatusReq 鏌ョ數姹犲拰 SNR 鐨勯棿闅旓紝0 = 绂佺敤銆傛祴璇曚笉闇€瑕佸紑 |

#### Join (OTAA) 閫夐」鍗?
| 瀛楁 | 鍊?| 璇存槑 |
|------|----|------|
| **Supports OTAA** | 鉁?鍕鹃€?| 琛ㄧず姝?Profile 鐨勮澶囦娇鐢?OTAA 鍏ョ綉銆侽TAA 浼樹簬 ABP锛氭瘡娆″叆缃戝姩鎬佹淳鍙?DevAddr 鍜?Session Key锛岄槻閲嶆斁鏀诲嚮 |

#### Class B / Class C 閫夐」鍗?
| 瀛楁 | 鍊?| 璇存槑 |
|------|----|------|
| Supports Class B | 涓嶅嬀 | Class B 闇€瑕佺綉鍏冲悓姝?Beacon锛岀洰鍓嶆湭娴嬭瘯 |
| Supports Class C | 涓嶅嬀 | Class C 鑺傜偣甯稿紑鎺ユ敹绐楀彛锛屽姛鑰楅珮锛岄渶瑕佽妭鐐瑰浐浠舵敮鎸併€侲77 鍏ョ綉鍚庡彲閫氳繃 `AT+CCLASS=C` 鍒囨崲锛屽眾鏃跺啀鍒涘缓鍗曠嫭 Profile |

#### Codec 閫夐」鍗?
| 瀛楁 | 璇存槑 |
|------|------|
| Payload codec | 鐢ㄤ簬鍦?GUI 涓嚜鍔ㄨВ鐮?payload銆傞€?`None` 鍒欐樉绀哄師濮?hex锛涢€?`CayenneLPP` 鍒欒嚜鍔ㄨВ鏋愪紶鎰熷櫒鏁版嵁鏍煎紡锛涗篃鍙～鑷畾涔?JavaScript 瑙ｇ爜鍣?|

> **灏忕粨**锛欴evice Profile 鎻忚堪鐨勬槸鑺傜偣"鑳藉仛浠€涔?锛堢増鏈€丆lass銆丄DR 鏀寔锛夛紝
> 涓嶆秹鍙婂叿浣撶殑 DevEUI / AppKey锛屽彲浠ヨ澶氬彴鍚屽瀷鍙疯澶囧鐢ㄣ€?
---

### 3.3 鍒涘缓搴旂敤锛圓pplication锛?
**Application** 鏄澶囩殑閫昏緫鍒嗙粍瀹瑰櫒銆傚悓涓€搴旂敤鍐呯殑璁惧鍏变韩锛?- 鍚屼竴涓?MQTT uplink topic锛坄application/<id>/device/+/event/up`锛?- 鍚屼竴濂?HTTP integration / webhook 閰嶇疆
- 鍚屼竴涓?API 閴存潈瑙嗗浘

Web UI 鈫?**Applications** 鈫?**Add application**

| 瀛楁 | 璇存槑 |
|------|------|
| **Name** | 濡?`GW-Validation-Test` |
| Description | 闅忔剰 |
| Tags | 鍙€?Key-Value 鍏冩暟鎹?|

> 鐢熶骇鍦烘櫙涓竴涓笟鍔＄郴缁熷搴斾竴涓?Application锛?> 璋冭瘯鍦烘櫙涓€鑸竴涓」鐩缓涓€涓?Application 鍗冲彲銆?
---

### 3.4 娉ㄥ唽璁惧锛圖evice锛夊苟濉啓瀵嗛挜

鍦?Application 鍐呬负姣忓彴鐗╃悊鑺傜偣鍒涘缓涓€涓?Device 鏉＄洰銆?
**璺緞锛?* Web UI 鈫?**Applications** 鈫?閫夋嫨搴旂敤 鈫?**Add device**

#### General 閫夐」鍗?
| 瀛楁 | 绀轰緥鍊?| 璇存槑 |
|------|--------|------|
| **Name** | `E77-Node-01` | 鏄剧ず鍚嶏紝闅忔剰 |
| **DevEUI** | `AABBCCDD11223344` | **鍏抽敭瀛楁**銆?瀛楄妭鍏ㄥ眬鍞竴璁惧鏍囪瘑绗︼紝鐑у啓鍦ㄨ妭鐐硅姱鐗囧唴锛岄€氳繃 `AT+CDEVEUI=?` 鏌ヨ |
| **AppEUI锛圝oinEUI锛?* | `0000000000000000` | 鏍囪瘑 Join Server 鐨?8瀛楄妭 EUI銆侺oRaWAN 1.0.x 绉?AppEUI锛?.1 绉?JoinEUI銆傝嚜寤烘祴璇曞叏濉?0 鍗冲彲锛涚敓浜у満鏅敱閮ㄧ讲鏂瑰垎閰?|
| **Device profile** | 閫夋嫨姝ラ 3.2 鍒涘缓鐨?Profile | 鍐冲畾 MAC 鐗堟湰銆丷egion銆丄DR 绛?|
| **Skip frame-counter checks** | 璋冭瘯鏈熼棿鍕鹃€?| **璋冭瘯蹇呭嬀**銆備笉鍕炬椂鑻ヨ妭鐐归噸鐑у悗 FCnt 浠?0 閲嶇疆锛孨S 浼氬洜涓?FCnt 鍥為€€鑰屾嫆缁濇墍鏈変笂琛屽寘锛堥槻閲嶆斁淇濇姢锛夈€傜敓浜х幆澧冧笉鍕?|
| Tags | 鍙€?| |
| Variables | 鍙€?| 鍙湪 JS codec 涓紩鐢ㄧ殑鑷畾涔夊彉閲?|

**DevEUI 浠庡摢鏉ワ紵**  
瀵逛簬 E77-400M22S锛岄€氳繃 AT 鎸囦护鏌ヨ锛?```bash
python3 scripts/e77_node_ctrl.py query --port /dev/ttyUSB0
# 鎴栫洿鎺ュ彂 AT 鎸囦护锛?AT+CDEVEUI=?
# 鈫?杩斿洖: AT+CDEVEUI=AABBCCDD11223344
```
姣忓潡妯″潡鍑哄巶鐑у啓鍞竴 DevEUI锛屼笉鑳戒慨鏀广€?
---

#### 濉啓瀵嗛挜锛圤TAA锛?
鐐瑰嚮 **Submit** 娣诲姞璁惧鍚庯紝鑷姩璺宠浆鍒拌澶囪鎯呴〉銆? 
杩涘叆 **Keys (OTAA)** 鏍囩椤碉細

| 瀛楁 | 瀛楄妭鏁?| 璇存槑 |
|------|--------|------|
| **Application key锛圓ppKey锛?* | 16B | **鏈€鍏抽敭鐨勫瘑閽?*銆傝妭鐐逛晶鍜?NS 渚у叡鍚屾寔鏈夛紝鐢ㄤ簬鎺ㄥ鎵€鏈?Session Key銆傚繀椤讳笌鑺傜偣鍥轰欢涓儳鍐欑殑 AppKey 瀹屽叏涓€鑷达紙澶у皬鍐欎笉鏁忔劅锛?|
| **NwkKey**锛堜粎 1.1锛墊 16B | LoRaWAN 1.1 鏂板鐨勭綉缁滃瘑閽ワ紝涓?AppKey 鍒嗙銆?.0.x 璁惧鏃犳瀛楁 |

**AppKey 鏄粈涔堬紝涓轰粈涔堥噸瑕侊紵**

AppKey 鏄牴瀵嗛挜锛圧oot Key锛夛紝OTAA 鍏ョ綉鏃讹細
```
NwkSKey = aes128_encrypt(AppKey, 0x01 || AppNonce || NetID || DevNonce || pad)
AppSKey = aes128_encrypt(AppKey, 0x02 || AppNonce || NetID || DevNonce || pad)
```
- `NwkSKey`锛圢etwork Session Key锛夛細鐢ㄤ簬 MAC 灞傚抚鐨?MIC 璁＄畻鍜屽姞瑙ｅ瘑
- `AppSKey`锛圓pplication Session Key锛夛細鐢ㄤ簬搴旂敤 payload 鐨勫姞瑙ｅ瘑

AppKey 娉勯湶 = Session Key 鍙鎺ㄧ畻 = 鎵€鏈夊巻鍙插拰鏈潵鏁版嵁琚В瀵嗐€?**涓嶅悓璁惧蹇呴』浣跨敤涓嶅悓鐨?AppKey**锛堝嚭鍘傛椂鍚勮嚜鐢熸垚锛屾垨鐢?NS 鎵归噺鐢熸垚鍚庣儳鍐欙級銆?
娴嬭瘯鏃跺彲鍦?ChirpStack 璁惧璇︽儏椤电偣鍑?**Generate** 鎸夐挳闅忔満鐢熸垚锛?鐒跺悗灏嗙敓鎴愮殑 32 浣?hex 瀛楃涓插～鍏ヨ妭鐐?`--appkey` 鍙傛暟銆?
---

#### 濉啓瀵嗛挜锛圓BP锛?
ABP 涓嶆墽琛?Join 娴佺▼锛孲ession Key 鐩存帴棰勭疆锛岄渶瑕佹墜鍔ㄥ～鍐欐縺娲诲弬鏁般€?
杩涘叆 **Activation** 鏍囩椤碉細

| 瀛楁 | 瀛楄妭鏁?| 璇存槑 |
|------|--------|------|
| **Device address锛圖evAddr锛?* | 4B | 缃戠粶鍐呭敮涓€鍦板潃锛孉BP 鏃剁敱鐢ㄦ埛鑷畾锛屽 `26011234`銆侽TAA 鏃剁敱 NS 鍔ㄦ€佸垎閰嶏紝鏃犻渶鎵嬪～ |
| **NwkSEncKey** | 16B | LoRaWAN 1.1锛氱綉缁滃眰鍔犲瘑 Session Key锛?.0.x 涓瓑鍚屼簬 NwkSKey锛?|
| **SNwkSIntKey** | 16B | LoRaWAN 1.1锛氭湇鍔″櫒渚х綉缁滃眰瀹屾暣鎬?Key锛?.0.x 涓?NwkSKey 鐩稿悓锛?|
| **FNwkSIntKey** | 16B | LoRaWAN 1.1锛氳浆鍙戜晶缃戠粶灞傚畬鏁存€?Key锛?.0.x 涓?NwkSKey 鐩稿悓锛?|
| **AppSKey** | 16B | 搴旂敤灞?Session Key锛岀敤浜?payload 鍔犺В瀵?|
| **Uplink frame-counter锛團CntUp锛?* | 4B int | 鑺傜偣涓婅甯ц鏁板櫒鍒濆鍊笺€傞€氬父濉?`0`锛屼笌鑺傜偣渚у悓姝?|
| **Downlink frame-counter锛圢FCntDown锛?* | 4B int | NS 涓嬭甯ц鏁板櫒鍒濆鍊硷紝閫氬父 `0` |

> **ABP 娉ㄦ剰**锛欰BP Session Key 鏄潤鎬佺殑锛屾案涓嶆洿鏂帮紝瀹夊叏鎬т綆浜?OTAA銆?> 涓?FCntUp 涓€鏃﹁秴杩囦笂闄愶紙2^32锛夋垨璁惧閲嶇儳锛團CntUp 褰掗浂锛夛紝
> 鑻?NS 鏈叧闂?FCnt 妫€鏌ワ紝鎵€鏈夊寘浼氳涓㈠純銆?> **璋冭瘯 ABP 鏃跺姟蹇呭嬀閫?Skip frame-counter checks**锛?> 骞跺湪 Device Profile 涓叧闂?FCnt rollover 妫€鏌ャ€?
---

### 3.5 楠岃瘉璁惧娉ㄥ唽鎴愬姛

娉ㄥ唽骞跺～鍐欏瘑閽ュ悗锛岃繍琛岃妭鐐硅剼鏈Е鍙?OTAA 鍏ョ綉锛?
```bash
python3 scripts/e77_node_ctrl.py otaa \
    --port /dev/ttyUSB0 --region 2 \
    --deveui <DEVEUI> --appkey <APPKEY> \
    --chanmask 0000:0000:0000:0000:0000:00FF
```

**鎴愬姛鏍囧織锛堟寜椤哄簭锛夛細**

| 搴忓彿 | 瑙傚療浣嶇疆 | 棰勬湡鐜拌薄 |
|------|---------|---------|
| 1 | 鑺傜偣涓插彛 | `+EVT:JOINED` |
| 2 | ChirpStack GUI 鈫?Device 鈫?**Events** | 鍑虹幇 `join` 浜嬩欢 |
| 3 | GUI 鈫?Device 鈫?**Activation** | 鏄剧ず褰撳墠 DevAddr / NwkSKey / AppSKey |
| 4 | 鍙戦€佷笂琛屽悗 鈫?**LoRaWAN frames** | 鍑虹幇 `UnconfirmedDataUp` 甯э紝payload 宸茶В瀵?|
| 5 | **Events** 鏍囩 | 鍑虹幇 `up` 浜嬩欢锛宒ata 瀛楁鏄剧ず base64 payload |

---

## 鍥涖€丱TAA 鍏ョ綉娴佺▼锛圝oin锛?
OTAA锛圤ver-The-Air Activation锛夋槸鎺ㄨ崘鐨勫叆缃戞柟寮忥細

```
鑺傜偣                            NS (ChirpStack)
 鈹? 鈹傗攢鈹€ JoinRequest锛堝惈 AppEUI/DevEUI/DevNonce锛夆攢鈹€鈫? 鈹?                                             鈹?鏌?AppKey锛岄獙璇?MIC
 鈹?                                             鈹?鐢熸垚 NwkSKey, AppSKey
 鈹?                                             鈹?鍒嗛厤 DevAddr
 鈹傗啇鈹€鈹€ JoinAccept锛堝惈 AppNonce/NetID/DevAddr锛夆攢鈹€鈹? 鈹? 鈹?鑺傜偣鐢?AppKey 瑙ｅ瘑 JoinAccept锛? 鈹?鎺ㄥ鍑?NwkSKey / AppSKey锛? 鈹?姝ゅ悗涓婅鏁版嵁鐢ㄨ繖涓や釜 Session Key 鍔犲瘑
```

**鍏抽敭鍙傛暟锛?*

| 鍙傛暟 | 澶у皬 | 瀛樺偍浣嶇疆 |
|------|------|---------|
| DevEUI | 8B | 鑺傜偣纭欢锛堥€氬父鐑у啓锛墊
| AppKey | 16B | 鑺傜偣鍥轰欢 + ChirpStack |
| DevAddr | 4B | NS 鍔ㄦ€佸垎閰?|
| NwkSKey | 16B | 鎺ㄥ鑷?AppKey锛堟瘡娆?Join 鏇存柊锛墊
| AppSKey | 16B | 鎺ㄥ鑷?AppKey锛堟瘡娆?Join 鏇存柊锛墊

---

## 浜斻€佹棩蹇楁煡鐪嬩笌璋冭瘯

### 5.1 鏌ョ湅瀹炴椂鏃ュ織

```bash
cd ~/chirpstack-docker

# 鏌ョ湅 NS 鏃ュ織锛堟渶甯哥敤锛?docker compose logs -f chirpstack

# 鏌ョ湅 gateway-bridge 鏃ュ織
docker compose logs -f chirpstack-gateway-bridge

# 鏌ョ湅鎵€鏈夊鍣?docker compose logs -f

# 鏌ョ湅鏈€杩?N 鍒嗛挓
docker compose logs --since=5m chirpstack
```

### 5.2 鍏抽敭鏃ュ織鍏抽敭瀛?
| 鍏抽敭瀛?| 鍚箟 |
|--------|------|
| `Gateway partially updated` | NS 鏀跺埌 stats锛岀綉鍏充笂绾?鉁?|
| `Uplink received` | 鏀跺埌涓婅鍖?鉁?|
| `region_id="cn470_10"` | 娑堟伅灞炰簬 cn470_10 region 鉁?|
| `MACPayload requires at least 7 bytes` | 涓婅鍖呬笉鏄悎娉?LoRaWAN 甯э紙瑁?LoRa 鍖咃級鈿狅笍 |
| `MIC error` | MIC 鏍￠獙澶辫触锛孉ppKey/NwkSKey 涓嶅 鉂?|
| `DevAddr not found` | 璁惧鏈敞鍐屾垨 DevAddr 閿欒 鉂?|
| `frame-counter did not increment` | FCnt 璁℃暟鍣ㄦ湭閫掑锛堥噸鏀炬敾鍑婚槻鎶よЕ鍙戯級鉂?|

### 5.3 MQTT 瀹炴椂鐩戝惉锛堣皟璇?topic锛?
```bash
# 瀹夎 mosquitto 瀹㈡埛绔?sudo apt install mosquitto-clients

# 璁㈤槄鎵€鏈?cn470_10 缃戝叧娑堟伅锛堜粠瀹夸富鏈鸿闂?Docker 鐨?mosquitto锛?mosquitto_sub -h localhost -p 1883 -t "cn470_10/#" -v

# 鍙湅涓婅鏁版嵁
mosquitto_sub -h localhost -p 1883 -t "cn470_10/gateway/+/event/up" -v
```

> Gateway Bridge 鍙戝竷鐨勬秷鎭槸 **Protobuf 搴忓垪鍖?*鐨勶紝
> 鐩存帴 `mosquitto_sub` 鐪嬪埌鐨勬槸浜岃繘鍒讹紝闇€瑕?protobuf 宸ュ叿瑙ｇ爜锛?> 鎴栬€呴€氳繃 ChirpStack Web GUI 鐨勮澶?LoRaWAN frames 椤甸潰鏌ョ湅瑙ｇ爜鍚庣殑鍐呭銆?
### 5.4 Web GUI 鏌ョ湅涓婅甯?
Web UI 鈫?**Applications** 鈫?閫夋嫨搴旂敤 鈫?**Devices** 鈫?閫夋嫨璁惧 鈫?**LoRaWAN frames** 鏍囩

杩欓噷浼氭樉绀猴細
- 甯ф椂闂?- 鎺ユ敹缃戝叧锛堝缃戝叧鏃舵樉绀烘墍鏈夌綉鍏?EUI 鍜?RSSI/SNR锛?- 涓婅甯х被鍨嬶紙JoinRequest / UnconfirmedDataUp 绛夛級
- 瑙ｅ瘑鍚庣殑 payload锛堝鏋?AppSKey 姝ｇ‘锛?
---

## 鍏€佸父瑙佹搷浣滃懡浠?
### 6.1 瀹瑰櫒绠＄悊

```bash
cd ~/chirpstack-docker

# 鍚姩鎵€鏈夋湇鍔?docker compose up -d

# 鍋滄鎵€鏈夋湇鍔?docker compose down

# 閲嶅惎鍗曚釜鏈嶅姟锛堜笉褰卞搷鍏朵粬瀹瑰櫒锛?docker compose up -d --no-deps chirpstack-gateway-bridge

# 閲嶆柊鍔犺浇閰嶇疆锛堜慨鏀?TOML 鍚庨渶閲嶅惎 chirpstack 瀹瑰櫒锛?docker compose restart chirpstack

# 鏌ョ湅瀹瑰櫒鐘舵€?docker compose ps
```

### 6.2 REST API 甯哥敤璇锋眰

ChirpStack 鎻愪緵瀹屾暣 REST API锛堥€氳繃 chirpstack-rest-api 浠ｇ悊锛岀鍙?8090锛夛細

```bash
BASE="http://localhost:8090"
# 鍏堣幏鍙?API Key锛圵eb UI 鈫?API Keys 鈫?Add API key锛屾垨浣跨敤 admin 瀵嗙爜鐩存帴鐧诲綍锛?TOKEN="Bearer <your-api-key>"

# 鍒楀嚭缃戝叧
curl -H "Authorization: $TOKEN" $BASE/api/gateways

# 鏌ョ湅缃戝叧璇︽儏锛堟浛鎹?EUI锛?curl -H "Authorization: $TOKEN" "$BASE/api/gateways/aa555a00000021fb"

# 鍒楀嚭 Device Profile
curl -H "Authorization: $TOKEN" $BASE/api/device-profiles
```

---

## 涓冦€佸 Region 鍚屾椂杩愯

ChirpStack v4 鏀寔鍦ㄥ悓涓€瀹炰緥涓繍琛屽涓?Region锛屾瘡涓?Region 鐙珛澶勭悊锛?
**chirpstack.toml锛?*
```toml
[network]
  enabled_regions=["cn470_10", "eu868", "us915"]
```

**docker-compose.yml 闇€瑕佸涓?gateway-bridge 瀹炰緥锛?*

```yaml
  # CN470_10 缃戝叧
  chirpstack-gateway-bridge-cn470:
    environment:
      - INTEGRATION__MQTT__EVENT_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/event/{{ .EventType }}
    ports:
      - "1700:1700/udp"

  # EU868 缃戝叧
  chirpstack-gateway-bridge-eu868:
    environment:
      - INTEGRATION__MQTT__EVENT_TOPIC_TEMPLATE=eu868/gateway/{{ .GatewayID }}/event/{{ .EventType }}
    ports:
      - "1701:1700/udp"   # 娉ㄦ剰绔彛涓嶈兘鍐茬獊
```

---

## 鍏€佹敞鎰忎簨椤规眹鎬?
| 娉ㄦ剰鐐?| 璇存槑 |
|--------|------|
| topic_prefix 澶у皬鍐欐晱鎰?| `CN470_10` 鈮?`cn470_10`锛屽繀椤诲皬鍐?|
| YAML 涓嶈兘鎹㈣ | 闀垮瓧绗︿覆妯℃澘涓嶈兘璺ㄨ锛屽惁鍒欒鎴柇 |
| GW EUI 澶у皬鍐?| gateway-bridge 鍙戝竷鏃惰浆鎴愬皬鍐欙紝NS 瀛樺偍鏃朵笉鍖哄垎澶у皬鍐?|
| FCnt 妫€鏌?| 璋冭瘯鏈熼棿鍙叧闂紝鍚﹀垯閲嶇儳鑺傜偣鍚?FCnt 浠庡ご寮€濮嬩細瀵艰嚧鍖呰涓?|
| LoRaWAN 1.0 vs 1.1 | Device Profile 鐨?MAC version 蹇呴』涓庤妭鐐瑰浐浠朵竴鑷?|
| ADR 鍜屼俊閬撴帺鐮?| NS 浼氶€氳繃 ADRReq 鍜?LinkADRReq 涓嬭鍛戒护璋冩暣鑺傜偣淇￠亾锛屽垵娆℃帴鍏ュ彲鑳介渶瑕佸嚑娆′笂琛屾墠绋冲畾 |

---

## 鍙傝€冭祫鏂?
- ChirpStack 瀹樻柟鏂囨。锛歨ttps://www.chirpstack.io/docs/
- ChirpStack GitHub锛歨ttps://github.com/chirpstack/chirpstack
- Docker 閮ㄧ讲绀轰緥锛歨ttps://github.com/chirpstack/chirpstack-docker
