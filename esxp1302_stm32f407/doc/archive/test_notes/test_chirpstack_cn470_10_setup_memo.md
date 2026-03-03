# ChirpStack 瀵规帴涓?CN470_10 棰戠巼璁″垝閰嶇疆澶囧繕褰?
**鏃ユ湡锛?* 2026-02-22  
**娴嬭瘯鐩殑锛?* 鎺掓煡缃戝叧閫氳繃 ChirpStack Gateway Bridge 鎺ュ叆 ChirpStack NS 鍚庯紝
GUI 濮嬬粓鏄剧ず缃戝叧"绂荤嚎"鐨勯棶棰橈紝骞跺畬鎴?CN470_10 棰戠巼璁″垝鐨勫畬鏁撮厤缃€?
---

## 涓€銆佺郴缁熸灦鏋?
```
LoRa 鑺傜偣锛堟櫘閫?LoRa / LoRaWAN 鑺傜偣锛?  鈹? 涓婅锛?86.3 ~ 487.7 MHz锛圕N470_10 ch80~ch87锛?  鈻?ESP32-S3 + SX1302锛圗SXP1302 缃戝叧锛?  鈹? Semtech UDP 鍗忚锛岀鍙?1700
  鈻?chirpstack-gateway-bridge锛圖ocker 瀹瑰櫒锛?  鈹? 鍗忚杞崲锛歎DP 鈫?MQTT
  鈹? 鍙戝竷 topic: cn470_10/gateway/<GW_ID>/event/up|stats
  鈻?mosquitto锛圡QTT Broker锛岀鍙?1883锛?  鈻?chirpstack锛圢etwork Server锛岀鍙?8080锛?  鈹? 璁㈤槄 topic: cn470_10/gateway/...
  鈹? 鎸?region_cn470_10 瑙勫垯瑙ｆ瀽涓婅鍖?  鈻?chirpstack-rest-api锛堢鍙?8090锛? Web GUI锛堢鍙?8080锛?```

### 鏈嶅姟閮ㄧ讲

鎵€鏈夋湇鍔￠€氳繃 Docker Compose 閮ㄧ讲锛岃矾寰勶細`~/chirpstack-docker/docker-compose.yml`

---

## 浜屻€侀亣鍒扮殑闂

### 闂 1锛氱綉鍏?GUI 濮嬬粓鏄剧ず绂荤嚎

**鐜拌薄锛?*
- 缃戝叧鏃ュ織鏄剧ず `PUSH_DATA acknowledged: 100%`锛孶DP 閫氫俊瀹屽叏姝ｅ父
- ChirpStack GUI 涓綉鍏充竴鐩存樉绀?"Never seen"锛堜粠鏈笂绾匡級
- `chirpstack-gateway-bridge` 鏃ュ織鏄剧ず鍙戝竷 topic 涓?`cn470/gateway/.../event/stats`

**鏍瑰洜锛歁QTT topic 鍓嶇紑涓嶅尮閰?*

| 缁勪欢 | topic 鍓嶇紑锛堜慨澶嶅墠锛墊
|------|-------------------|
| gateway-bridge 鍙戝竷 | `cn470/...` |
| ChirpStack NS 璁㈤槄 | `cn470_10/...` |

涓よ€?topic 涓嶅湪鍚屼竴棰戦亾锛孨S 鏀朵笉鍒?stats 娑堟伅锛岀綉鍏虫案杩滄樉绀虹绾裤€?
**闄勫姞闂锛?* `docker-compose.yml` 涓?`EVENT_TOPIC_TEMPLATE` 閰嶇疆椤瑰瓨鍦ㄩ潪娉曟崲琛岀锛?瀵艰嚧妯℃澘瀛楃涓茶鎴柇锛岃繘涓€姝ュ鑷?topic 瑙ｆ瀽寮傚父銆?
**淇鏂规硶锛?*

缂栬緫 `~/chirpstack-docker/docker-compose.yml`锛屽皢 `chirpstack-gateway-bridge` 鏈嶅姟鐨?鐜鍙橀噺涓?topic 鍓嶇紑浠?`cn470` 鏀逛负 `cn470_10`锛屽悓鏃朵慨澶嶆崲琛岀锛?
```yaml
  chirpstack-gateway-bridge:
    environment:
      - INTEGRATION__MQTT__EVENT_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/event/{{ .EventType }}
      - INTEGRATION__MQTT__STATE_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/state/{{ .StateType }}
      - INTEGRATION__MQTT__COMMAND_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/command/#
```

