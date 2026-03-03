# ESXP1302 鏃堕棿绯荤粺鍏ㄨ矊

> 鍐欎綔鑳屾櫙锛?026-03-02 瀹屾垚 BUG-011 淇锛坱mms 鏄剧ず 1980 GPS 绾厓锛夛紝
> 鍊熸鏈轰細绯荤粺姊崇悊 ESXP1302 缃戝叧涓墍鏈夋椂闂寸浉鍏虫蹇电殑鏉ラ緳鍘昏剦锛?> 娑电洊浠庣‖浠舵櫠鎸埌 ChirpStack JSON 瀛楁鐨勫畬鏁撮摼璺€?> 鏈枃鍖呭惈瀵瑰叧閿唬鐮佽矾寰勭殑閫愯瑙ｆ瀽锛岄厤鍚堟簮鐮侀槄璇讳娇鐢ㄣ€?>
> 娑夊強婧愭枃浠讹細
> - `main/libloragw/loragw_gps.c` 鈥?GPS 椹卞姩鍜屾椂闂磋浆鎹㈠嚱鏁板簱
> - `main/packet_forwarder/lora_pkt_fwd.c` 鈥?鍖呰浆鍙戜富閫昏緫锛屽惈鎵€鏈夌嚎绋?
---

## 鐩綍

