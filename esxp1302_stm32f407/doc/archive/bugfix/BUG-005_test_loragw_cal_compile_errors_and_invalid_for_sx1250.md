# BUG-005锛歵est_loragw_cal.c 澶氬缂栬瘧閿欒鍙婃祴璇曟棤鏁堝０鏄庯紙SX1250 涓嶉€傜敤锛?
- **鏃ユ湡**锛?026-02-20  
- **鏂囦欢**锛歚main/test/test_loragw_cal.c`  
- **涓ラ噸绾у埆**锛氱紪璇戝け璐?+ 鍔熻兘鎬ф棤鏁?
---

## 闂鎻忚堪

缂栬瘧 `test_loragw_cal.c` 鏃舵姤鍑?11 澶勯敊璇紙鍑芥暟鏈０鏄庛€佹牸寮忕涓嶅尮閰嶃€佸弬鏁颁笉瓒炽€丗reeRTOS 绗﹀彿鏈０鏄庯級銆備慨澶嶇紪璇戦敊璇悗鍦?SX1250 纭欢涓婅繍琛岋紝娴嬭瘯鍏ㄩ儴澶辫触锛孲X125x 瀵勫瓨鍣ㄨ闂姤閿欍€?
**缁撹锛氳娴嬭瘯鏂囦欢浠呴€傜敤浜?SX1255/SX1257 纭欢锛屼笉鏀寔 SX1250锛屽湪 SX1250 骞冲彴涓婃祴璇曠粨鏋滄棤鏁堛€?*

---

## 缂栬瘧閿欒璇︽儏涓庝慨澶?
### 閿欒 1鈥?锛歚lgw_sx125x_reg_w` / `lgw_sx125x_reg_r` 鏈０鏄?
```
error: implicit declaration of function 'lgw_sx125x_reg_w';
       did you mean 'sx125x_reg_w'?
error: implicit declaration of function 'lgw_sx125x_reg_r';
       did you mean 'sx125x_reg_r'?
```

**鍘熷洜**锛氬師濮?Semtech 浠ｇ爜涓娇鐢ㄤ簡鏃х増鍑芥暟鍚?`lgw_sx125x_reg_w/r`锛屽綋鍓?HAL 搴撲腑瀵瑰簲鎺ュ彛涓?`sx125x_reg_w/r`锛堟棤 `lgw_` 鍓嶇紑锛夈€?
**淇**锛氬皢 `setup_tx_dc_offset()` 涓墍鏈?`lgw_sx125x_reg_w` / `lgw_sx125x_reg_r` 鏇挎崲涓?`sx125x_reg_w` / `sx125x_reg_r`銆?
---

### 閿欒 3鈥?锛歚printf` 鏍煎紡绗︿笌 `int32_t` 绫诲瀷涓嶅尮閰?
```
error: format '%d' expects argument of type 'int',
       but argument has type 'int32_t' {aka 'long int'}
error: format '%u' expects argument of type 'unsigned int',
       but argument has type 'int32_t' {aka 'long int'}
```

**鍘熷洜**锛歚i_offset`銆乣q_offset`銆乣f_offset`銆乣val_min`銆乣val_max`銆乣val_mean` 鍧囦负 `int32_t`锛屽湪 Xtensa 骞冲彴绛夊悓浜?`long int`锛宍%d`/`%u` 瀵瑰簲 `int`/`unsigned int`锛宍-Werror=format=` 灏嗚鍛婂崌涓洪敊璇€?
**淇**锛?
```c
// 淇鍓?printf("i_offset:%d q_offset:%d f_offset:%d ...", i_offset, q_offset, f_offset, ...);
printf(" min:%u max:%u mean:%u std:%f\n", val_min, val_max, val_mean, val_std);

// 淇鍚?printf("i_offset:%ld q_offset:%ld f_offset:%ld ...", (long)i_offset, (long)q_offset, (long)f_offset, ...);
printf(" min:%ld max:%ld mean:%ld std:%f\n", (long)val_min, (long)val_max, (long)val_mean, val_std);
```

---

### 閿欒 9锛歚lgw_connect()` 缂哄皯蹇呰鍙傛暟

```
error: too few arguments to function 'lgw_connect'
note: declared here: int lgw_connect(const lgw_com_type_t com_type, const char * com_path);
```

**鍘熷洜**锛氬師濮嬩唬鐮佽皟鐢?`lgw_connect()` 鏃犲弬鏁帮紝褰撳墠鎺ュ彛瑕佹眰浼犲叆閫氫俊绫诲瀷鍜岃矾寰勫瓧绗︿覆銆傚悓鏃朵紶鍏?`NULL` 浼氳 `lgw_connect` 鍐呴儴鐨勭┖鎸囬拡妫€鏌ョ洿鎺ヨ繑鍥為敊璇紙`if (com_path == NULL) return LGW_REG_ERROR`锛夈€?
**淇**锛?
```c
// 淇鍓?x = lgw_connect();

// 淇鍚?x = lgw_connect(LGW_COM_SPI, "spi");
```

> **璇存槑**锛欵SP32 SPI 瀹炵幇锛坄lgw_spi_open`锛変笉浣跨敤 `com_path` 瀛楃涓插唴瀹癸紝浣嗚鍙傛暟蹇呴』闈?NULL銆?
鍚屾椂鍦?`#include` 涓坊鍔?`"loragw_com.h"` 浠ヨ幏寰?`LGW_COM_SPI` 瀹氫箟銆?
---

