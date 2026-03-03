# BUG-009锛氱綉鍏?Web 閰嶇疆椤甸潰杩斿洖 HTTP 431 Request Header Fields Too Large

- **鏃ユ湡**锛?026-02-20  
- **鏂囦欢**锛歚sdkconfig`銆乣sdkconfig.defaults`  
- **涓ラ噸绾у埆**锛氬姛鑳芥€ч敊璇紙Web 閰嶇疆鐣岄潰鏃犳硶璁块棶锛?
---

## 鐜拌薄

鍦ㄦ祻瑙堝櫒涓闂綉鍏?Web 閰嶇疆椤甸潰锛坄http://<璁惧IP>/`锛夋椂锛屾湇鍔″櫒杩斿洖锛?
```
431 Request Header Fields Too Large
```

鐜颁唬娴忚鍣ㄥ彂閫佺殑 HTTP 璇锋眰澶达紙鍖呭惈 Host銆乁ser-Agent銆丄ccept銆丄uthorization 绛夛級鎬婚暱搴﹂€氬父瓒呰繃 512 瀛楄妭銆?
---

## 鏍规湰鍘熷洜

ESP-IDF `esp_http_server` 缁勪欢鐨勬渶澶ц姹傚ご闀垮害鐢?Kconfig 閫夐」 `CONFIG_HTTPD_MAX_REQ_HDR_LEN` 鎺у埗锛岄粯璁ゅ€间负 **512 瀛楄妭**銆?
`httpd_config_t` 缁撴瀯浣?*娌℃湁** `max_req_hdr_len` 杩愯鏃跺瓧娈碉紝璇ラ檺鍒跺彧鑳介€氳繃 Kconfig 鍦ㄧ紪璇戞椂閰嶇疆銆?
---

## 閿欒灏濊瘯

灏濊瘯鍦?`http_server.c` 涓€氳繃缁撴瀯浣撳瓧娈佃鐩栵細

```c
httpd_config_t config = HTTPD_DEFAULT_CONFIG();
config.max_req_hdr_len = 2048;   // 鈫?缂栬瘧鎶ラ敊锛歴truct 鏃犳鎴愬憳
```

璇ュ瓧娈典笉瀛樺湪锛屾鏂规涓嶅彲琛屻€?
---

## 淇

鍦?`sdkconfig.defaults` 涓鍔狅細

```
CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048
```

鍚屾椂鍦?`sdkconfig` 涓皢瀵瑰簲椤逛粠 512 鏀逛负 2048锛岄噸鏂扮紪璇戠儳褰曞悗 Web 閰嶇疆椤甸潰鍙甯歌闂€?
---

## 楠岃瘉

淇鍚庢祻瑙堝櫒璁块棶 `http://<璁惧IP>/`锛屾甯告樉绀虹櫥褰曟鍙婇厤缃〉闈紝HTTP Basic Auth锛坄iot` / `lora`锛夐€氳繃锛岄厤缃繚瀛樺姛鑳芥甯搞€?