淇鍚庨噸鍚鍣細
```bash
cd ~/chirpstack-docker
docker compose up -d --no-deps chirpstack-gateway-bridge
```

**楠岃瘉锛?* 鏌ョ湅 gateway-bridge 鏃ュ織锛岀‘璁?topic 宸叉洿鏂帮細
```
topic=cn470_10/gateway/aa555a00000021fb/event/stats
topic=cn470_10/gateway/aa555a00000021fb/state/conn
```

---

### 闂 2锛氱綉鍏冲彲鏄剧ず鍦ㄧ嚎锛屼絾鑺傜偣涓婅鏁版嵁琚?NS 涓㈠純

**鐜拌薄锛?*
```
ERROR chirpstack::uplink: Deduplication error error=MACPayload requires at least 7 bytes
```

**鍘熷洜 A锛堝凡瑙ｅ喅锛夛細棰戠巼涓嶅尮閰?*

淇鍓嶇綉鍏?radio 棰戠巼锛圢VS 涓級涓?480.4/481.2 MHz锛堢害涓?CN470_6 鑼冨洿锛夛紝
鑰?NS 閰嶇疆鐨?`region_cn470_10` 鏈熸湜淇￠亾涓?486.3~487.7 MHz锛屼袱鑰呯浉宸害 6 MHz锛?涓婅鍖呴鐜囦笉鍦?NS 鐨勪俊閬撳垪琛ㄥ唴锛岃鐩存帴涓㈠純銆?
淇鏂规硶锛氶€氳繃缃戦〉閰嶇疆鐣岄潰锛圫oft AP 妯″紡锛夋洿鏂?NVS 涓殑 radio 棰戠巼锛堣瑙佺涓夎妭锛夈€?
**鍘熷洜 B锛堟祴璇曢樁娈甸鏈熺幇璞★級锛氳８ LoRa 鍖呮病鏈?LoRaWAN MAC 甯уご**

娴嬭瘯鑺傜偣鍙戦€佺殑鏄８ LoRa 鏁版嵁锛坧ayload = "hello world"锛夛紝
涓嶅寘鍚?LoRaWAN 瑙勫畾鐨?MHDR + DevAddr + FCtrl + FCnt + MIC 绛夊瓧娈碉紙鏈€灏?12 瀛楄妭锛夛紝
NS 鍦ㄨВ鏋?MAC 灞傛椂澶辫触鎶ラ敊銆?
杩欐槸**棰勬湡鐜拌薄**锛岀瓑姝ｅ紡 LoRaWAN 鑺傜偣涓婄嚎鍚庤嚜鍔ㄨВ鍐筹紝鏃犻渶澶勭悊銆?
---

## 涓夈€丆N470_10 瀹屾暣棰戠巼閰嶇疆鏂规

### 3.1 CN470 棰戞璁″垝璇存槑