### 閿欒 10鈥?1锛歚vTaskDelay` / `portTICK_PERIOD_MS` 鏈０鏄?
```
error: implicit declaration of function 'vTaskDelay'
error: 'portTICK_PERIOD_MS' undeclared (first use in this function)
```

**鍘熷洜**锛歚info_and_delay()` 鍜?`app_main()` 涓娇鐢ㄤ簡 FreeRTOS API锛屼絾鏂囦欢鏈寘鍚搴斿ご鏂囦欢銆?
**淇**锛氬湪鏂囦欢椤堕儴娣诲姞锛?
```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
```

---

### 璀﹀憡锛堟彁鍗囦负閿欒锛夛細鏈娇鐢ㄧ殑灞€閮ㄥ彉閲?
```
warning: unused variable 'arg_u', 'arg_d', 'i'
```

**淇**锛氫粠 `main_test()` 涓垹闄ゆ湭浣跨敤鐨勫彉閲忓０鏄?`int i`銆乣double arg_d`銆乣unsigned int arg_u`銆?
---

## 杩愯鏃跺け璐ヤ笌鏍规湰鍘熷洜鍒嗘瀽锛圫X1250 涓嶉€傜敤澹版槑锛?
缂栬瘧淇鍚庡湪鎼浇 **SX1250** 灏勯鑺墖鐨勭‖浠跺钩鍙颁笂杩愯锛岃緭鍑哄涓嬶細

```
INFO: Configuring SX1250_0 in single input mode
Loading CAL fw for sx125x
-------------------------------------
ERROR: sx125x register 5 write failed (w:216 r:210)!!
...
ERROR: PLL failed to lock
```

### 鍘熷洜鍒嗘瀽

`test_loragw_cal.c` 鏄?Semtech 涓撲负 **SX1255/SX1257** 璁捐鐨?TX DC 鍋忕疆鏍″噯娴嬭瘯锛屽叾鏍稿績鍑芥暟 `setup_tx_dc_offset()` 閫氳繃 SX1302 鐨?SX125x 瀛?SPI 鎬荤嚎鐩存帴鎿嶄綔 SX125x PLL/DAC/MIX 瀵勫瓨鍣ㄣ€係X1250 涓?SX125x 鏄畬鍏ㄤ笉鍚岀殑灏勯鏋舵瀯锛?
| 瀵规瘮椤?| SX1255/SX1257 | SX1250 |
|--------|--------------|--------|
| 鎺ュ彛 | SX1302 閫氳繃涓撶敤瀛?SPI 璁块棶 | SX1302 閫氳繃鍐呴儴 SPIB 璁块棶锛屽崗璁笉鍚?|
| TX DC 鍋忕疆鏍″噯 | `cal_firmware_sx125x` + `sx1302_cal_start()` | SX1250 鍐呴儴鑷姩瀹屾垚锛岀敱 `sx1250_calibrate()` 瑙﹀彂 |
| 瀵勫瓨鍣ㄦ搷浣?| `sx125x_reg_w/r()` | `sx1250_reg_w/r()`锛屽瘎瀛樺櫒鍦板潃鏄犲皠瀹屽叏涓嶅悓 |

璇诲洖鍊?`0xD2 (210)` 鏄?SX125x 瀛?SPI 鎬荤嚎涓婃棤璁惧鍝嶅簲鏃剁殑娴┖鍣０鍊硷紝纭 SX1250 纭欢涓嶅瓨鍦?SX125x 瀵勫瓨鍣ㄧ┖闂淬€?
### 缁撹

**鍦?SX1250 骞冲彴涓婏紝`test_loragw_cal.c` 鐨勬祴璇曠粨鏋滃畬鍏ㄦ棤鏁堬紝涓嶅彲鐢ㄤ簬楠岃瘉浠讳綍鏍″噯鍔熻兘銆?*

SX1250 骞冲彴鐨勫皠棰戞牎鍑嗗簲浣跨敤锛?- `sx1250_calibrate()`锛堝凡闆嗘垚浜?`lgw_start()` 娴佺▼涓紝鑷姩鎵ц锛?- `main/test/test_loragw_spi_sx1250.c`锛堥獙璇?SX1250 SPI 閫氫俊锛?
---

## 鍙樻洿鎽樿

| 淇敼浣嶇疆 | 淇敼鍐呭 |
|----------|----------|
| 澶存枃浠跺尯 | 娣诲姞 `freertos/FreeRTOS.h`銆乣freertos/task.h`銆乣loragw_com.h` |
| `setup_tx_dc_offset()` | `lgw_sx125x_reg_w/r` 鈫?`sx125x_reg_w/r` |
| `cal_tx_dc_offset()` printf | `%d`/`%u` 鈫?`%ld`锛宍int32_t` 鍙傛暟鍔?`(long)` 杞崲 |
| `main_test()` | `lgw_connect()` 鈫?`lgw_connect(LGW_COM_SPI, "spi")` |
| `main_test()` | 鍒犻櫎鏈娇鐢ㄥ彉閲?`i`銆乣arg_d`銆乣arg_u` |
