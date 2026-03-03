# Windows 鏋勫缓宸ュ叿杩涘睍澶囧繕褰?
**鏃ユ湡锛?* 2026-02-22  
**鐩殑锛?* 鍦?Windows 涓婃浛浠?`run_me.sh`锛屽疄鐜颁竴鏉″懡浠ゅ畬鎴?鏇存柊閰嶇疆澶存枃浠?+ 缂栬瘧"锛?褰诲簳閬垮厤鍥?`global_json.h` 鏈洿鏂板鑷村浐浠朵笌 JSON 閰嶇疆涓嶄竴鑷寸殑闂銆?
---

## 涓€銆佽儗鏅笌闂

### 鍘熸湁娴佺▼锛圠inux only锛?
```
淇敼 global_conf.*.json
    鈫?./run_me.sh make
    鈹溾攢 scripts/json_to_hex_array.py 鈫?main/global_json.h锛坔ex 瀛楄妭鏁扮粍锛?    鈹溾攢 scripts/dump_html.py         鈫?main/packet_forwarder/webpage.h
    鈹斺攢 idf.py -DCONFIG_LIBLORAGW_TEST=0 app
```

`global_json.h` 鏄笁涓湴鍖?JSON 閰嶇疆**缂栬瘧杩涘浐浠?*鐨勫瓧鑺傛暟缁勶紝杩愯鏃舵墠瑙ｆ瀽銆? 
Windows 鏃犳硶鐩存帴杩愯 `.sh`锛屼互鍓嶅彧鑳界敤 WSL 鎴栨墜鍔ㄨ皟鐢ㄥ悇鑴氭湰銆?
### 鍙戠幇鐨勫叿浣?Bug

鏃ュ織鏄剧ず锛?```
INFO: upstream PUSH_DATA time-out is configured to 1800 ms
```
鑰?`global_conf.cn490.json` 涓槑鏄庡凡鏀逛负 `"push_timeout_ms": 500`銆?
鏍规湰鍘熷洜锛歚global_json.h` 鏄?`run_me.sh` 鐨勪骇鐗╋紝**鍦?Windows 涓婄洿鎺ユ敼 JSON 鍚庤嫢涓嶉噸鏂扮敓鎴愯鏂囦欢骞堕噸缂栬瘧锛屾敼鍔ㄤ笉浼氱敓鏁?*銆傚浐浠跺唴鐑у綍鐨勪粛鏄棫鍊?`1800`銆?
---

## 浜屻€佽В鍐虫柟妗?
### 鏂板鑴氭湰

| 鏂囦欢 | 璇存槑 |
|------|------|
| `run_me.py` | 瀹屾暣鏇夸唬 `run_me.sh`锛屾敮鎸?make / make_all / flash / flash_all / run |
| `scripts/gen_global_json.py` | 鍗曠嫭閲嶆柊鐢熸垚 `main/global_json.h` 鐨勭嫭绔嬪伐鍏?|

### run_me.py 鐢ㄦ硶

鍦?**ESP-IDF Terminal** 涓墽琛岋細

```powershell
# 缂栬瘧锛堟渶甯哥敤锛?python run_me.py make

# 瀹屾暣缂栬瘧锛堝惈 bootloader锛?python run_me.py make_all

# 鐑у綍 app
python run_me.py flash --port COM3

# 鐑у綍鍏ㄩ儴锛堥娆＄儳褰曟垨 bootloader 鏈夊彉鍔級
python run_me.py flash_all --port COM3

# 鐑у綍鍚庣珛鍗虫墦寮€鐩戣鍣?python run_me.py flash run --port COM3
```

> `--port` 榛樿 `COM3`锛宍--baud` 榛樿 `921600`銆?
### gen_global_json.py 鐢ㄦ硶

鍙渶鏇存柊閰嶇疆澶存枃浠惰€屼笉閲嶆柊缂栬瘧鏃讹細

```powershell
python scripts/gen_global_json.py
```

杈撳嚭绀轰緥锛?```
Generated: D:\...\main\global_json.h
  global_cn_conf: push_timeout_ms": 500,
  global_eu_conf: push_timeout_ms": 100,
  global_us_conf: push_timeout_ms": 100,
```

---

## 涓夈€丗lash 鍙傛暟淇

鍘?`run_me.sh` 鐨?flash 鍙傛暟閽堝鏃?ESP32锛屽鏈」鐩?ESP32-S3 鏈夎锛宍run_me.py` 宸蹭慨姝ｏ細

| 鍙傛暟 | run_me.sh锛堥敊璇級 | run_me.py锛堟纭級 |
|------|-----------------|-----------------|
| `--chip` | `esp32` | `esp32s3` |
| Bootloader 鍦板潃 | `0x1000` | `0x0` |
| `--flash_freq` | `40m` | `80m` |
| `--flash_size` | `detect` | `16MB` |

鍙傛暟鏉ユ簮锛歚build/flasher_args.json`锛圕Make 鐢熸垚锛屽缁堜笌瀹為檯纭欢閰嶇疆涓€鑷达級銆?
---

## 鍥涖€佹敞鎰忎簨椤?
1. **蹇呴』鍦?ESP-IDF Terminal 涓繍琛?*锛屾櫘閫?PowerShell 娌℃湁 `idf.py` / `esptool.py` 鐜銆?2. **姣忔淇敼 JSON 閰嶇疆鍚庡姟蹇呴噸鏂拌繍琛?`python run_me.py make`**锛屼笉鑳藉彧鏀?JSON 灏辩儳褰曘€?3. `main/packet_forwarder/global_json.h` 宸茶 `.gitignore` 蹇界暐锛堟纭紝灞炰簬鐢熸垚浜х墿锛夛紱  
   `main/global_json.h` 闇€瑕佹彁浜わ紙鍘嗗彶鎯緥锛屽弬涓庣紪璇戯級銆?4. 鑻ュ垏鎹㈠垎鏀悗鍑虹幇鏃ュ織鍊间笌閰嶇疆涓嶇锛岀涓€鍙嶅簲鏄鏌?`global_json.h` 鏄惁杩囨湡锛岃繍琛?`gen_global_json.py` 鍐嶉噸缂栧嵆鍙€?
---

## 浜斻€佹彁浜よ褰?
| Commit | 璇存槑 |
|--------|------|
| `2ff4f9b` | `build: add run_me.py to replace run_me.sh on Windows` |

---

## 鍏€佸緟鍔?/ 鍚庣画鏀硅繘

- [ ] 鑰冭檻鍦?CMakeLists.txt 涓坊鍔?cmake custom target锛岃嚜鍔ㄥ湪缂栬瘧鍓嶈皟鐢?`gen_global_json.py`锛屽交搴曢槻姝㈤仐蹇?- [ ] `run_me.py flash` 鐨勪覆鍙ｅ彿鍙粠 `sdkconfig` 鎴栫幆澧冨彉閲忚鍙栵紝閬垮厤姣忔鎵嬪姩鎸囧畾