CN470 鏄?470~510 MHz 棰戞鐨勭粺绉般€侺oRa Alliance 灏嗗叾缁嗗垎涓?12 涓瓙淇￠亾璁″垝
锛坄cn470_0` ~ `cn470_11`锛夛紝姣忎釜璁″垝瑕嗙洊 8 涓笂琛屼俊閬擄紙200 kHz 闂磋窛锛夈€?
ChirpStack 涓瘡涓瓙璁″垝瀵瑰簲涓€涓嫭绔嬬殑 region 閰嶇疆鏂囦欢鍜?MQTT topic 鍓嶇紑锛?**涓夎€呭繀椤讳繚鎸佷竴鑷达細缃戝叧纭欢棰戠巼 = Gateway Bridge topic 鍓嶇紑 = NS region**銆?
鏈」鐩娇鐢?**CN470_10**锛圠oRa Alliance ch80~ch87锛夈€?
---

### 3.2 缃戝叧纭欢閰嶇疆锛圢VS 鎴?global_conf.json锛?
SX1302 鍐呴儴鏈?2 涓皠棰戝墠绔紙radio_0 / radio_1锛夛紝姣忎釜瑕嗙洊 卤400 kHz锛?鍚堣 8 涓笂琛屼俊閬擄紝闂磋窛 200 kHz銆?
**radio 涓績棰戠巼鍏紡锛?*

$$f_{radio\_1} = f_{radio\_0} + 800\,\text{kHz}$$

**CN470_10 鐨?radio 閰嶇疆锛?*

| 鍙傛暟 | 鍊?|
|------|----|
| `radio_0` 涓績棰戠巼 | `486600000` Hz锛?86.6 MHz锛墊
| `radio_1` 涓績棰戠巼 | `487400000` Hz锛?87.4 MHz锛墊

**8 涓笂琛屾帴鏀朵俊閬擄紙IF 鍋忕Щ鍥哄畾锛夛細**

| 淇￠亾鍚?| LoRa ch | Radio | IF 鍋忕Щ | 瀹為檯鎺ユ敹棰戠巼 |
|--------|---------|-------|---------|------------|
| multiSF_0 | ch80 | radio_0 | -300 kHz | **486.3 MHz** |
| multiSF_1 | ch81 | radio_0 | -100 kHz | **486.5 MHz** |
| multiSF_2 | ch82 | radio_0 | +100 kHz | **486.7 MHz** |
| multiSF_3 | ch83 | radio_0 | +300 kHz | **486.9 MHz** |
| multiSF_4 | ch84 | radio_1 | -300 kHz | **487.1 MHz** |
| multiSF_5 | ch85 | radio_1 | -100 kHz | **487.3 MHz** |
| multiSF_6 | ch86 | radio_1 | +100 kHz | **487.5 MHz** |
| multiSF_7 | ch87 | radio_1 | +300 kHz | **487.7 MHz** |

> 杩?8 涓鐜囧嵆涓鸿妭鐐逛笂琛屽彂灏勯鐜囷紝鑺傜偣鍙渶閰嶇疆杩?8 涓紝涓嬭鐢?NS 鑷姩璁＄畻銆?
**閫氳繃缃戦〉閰嶇疆鐣岄潰淇敼 radio 棰戠巼锛堟帹鑽愭柟寮忥級锛?*

1. 涓婄數鏃舵寜浣?**IO0锛堝乏閿級** 杩涘叆 Soft AP 妯″紡
2. 鎵嬫満/鐢佃剳杩炴帴 WiFi锛歋SID=`esp32`锛屽瘑鐮?`esp32wifi`
3. 娴忚鍣ㄨ闂?`http://192.168.4.1`锛岀櫥褰曪紙鐢ㄦ埛鍚?`iot`锛屽瘑鐮?`lora`锛?4. 濉啓锛?   - **radio0 棰戠巼**锛歚486600000`
   - **radio1 棰戠巼**锛歚487400000`
5. 鐐瑰嚮 **Apply** 鈫?**Reboot**
6. Soft AP 妯″紡鏈?**10 鍒嗛挓瓒呮椂**锛屽姟蹇呭湪瓒呮椂鍓嶅畬鎴愭搷浣?
---

### 3.3 Gateway Bridge 閰嶇疆

鏂囦欢璺緞锛歚~/chirpstack-docker/docker-compose.yml`

```yaml
chirpstack-gateway-bridge:
  environment:
    - INTEGRATION__MQTT__EVENT_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/event/{{ .EventType }}
    - INTEGRATION__MQTT__STATE_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/state/{{ .StateType }}
    - INTEGRATION__MQTT__COMMAND_TOPIC_TEMPLATE=cn470_10/gateway/{{ .GatewayID }}/command/#
```

> **鍏抽敭锛歵opic 鍓嶇紑蹇呴』涓?NS 鐨?region id 瀹屽叏涓€鑷?*锛屽寘鎷笅鍒掔嚎鍜屾暟瀛椼€?> `cn470` 鈮?`cn470_10`锛屽樊涓€涓瓧绗﹀氨鍏ㄩ儴澶辨晥銆?
---

### 3.4 ChirpStack NS 閰嶇疆

**涓婚厤缃枃浠讹細** `~/chirpstack-docker/configuration/chirpstack/chirpstack.toml`

纭 `cn470_10` 鍦?`enabled_regions` 鍒楄〃涓細
```toml
enabled_regions=[
  "cn470_10",
  ...
]
```

**鍖哄煙閰嶇疆鏂囦欢锛?* `~/chirpstack-docker/configuration/chirpstack/region_cn470_10.toml`

鍏抽敭鍙傛暟锛?
| 鍙傛暟 | 鍊?| 璇存槑 |
|------|----|------|
| `topic_prefix` | `cn470_10` | 蹇呴』涓?gateway-bridge 涓€鑷?|
| `enabled_uplink_channels` | `[80,81,82,83,84,85,86,87]` | ch80~ch87 |
| `rx2_frequency` | `505300000` Hz | RX2 绐楀彛鍥哄畾棰戠巼 |
| `rx1_delay` | 1 绉?| 涓婅鍚?RX1 绐楀彛鏃跺欢 |

