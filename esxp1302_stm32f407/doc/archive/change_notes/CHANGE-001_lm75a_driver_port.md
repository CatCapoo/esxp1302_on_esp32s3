# CHANGE-001锛氱Щ妞?NXP LM75A 娓╁害浼犳劅鍣ㄩ┍鍔紙鏇挎崲 ST STTS751锛?
- **鏃ユ湡**锛?026-02-27  
- **鍒嗘敮**锛歚bringup/temp`  
- **娑夊強鏂囦欢**锛歚main/libloragw/loragw_lm75a.c/.h`銆乣loragw_i2c.c/.h`銆乣loragw_hal.c`銆乣main/CMakeLists.txt`銆乣test/test_loragw_i2c.c`

---

## 鑳屾櫙

鍘熷伐绋嬩娇鐢?ST STTS751 浣滀负 SX1302 HAL 灞傜殑娓╁害浼犳劅鍣紝鐢ㄤ簬锛?1. **RSSI 娓╁害琛ュ伩**锛氭瘡娆?`lgw_receive()` 鍚庤皟鐢?`lgw_get_temperature()`锛屾寜 cn490 绯绘暟澶氶」寮忎慨姝?RSSI
2. **鐘舵€佷笂鎶?*锛歱acket forwarder 姣?30 绉掑湪缁熻 JSON 涓笂鎶?`"temp"` 瀛楁

纭欢鏉夸笂瀹為檯鐒婃帴鐨勬槸 NXP LM75A锛屼笌 STTS751 寮曡剼/瀵勫瓨鍣ㄥ潎涓嶅吋瀹癸紝闇€瑕侀噸鏂扮Щ妞嶉┍鍔ㄣ€?
---

## LM75A 涓?STTS751 宸紓瀵规瘮

| 椤圭洰 | STTS751 | LM75A |
|------|---------|-------|
| 鍒嗚鲸鐜?| 12-bit锛?.0625掳C | 11-bit锛?.125掳C |
| 娓╁害瀵勫瓨鍣ㄥ湴鍧€ | MSB=0x00锛孡SB=0x02锛堝垎涓ゆ璇伙級 | 0x00锛堜竴娆¤ 2 瀛楄妭锛?|
| 閰嶇疆瀵勫瓨鍣?| 0x03 | 0x01 |
| 杞崲閫熺巼瀵勫瓨鍣?| 0x04锛堥渶鍗曠嫭璁剧疆锛?| 鏃狅紙杩炵画杞崲锛屽浐瀹氾級 |
| 浜у搧/鍘傚晢 ID 瀵勫瓨鍣?| 0xFD/0xFE/0xFF | 鏃?|
| I2C 鍦板潃鑼冨洿 | 0x48~0x4F锛? 浣嶅湴鍧€寮曡剼锛?| 0x48~0x4F锛? 浣嶅湴鍧€寮曡剼锛?|

---

## 瀹炵幇鍐呭

### 1. 鏂板 `loragw_lm75a.h` / `loragw_lm75a.c`

- **`lm75a_configure(uint8_t i2c_addr)`**  
  璇婚厤缃瘎瀛樺櫒锛?x01锛夌‘璁よ澶囧湪绾匡紝鍐?`0x00` 璁句负姝ｅ父宸ヤ綔妯″紡锛堥潪 shutdown銆丱S 姣旇緝鍣ㄦā寮忋€佷綆鐢靛钩鏈夋晥锛?
- **`lm75a_get_temperature(uint8_t i2c_addr, float *temperature)`**  
  璋冪敤 `i2c_esp32_read_word()` 涓€娆¤鍙?2 瀛楄妭锛屾嫾瑁呬负 `int16_t` 鍚庨櫎浠?256.0 寰楀埌 掳C  
  鍏紡锛歚raw = (buf[0]<<8 | buf[1])`锛堟湁绗﹀彿锛夛紝`temp = raw / 256.0f`

- **鍦板潃鎵弿鏁扮粍**锛坄loragw_lm75a.h` 涓畾涔夛級锛? 
  `{0x48, 0x49, 0x4A, 0x4B}`锛?x48 鎺掔涓€锛堥伩鍏?NACK 姹℃煋鍚庣画鍦板潃鎵弿鐨勬€荤嚎鐘舵€侊級

### 2. 鏂板 `i2c_esp32_read_word()`锛坄loragw_i2c.c/.h`锛?
STTS751 涓や釜娓╁害瀛楄妭鍦ㄤ笉鍚屽瘎瀛樺櫒鍦板潃锛屽彲鐢ㄤ袱娆?`i2c_esp32_read()` 璇诲彇銆? 
LM75A 鐨?16-bit 娓╁害瀵勫瓨鍣ㄩ渶瑕佸湪鍚屼竴 I2C 浜嬪姟鍐呰繛缁鍑?MSB + LSB锛屽惁鍒欎袱娆¤涔嬮棿浼犳劅鍣ㄥ彲鑳芥洿鏂版暟鎹鑷村瓧鑺備笉涓€鑷淬€?
```c
// 鍗曚簨鍔¤ 2 瀛楄妭锛氬厛鍐欏瘎瀛樺櫒鍦板潃锛岄噸鏂?START 鍚庤繛璇?MSB(ACK)+LSB(NACK)
esp_err_t i2c_esp32_read_word(uint8_t device_addr, uint8_t reg_addr, uint8_t *data);
```

### 3. 鏇存柊 `loragw_hal.c`

```c
// 鏃?#include "loragw_stts751.h"
err = stts751_configure(ts_addr);
err = stts751_get_temperature(ts_addr, temperature);

// 鏂?#include "loragw_lm75a.h"
err = lm75a_configure(ts_addr);
err = lm75a_get_temperature(ts_addr, temperature);
```

