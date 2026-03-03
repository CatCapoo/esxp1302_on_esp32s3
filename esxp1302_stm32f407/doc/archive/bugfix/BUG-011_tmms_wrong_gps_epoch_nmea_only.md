# BUG-011锛歵mms 瀛楁杩斿洖 1980 GPS 绾厓闄勮繎鏃堕棿锛圢MEA-only 妯″潡锛?
- **鏃ユ湡**锛?026-03-02  
- **鏂囦欢**锛歚main/libloragw/loragw_gps.c`  
- **涓ラ噸绾у埆**锛氬姛鑳芥€ч敊璇紙ChirpStack `timeSinceGpsEpoch` 鏄剧ず涓?1.138s/523s锛屽簲涓?~1456477139s锛? 
- **commit**锛歚70eb10d` fix: derive GPS time from NMEA UTC for NMEA-only modules (ATGM336H)

---

## 鐜拌薄

PPS 宸叉帴鍏ュ苟姝ｅ父宸ヤ綔锛坄trig_tstamp` 姣忕鏇存柊锛宍delta=1000001`锛夛紝
浣?ChirpStack NS 鐨勪笂琛屼簨浠朵腑锛?
```
timeSinceGpsEpoch: "1.138s"         鈫?搴斾负绾?1456477139s锛?026-03-02锛?gwTime: "2026-03-02T08:58:41Z"      鈫?姝ｅ父锛堟潵鑷郴缁熸椂閽燂紝涓嶄緷璧?GPS 鏃堕棿锛?```

缃戝叧涓婅 JSON 涓?`tmms` 瀛楁锛?
```json
{"tmms": 523537, ...}   鈫?搴斾负绾?1456477117685锛?3 浣嶆绉掓暟锛?```

`523537 ms 梅 1000 = 523 s`鈥斺€旀濂界瓑浜庤澶囧惎鍔ㄥ悗鐨勮繍琛屾椂闂达紝鑰岄潪 GPS 绾厓缁濆鏃堕棿銆?
---

## 鏍规湰鍘熷洜

`loragw_gps.c` 涓淮鎶や袱濂楃嫭绔嬬殑鍏ㄥ眬鏃堕棿鍙橀噺锛?
| 鍙橀噺 | 鐢辫皝鏇存柊 | ATGM336H 鏄惁鏇存柊 |
|------|----------|------------------|
| `gps_yea/mon/day/hou/min/sec` | NMEA `$GNRMC` 瑙ｆ瀽 | 鉁?姣忕鏇存柊 |
| `gps_week` / `gps_iTOW` / `gps_fTOW` | UBX `NAV-TIMEGPS` 瑙ｆ瀽 | 鉂?姘歌繙涓?0 |

鍘熷 `lgw_gps_get()` 鐨?`gps_time` 鍒嗘敮锛堜慨澶嶅墠锛夛細

```c
/* 淇鍓嶏細鐩存帴浣跨敤 gps_week/iTOW/fTOW 鍙橀噺 */
fractpart = modf(((double)gps_iTOW / 1E3) + ((double)gps_fTOW / 1E9), &intpart);
gps_time->tv_sec  = (time_t)intpart;
gps_time->tv_sec += (time_t)gps_week * 604800;   // gps_week=0 鈫?tv_sec=0
gps_time->tv_nsec = (long)(fractpart * 1E9);
```

ATGM336H 鏄?NMEA-only 妯″潡锛堟棤 u-blox UBX 鍗忚锛夛紝`gps_week` 濮嬬粓涓?0锛?`gps_iTOW` 濮嬬粓涓?0锛屽鑷?`gps_time = {0, 0}`锛圙PS 绾厓 1980-01-06锛夈€?
`lgw_gps_sync()` 灏嗘 `{0,0}` 鍐欏叆 `time_reference_gps.gps`锛?闅忓悗 `lgw_cnt2gps()` 璁＄畻 `tmms` 鏃讹細

```
gps_time = ref.gps + delta_from_ref
         = {0, 0}  + (count_us - ref.count_us) / TS_CPS
         鈮?璁惧杩愯鏃堕棿锛堢锛?```

