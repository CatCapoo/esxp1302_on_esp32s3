# BUG-006锛歮ain/CMakeLists.txt 澶氭澹版槑 INCLUDE_DIRS锛屼粎鏈€鍚庝竴鏉＄敓鏁?
- **鏃ユ湡**锛?026-02-20  
- **鏂囦欢**锛歚main/CMakeLists.txt`  
- **涓ラ噸绾у埆**锛氱紪璇戝け璐ワ紙鍏抽敭澶存枃浠舵棤娉曟壘鍒帮級

---

## 鐜拌薄

浣跨敤 ESP-IDF 鏋勫缓鏃舵姤閿欙細

```
fatal error: global_json.h: No such file or directory
```

`global_json.h` 浣嶄簬 `main/` 鐩綍涓嬶紝浣?`packet_forwarder/` 瀛愮洰褰曚腑鐨勬簮鏂囦欢鏃犳硶鎵惧埌瀹冦€?
---

## 鏍规湰鍘熷洜

`idf_component_register()` 鍚屼竴璋冪敤涓娆″嚭鐜?`INCLUDE_DIRS` 鍏抽敭瀛楁椂锛孋Make 鍙繚鐣?*鏈€鍚庝竴鏉?*锛屽墠闈㈢殑鍏ㄩ儴琚潤榛樹涪寮冦€?
鍘熷 `main/CMakeLists.txt`锛?
```cmake
idf_component_register(
    SRCS ...
    INCLUDE_DIRS "libloragw"
    INCLUDE_DIRS "libtools"
    INCLUDE_DIRS "libloragw-test"
    INCLUDE_DIRS "packet_forwarder"
    ...
)
```

瀹為檯鐢熸晥鐨勫彧鏈?`"packet_forwarder"`锛宍main/` 鐩綍锛堝惈 `global_json.h`锛変互鍙婂叾浣欏瓙鐩綍鍧囨湭琚寘鍚€?
---

## 淇

灏嗘墍鏈夎矾寰勫悎骞跺埌鍗曚釜 `INCLUDE_DIRS` 鏉＄洰锛屽苟琛ュ厖 `"."` 浠ユ毚闇?`main/` 鏍圭洰褰曪細

```cmake
idf_component_register(
    SRCS ...
    INCLUDE_DIRS "." "libloragw" "libtools" "libloragw-test" "packet_forwarder"
    ...
)
```

---

## 楠岃瘉

淇鍚?`idf.py build` 閫氳繃锛宍global_json.h` 浠ュ強鎵€鏈夊瓙鐩綍澶存枃浠跺潎鍙甯哥储寮曘€?
