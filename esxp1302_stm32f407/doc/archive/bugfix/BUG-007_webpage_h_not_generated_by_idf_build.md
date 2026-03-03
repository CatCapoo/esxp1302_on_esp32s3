# BUG-007锛歸ebpage.h 鍦?ESP-IDF 鏋勫缓涓笉浼氳嚜鍔ㄧ敓鎴?
- **鏃ユ湡**锛?026-02-20  
- **鏂囦欢**锛歚main/packet_forwarder/webpage.h`锛堢敓鎴愪骇鐗╋級銆乣scripts/dump_html.py`  
- **涓ラ噸绾у埆**锛氱紪璇戝け璐ワ紙缂哄皯鐢熸垚鏂囦欢锛?
---

## 鐜拌薄

浣跨敤 `idf.py build` 鏋勫缓鏃舵姤閿欙細

```
fatal error: webpage.h: No such file or directory
```

`http_server.c` 涓?`#include "webpage.h"`锛屼絾璇ユ枃浠跺湪浠ｇ爜浠撳簱涓笉瀛樺湪銆?
---

## 鏍规湰鍘熷洜

`webpage.h` 鏄敱 `scripts/dump_html.py` 浠?`main/packet_forwarder/webpage.html` 鐢熸垚鐨?C 瀛楄妭鏁扮粍澶存枃浠躲€?
**PlatformIO** 宸ョ▼閫氳繃 `platformio_pre.py` 棰勬瀯寤洪挬瀛愯嚜鍔ㄨ繍琛岃鑴氭湰锛涗絾 **ESP-IDF CMake** 鏋勫缓涓嶈皟鐢?PlatformIO 閽╁瓙锛屽洜姝ゆ瘡娆?first-build 鎴?clean 鍚庨兘闇€瑕佹墜鍔ㄧ敓鎴愩€?
---

## 淇锛堜复鏃讹級

鍦ㄩ娆℃瀯寤烘垨娓呯悊鍚庯紝鍦ㄩ」鐩牴鐩綍鎵嬪姩鎵ц锛?
```bash
cd main/packet_forwarder
python ../../scripts/dump_html.py webpage.html webpage_str > webpage.h
```

鎴栧湪 Windows PowerShell锛?
```powershell
cd main\packet_forwarder
python ..\..\scripts\dump_html.py webpage.html webpage_str > webpage.h
```

---

## 寤鸿锛堥暱鏈燂級

鍦?`main/CMakeLists.txt` 涓€氳繃 `add_custom_command` / `add_custom_target` 灏嗙敓鎴愭楠ら泦鎴愬埌 CMake 鏋勫缓锛?
```cmake
add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_SOURCE_DIR}/packet_forwarder/webpage.h
    COMMAND python ${PROJECT_SOURCE_DIR}/scripts/dump_html.py
                   ${CMAKE_CURRENT_SOURCE_DIR}/packet_forwarder/webpage.html
                   webpage_str
                   > ${CMAKE_CURRENT_SOURCE_DIR}/packet_forwarder/webpage.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/packet_forwarder/webpage.html
    COMMENT "Generating webpage.h from webpage.html"
)
```

---

## 娉ㄦ剰

`webpage.h` 宸插姞鍏?`.gitignore`锛屼笉闇€瑕佹彁浜ゅ埌鐗堟湰搴撱€?