**缁撹**锛歚tmms` 鈮?璁惧杩愯鏃堕棿 脳 1000锛堟绉掞級锛岃€岄潪 GPS 绾厓缁濆姣鏁般€?
### 璋冭瘯楠岃瘉

鍔犲叆 debug 鏃ュ織鍚庯紝浠ヤ笅鏁版嵁浜掔浉鍚诲悎锛?
```
trig_tstamp = 201660610
tmst (uplink count_us) = 202346228
delta from ref = 202346228 - 201660610 = 685618 碌s = 685 ms
tmms = 1138 ms  鈫?璇存槑 ref.gps.tv_sec = 0锛宼mms 鍙湁 delta 鍒嗛噺
```

---

## 淇鏂规

鍦?`lgw_gps_get()` 涓紝浠?`gps_week` 鏄惁涓洪潪闆跺€兼潵鍖哄垎 UBX 妯″紡鍜?NMEA-only 妯″紡锛?
```c
if (gps_week != 0) {
    /* UBX 妯″紡锛氫娇鐢?iTOW + week锛堝師閫昏緫锛岀簿搴﹂珮锛岀撼绉掔骇锛?*/
    fractpart = modf(((double)gps_iTOW / 1E3) + ((double)gps_fTOW / 1E9), &intpart);
    gps_time->tv_sec  = (time_t)intpart + (time_t)gps_week * 604800;
    gps_time->tv_nsec = (long)(fractpart * 1E9);
} else {
    /* NMEA-only 妯″紡锛氫粠 NMEA 鏃ユ湡鏃堕棿瀛楁娲剧敓 GPS 鏃堕棿
       鍏紡锛欸PS_time = UTC_unix - UNIX_GPS_EPOCH_OFFSET + GPS_LEAP_SECONDS */
    struct tm gps_tm = {0};
    gps_tm.tm_year = (gps_yea < 100) ? gps_yea + 100 : gps_yea - 1900;
    gps_tm.tm_mon  = gps_mon - 1;
    gps_tm.tm_mday = gps_day;
    gps_tm.tm_hour = gps_hou;
    gps_tm.tm_min  = gps_min;
    gps_tm.tm_sec  = gps_sec;
    time_t gps_unix = mktime(&gps_tm);
    gps_time->tv_sec  = gps_unix - UNIX_GPS_EPOCH_OFFSET + GPS_LEAP_SECONDS;
    gps_time->tv_nsec = (int32_t)(gps_fra * 1e9);
}
```

鍚屾鏂板涓や釜鍏峰悕甯搁噺锛堝彇浠ｉ瓟娉曟暟瀛楋級锛?
```c
#define UNIX_GPS_EPOCH_OFFSET   315964800   /* 1970-01-01 鍒?1980-01-06 鐨勭鏁板樊 */
#define GPS_LEAP_SECONDS        18          /* GPS-UTC 闂扮鍋忕Щ锛?017 骞磋嚦浠婃湁鏁堬級 */
```

---

## 鏁堟灉

| 瀛楁 | 淇鍓?| 淇鍚?|
|------|--------|--------|
| `tmms` | `523537`锛坢s锛岀害 8 鍒嗛挓锛?| `1456477117685`锛坢s锛?026-03-02锛?|
| `timeSinceGpsEpoch` | `"1.138s"` | `"1456477139.731s"` |
| `gwTime` | `"2026-03-02T..."` 鉁咃紙涓嶅彈褰卞搷锛?| `"2026-03-02T..."` 鉁?|

鎹㈢畻楠岃瘉锛?
```
gps_utc_unix = 1772441099  锛?026-03-02T08:58:19 UTC 鐨?Unix 鏃堕棿鎴筹級
gps_time.tv_sec = 1772441099 - 315964800 + 18 = 1456476317

tmms = (gps_time.tv_sec + delta) 脳 1000 + ms
     鈮?1456477117685 鉁?```

---

## 閫傜敤鑼冨洿

姝や慨澶嶄笓涓?NMEA-only GPS 妯″潡璁捐锛堝 ATGM336H銆佷腑绉戝井 AT6558 绯诲垪绛夛級锛?瀵?u-blox 妯″潡锛坄gps_week != 0`锛変笉浜х敓浠讳綍褰卞搷锛屽師閫昏緫瀹屾暣淇濈暀銆?
---

## 鐩稿叧鏂囦欢

- `main/libloragw/loragw_gps.c`锛堜富瑕佷慨鏀癸級
- `main/board_config.h`锛堝悓姝ユ洿鏂帮紝鏂板 GPS 鐩稿叧閰嶇疆娉ㄩ噴锛?- 娴嬭瘯璁板綍锛歔test_gps_pps_tmms_chirpstack_memo.md](../test_notes/test_gps_pps_tmms_chirpstack_memo.md)
- 瀛︿範绗旇锛歔esxp1302_timestamp_system.md](../learning/esxp1302_timestamp_system.md)