1. [鏃堕棿绯荤粺鎬昏](#涓€鏃堕棿绯荤粺鎬昏)
2. [纭欢灞傦細SX1302 璁℃暟鍣╙(#浜岀‖浠跺眰sx1302-璁℃暟鍣?
3. [GPS 灞傦細鏁版嵁娴佷笌鏃堕棿鍙傝€冨缓绔媇(#涓塯ps-灞傛暟鎹祦涓庢椂闂村弬鑰冨缓绔?
4. [浠ｇ爜瑙ｆ瀽锛歵hread_gps](#鍥涗唬鐮佽В鏋恡hread_gps)
5. [浠ｇ爜瑙ｆ瀽锛歡ps_process_sync](#浜斾唬鐮佽В鏋恎ps_process_sync)
6. [浠ｇ爜瑙ｆ瀽锛歭gw_gps_get](#鍏唬鐮佽В鏋恖gw_gps_get)
7. [浠ｇ爜瑙ｆ瀽锛歭gw_gps_sync](#涓冧唬鐮佽В鏋恖gw_gps_sync)
8. [浠ｇ爜瑙ｆ瀽锛歵hread_valid 涓庢櫠鎸牎姝(#鍏唬鐮佽В鏋恡hread_valid-涓庢櫠鎸牎姝?
9. [浠ｇ爜瑙ｆ瀽锛歵hread_up 鏃堕棿瀛楁鐢熸垚](#涔濅唬鐮佽В鏋恡hread_up-鏃堕棿瀛楁鐢熸垚)
10. [浠ｇ爜瑙ｆ瀽锛歵hread_down 涓嬭鏃堕棿璋冨害](#鍗佷唬鐮佽В鏋恡hread_down-涓嬭鏃堕棿璋冨害)
11. [鏃堕棿杞崲鍑芥暟璇﹁В](#鍗佷竴鏃堕棿杞崲鍑芥暟璇﹁В)
12. [ChirpStack 灞傦細NS 瑙嗚鐨勬椂闂村瓧娈礭(#鍗佷簩chirpstack-灞俷s-瑙嗚鐨勬椂闂村瓧娈?
13. [PPS 鐨勪綔鐢ㄤ笌缂哄け鐨勫奖鍝峕(#鍗佷笁pps-鐨勪綔鐢ㄤ笌缂哄け鐨勫奖鍝?
14. [BUG-011 鏍瑰洜涓庝慨澶峕(#鍗佸洓bug-011-鏍瑰洜涓庝慨澶?
15. [鏃堕棿绯荤粺甯歌闂 FAQ](#鍗佷簲鏃堕棿绯荤粺甯歌闂-faq)

---

## 涓€銆佹椂闂寸郴缁熸€昏

ESXP1302 涓悓鏃跺瓨鍦?**鍥涘鐩镐簰鍏宠仈浣嗕笉鍚岃川鐨勬椂闂?*锛?
```
鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹? 銆?銆慡X1302 纭欢璁℃暟鍣紙count_us / tmst锛?                         鈹?鈹?      鍗曚綅锛氬井绉掞紙碌s锛?                                             鈹?鈹?      璧风偣锛氱綉鍏充笂鐢靛浣嶏紝浠?0 寮€濮?                                鈹?鈹?      椹卞姩锛?2 MHz 鏅舵尟锛岀粡鍒嗛鍚庝互 1 MHz 閫熺巼閫掑锛圱S_CPS=1E6锛?  鈹?鈹?      鐗圭偣锛氱浉瀵规椂闂达紝鍗曡皟閫掑锛屾棤闂扮锛?2 bit 绾?71.6 鍒嗛挓鍥炵粫     鈹?鈹?      鐢ㄩ€旓細甯ф帴鏀舵椂鍒绘爣璁帮紙tmst锛夛紝涓嬭鍙戝皠绮剧‘璋冨害                鈹?鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹? 銆?銆慓PS UTC 鏃堕棿锛坲tc / "time" 瀛楁锛?                            鈹?鈹?      鍗曚綅锛氱 + 绾崇锛坰truct timespec锛?                          鈹?鈹?      璧风偣锛?970-01-01 00:00:00 UTC锛圲nix 绾厓锛?                   鈹?鈹?      鏉ユ簮锛歂MEA $GNRMC / $GNZDA 璇彞瑙ｆ瀽                           鈹?鈹?      绮惧害锛氭绉掔骇锛圢MEA 鎶ュ憡鍒?.sss锛?                             鈹?鈹?      鐢ㄩ€旓細涓婅 JSON "time" 瀛楁锛屼汉绫诲彲璇荤殑缁濆鏃堕棿               鈹?鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹? 銆?銆慓PS 绾厓鏃堕棿锛坓ps_time / "tmms" 瀛楁锛?                       鈹?鈹?      鍗曚綅锛氱 + 绾崇锛坰truct timespec锛夋垨姣锛坲int64_t tmms锛?   鈹?鈹?      璧风偣锛?980-01-06 00:00:00 GPS锛圙PS 绾厓锛?                    鈹?鈹?      鏉ユ簮锛氣憼 UBX NAV-TIMEGPS锛坲-blox 涓撴湁锛屽惈鍛ㄥ彿+iTOW锛?         鈹?鈹?              鈶?NMEA UTC 鎹㈢畻锛欸PS = UTC_unix - 315964800 + 18     鈹?鈹?      鐗圭偣锛氫笉璺抽棸绉掞紝姣?UTC 蹇?18 绉掞紙鎴嚦 2026锛?                 鈹?鈹?      鐢ㄩ€旓細涓婅 JSON "tmms" 瀛楁锛孋lass B Beacon 鏃堕棿鎴?           鈹?鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹? 銆?銆戠郴缁熸椂閽燂紙time(NULL) / systime锛?                             鈹?鈹?      鍗曚綅锛氱锛坱ime_t锛?                                           鈹?鈹?      鏉ユ簮锛欵SP32 鍐呴儴 RTC锛堟棤 NTP锛屼粎浣滅湅闂ㄧ嫍鐢級                  鈹?鈹?      鐢ㄩ€旓細GPS 鍙傝€冨勾榫勬娴嬶紙thread_valid锛夛紝鏃ュ織鏃堕棿鎴?            鈹?鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?```

杩欏洓濂楁椂闂撮€氳繃 **`time_reference_gps`锛坰truct tref锛?* 缁戝畾鍦ㄤ竴璧枫€?`struct tref` 瀹氫箟鍦?`loragw_gps.h`锛?
```c
struct tref {
    time_t      systime;   // 绯荤粺鏃堕挓锛?鏈鍚屾鍙戠敓鍦ㄤ綍鏃?
    uint32_t    count_us;  // 鍚屾鏃跺埢鐨?SX1302 璁℃暟鍣紙鏉ヨ嚜 trig_tstamp锛屽嵆 PPS 閿佸瓨鍊硷級
    struct timespec utc;   // 鍚屾鏃跺埢鐨?UTC 鏃堕棿锛堟潵鑷?NMEA 瑙ｆ瀽锛?    struct timespec gps;   // 鍚屾鏃跺埢鐨?GPS 绾厓鏃堕棿锛堜慨澶嶅墠鎬绘槸 {0,0}锛?    double      xtal_err;  // 鏅舵尟璇樊锛歝nt_diff / utc_diff锛堢悊鎯冲€?1.0锛?};
```

鏈変簡杩欎釜閿氱偣锛屽彲浠ュ湪浠绘剰鏃跺埢閫氳繃璁℃暟鍣ㄥ€兼帹绠楀嚭 UTC 鎴?GPS 绾厓鏃堕棿銆?
---

## 浜屻€佺‖浠跺眰锛歋X1302 璁℃暟鍣?
### 2.1 涓や釜璁℃暟鍣?
SX1302 鍐呴儴鏈変袱涓敱鐩稿悓鏅舵尟椹卞姩鐨勮鏁板櫒锛屽潎浠?1 MHz 閫熺巼閫掑锛?
| 璁℃暟鍣?| 鏇存柊鏉′欢 | 璇诲彇鍑芥暟 | 鐢ㄩ€?|
|--------|----------|----------|------|
| `TIMESTAMP_INST`锛堝嵆鏃讹級 | 姣忓井绉掕嚜鍔ㄩ€掑 | `lgw_get_instcnt()` | 甯ф帴鏀舵椂鍒伙紙`tmst`锛夛紝JIT 璋冨害鍩哄噯 |
| `TIMESTAMP_PPS`锛圥PS 鎹曡幏锛?| GPS PPS 涓婂崌娌挎椂纭欢閿佸瓨 INST 鍊?| `lgw_get_trigcnt()` | GPS 鍚屾閿氱偣锛坄trig_tstamp`锛?|

**鍏抽敭鐞嗚В**锛氫袱涓鏁板櫒鐨勬椂閽熸簮瀹屽叏鐩稿悓锛屾病鏈?PPS 鏃舵櫠鎸収鏍疯窇锛?鍖哄埆鍦ㄤ簬 PPS 璁℃暟鍣ㄦ槸**纭欢閿佸瓨鍣?*鈥斺€擯PS 鑴夊啿鍒版潵鐨勭灛闂存妸 INST 鐨勫€?蹇収"杩涘幓锛?涔嬪悗涓嶇 INST 缁х画璺戝杩滐紝PPS 瀵勫瓨鍣ㄩ噷鐨勫€间繚鎸佷笉鍙橈紝鐩村埌涓嬩竴涓?PPS 鑴夊啿銆?
### 2.2 tmst 瀛楁鐨勬潵婧?
```
LoRa 甯у埌杈?SX1302 澶╃嚎
       鈫?SX1302 纭欢璁板綍鎺ユ敹瀹屾垚鏃跺埢锛堢簿纭埌甯у熬鏈€鍚庝竴涓鍙凤級
lgw_receive() 璇诲嚭 rxpkt[i].count_us锛圛NST 璁℃暟鍣ㄥ綋鍓嶅€硷級
       鈫?thread_up: p->count_us 鈫?JSON "tmst" 瀛楁锛堣 lora_pkt_fwd.c 绾?2330 琛岋級

// lora_pkt_fwd.c ~L2330
j = snprintf(..., ",\"tmst\":%u", p->count_us);
```

`tmst` 鏄?*鐩稿鏃堕棿鎴?*锛屽崟浣嶅井绉掞紝鏄綉鍏充笂鐢靛悗璁℃暟鍣ㄧ殑绱鍊笺€?涓嶅悓缃戝叧涔嬮棿 `tmst` 瀹屽叏娌℃湁鍙瘮鎬э紱鍚屼竴缃戝叧閲嶅惎鍚庝篃閲嶆柊浠?0 寮€濮嬨€?
### 2.3 璁℃暟鍣ㄦ孩鍑?
`count_us` 鏄?32 浣嶆棤绗﹀彿鏁村瀷锛?
$$2^{32} - 1 = 4{,}294{,}967{,}295\ \mu s \approx 4295\ s \approx 71.6\ \text{鍒嗛挓}$$

姣?71.6 鍒嗛挓璁℃暟鍣ㄥ洖缁曚负 0銆傛椂闂村樊璁＄畻鍒╃敤鏃犵鍙锋暣鏁拌嚜鐒舵孩鍑猴紝鏃犻渶鐗规畩澶勭悊锛?
```c
// loragw_gps.c - lgw_cnt2utc() 涓殑宸€艰绠?// count_us 鍜?ref.count_us 閮芥槸 uint32_t锛岀浉鍑忚嚜鍔ㄥ鐞嗘孩鍑?delta_sec = (double)(count_us - ref.count_us) / (TS_CPS * ref.xtal_err);
```

鑻ヤ娇鐢ㄦ湁绗﹀彿姣旇緝鍒欐孩鍑烘椂缁撴灉閿欒锛岃繖鏄釜甯歌闄烽槺銆?
---

## 涓夈€丟PS 灞傦細鏁版嵁娴佷笌鏃堕棿鍙傝€冨缓绔?
### 3.1 瀹屾暣鏁版嵁娴侊紙ATGM336H NMEA-only 妯″紡锛?
```
ATGM336H GPS 妯″潡锛圲ART1, GPIO19 RX/GPIO20 TX, 9600 baud锛?  鈹?  鈹溾攢 姣忕杈撳嚭澶氭潯 NMEA 璇彞锛?GNRMC, $GNGGA, $GNGSV 绛夛級
  鈹?  鈹? thread_gps锛團reeRTOS 浠诲姟锛屼紭鍏堢骇 6锛屾爤 8KB锛?  鈹?   鈹?  鈹?   鈹溾攢 uart_read_bytes() 闃诲璇伙紝瓒呮椂 1000ms
  鈹?   鈹?  鍐欏叆 serial_buff[128] 鐨?[wr_idx..] 浣嶇疆
  鈹?   鈹?  鈹?   鈹溾攢 鎵弿缂撳啿鍖猴紙rd_idx 浠?0 鍒?wr_idx锛?  鈹?   鈹?  鈹溾攢 閬囧埌 '$'锛?x24锛孨MEA 璧峰锛夛細
  鈹?   鈹?  鈹?  memchr 鎵?'\n'锛?x0a锛岀粨鏉熺锛?  鈹?   鈹?  鈹?  lgw_parse_nmea() 鈫?瑙ｆ瀽 $GNRMC锛?  鈹?   鈹?  鈹?    鏇存柊 gps_yea/mon/day/hou/min/sec/fra锛堥潤鎬佸叏灞€鍙橀噺锛?  鈹?   鈹?  鈹?    鏇存柊 gps_dla/mla/ola/dlo/mlo/olo/alt锛堝潗鏍囷級
  鈹?   鈹?  鈹?    璁剧疆 gps_time_ok = true锛坒ix 鐘舵€佷负 A 鏃讹級
  鈹?   鈹?  鈹?    杩斿洖 NMEA_RMC
  鈹?   鈹?  鈹?      鈫?  鈹?   鈹?  鈹?  gps_process_coords() 鈫?鏇存柊 meas_gps_coord
  鈹?   鈹?  鈹?  gps_process_sync()   鈫?寤虹珛鏃堕棿鍙傝€冿紙璇﹁绗簲鑺傦級
  鈹?   鈹?  鈹?  鈹?   鈹?  鈹斺攢 閬囧埌 0xB5锛圲BX 璧峰锛夛細
  鈹?   鈹?      lgw_parse_ubx() 鈫?ATGM336H 鏃犳杈撳嚭锛岄€氬父 IGNORED
  鈹?   鈹?  鈹?   鈹斺攢 婊戝姩绐楀彛缂撳啿鍖虹鐞嗭紙璇﹁ 3.2锛?  鈹?  鈹溾攢 PPS 寮曡剼锛? Hz锛岀簿纭爣璁版暣绉掕竟鐣岋級
  鈹?    鈫?SX1302 纭欢鍦ㄤ笂鍗囨部閿佸瓨 INST 鈫?TIMESTAMP_PPS 瀵勫瓨鍣?  鈹?    鈫?lgw_get_trigcnt() 璇诲嚭 trig_tstamp
  鈹?    鈹斺攢 鍦?gps_process_sync() 涓笌 NMEA UTC 缁戝畾
  鈹?  鈹斺攢 thread_valid锛堟瘡绉掓鏌ュ弬鑰冩湁鏁堟湡锛岀淮鎶ゆ櫠鎸牎姝ｇ郴鏁帮級
```

### 3.2 缂撳啿鍖虹鐞嗭紙婊戝姩绐楀彛锛?
`thread_gps` 涓嶄娇鐢ㄧ粡鍏哥幆褰㈢紦鍐插尯锛岃€屾槸鐢ㄤ竴涓?128 瀛楄妭鏁扮粍 + 涓や釜绱㈠紩瀹炵幇婊戝姩绐楀彛锛?
```c
// lora_pkt_fwd.c - thread_gps() 鍐?char serial_buff[128];   // 闈欐€佺紦鍐插尯
size_t wr_idx = 0;       // 鏈夋晥鏁版嵁缁撴潫浣嶇疆锛堟柊鏁版嵁杩藉姞鍒版澶勶級

// 姣忔寰幆锛?// 1. 杩藉姞璇诲彇
int nb_char = uart_read_bytes(gps_tty_fd,
    (uint8_t *)(serial_buff + wr_idx),  // 杩藉姞鍒板凡鏈夋暟鎹箣鍚?    LGW_GPS_MIN_MSG_SIZE,               // 鏈€灏戣 LGW_GPS_MIN_MSG_SIZE 瀛楄妭
    pdMS_TO_TICKS(1000));               // 闃诲绛夊緟鏈€澶?1 绉?wr_idx += (size_t)nb_char;

// 2. 鎵弿闃舵锛坮d_idx 浠庡ご鎵埌 wr_idx锛?size_t rd_idx = 0;
size_t frame_end_idx = 0;
while (rd_idx < wr_idx) {
    size_t frame_size = 0;
    if (serial_buff[rd_idx] == '$') {
        // 鎵惧埌 NMEA 甯э紝瑙ｆ瀽...
        frame_size = ...; // 鏈抚瀛楄妭鏁?    }
    if (frame_size > 0) {
        rd_idx += frame_size;
        frame_end_idx = rd_idx; // 璁板綍鏈€鍚庝竴涓垚鍔熷抚鐨勭粨鏉熶綅缃?    } else {
        rd_idx++; // 鏈瘑鍒瓧绗︼紝璺宠繃
    }
}

// 3. 娓呴櫎宸插鐞嗘暟鎹紙灏嗗墿浣欐暟鎹Щ鍒扮紦鍐插尯寮€澶达級
if (frame_end_idx) {
    memcpy(serial_buff, &serial_buff[frame_end_idx], wr_idx - frame_end_idx);
    wr_idx -= frame_end_idx;
}

// 4. 婧㈠嚭淇濇姢
if ((sizeof(serial_buff) - wr_idx) < LGW_GPS_MIN_MSG_SIZE) {
    memcpy(serial_buff, &serial_buff[LGW_GPS_MIN_MSG_SIZE], wr_idx - LGW_GPS_MIN_MSG_SIZE);
    wr_idx -= LGW_GPS_MIN_MSG_SIZE;
}
```

杩欎笉鏄幆褰㈢紦鍐插尯锛岃€屾槸**婊戝姩绐楀彛**锛歚memcpy` 姣忔鎶婃湭澶勭悊鐨勬暟鎹惉鍒扮紦鍐插尯澶撮儴锛?浠ｄ环鏄?O(n) 鎷疯礉锛屼絾瀵逛綆閫?NMEA锛?600 baud锛夊畬鍏ㄥ鐢ㄣ€?
---

## 鍥涖€佷唬鐮佽В鏋愶細thread_gps

> 婧愭枃浠讹細`lora_pkt_fwd.c`锛屽嚱鏁?`thread_gps()`锛岀害 L3653 璧?
### 4.1 浠诲姟鍒涘缓鏂瑰紡

鍘熷浠ｇ爜浣跨敤 POSIX `pthread_create`锛孍SP32 绉绘鏃舵敼涓?FreeRTOS锛?
```c
// lora_pkt_fwd.c ~L1852
xTaskCreatePinnedToCore(
    (TaskFunction_t) thread_gps,
    "thread_gps",
    4096*2,         // 8KB 鏍堬紙GPS UART + NMEA 瑙ｆ瀽鐢級
    NULL,
    6,              // 浼樺厛绾?6锛堥珮浜庨粯璁?5锛岀‘淇?GPS 鏁版嵁涓嶄涪澶憋級
    NULL,
    tskNO_AFFINITY  // 涓嶉攣瀹?CPU 鏍稿績
);
```

### 4.2 NMEA 甯ф娴嬮€昏緫

```c
} else if (serial_buff[rd_idx] == (char)LGW_GPS_NMEA_SYNC_CHAR) {  // '$' = 0x24
    // 鎵惧抚缁撴潫绗?LF锛?x0A锛?    char* nmea_end_ptr = memchr(&serial_buff[rd_idx], 0x0a, (wr_idx - rd_idx));

    if (nmea_end_ptr) {
        frame_size = nmea_end_ptr - &serial_buff[rd_idx] + 1; // 鍚?LF
        latest_msg = lgw_parse_nmea(&serial_buff[rd_idx], frame_size);

        if (latest_msg == NMEA_RMC) {
            gps_process_coords(); // 鏇存柊缁忕含搴︼紙鐙珛浜庢椂闂达級
            gps_process_sync();   // 寤虹珛/鏇存柊鏃堕棿鍙傝€?        }
    }
    // 娉細鑻ョ紦鍐插尯涓彧鏈夊抚澶存病鏈?LF锛宯mea_end_ptr 涓?NULL
    //     frame_size 淇濇寔 0锛屼笅娆¤鍒版洿澶氭暟鎹悗鍐嶉噸璇?}
```

ATGM336H 鍙緭鍑?NMEA锛屾墍浠?UBX 鍒嗘敮锛坄0xB5` 璧峰锛夊湪姝ｅ父宸ヤ綔涓案杩滀笉浼氬懡涓€?
### 4.3 涓轰粈涔堝湪 NMEA_RMC 鑰屼笉鏄?NMEA_GGA 涓婅Е鍙戝悓姝?
`$GNRMC` 鏄敮涓€鍚屾椂鍖呭惈**鏃堕棿**鍜?*鏃ユ湡**鐨?NMEA 璇彞锛坄$GNGGA` 鍙湁鏃堕棿锛屾病鏈夋棩鏈燂級銆?`lgw_gps_get()` 闇€瑕佸畬鏁寸殑骞存湀鏃ユ椂鍒嗙鎵嶈兘璋冪敤 `mktime()` 寰楀埌缁濆 Unix 鏃堕棿鎴筹紝
鍥犳蹇呴』绛?RMC 瑙ｆ瀽瀹屾垚鍚庢墠鑳借Е鍙戞椂闂村悓姝ャ€?
---

## 浜斻€佷唬鐮佽В鏋愶細gps_process_sync

> 婧愭枃浠讹細`lora_pkt_fwd.c`锛屽嚱鏁?`gps_process_sync()`锛岀害 L3591 璧?
杩欐槸杩炴帴 GPS 鏃堕棿鍜?SX1302 璁℃暟鍣ㄧ殑鏍稿績鍑芥暟锛屾瘡绉掑湪 `NMEA_RMC` 鍒拌揪鏃惰皟鐢ㄤ竴娆°€?
```c
static void gps_process_sync(void)
{
    struct timespec gps_time;  // GPS 绾厓鏃堕棿锛?980-01-06 璧凤級
    struct timespec utc;       // UTC 鏃堕棿锛?970-01-01 璧凤級
    unsigned int trig_tstamp;  // PPS 閿佸瓨鐨?SX1302 璁℃暟鍣ㄥ€?
    // 鈶?浠?NMEA 瑙ｆ瀽缁撴灉涓瀯閫?UTC 鍜?GPS 绾厓鏃堕棿
    int i = lgw_gps_get(&utc, &gps_time, NULL, NULL);
    if (i != LGW_GPS_SUCCESS) {
        // gps_time_ok == false锛堟棤瀹氫綅鎴?NMEA 鏈В鏋愭垚鍔燂級
        return;
    }

    // 鈶?璇诲彇 PPS 纭欢閿佸瓨鐨勮鏁板櫒鍊硷紙闇€瑕佹寔 mx_concent 閿侊級
    xSemaphoreTake(mx_concent, portMAX_DELAY);
    i = lgw_get_trigcnt(&trig_tstamp);
    xSemaphoreGive(mx_concent);
    // trig_tstamp = PPS 鑴夊啿鍒版潵鐬棿鐨?INST 璁℃暟鍣ㄥ揩鐓?    // 鑻ユ棤 PPS锛歵rig_tstamp = 0锛堝瘎瀛樺櫒浠庢湭琚攣瀛樿繃锛?
    // 鈶?鐢?(trig_tstamp, utc, gps_time) 涓夊厓缁勬洿鏂版椂闂村弬鑰?    xSemaphoreTake(mx_timeref, portMAX_DELAY);
    i = lgw_gps_sync(&time_reference_gps, trig_tstamp, utc, gps_time);
    xSemaphoreGive(mx_timeref);
    // time_reference_gps.count_us = trig_tstamp锛圥PS 鏃跺埢鐨勮鏁板櫒锛?    // time_reference_gps.utc      = utc锛堣 PPS 鏁寸瀵瑰簲鐨?UTC锛?    // time_reference_gps.gps      = gps_time锛堝悓涓婏紝GPS 绾厓琛ㄧず锛?    // time_reference_gps.xtal_err = 瀹炴祴 slope锛堜慨姝ｆ櫠鎸亸宸級

    if (i == LGW_GPS_SUCCESS) {
        // 鎵撳嵃鍚屾鏃ュ織锛圙PS_LOG_VERBOSE >= 1 鏃讹級
        // "INFO: [gps] synced UTC time: 2026-03-02T08:58:19.000Z (trig_tstamp=201660610)"
    }
}
```

**鍏抽敭绾︽潫**锛歚gps_process_sync()` 姣忔璋冪敤閮戒娇鐢?鏈€鏂扮殑 PPS 閿佸瓨鍊?锛?鑰屼笉鏄瘡娆?PPS 鑴夊啿鏃剁珛鍒昏Е鍙戯紙ATGM336H 涓嶆敮鎸?PPS 涓柇閫氱煡杞欢锛夈€?鍥犳瀹為檯閫昏緫鏄細**NMEA RMC 鍒拌揪鏃讹紝椤轰究璇讳竴涓嬪綋鍓嶇殑 PPS 瀵勫瓨鍣?*锛?PPS 鍊煎拰 NMEA 鏃堕棿瀵瑰簲鍚屼竴鏁寸锛堝彧瑕?NMEA 鍜?PPS 鐨勪紶杈撳欢杩熷彲浠ュ拷鐣ワ級銆?
---

## 鍏€佷唬鐮佽В鏋愶細lgw_gps_get

> 婧愭枃浠讹細`loragw_gps.c`锛屽嚱鏁?`lgw_gps_get()`锛岀害 L681 璧?
`lgw_gps_get()` 鏄皢 NMEA 瑙ｆ瀽鍑虹殑闈欐€佸叏灞€鍙橀噺杞崲涓?`struct timespec` 鐨勬ˉ姊併€?
### 6.1 UTC 鍒嗘敮锛圢MEA 璺緞锛屾棤 BUG锛?
```c
// loragw_gps.c - lgw_gps_get(), utc 鍒嗘敮
if (utc != NULL) {
    if (!gps_time_ok) return LGW_GPS_ERROR;  // NMEA 鏈В鏋愭垚鍔?
    struct tm x = {0};
    // 灏?NMEA 瀛楁缁勮鎴?broken-down time
    x.tm_year = (gps_yea < 100) ? gps_yea + 100 : gps_yea - 1900;
    x.tm_mon  = gps_mon - 1;   // tm_mon 鏄?[0,11]锛孨MEA 鏄?[1,12]
    x.tm_mday = gps_day;
    x.tm_hour = gps_hou;
    x.tm_min  = gps_min;
    x.tm_sec  = gps_sec;

    time_t y = mktime(&x);  // 杞崲涓?Unix 鏃堕棿鎴筹紙绉掞級
    // 娉細mktime 鍋囪杈撳叆鏄湰鍦版椂闂达紝浣?tzset() 宸插湪 lgw_gps_enable() 涓皟鐢?    // ESP-IDF 榛樿鏃跺尯 UTC+0锛屾墍浠ョ粨鏋滄纭?
    utc->tv_sec  = y;
    utc->tv_nsec = (int32_t)(gps_fra * 1e9);  // 灏忔暟绉掗儴鍒嗭紙绾崇锛?}
```

`gps_fra` 鏉ヨ嚜 `$GNRMC` 涓殑 `hhmmss.sss` 鏍煎紡锛?渚嬪 `085819.685` 鈫?`gps_fra = 0.685` 鈫?`tv_nsec = 685000000 ns`銆?
### 6.2 gps_time 鍒嗘敮锛圔UG-011 淇鐐癸級

淇鍓嶇殑鍘熷浠ｇ爜锛堜娇鐢?UBX 璺緞锛孨MEA-only 妯″潡姘歌繙涓嶆洿鏂帮級锛?
```c
// 淇鍓嶏紙BUGGY锛夛細
if (gps_time != NULL) {
    // gps_week, gps_iTOW, gps_fTOW 鍙敱 UBX_NAV_TIMEGPS 娑堟伅鏇存柊
    // ATGM336H 鏃?UBX 杈撳嚭 鈫?杩欎笁涓彉閲忔案杩滄槸 0
    fractpart = modf(((double)gps_iTOW / 1E3) + ((double)gps_fTOW / 1E9), &intpart);
    gps_time->tv_sec  = (time_t)intpart + (time_t)gps_week * 604800;
    // = 0 + 0 * 604800 = 0
    gps_time->tv_nsec = (long)(fractpart * 1E9);  // = 0
    // 缁撴灉锛歡ps_time = {0, 0}锛圙PS 绾厓 1980-01-06锛屼笉鏄綋鍓嶆椂闂达紒锛?}
```

淇鍚庯紙浠?`gps_week` 鏄惁涓?0 鍒ゆ柇妯″紡锛夛細

```c
// 淇鍚庯紙loragw_gps.c 褰撳墠浠ｇ爜锛夛細
if (gps_time != NULL) {
    if (!gps_time_ok) return LGW_GPS_ERROR;

    if (gps_week != 0) {
        // 鈹€鈹€ UBX 妯″紡锛坲-blox 妯″潡锛夛細浣跨敤楂樼簿搴?iTOW + week 鈹€鈹€
        fractpart = modf(((double)gps_iTOW / 1E3) + ((double)gps_fTOW / 1E9), &intpart);
        gps_time->tv_sec  = (time_t)intpart + (time_t)gps_week * 604800;
        gps_time->tv_nsec = (long)(fractpart * 1E9);
    } else {
        // 鈹€鈹€ NMEA-only 妯″紡锛圓TGM336H 绛夛級锛氫粠 NMEA 鏃ユ湡鏃堕棿瀛楁娲剧敓 鈹€鈹€
        struct tm gps_tm = {0};
        gps_tm.tm_year = (gps_yea < 100) ? gps_yea + 100 : gps_yea - 1900;
        gps_tm.tm_mon  = gps_mon - 1;
        gps_tm.tm_mday = gps_day;
        gps_tm.tm_hour = gps_hou;
        gps_tm.tm_min  = gps_min;
        gps_tm.tm_sec  = gps_sec;
        time_t gps_unix = mktime(&gps_tm);  // 鈫?Unix 鏃堕棿鎴?
        // GPS 绾厓 = Unix 鏃堕棿鎴?- GPS绾厓鍋忕Щ + 闂扮
        // GPS_epoch_offset = 315964800 锛?970-01-01 鍒?1980-01-06 鐨勭鏁帮級
        // GPS_LEAP_SECONDS  = 18        锛圙PS 姣?UTC 蹇?18 绉掞紝2017骞磋嚦浠婏級
        gps_time->tv_sec  = gps_unix - UNIX_GPS_EPOCH_OFFSET + GPS_LEAP_SECONDS;
        gps_time->tv_nsec = (int32_t)(gps_fra * 1e9);
        // 缁撴灉绀轰緥锛?772441099 - 315964800 + 18 = 1456476317
        //           瀵瑰簲 2026-03-02T08:58:37 GPS锛岀害 1456476317 绉掕嚜 1980-01-06
    }
}
```

---

## 涓冦€佷唬鐮佽В鏋愶細lgw_gps_sync

> 婧愭枃浠讹細`loragw_gps.c`锛屽嚱鏁?`lgw_gps_sync()`锛岀害 L610 璧?
`lgw_gps_sync()` 姣忔鏀跺埌 GPS 鏃堕棿鍚庤璋冪敤锛岃礋璐ｏ細
1. 妫€娴嬭娆″悓姝ユ槸鍚?寮傚父"锛坰lope 瓒呭嚭 卤10 ppm锛?2. 鑻ユ甯革紝灏嗕笁鍏冪粍 `(count_us, utc, gps_time)` 鍐欏叆 `time_reference_gps`
3. 鑻ヨ繛缁?3 娆″紓甯革紝寮哄埗閲嶇疆锛堟帓闄ゅ崼鏄熶俊鍙疯川閲忛棶棰樺鑷寸殑鎸佺画澶遍攣锛?
```c
int lgw_gps_sync(struct tref *ref, unsigned int count_us,
                 struct timespec utc, struct timespec gps_time) {

    // 璁＄畻璁℃暟鍣ㄥ樊鍊硷紙绉掞紝鏈粡鏅舵尟鏍℃锛?    double cnt_diff = (double)(count_us - ref->count_us) / TS_CPS;
    // 璁＄畻 UTC 宸€硷紙绉掞級
    double utc_diff = (double)(utc.tv_sec - ref->utc.tv_sec)
                    + 1E-9 * (double)(utc.tv_nsec - ref->utc.tv_nsec);

    // 璁＄畻 slope = 璁℃暟鍣ㄩ€熺巼 / UTC 閫熺巼
    // 鐞嗘兂鍊?= 1.0锛堣鏁板櫒姣忕璧版伆濂?1,000,000 涓崟浣嶏級
    // 瀹為檯鍊?= 1.0 卤 鍑?ppm锛堟櫠鎸亸宸級
    double slope = cnt_diff / utc_diff;

    // 寮傚父妫€娴嬶細slope 瓒呭嚭 卤10 ppm 鑼冨洿锛圥LUS_10PPM=1.00001, MINUS_10PPM=0.99999锛?    bool aber_n0 = (slope > PLUS_10PPM) || (slope < MINUS_10PPM) || (utc_diff == 0);

    if (!aber_n0) {
        // 鈹€鈹€ 姝ｅ父锛氱洿鎺ユ洿鏂版椂闂村弬鑰?鈹€鈹€
        ref->systime     = time(NULL);      // 绯荤粺鏃堕挓锛堝綋鍓嶆椂鍒伙級
        ref->count_us    = count_us;        // PPS 鏃跺埢鐨勮鏁板櫒鍊?        ref->utc         = utc;             // PPS 瀵瑰簲鐨?UTC
        ref->gps         = gps_time;        // PPS 瀵瑰簲鐨?GPS 绾厓鏃堕棿锛堜慨澶嶅悗姝ｇ‘锛?        ref->xtal_err    = slope;           // 鏈娴嬪緱鐨勬櫠鎸€熺巼
        return LGW_GPS_SUCCESS;

    } else if (aber_n0 && aber_min1 && aber_min2) {
        // 鈹€鈹€ 杩炵画 3 娆″紓甯革細寮哄埗閲嶇疆 鈹€鈹€
        ref->systime  = time(NULL);
        ref->count_us = count_us;
        ref->utc      = utc;
        ref->gps      = gps_time;
        // xtal_err 鍙湪瓒婄晫鏃舵墠閲嶇疆涓?1.0锛屽惁鍒欎繚鐣欎笂娆＄殑鍊?        if ((ref->xtal_err > PLUS_10PPM) || (ref->xtal_err < MINUS_10PPM))
            ref->xtal_err = 1.0;
        return LGW_GPS_SUCCESS;

    } else {
        // 鈹€鈹€ 鍙湁 1 鎴?2 娆″紓甯革細蹇界暐锛屼繚鐣欐棫鍙傝€?鈹€鈹€
        return LGW_GPS_ERROR;
        // 璋冪敤鏂?(gps_process_sync) 鏀跺埌 ERROR 鍚庢墦鍗?        // "WARNING: [gps] GPS out of sync, keeping previous time reference"
    }
}
```

**鏃?PPS 鏃剁殑 slope 璁＄畻**锛?- `count_us = trig_tstamp = 0`锛圥PS 瀵勫瓨鍣ㄤ粠鏈洿鏂帮級
- `cnt_diff = (0 - ref->count_us)`锛氱涓€娆?`ref->count_us=0` 鏃跺樊鍊?0锛宻lope=0 鈫?寮傚父
- 鍚庣画姣忔 `count_us` 浠嶇劧鏄?0锛岃€?`ref->count_us` 涔熻鏇存柊涓?0
- 瀹為檯涓?slope 涓€鐩存槸 0/1 = 0锛岃繛缁?3 娆″紓甯?鈫?寮哄埗閲嶇疆
- 寮哄埗閲嶇疆鍚庯細`ref->count_us = 0, ref->xtal_err` 淇濈暀鎴栭噸缃负 1.0
- 涔嬪悗 `lgw_cnt2utc(count_us=X, ref.count_us=0)` 鈫?`delta = X/1E6` 绉掞紙璁惧杩愯鏃堕棿锛?
---

## 鍏€佷唬鐮佽В鏋愶細thread_valid 涓庢櫠鎸牎姝?
> 婧愭枃浠讹細`lora_pkt_fwd.c`锛屽嚱鏁?`thread_valid()`锛岀害 L3775 璧?
`thread_valid` 姣忕鎵ц涓€娆★紝璐熻矗涓や欢浜嬶細
1. **妫€鏌ュ弬鑰冩湁鏁堟湡**锛氳秴杩?`GPS_REF_MAX_AGE=30` 绉掓病鏈夋垚鍔熷悓姝ワ紝鍒?`gps_ref_valid = false`
2. **缁存姢鏅舵尟鏍℃绯绘暟**锛氬 `xtal_err` 鍋氫綆閫氭护娉紝寰楀埌绋冲畾鐨?`xtal_correct`

```c
void thread_valid(void) {
    long gps_ref_age;
    double xtal_err_cpy;
    unsigned init_cpt = 0;
    double init_acc = 0.0;

    while (!exit_sig && !quit_sig) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // 姣忕鎵ц涓€娆?
        // 鈹€鈹€ 绗竴姝ワ細妫€鏌ュ弬鑰冨勾榫?鈹€鈹€
        xSemaphoreTake(mx_timeref, portMAX_DELAY);
        gps_ref_age = (long)difftime(time(NULL), time_reference_gps.systime);
        if (gps_ref_age >= 0 && gps_ref_age <= GPS_REF_MAX_AGE) {  // 30 绉掑唴
            gps_ref_valid = true;
            xtal_err_cpy = time_reference_gps.xtal_err;
        } else {
            gps_ref_valid = false;  // 鍙傝€冭繃鏃э紝绂佺敤鏃堕棿鎴?        }
        xSemaphoreGive(mx_timeref);

        // 鈹€鈹€ 绗簩姝ワ細鏅舵尟鏍℃绯绘暟绠＄悊 鈹€鈹€
        if (!gps_ref_valid) {
            // GPS 澶遍攣锛氶噸缃紝涓嶅仛棰戠巼琛ュ伩
            xtal_correct_ok = false;
            xtal_correct = 1.0;
            init_cpt = 0;
            init_acc = 0.0;
        } else if (init_cpt < XERR_INIT_AVG) {
            // 鍒濆鍖栭樁娈碉紙鍓?XERR_INIT_AVG=4 娆★級锛氱疮绉眰鍜?            init_acc += xtal_err_cpy;
            ++init_cpt;
        } else if (init_cpt == XERR_INIT_AVG) {
            // 鍒濆鍧囧€硷細xtal_correct = N / sum(slope_i)
            // 鍗?slope 鐨勮皟鍜屽钩鍧囨暟鐨勫€掓暟
            xtal_correct = (double)XERR_INIT_AVG / init_acc;
            xtal_correct_ok = true;
            ++init_cpt;
        } else {
            // 绋虫€佽窡韪細涓€闃?IIR 浣庨€氭护娉紙绯绘暟 1/XERR_FILT_COEF=1/256锛?            double x = 1.0 / xtal_err_cpy;
            xtal_correct = xtal_correct
                         - xtal_correct / XERR_FILT_COEF
                         + x / XERR_FILT_COEF;
            // 绛変环浜庯細xtal_correct = xtal_correct * (1 - 伪) + x * 伪锛屛?= 1/256
        }
    }
}
```

`xtal_correct` 鏄?`1/xtal_err` 鐨勪綆閫氭护娉㈠€硷紙1/slope锛夛紝
鑰屼笉鏄?`xtal_err`锛坰lope锛夋湰韬€斺€斿洜涓烘椂闂磋浆鎹㈠叕寮忎腑闄や互 `xtal_err` 绛夊悓浜庝箻浠?`xtal_correct`锛?
```c
// lgw_cnt2utc() 涓細
delta_sec = (count_us - ref.count_us) / (TS_CPS * ref.xtal_err);

// beacon 棰戠巼鏍℃涓紙浣跨敤骞虫粦鍚庣殑 xtal_correct锛夛細
pkt.freq_hz = (unsigned int)(xtal_correct * (double)pkt.freq_hz);
```

---

## 涔濄€佷唬鐮佽В鏋愶細thread_up 鏃堕棿瀛楁鐢熸垚

> 婧愭枃浠讹細`lora_pkt_fwd.c`锛屽嚱鏁?`thread_up()`锛岀害 L2333 璧?
姣忔敹鍒颁竴鎵逛笂琛屽寘锛宍thread_up` 鍏堝揩鐓т竴浠?GPS 鏃堕棿鍙傝€冿紝鍐嶄负姣忎釜鍖呯敓鎴愭椂闂村瓧娈碉細

```c
// thread_up() - GPS 鍙傝€冨揩鐓?if ((nb_pkt > 0) && (gps_enabled == true)) {
    xSemaphoreTake(mx_timeref, portMAX_DELAY);
    ref_ok = gps_ref_valid;       // 鍙傝€冩槸鍚︽湁鏁堬紙鏈€杩?30 绉掑唴鎴愬姛鍚屾杩囷級
    local_ref = time_reference_gps; // 娣辨嫹璐濓紝閬垮厤澶勭悊杩囩▼涓 gps_process_sync 淇敼
    xSemaphoreGive(mx_timeref);
} else {
    ref_ok = false;
}

// 瀵规瘡涓敹鍒扮殑鍖?p锛?
// tmst锛氬缁堝啓鍏ワ紙鏃犻渶 GPS锛?j = snprintf(..., ",\"tmst\":%u", p->count_us);
// p->count_us = SX1302 INST 璁℃暟鍣紝甯ф帴鏀舵椂鍒?
if (ref_ok == true) {
    // 鈹€鈹€ time 瀛楁锛圲TC锛夆攢鈹€
    j = lgw_cnt2utc(local_ref, p->count_us, &pkt_utc_time);
    // 鍐呴儴璁＄畻锛歞elta_sec = (p->count_us - local_ref.count_us) / (1E6 * xtal_err)
    //          pkt_utc_time = local_ref.utc + delta_sec
    if (j == LGW_GPS_SUCCESS) {
        x = gmtime(&pkt_utc_time.tv_sec);
        snprintf(..., ",\"time\":\"%04i-%02i-%02iT%02i:%02i:%02i.%06liZ\"",
            x->tm_year+1900, x->tm_mon+1, x->tm_mday,
            x->tm_hour, x->tm_min, x->tm_sec,
            pkt_utc_time.tv_nsec / 1000);  // 绮剧‘鍒板井绉?    }

    // 鈹€鈹€ tmms 瀛楁锛圙PS 绾厓姣锛夆攢鈹€
    j = lgw_cnt2gps(local_ref, p->count_us, &pkt_gps_time);
    // 鍐呴儴璁＄畻锛歞elta_sec = (p->count_us - local_ref.count_us) / (1E6 * xtal_err)
    //          pkt_gps_time = local_ref.gps + delta_sec
    if (j == LGW_GPS_SUCCESS) {
        pkt_gps_time_ms = pkt_gps_time.tv_sec * 1000
                        + pkt_gps_time.tv_nsec / 1000000;
        // = 1456477117685 ms锛堜慨澶嶅悗锛?        snprintf(..., ",\"tmms\":%" PRIu64 "", pkt_gps_time_ms);
    }
}
// ref_ok == false 鏃讹細time 鍜?tmms 瀛楁鍧囦笉鍐欏叆 JSON
```

### 9.1 time 瀛楁鏁板€奸獙璇侊紙瀹為檯鏃ュ織锛?
```
INFO: [gps] synced UTC time: 2026-03-02T08:58:19.000Z  (trig_tstamp=201660610)

鏀跺埌甯э細p->count_us = 202346228
鍚屾鐐癸細local_ref.count_us = 201660610锛宭ocal_ref.utc.tv_sec = 2026-03-02T08:58:19

delta_us = 202346228 - 201660610 = 685618 碌s
delta_sec = 685618 / 1E6 / 1.0 = 0.685618 s

pkt_utc_time = 2026-03-02T08:58:19 + 0.685618 = 2026-03-02T08:58:19.685618

JSON: "time":"2026-03-02T08:58:19.685616Z"  鉁咃紙涓庢棩蹇椾竴鑷达紝卤2碌s 璇樊锛?```

### 9.2 tmms 瀛楁鏁板€奸獙璇?
```
local_ref.gps.tv_sec = 1456476317锛堟潵鑷?lgw_gps_sync锛屼慨澶嶅悗姝ｇ‘锛?delta_sec = 0.685618 s

pkt_gps_time.tv_sec = 1456476317 + 0 = 1456476317锛坉elta 鏁存暟閮ㄥ垎涓?0锛?pkt_gps_time.tv_nsec = 685618000 ns

pkt_gps_time_ms = 1456476317 * 1000 + 685618000 / 1000000
                = 1456476317000 + 685
                鈮?1456477117685 ms锛堟敞鎰忓悓姝ョ偣姣忔鏇存柊锛?
JSON: "tmms":1456477117685  鉁?```

---

## 鍗併€佷唬鐮佽В鏋愶細thread_down 涓嬭鏃堕棿璋冨害

> 婧愭枃浠讹細`lora_pkt_fwd.c`锛屽嚱鏁?`thread_down()`

### 10.1 Class A锛氫娇鐢?tmst锛堣鏁板櫒缁濆鍊硷級

NS 鍦ㄦ敹鍒颁笂琛屽悗锛岃绠?RX1/RX2 绐楀彛鐨勮鏁板櫒鍊煎苟涓嬪彂 `tmst` 瀛楁锛?
```json
// NS 涓嬪彂鐨?txpk锛圕lass A锛孯X1 绐楀彛锛?{"txpk":{"tmst":203346228,"freq":507.7,...}}
```

```c
// thread_down() 澶勭悊锛?val = json_object_get_value(txpk_obj, "tmst");
if (val != NULL) {
    txpkt.count_us = (unsigned int)json_value_get_number(val);
    // = 203346228锛堢洿鎺ヤ綔涓?SX1302 鍙戝皠鏃跺埢锛?    downlink_type = JIT_PKT_TYPE_DOWNLINK_CLASS_A;
}
// JIT 闃熷垪鍦?count_us = 203346228 鏃惰Е鍙?lgw_send()
```

RX1 鏃跺埢璁＄畻锛堢敱 NS 瀹屾垚锛岀綉鍏虫棤闇€鍙備笌锛夛細
```
RX1_tmst = uplink_tmst + RX1Delay 脳 1E6
         = 202346228 + 1 脳 1,000,000
         = 203346228
```

### 10.2 Class B锛氫娇鐢?tmms锛圙PS 绾厓姣锛?
```c
val = json_object_get_value(txpk_obj, "tmms");
if (val != NULL) {
    uint64_t x2 = (uint64_t)json_value_get_number(val);

    // 姣 鈫?struct timespec
    double x3, x4;
    x3 = modf((double)x2 / 1E3, &x4);
    gps_tx.tv_sec  = (time_t)x4;        // 鏁存暟绉掗儴鍒?    gps_tx.tv_nsec = (long)(x3 * 1E9);  // 灏忔暟绉掗儴鍒嗭紙绾崇锛?
    // GPS 绾厓鏃堕棿 鈫?SX1302 璁℃暟鍣ㄥ€?    i = lgw_gps2cnt(local_ref, gps_tx, &txpkt.count_us);
    // 鍐呴儴锛歞elta = gps_tx - ref.gps
    //       count_us = ref.count_us + delta 脳 1E6 脳 xtal_err
    downlink_type = JIT_PKT_TYPE_DOWNLINK_CLASS_B;
}
```

Class B 蹇呴』 `gps_ref_valid == true`锛屽惁鍒欑綉鍏宠繑鍥?`JIT_ERROR_GPS_UNLOCKED` 鎷掔粷鍙戦€併€?
---

## 鍗佷竴銆佹椂闂磋浆鎹㈠嚱鏁拌瑙?
`loragw_gps.c` 鎻愪緵鍥涗釜鏃堕棿杞崲鍑芥暟锛屾瀯鎴愬弻鍚戞槧灏勶細

```
SX1302 璁℃暟鍣?(count_us, uint32_t)
        鈫?  lgw_cnt2utc()  鈫愨啋  lgw_utc2cnt()
        鈫?  UTC (struct timespec锛孶nix 绾厓 1970-01-01)

SX1302 璁℃暟鍣?(count_us, uint32_t)
        鈫?  lgw_cnt2gps()  鈫愨啋  lgw_gps2cnt()
        鈫?  GPS 绾厓鏃堕棿 (struct timespec锛孏PS 绾厓 1980-01-06)
```

### 11.1 lgw_cnt2utc()

```c
int lgw_cnt2utc(struct tref ref, unsigned int count_us, struct timespec *utc) {
    // 鏈夋晥鎬ф鏌ワ細鍙傝€冨繀椤昏鍒濆鍖栬繃锛屼笖 xtal_err 鍦ㄥ悎鐞嗚寖鍥村唴
    if (ref.systime == 0 || ref.xtal_err > PLUS_10PPM || ref.xtal_err < MINUS_10PPM)
        return LGW_GPS_ERROR;

    // delta_sec锛氫粠鍙傝€冩椂鍒诲埌鐩爣鏃跺埢鐨勬椂闂村樊锛堝凡鍋氭櫠鎸牎姝ｏ級
    double delta_sec = (double)(count_us - ref.count_us) / (TS_CPS * ref.xtal_err);
    // 娉細count_us 鍜?ref.count_us 閮芥槸 uint32_t锛屽噺娉曡嚜鍔ㄥ鐞?32-bit 鍥炵粫

    double intpart, fractpart;
    fractpart = modf(delta_sec, &intpart);

    long tmp = ref.utc.tv_nsec + (long)(fractpart * 1E9);
    if (tmp < (long)1E9) {
        utc->tv_sec  = ref.utc.tv_sec + (time_t)intpart;
        utc->tv_nsec = tmp;
    } else {  // 绾崇杩涗綅
        utc->tv_sec  = ref.utc.tv_sec + (time_t)intpart + 1;
        utc->tv_nsec = tmp - (long)1E9;
    }
    return LGW_GPS_SUCCESS;
}
```

### 11.2 lgw_cnt2gps()

涓?`lgw_cnt2utc()` 瀹屽叏瀵圭О锛屽彧鏄熀鍑嗕粠 `ref.utc` 鎹㈡垚 `ref.gps`锛?
```c
int lgw_cnt2gps(struct tref ref, unsigned int count_us, struct timespec *gps_time) {
    double delta_sec = (double)(count_us - ref.count_us) / (TS_CPS * ref.xtal_err);

    // 浠?ref.gps 涓哄熀鍑嗭紙淇鍓?ref.gps = {0,0}锛屼慨澶嶅悗涓烘纭?GPS 绾厓鏃堕棿锛?    long tmp = ref.gps.tv_nsec + (long)(fractpart * 1E9);
    gps_time->tv_sec  = ref.gps.tv_sec + (time_t)intpart [+ 杩涗綅];
    gps_time->tv_nsec = tmp [- 1E9 杩涗綅];
}
```

### 11.3 涓変釜鏃堕棿绾厓鐨勬崲绠楀叧绯?
```
Unix 绾厓   1970-01-01 00:00:00 UTC
            鈫?+315964800 绉掞紙绾?10 骞达紝绮剧‘鍒扮锛?GPS 绾厓    1980-01-06 00:00:00 GPS
            鈫?GPS = UTC + 18 绉掞紙闂扮锛?017骞磋嚦浠婃湁鏁堬級

鍏紡姹囨€伙細
  GPS_sec   = Unix_sec - 315964800 + 18
  Unix_sec  = GPS_sec  + 315964800 - 18
  tmms (ms) = GPS_sec 脳 1000 + GPS_nsec / 1,000,000

鏁板€肩ず渚嬶紙2026-03-02T08:58:19 UTC锛夛細
  Unix_sec  = 1772441099
  GPS_sec   = 1772441099 - 315964800 + 18 = 1456476317
  tmms 鈮?1456476317 脳 1000 + 685 = 1456476317685
```

---

## 鍗佷簩銆丆hirpStack 灞傦細NS 瑙嗚鐨勬椂闂村瓧娈?
ChirpStack 鏀跺埌缃戝叧涓婃姤鐨?`rxpk` JSON 鍚庯紝鍦ㄤ笂琛屼簨浠朵腑鏆撮湶浠ヤ笅鏃堕棿瀛楁锛?
| 瀛楁 | 鏉ユ簮 | 璇存槑 |
|------|------|------|
| `gwTime` | rxpk `time` 瀛楁锛堢綉鍏虫祴閲忥級 | 缃戝叧娴嬮噺鐨勫抚鍒拌揪 UTC 鏃堕棿锛岀簿纭埌寰 |
| `nsTime` | ChirpStack 鏈嶅姟鍣ㄧ郴缁熸椂閽?| NS 鏀跺埌鍖呯殑鏃跺埢锛堜笌 gwTime 鏈夊嚑鍗?ms 宸級 |
| `timeSinceGpsEpoch` | rxpk `tmms` 瀛楁鎹㈢畻 | `tmms / 1000` 绉掞紝鐢ㄤ簬 Class B 璋冨害 |

瀹炴祴鏁版嵁锛圔UG-011 淇鍚庯紝2026-03-02锛夛細

```
gwTime:             2026-03-02T08:58:41.731513+00:00
nsTime:             2026-03-02T08:58:41.707673068+00:00
timeSinceGpsEpoch:  1456477139.731s
```

`gwTime - nsTime 鈮?24 ms`锛歎DP 鍗曞悜浼犺緭寤惰繜锛屾甯歌寖鍥淬€?
**timeSinceGpsEpoch 鎹㈢畻楠岃瘉**锛?
$$\frac{1456477139}{86400 \times 7} = 2409.28\ \text{GPS 鍛▆$$

$$1980\text{-}01\text{-}06 + 2409.28\ \text{鍛▆ \approx 2026\text{-}03\text{-}02\ \checkmark$$

---

## 鍗佷笁銆丳PS 鐨勪綔鐢ㄤ笌缂哄け鐨勫奖鍝?
### 13.1 鏈?PPS 鏃剁殑鍚屾绮惧害

```
GPS PPS 涓婂崌娌匡紙缁濆绮惧害 卤100 ns锛屽彈鍗槦淇″彿璐ㄩ噺褰卞搷锛?       鈫?SX1302 纭欢妫€娴嬩笂鍗囨部锛屽皢 INST 璁℃暟鍣ㄥ€煎啓鍏?PPS 瀵勫瓨鍣?  TIMESTAMP_PPS = TIMESTAMP_INST锛堥攣瀛樼灛闂达紝鍚屾绮惧害 卤1 碌s锛?       鈫?lgw_get_trigcnt(&trig_tstamp) 璇诲嚭蹇収鍊?       鈫?gps_process_sync() 璋冪敤 lgw_gps_sync(trig_tstamp, utc, gps_time)
  寤虹珛閿氱偣锛歝ount_us = trig_tstamp 瀵瑰簲 utc锛圲TC 鏁寸锛?
绮惧害鍒嗘瀽锛?  trig_tstamp 璇樊锛毬? 碌s锛圫X1302 璁℃暟鍣ㄥ垎杈ㄧ巼锛?  utc 璇樊锛?      卤5 ms锛圢MEA 瀛楃浠?GPS 浼犳潵鐨勪覆鍙ｅ欢杩燂級
  缁煎悎绮惧害锛氫互璁℃暟鍣ㄤ负鍑嗭紝绾?卤1 碌s
  瀹炴祴楠岃瘉锛氱浉閭?trig_tstamp 宸€?= 1,000,001 碌s锛堝嚑涔庣簿纭?1 绉掞級
```

### 13.2 鏃?PPS 鏃剁殑閫€鍖栬涓?
```
TIMESTAMP_PPS 瀵勫瓨鍣ㄤ粠涓嶆洿鏂?       鈫?lgw_get_trigcnt() 杩斿洖 0锛堝瘎瀛樺櫒鍒濆鍊硷級
       鈫?gps_process_sync() 涓細
  cnt_diff = (0 - 0) = 0  (绗竴娆?
  slope = 0 / 1 = 0  鈫?寮傚父锛? MINUS_10PPM锛?
  姣忔 lgw_gps_sync 閮戒細锛?    绗?娆″紓甯革細淇濈暀鏃у弬鑰?    绗?娆″紓甯革細淇濈暀鏃у弬鑰?    绗?娆″紓甯革紙杩炵画锛夛細寮哄埗閲嶇疆
      ref.count_us = 0
      ref.utc = 褰撳墠 NMEA UTC
      ref.gps = gps_time锛堜慨澶嶅悗姝ｇ‘锛屼慨澶嶅墠 {0,0}锛?       鈫?涔嬪悗姣忔閮介噸澶嶄笂闈㈣繃绋嬶紝ref.count_us 涓€鐩存槸 0锛宺ef.utc 涓€鐩存洿鏂?
鏁堟灉锛坮ef.count_us = 0锛夛細
  delta_sec = count_us / 1E6锛堣澶囪繍琛屾椂闂达級
  time = ref.utc + delta锛圲TC 鍩哄噯姣忕鏇存柊锛屾墍浠?time 浠嶇劧姝ｇ‘锛?  tmms = ref.gps + delta
       = gps_time + delta锛堜慨澶嶅悗 gps_time 姝ｇ‘锛?       = gps_time + 璁惧杩愯鏃堕棿锛堟湁鍑犵鐨勭粷瀵硅宸紝浣嗛噺绾ф纭級
```

### 13.3 PPS 鍋ュ悍妫€鏌ユ柟娉?
```
杩炴帴 PPS锛?  trig_tstamp[n+1] - trig_tstamp[n] 鈮?1,000,000 碌s锛埪?0 碌s 鍐咃級

鏈繛鎺?PPS锛?  trig_tstamp 涓嶅彉 鈫?delta = 0

璇婃柇鍛戒护锛堜粠涓插彛鏃ュ織锛夛細
  "INFO: [gps] synced UTC time: ...  (trig_tstamp=201660610)"
  鐪嬬浉閭讳袱琛岀殑 trig_tstamp 宸€?```

---

## 鍗佸洓銆丅UG-011 鏍瑰洜涓庝慨澶?
### 14.1 闂閾?
```
ATGM336H 鏄?NMEA-only 妯″潡锛堟棤 u-blox 涓撴湁鍗忚锛?       鈫?锛堜粠涓嶈緭鍑?UBX_NAV_TIMEGPS锛?loragw_gps.c 涓殑闈欐€佸叏灞€鍙橀噺锛?  gps_week = 0    鈫?鍙敱 UBX NAV-TIMEGPS 娑堟伅鏇存柊
  gps_iTOW = 0    鈫?鍚屼笂
  gps_fTOW = 0    鈫?鍚屼笂
锛堣繖涓変釜鍙橀噺鍦ㄨ澶囨暣涓繍琛屾湡闂存案杩滄槸 0锛?       鈫?lgw_gps_get() 鐨?gps_time 鍒嗘敮锛堜慨澶嶅墠锛夛細
  gps_time->tv_sec  = iTOW/1000 + week*604800 = 0 + 0 = 0
  gps_time->tv_nsec = 0
  鈫?gps_time = {0, 0}锛圙PS 绾厓 1980-01-06 00:00:00锛?       鈫?lgw_gps_sync() 灏?{0, 0} 鍐欏叆 ref.gps锛?  time_reference_gps.gps = {0, 0}
       鈫?lgw_cnt2gps(ref, count_us, &pkt_gps_time)锛?  pkt_gps_time = {0, 0} + delta_sec
               = delta_sec锛堣澶囪繍琛屾椂闂达紝鍗曚綅绉掞級
       鈫?pkt_gps_time_ms = pkt_gps_time.tv_sec * 1000 + ...
                = delta_sec * 1000 ms
                鈮?1138 ms锛堣澶囪繍琛岀害 1.138 绉掓椂鏀跺埌鐨勫抚锛?       鈫?JSON: "tmms":1138
       鈫?ChirpStack: timeSinceGpsEpoch = "1.138s"
            鎹㈢畻 = 1980-01-06T00:00:01.138 GPS锛?980 骞达紒锛?```

### 14.2 淇閫昏緫

鍒ゆ柇渚濇嵁锛歚gps_week == 0` 鈫?NMEA-only 妯″紡锛屼粠 NMEA 鏃堕棿瀛楁娲剧敓 GPS 绾厓鏃堕棿锛?
```
NMEA $GNRMC 瑙ｆ瀽缁撴灉锛堝叏灞€鍙橀噺锛夛細
  gps_yea=26, gps_mon=3, gps_day=2, gps_hou=8, gps_min=58, gps_sec=19, gps_fra=0.685
       鈫?struct tm gps_tm = {.tm_year=126, .tm_mon=2, .tm_mday=2, .tm_hour=8, ...}
       鈫?gps_unix = mktime(&gps_tm) = 1772441099锛圲nix 鏃堕棿鎴筹級
       鈫?gps_time->tv_sec = 1772441099 - 315964800 + 18 = 1456476317
gps_time->tv_nsec = 685000000锛堟潵鑷?gps_fra锛?```

### 14.3 鏂板甯搁噺

```c
// loragw_gps.c 绉佹湁甯搁噺鍖猴紙淇涓柊澧烇級
#define UNIX_GPS_EPOCH_OFFSET  315964800  // 1970-01-01 鍒?1980-01-06 鐨勭鏁?#define GPS_LEAP_SECONDS       18         // GPS 姣?UTC 蹇?18 绉掞紙2017骞磋嚦浠婏級
```

杩欎袱涓父閲忓皢鍘熸潵鏁ｈ惤鍦ㄤ唬鐮佷腑鐨勯瓟娉曟暟瀛楅泦涓畾涔夛紝渚夸簬鏈潵锛堝闂扮璋冩暣鏃讹級缁存姢銆?
---

## 鍗佷簲銆佹椂闂寸郴缁熷父瑙侀棶棰?FAQ

**Q: tmst 鍜?tmms 鏈変粈涔堝尯鍒紵**

A: `tmst` 鏄?*鐩稿**鏃堕棿锛堢綉鍏充笂鐢靛悗鐨勫井绉掓暟锛夛紝`tmms` 鏄?GPS 绾厓**缁濆**鏃堕棿锛堟绉掞級銆?涓嶅悓缃戝叧鐨?`tmst` 鏃犳硶鐩存帴姣旇緝锛屼絾 `tmms` 鍙互鐢ㄤ簬澶氱綉鍏?TDOA 瀹氫綅銆?
---

**Q: 娌℃湁 GPS 鏃?time 瀛楁杩樻湁鍚楋紵**

A: `gps_ref_valid = false` 鏃?`time` 鍜?`tmms` 瀛楁鍧囦笉鍐欏叆 JSON銆?ChirpStack NS 浼氱敤 `nsTime`锛堟湇鍔″櫒鏀跺寘鏃跺埢锛変唬鏇裤€?
---

**Q: GPS 娌℃湁瀹氫綅浣嗘湁鏃堕棿淇″彿锛宼ime 瀛楁鏈夊悧锛?*

A: 鏈夈€俙gps_process_sync()`锛堟椂闂达級鍜?`gps_process_coords()`锛堝潗鏍囷級鏄嫭绔嬬殑銆?鍙 `$GNRMC` 涓?fix 鐘舵€佷负 `A`锛圓ctive锛夛紝`gps_time_ok=true`锛屾椂闂村弬鑰冨氨鑳藉缓绔嬨€?鍧愭爣淇℃伅鍙奖鍝?`stat` JSON 涓殑 `lati`/`long`/`alti` 瀛楁銆?
---

**Q: 闂扮 18 绉掍細鍙樺悧锛?*

A: 闂扮鐢?IERS 鍐冲畾锛屾渶杩戜竴娆℃槸 2017 骞?1 鏈?1 鏃ワ紙绗?18 娆★級銆?涓嬫璋冩暣锛堝鏈夛級闇€鎵嬪姩鏇存柊 `loragw_gps.c` 涓殑 `GPS_LEAP_SECONDS` 甯搁噺銆?IERS 鎻愬墠 6 涓湀鍙戝竷鍏憡锛屾埅鑷?2026 骞?18 绉掍粛姝ｇ‘銆?
---

**Q: xtal_err 鍋忕 1.0 浼氭€庢牱锛?*

A: `lgw_gps_sync()` 璁剧疆 卤10 ppm 瀹夊叏闃堝€硷紙`PLUS_10PPM=1.00001`锛宍MINUS_10PPM=0.99999`锛夈€?瓒呰繃鑼冨洿鐨勭偣琚爣璁颁负"寮傚父"锛岃繛缁?3 娆″紓甯稿悗寮哄埗閲嶇疆銆?姝ｅ父鏅舵尟鍋忓樊绾?卤2~5 ppm锛岃繙鍦ㄥ畨鍏ㄨ寖鍥村唴銆?
---

**Q: Class A 鑺傜偣鍏ョ綉闇€瑕?GPS 鏃堕棿鍚楋紵**

A: 涓嶉渶瑕併€侰lass A 鐨勪笅琛岀獥鍙ｇ敤 `tmst`锛堢浉瀵规椂闂达級璋冨害锛?`RX1_count_us = uplink_tmst + 1,000,000`锛? 绉掑悗锛夈€?GPS 鏃堕棿鍙奖鍝?`time`/`tmms` 涓婃姤瀛楁浠ュ強 Class B/Class C GPS 璋冨害銆?
---

**Q: 涓轰粈涔?time 瀛楁绮剧‘鍒板井绉掕€?NMEA 鍙湁姣锛?*

A: `time` 瀛楁鐨勭簿搴︽潵鑷?SX1302 璁℃暟鍣紙碌s 绾э級锛屼笉鏄?NMEA 瀛楃涓层€?璁＄畻璺緞鏄細`delta_us = p->count_us - ref.count_us`锛埪祍 绮惧害锛夛紝
鍐嶅姞鍒?`ref.utc`锛坢s 绮惧害鐨?NMEA 鏃堕棿锛変笂锛屽緱鍒?碌s 绾х殑缁濆鏃堕棿銆?瀹為檯涓?NMEA 鏃堕棿鐨?ms 绮惧害鏄敋鐐硅宸紝碌s 绾х殑鐩稿绮惧害鏉ヨ嚜鏅舵尟銆?
---

## 鐩稿叧鏂囨。

- [BUG-011 淇璁板綍](../bugfix/BUG-011_tmms_wrong_gps_epoch_nmea_only.md)
- [GPS PPS + tmms 楠岃瘉娴嬭瘯](../test_notes/test_gps_pps_tmms_chirpstack_memo.md)
- [ESXP1302 浠ｇ爜鍏ㄨ矊](esxp1302_code_walkthrough.md)