---

### 3.5 涓婁笅琛岄鐜囧叧绯?
CN470 閲囩敤**棰戝垎鍙屽伐锛團DD锛?*璁捐锛屼笂涓嬭鏁呮剰鍒嗗紑鍒颁笉鍚岄娈碉紝闃叉缃戝叧鑷彂鑷敹骞叉壈銆?
| 鏂瑰悜 | 棰戠巼鑼冨洿 |
|------|---------|
| 涓婅锛堣妭鐐光啋缃戝叧锛墊 470~490 MHz |
| 涓嬭锛堢綉鍏斥啋鑺傜偣锛墊 500~510 MHz |

**CN470_10 鐨?RX1 涓嬭棰戠巼鏄犲皠锛圢S 鑷姩璁＄畻锛屾棤闇€鎵嬪姩閰嶇疆锛夛細**

| 涓婅淇￠亾 | 涓婅棰戠巼 | 瀵瑰簲 RX1 涓嬭棰戠巼 |
|---------|---------|-----------------|
| ch80 | 486.3 MHz | 506.7 MHz |
| ch81 | 486.5 MHz | 506.9 MHz |
| ch82 | 486.7 MHz | 507.1 MHz |
| ch83 | 486.9 MHz | 507.3 MHz |
| ch84 | 487.1 MHz | 507.5 MHz |
| ch85 | 487.3 MHz | 507.7 MHz |
| ch86 | 487.5 MHz | 507.9 MHz |
| ch87 | 487.7 MHz | 508.1 MHz |

RX1 鏄犲皠鍏紡锛?
$$f_{RX1} = 500.3 + (ch \bmod 48) \times 0.2 \text{ MHz}$$

渚嬶細ch82 mod 48 = 34锛?500.3 + 34 \times 0.2 = 507.1$ MHz 鉁?
**RX2 鍥哄畾棰戠巼锛?05.3 MHz锛孲F12BW125**锛堜笂琛屽悗 2 绉掞級

---

### 3.6 鍒囨崲鍏朵粬棰戠巼璁″垝鐨勬楠わ紙鎵嬪姩鎿嶄綔鎸囧崡锛?
濡傞渶鍒囨崲鍒板叾浠?CN470 瀛愯鍒掞紙濡?cn470_0锛夛紝鎸変互涓嬫楠ゆ搷浣滐細

**绗竴姝ワ細纭畾鐩爣璁″垝鐨?radio 棰戠巼**

涓嶅悓璁″垝鐨?radio_0 璧峰棰戠巼锛?
| 璁″垝 | radio_0 | radio_1 | 涓婅淇￠亾 |
|------|---------|---------|---------|
| cn470_0 | 470600000 | 471400000 | 470.3~471.7 MHz |
| cn470_1 | 472600000 | 473400000 | 472.3~473.7 MHz |
| cn470_2 | 474600000 | 475400000 | 474.3~475.7 MHz |
| cn470_3 | 476600000 | 477400000 | 476.3~477.7 MHz |
| cn470_4 | 478600000 | 479400000 | 478.3~479.7 MHz |
| cn470_5 | 480600000 | 481400000 | 480.3~481.7 MHz |
| cn470_6 | 482600000 | 483400000 | 482.3~483.7 MHz |
| cn470_7 | 484600000 | 485400000 | 484.3~485.7 MHz |
| cn470_8 | 484600000 | 485400000 | 485.3~486.7 MHz |锛堜笌7鏈夐噸鍙狅紝璇锋煡瀹樻柟鏂囨。纭锛?| cn470_9 | 486600000 | 487400000 | 486.3~487.7 MHz |锛堟敞锛氫笌cn470_10鐩稿悓涓婅锛屼笅琛屼笉鍚岋級
| **cn470_10** | **486600000** | **487400000** | **486.3~487.7 MHz** |
| cn470_11 | 488600000 | 489400000 | 488.3~489.7 MHz |

> 绮剧‘棰戠巼浠?`~/chirpstack-docker/configuration/chirpstack/region_cn470_XX.toml` 涓殑 `frequency` 瀛楁涓哄噯銆?
**绗簩姝ワ細鏇存柊缃戝叧 radio 棰戠巼**锛堣 3.2 鑺傜綉椤甸厤缃楠わ級

**绗笁姝ワ細鏇存柊 gateway-bridge topic 鍓嶇紑**

