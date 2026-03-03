# Learning 瀛︿範绗旇鐩綍

鏈洰褰曟敹褰曢」鐩紑鍙戣繃绋嬩腑娌夋穩鐨勪笓棰樺涔犵瑪璁帮紝姣忕瘒瀵瑰簲涓€涓叿浣撶煡璇嗙偣鎴栬皟璇曚簨浠讹紝
渚ч噸"涓轰粈涔?鍜?鏄粈涔?锛屼笌 `test_notes/`锛堟祴璇曡繃绋嬭褰曪級鍜?`bugfix/`锛堥棶棰樹慨澶嶈褰曪級浜掍负琛ュ厖銆?
---

## 鏂囨。鍒楄〃

### LoRaWAN 鍗忚涓庨鐜囪鍒?
| 鏂囦欢 | 鍐呭鎽樿 | 鏉ユ簮浜嬩欢 |
|------|---------|---------|
| [lora_cn470_frequency_plan.md](lora_cn470_frequency_plan.md) | CN470 棰戠巼浣撶郴锛?2 涓瓙璁″垝銆乺adio 涓績棰戠巼璁＄畻銆丷X1/RX2 鍏紡銆丗DD 涓婁笅琛屽垎绂绘満鍒躲€乧hanmask 缁撴瀯 | 2026-02-22 ChirpStack 鎺ュ叆璋冭瘯 |
| [lorawan_gateway_ns_protocol.md](lorawan_gateway_ns_protocol.md) | Semtech UDP 鍖呰浆鍙戝崗璁紙PUSH_DATA / PULL_DATA / PULL_RESP锛夈€丟ateway Bridge MQTT topic 缁撴瀯銆佷笂涓嬭鏁版嵁娴佸叏閾捐矾 | 2026-02-22 缃戝叧 NS 瀵规帴璋冭瘯 |

### ChirpStack

| 鏂囦欢 | 鍐呭鎽樿 | 鏉ユ簮浜嬩欢 |
|------|---------|---------|
| [chirpstack_v4_usage.md](chirpstack_v4_usage.md) | ChirpStack v4 閰嶇疆浣撶郴锛圧egion / Gateway / Device Profile / Application / Device锛夈€丱TAA/ABP 娉ㄥ唽姝ラ銆丮QTT topic 涓?region_id 鐨勫搴斿叧绯汇€佸父瑙佹晠闅滄帓鏌?| 2026-02-22 ChirpStack 鎺ュ叆璋冭瘯 |

### LoRaWAN 鑺傜偣璋冭瘯

| 鏂囦欢 | 鍐呭鎽樿 | 鏉ユ簮浜嬩欢 |
|------|---------|---------|
| [lorawan_node_e77_otaa_debug.md](lorawan_node_e77_otaa_debug.md) | E77-400M22S AT 鎸囦护鎺ュ彛瑕佺偣銆丱TAA JOIN FAILED 鐨勫垎灞傝瘖鏂硶锛圧F灞傗啋NS灞傗啋RX1灞傦級銆丆N470 chanmask 鍙岄噸浣滅敤锛堜笂琛岄鐜?+ RX1 璁＄畻鍩哄噯锛夈€乸yserial DTR/RTS 澶嶄綅闄烽槺 | 2026-02-27 E77 鑺傜偣鍏ㄩ摼璺獙璇?|

### 鍥轰欢浠ｇ爜

| 鏂囦欢 | 鍐呭鎽樿 | 鏉ユ簮浜嬩欢 |
|------|---------|---------|
| [esxp1302_code_walkthrough.md](esxp1302_code_walkthrough.md) | ESXP1302 宸ョ▼瀹屾暣浠ｇ爜瑙ｆ瀽锛氫换鍔℃灦鏋勩€丼X1302 HAL 鍒濆鍖栨祦绋嬨€乸acket forwarder 鏀跺彂閫昏緫銆丯VS 閰嶇疆璇诲啓 | 2026-02-22 浠ｇ爜璧拌 |

### 椹卞姩绉绘

| 鏂囦欢 | 鍐呭鎽樿 | 鏉ユ簮浜嬩欢 |
|------|---------|---------|
| [lm75a_i2c_driver_porting.md](lm75a_i2c_driver_porting.md) | LM75A 娓╁害浼犳劅鍣?I2C 椹卞姩绉绘锛欵SP-IDF I2C API銆?1-bit 鏈夌鍙锋俯搴︽暟鎹В鏋愩€両2C 鎬荤嚎鍗℃鎭㈠鏈哄埗銆丷SSI 娓╁害琛ュ伩鎺ュ叆 | 2026-02-27 LM75A 椹卞姩绉绘 |

### Bug 娣卞害澶嶇洏

| 鏂囦欢 | 鍐呭鎽樿 | 鏉ユ簮浜嬩欢 |
|------|---------|---------|
| [BUG-010_notes.md](BUG-010_notes.md) | PUSH_DATA ackr 浣庯紙22%锛夋牴鍥犲叏鍒嗘瀽锛歁QTT TLS 闃诲銆乣tv_usec` 婧㈠嚭锛?,000,000 渭s 闈炴硶鍊硷級銆乨rain setsockopt 绔炴€併€乄iFi Modem Sleep AP 缂撳啿寤惰繜 | 2026-02-21 BUG-010 |

---

## 鐭ヨ瘑绱㈠紩

鎸夊叧閿瘝蹇€熷畾浣嶏細

| 鍏抽敭璇?| 鐩稿叧鏂囨。 |
|--------|---------|
| CN470 瀛愰娈?/ chanmask / RX1 鍏紡 | [lora_cn470_frequency_plan.md](lora_cn470_frequency_plan.md)銆乕lorawan_node_e77_otaa_debug.md](lorawan_node_e77_otaa_debug.md) |
| OTAA JOIN FAILED 璇婃柇 | [lorawan_node_e77_otaa_debug.md](lorawan_node_e77_otaa_debug.md) |
| ChirpStack MQTT topic 涓嶅尮閰?/ region_id | [chirpstack_v4_usage.md](chirpstack_v4_usage.md)銆乕lorawan_gateway_ns_protocol.md](lorawan_gateway_ns_protocol.md) |
| Semtech UDP / PUSH_DATA / PULL_RESP | [lorawan_gateway_ns_protocol.md](lorawan_gateway_ns_protocol.md)銆乕BUG-010_notes.md](BUG-010_notes.md) |
| pyserial DTR/RTS 澶嶄綅 | [lorawan_node_e77_otaa_debug.md](lorawan_node_e77_otaa_debug.md) |
| E77 AT 鎸囦护 / AT_CCLASS 鍏ョ綉鍓嶆姤閿?| [lorawan_node_e77_otaa_debug.md](lorawan_node_e77_otaa_debug.md) |
| ADR / LinkADRReq | [lorawan_node_e77_otaa_debug.md](lorawan_node_e77_otaa_debug.md) |
| ESP-IDF I2C / LM75A | [lm75a_i2c_driver_porting.md](lm75a_i2c_driver_porting.md) |
| tv_usec 婧㈠嚭 / ackr 浣?| [BUG-010_notes.md](BUG-010_notes.md) |
| SX1302 HAL / packet forwarder 鏋舵瀯 | [esxp1302_code_walkthrough.md](esxp1302_code_walkthrough.md) |

---

*鏈€鍚庢洿鏂帮細2026-02-27*
