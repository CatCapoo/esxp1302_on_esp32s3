# BUG-004锛歭oragw_gpio.h GPIO_PIN mask error 鈥?棰勫鐞嗗櫒鏃犳硶姹傚€兼灇涓剧

- **鏃ユ湡**锛?026-02-19  
- **鏂囦欢**锛歚main/libloragw/loragw_gpio.h`  
- **涓ラ噸绾у埆**锛氬姛鑳芥€ч敊璇紙GPIO 鍒濆鍖栧け璐ワ紝SX1302 澶嶄綅寮曡剼涓嶅彈鎺э級

---

## 鐜拌薄

杩愯浠讳綍璋冪敤 `lgw_reset()` 鐨勬祴璇曟椂锛屼覆鍙ｈ緭鍑猴細

```
E (2493) gpio: GPIO_PIN mask error
```

鍗充娇涔嬪墠宸插皢 `1 << pin` 淇涓?`1ULL << pin`锛岄噸鏂扮紪璇戠儳褰曞悗閿欒渚濈劧瀛樺湪銆?
---

## 鏍规湰鍘熷洜

`loragw_gpio.h` 涓師浠ｇ爜锛?
```c
#ifndef SX1302_POWER_EN_PIN
#define SX1302_POWER_EN_PIN       GPIO_NUM_NC   // 鈫?闂鏍规簮
#endif

#if SX1302_POWER_EN_PIN >= 0                    // 鈫?棰勫鐞嗗櫒鏉′欢鍒ゆ柇
#define SX1302_GPIO_PIN_SEL \
    ((1ULL << SX1302_RESET_PIN) | (1ULL << SX1302_POWER_EN_PIN))
#else
#define SX1302_GPIO_PIN_SEL \
    (1ULL << SX1302_RESET_PIN)
#endif
```

**鍏抽敭闂**锛歚GPIO_NUM_NC` 鏄?C 鏋氫妇鍊硷紙`enum gpio_num_t { GPIO_NUM_NC = -1 }`锛夛紝鑰?C 棰勫鐞嗗櫒 `#if` 鎸囦护**鏃犳硶璇嗗埆鏋氫妇鏍囪瘑绗?*锛屼細灏嗘湭鐭ユ爣璇嗙鐩存帴鏇挎崲涓?`0`銆?
鍥犳锛?
| 姝ラ | 棰勫鐞嗗櫒琛屼负 |
|------|-------------|
| `#if SX1302_POWER_EN_PIN >= 0` | `GPIO_NUM_NC` 鈫?`0`锛屾潯浠跺彉涓?`0 >= 0` 鈫?**TRUE** |
| 灞曞紑 `SX1302_GPIO_PIN_SEL` | `(1ULL << 2) \| (1ULL << GPIO_NUM_NC)` |
| 缂栬瘧鍣ㄦ眰鍊?`GPIO_NUM_NC` | 姝ゆ椂鏋氫妇鍊?= `-1` |
| `1ULL << (-1)` | 绉讳綅閲忎负璐熸暟锛屽睘浜?*鏈畾涔夎涓?*锛孏CC 瀹為檯缁撴灉涓?`1ULL << 63 = 0x8000000000000000` |
| `pin_bit_mask` 鏈€缁堝€?| `0x8000000000000004`锛宐it 63 瓒呭嚭 ESP32-S3 鏈夋晥 GPIO 鑼冨洿锛?鈥?8锛?|
| `gpio_config()` 鏍￠獙 | 妫€娴嬪埌鏃犳晥 bit 鈫?鎶?`GPIO_PIN mask error` |

---

## 淇

灏嗛粯璁ゅ€兼敼涓烘暣鏁板瓧闈㈤噺 `-1`锛屼娇棰勫鐞嗗櫒 `#if` 鑳芥纭眰鍊硷細

```c
// 淇鍓?#define SX1302_POWER_EN_PIN       GPIO_NUM_NC

// 淇鍚?#define SX1302_POWER_EN_PIN       -1  /* 浣跨敤鏁存暟瀛楅潰閲忥紝鏋氫妇鍊间笉鑳界敤浜?#if 鏉′欢 */
```

**缁撴灉**锛?
| 姝ラ | 淇鍚庤涓?|
|------|-----------|
| `#if -1 >= 0` | 鏉′欢涓?**FALSE** |
| `SX1302_GPIO_PIN_SEL` | `(1ULL << 2)` = `0x4`锛屼粎 GPIO 2 |
| `gpio_config()` 鏍￠獙 | 閫氳繃 |
| 杩愯鏃?`if (SX1302_POWER_EN_PIN != GPIO_NUM_NC)` | `-1 != -1` = FALSE锛宍POWER_EN` 鐩稿叧鎿嶄綔琚烦杩囷紝琛屼负姝ｇ‘ |

---

## 缁忛獙鏁欒

C 棰勫鐞嗗櫒 `#if` 鍙兘澶勭悊鏁存暟瀛楅潰閲忓拰宸茬敤 `#define` 瀹氫箟鐨勫畯锛?*鏃犳硶澶勭悊 `enum` 鏋氫妇鍊?*銆? 
鍑＄敤浜?`#if` 鏉′欢鍒ゆ柇鐨勫畯锛屽叾榛樿鍊煎繀椤绘槸鏁存暟瀛楅潰閲忥紝鑰岄潪鏋氫妇銆乣const` 鍙橀噺鎴栧叾浠栨爣璇嗙銆?
---

## 楠岃瘉

閲嶆柊缂栬瘧鐑у綍鍚庯紝涓插彛涓嶅啀鍑虹幇 `GPIO_PIN mask error`锛宍lgw_reset()` 姝ｅ父鎵ц銆?