### 4. 鏇存柊 `main/CMakeLists.txt`

```cmake
# 鏃?"libloragw/loragw_stts751.c"
# 鏂?"libloragw/loragw_lm75a.c"
```

娉細`loragw_stts751.c/.h` 鏂囦欢淇濈暀鍦ㄦ枃浠剁郴缁燂紝涓嶅弬涓庣紪璇戯紝浠呬綔鍘嗗彶鍙傝€冦€?
### 5. 閲嶅啓 `test/test_loragw_i2c.c`

绉婚櫎 STTS751 鐨?Product ID / Manufacturer ID 鏍￠獙閫昏緫锛屾敼涓猴細
- 璇?LM75A 閰嶇疆瀵勫瓨鍣紙0x01锛夐獙璇佽澶囧湪绾?- 寰幆 100 娆¤鍙栨俯搴︼紝姣忔璋冪敤 `i2c_esp32_read_word()`锛屾墦鍗板師濮嬪瓧鑺傚拰鎹㈢畻鍚庢俯搴?
---

## 璋冭瘯杩囩▼涓亣鍒扮殑闂

### 闂 1锛氬湴鍧€寮曡剼鎮┖

**鐜拌薄**锛氭祴璇曠▼搴忓 0x48 鑳借鍒版暟鎹紝涓荤▼搴忔壂鎻忓け璐? 
**鍘熷洜**锛氬湴鍧€寮曡剼 A0/A1/A2 鎮┖锛屽疄闄呭湴鍧€涓嶇‘瀹氾紱鍚庣‘璁?A0=A1=A2=GND 鈫?鍦板潃 0x48  
**瑙ｅ喅**锛氬皢鍦板潃寮曡剼纭疄鎺ュ湴锛屽悓鏃跺皢鎵弿鏁扮粍棣栦綅鏀逛负 0x48

### 闂 2锛歂ACK 姹℃煋鎬荤嚎

**鐜拌薄**锛氫富绋嬪簭鎵弿鏃讹紝濡傛灉鍏堟壂鎻忓埌鏃犳晥鍦板潃浜х敓 NACK锛屽悗缁姝ｇ‘鍦板潃鐨勮闂篃澶辫触  
**鍘熷洜**锛欵SP-IDF legacy I2C driver 鍦?NACK 鍚庢湭瀹屾暣閲婃斁鎬荤嚎锛圫DA 琚粠璁惧鎷変綆锛夛紝瀵艰嚧涓嬩竴娆′簨鍔″惎鍔ㄥけ璐? 
**瑙ｅ喅**锛氬皢宸茬煡瀛樺湪鐨?0x48 鏀惧湪鎵弿鏁扮粍绗竴浣嶏紝閬垮厤鍏堟壂鍒版棤鏁堝湴鍧€

### 闂 3锛歚pkt_fwd` 浠诲姟鏍堟孩鍑?
**鐜拌薄**锛氫富绋嬪簭杩愯涓€娈垫椂闂村悗宕╂簝锛屾棩蹇楀嚭鐜?stack overflow  
**鍘熷洜**锛氫换鍔℃爤璁句负 `1*4096`锛? KB锛夛紝HAL + I2C + JSON 缁勫抚鎿嶄綔瓒呭嚭  
**瑙ｅ喅**锛氭敼涓?`4*4096`锛?6 KB锛?
### 闂 4锛歚CHECK_NULL` 瀹忎娇鐢ㄤ簡閿欒鐨勮繑鍥炲€煎父閲?
**鐜拌薄**锛氱紪璇戝け璐ワ紝`LGW_REG_ERROR` 鏈０鏄? 
**鍘熷洜**锛氫粠 `loragw_stts751.c` 澶嶅埗瀹忓畾涔夋椂鏈浛鎹㈣繑鍥炲€煎父閲? 
**瑙ｅ喅**锛氭敼涓?`LGW_I2C_ERROR`

---

## 楠岃瘉缁撴灉

| 楠岃瘉椤?| 缁撴灉 |
|--------|------|
| 娴嬭瘯绋嬪簭 100 娆¤娓?| 鉁?22~23.75掳C锛屽垎杈ㄧ巼 0.125掳C |
| 涓荤▼搴忔壘鍒颁紶鎰熷櫒 | 鉁?`INFO: found temperature sensor on port 0x48` |
| 缁熻 JSON 娓╁害瀛楁 | 鉁?`"temp":22.5` |
| RSSI 娓╁害琛ュ伩鐢熸晥 | 鉁?`RSSI temperature offset applied: 0.918 dB (current temperature 22.9 C)` |
| 缂栬瘧鏃犻敊璇?璀﹀憡 | 鉁?|

---

## 纭欢鎺ョ嚎澶囨敞

| 淇″彿 | GPIO |
|------|------|
| SDA | GPIO 4 |
| SCL | GPIO 5 |
| VCC | 3.3 V |
| GND | GND |
| A0/A1/A2 | 鍏ㄩ儴鎺?GND 鈫?鍦板潃 0x48 |

**娉ㄦ剰浜嬮」**锛?- LM75A VCC 寮曡剼闇€骞惰仈 100nF 闄剁摲鍘昏€︾數瀹癸紝鍚﹀垯鍙兘寮曡捣 I2C 鎬荤嚎鍣０锛岃繘鑰屽鑷?SX1302 鏀跺寘 CRC 澶辫触鐜囦笂鍗?- I2C 杩炵嚎搴斿敖閲忕煭锛?10cm锛夛紝杩滅澶╃嚎鍜?SX1302 RF 璧扮嚎
