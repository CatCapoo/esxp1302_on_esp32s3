# Windows 鏋勫缓鎸囧崡

> 鏈」鐩師濮嬫瀯寤鸿剼鏈负 `run_me.sh`锛屼粎閫傜敤浜?Linux/macOS銆? 
> `run_me.py` 鏄?Windows 涓嬬殑瀹屾暣绛変环鏇夸唬锛屽姛鑳姐€佽涓轰笌 `run_me.sh` 涓€鑷淬€?
---

## 鍓嶆彁鏉′欢

1. 瀹夎 [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/)锛堟帹鑽愰€氳繃瀹樻柟 Windows Installer锛夈€?2. 浣跨敤 **ESP-IDF Terminal**锛堝畨瑁呭悗寮€濮嬭彍鍗曞彲瑙侊級锛岃缁堢宸茶嚜鍔ㄥ鍑?`IDF_PATH`銆乣idf.py`銆乣esptool.py` 绛夋墍鏈夊繀瑕佺幆澧冨彉閲忋€? 
   **鏅€?PowerShell / cmd 涓嶅彲鐢ㄤ簬 `make` / `flash` 鍛戒护**銆?3. Python 3.10+锛圗SP-IDF 鑷甫锛屾棤闇€棰濆瀹夎锛夈€?
---

## 鍛戒护涓€瑙?
鍦?**ESP-IDF Terminal** 涓紝浜庨」鐩牴鐩綍鎵ц锛?
| 鍛戒护 | 绛変环 run_me.sh | 璇存槑 |
|------|----------------|------|
| `python run_me.py make` | `./run_me.sh make` | 鐢熸垚澶存枃浠?鈫?缂栬瘧 app锛堣烦杩囨祴璇曪級 |
| `python run_me.py make_all` | `./run_me.sh make_all` | 鐢熸垚澶存枃浠?鈫?瀹屾暣缂栬瘧锛堝惈 bootloader锛?|
| `python run_me.py flash --port COMx` | `./run_me.sh flash` | 浠呯儳褰?app |
| `python run_me.py flash_all --port COMx` | `./run_me.sh flash_all` | 鐑у綍 bootloader + app + 鍒嗗尯琛?|
| `python run_me.py run --port COMx` | `./run_me.sh run` | 鎵撳紑涓插彛鐩戣鍣?|
| `python run_me.py flash run --port COMx` | `./run_me.sh flash run` | 鐑у綍鍚庣珛鍗虫墦寮€鐩戣鍣?|

### 鍙€夊弬鏁?
| 鍙傛暟 | 榛樿鍊?| 璇存槑 |
|------|--------|------|
| `--port COMx` | `COM3` | 涓插彛鍙凤紝鏌ョ湅璁惧绠＄悊鍣ㄧ‘璁?|
| `--baud N` | `921600` | 鐑у綍娉㈢壒鐜?|

---

## 鐢熸垚鐨勫ご鏂囦欢

`make` / `make_all` 鎵ц鍓嶏紝鑴氭湰浼氳嚜鍔紙閲嶆柊锛夌敓鎴愪互涓嬩袱涓枃浠讹細

| 鏂囦欢 | 鏉ユ簮 | 璇存槑 |
|------|------|------|
| `main/packet_forwarder/webpage.h` | `webpage.html` | Web UI 椤甸潰鍐呭锛岀紪璇戣繘鍥轰欢 |
| `main/global_json.h` | `global_conf.cn490.json` / `eu868.json` / `us915.json` | 涓変釜鍦板尯鐨勯粯璁ら厤缃紝缂栬瘧杩涘浐浠?|

> **閲嶈**锛氭瘡娆′慨鏀?`global_conf.*.json` 鎴?`webpage.html` 鍚庯紝**蹇呴』閲嶆柊杩愯 `python run_me.py make`**锛屽惁鍒欐敼鍔ㄤ笉浼氱敓鏁堬紙鍥轰欢鐑у綍鐨勪粛鏄棫 hex 瀛楄妭鏁扮粍锛夈€? 
> 涔熷彲浠ュ崟鐙繍琛?`python scripts/gen_global_json.py` 鍙洿鏂?`global_json.h`銆?
---

## Flash 鍙傛暟璇存槑

閽堝鏈」鐩?ESP32-S3 纭欢锛宍run_me.py` 浣跨敤鐨?flash 鍙傛暟鏉ヨ嚜 `build/flasher_args.json`锛屼笌鍘?`run_me.sh` 涓拡瀵规棫 ESP32 鐨勫弬鏁?*涓嶅悓**锛?
| 鍙傛暟 | run_me.sh锛堟棫/閿欙級 | run_me.py锛堟纭級 |
|------|-------------------|-----------------|
| `--chip` | `esp32` | `esp32s3` |
| Bootloader 鍦板潃 | `0x1000` | `0x0` |
| `--flash_freq` | `40m` | `80m` |
| `--flash_size` | `detect` | `16MB` |

---

## 甯歌闂

### `OSError: [WinError 193] %1 涓嶆槸鏈夋晥鐨?Win32 搴旂敤绋嬪簭`
`idf.py` 鍦?Windows 涓婃槸 Python 鑴氭湰锛屼笉鑳界洿鎺ヤ綔涓哄彲鎵ц鏂囦欢璋冪敤銆? 
`run_me.py` 宸查€氳繃 `shell=True` 瑙ｅ喅姝ら棶棰橈紝閬囧埌姝ら敊璇纭浣跨敤鐨勬槸鏈€鏂扮増 `run_me.py`銆?
### 鏃ュ織鏄剧ず `push_timeout_ms` 涓?JSON 閰嶇疆涓嶇
`global_json.h` 鏄紪璇戣繘鍥轰欢鐨?hex 瀛楄妭鏁扮粍锛屼慨鏀?JSON 鍚庡繀椤婚噸鏂扮敓鎴愬苟閲嶆柊缂栬瘧銆? 
鍘嗗彶涓婃浘鍑虹幇 `global_conf.cn490.json` 宸叉敼涓?`500ms`锛屼絾鍥轰欢浠嶆樉绀?`1800ms` 鐨勯棶棰橈紝鍘熷洜灏辨槸 `global_json.h` 鏈洿鏂般€?
### 涓插彛鍙蜂笉鐭ラ亾鏄摢涓?鎵撳紑 **璁惧绠＄悊鍣?鈫?绔彛 (COM 鍜?LPT)**锛屾彃鎷旇澶囪瀵熸柊澧炵殑 COM 鍙ｅ嵆涓虹洰鏍囦覆鍙ｃ€?
### `idf.py` 鎵句笉鍒?鏈湪 ESP-IDF Terminal 涓繍琛屻€傛櫘閫?PowerShell 娌℃湁 ESP-IDF 鐜锛岄渶鍒囨崲鍒?ESP-IDF Terminal銆?
---

## 鑴氭湰浣嶇疆

| 鑴氭湰 | 鐢ㄩ€?|
|------|------|
| `run_me.py` | 涓绘瀯寤鸿剼鏈紝鏇夸唬 `run_me.sh` |
| `scripts/gen_global_json.py` | 鍗曠嫭閲嶆柊鐢熸垚 `global_json.h` |
| `scripts/json_to_hex_array.py` | 搴曞眰宸ュ叿锛屼緵 `run_me.sh` / gen 鑴氭湰璋冪敤 |
| `scripts/dump_html.py` | 搴曞眰宸ュ叿锛岀敓鎴?`webpage.h` |