```bash
# 缂栬緫 docker-compose.yml锛屽皢鎵€鏈?cn470_10 鏀逛负鐩爣璁″垝鍚?vi ~/chirpstack-docker/docker-compose.yml

# 閲嶅惎 gateway-bridge
cd ~/chirpstack-docker
docker compose up -d --no-deps chirpstack-gateway-bridge
```

**绗洓姝ワ細纭 NS 宸插惎鐢ㄧ洰鏍?region**

```bash
grep "enabled_regions" ~/chirpstack-docker/configuration/chirpstack/chirpstack.toml
```

濡傜洰鏍?region 涓嶅湪鍒楄〃涓紝娣诲姞鍚庨噸鍚?chirpstack锛?```bash
docker compose restart chirpstack
```

**绗簲姝ワ細楠岃瘉**

```bash
# 鏌ョ湅 gateway-bridge 鏃ュ織锛岀‘璁?topic 鍓嶇紑姝ｇ‘
docker compose logs --tail=20 chirpstack-gateway-bridge | grep topic

# 鏌ョ湅 NS 鏃ュ織锛岀‘璁ゆ敹鍒版潵鑷纭?region 鐨勬秷鎭?docker compose logs --tail=20 chirpstack | grep region_id
```

---

## 鍥涖€佹祴璇曠幇璞¤褰?
### 4.1 淇鍓嶏紙topic 鍓嶇紑 cn470锛?
| 鐜拌薄 | 璇存槑 |
|------|------|
| 缃戝叧 UDP ackr = 100% | Gateway Bridge 鏀跺埌 UDP 鍖呭苟 ACK锛屾甯?|
| NS GUI 鏄剧ず"Never seen" | NS 浠庢湭鏀跺埌鏉ヨ嚜缃戝叧鐨?stats锛宼opic 涓嶅尮閰?|
| chirpstack 鏃ュ織鏃犱换浣?gateway 鐩稿叧杈撳嚭 | NS 鏍规湰娌℃湁璁㈤槄 cn470/... topic |

### 4.2 淇鍚庯紙topic 鍓嶇紑 cn470_10锛宺adio 棰戠巼 486.6/487.4 MHz锛?
| 鐜拌薄 | 璇存槑 |
|------|------|
| gateway-bridge 鏃ュ織 | `topic=cn470_10/gateway/.../event/stats` 鉁?|
| NS 鏃ュ織 | `region_id="cn470_10"` 鏀跺埌娑堟伅 鉁?|
| NS Gateway 鐘舵€?| `Gateway partially updated` 鉁?缃戝叧鍦ㄧ嚎 |
| 鑺傜偣涓婅 | `freq=486.700000, chan=2, CRC_OK=100%` 鉁?|
| NS 涓婅澶勭悊 | `MACPayload requires at least 7 bytes` 鈿狅笍 棰勬湡鍐咃紙瑁?LoRa 娴嬭瘯鍖咃級|

### 4.3 鑺傜偣淇″彿璐ㄩ噺锛堟祴璇曠幆澧冿級

```json
{
  "freq": 486.700000,
  "chan": 2,
  "datr": "SF12BW125",
  "rssi": -73,
  "lsnr": 4.0,
  "foff": 392
}
```

| 鍙傛暟 | 鍊?| 璇勪环 |
|------|----|------|
| RSSI | -73 dBm | 鑹ソ锛堝鍐呰繎璺濈锛墊
| SNR | 4.0 dB | 姝ｅ父锛圫F12 鍙敤鑼冨洿 -20~+10 dB锛墊
| foff锛堥鐜囧亸宸級| ~400 Hz | 姝ｅ父锛屾櫠鎸宸寖鍥村唴 |
| CRC | 100% OK | 淇″彿璐ㄩ噺浼樼 |

---

## 浜斻€佷笅涓€姝?
- [ ] 閮ㄧ讲 LoRaWAN 鍗忚鏍堝埌鑺傜偣锛圤TAA 鎴?ABP锛?- [ ] 鍦?ChirpStack 鍒涘缓 Application + Device Profile + 娉ㄥ唽 Device锛堝～鍐?DevEUI/AppKey锛?- [ ] 楠岃瘉 OTAA Join 娴佺▼锛圝oin Request 鈫?Join Accept锛?- [ ] 楠岃瘉涓婅鏁版嵁甯у湪 ChirpStack GUI 涓甯告樉绀?- [ ] 楠岃瘉涓嬭 ACK锛圧X1 507.1 MHz 鎴?RX2 505.3 MHz锛?
---

*璁板綍浜猴細cuckooshan锛?026-02-22*
