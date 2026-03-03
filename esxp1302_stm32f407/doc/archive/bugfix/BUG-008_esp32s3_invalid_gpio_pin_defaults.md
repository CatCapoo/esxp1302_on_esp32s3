# BUG-008锛欵SP32-S3 鏃犳晥 GPIO 寮曡剼榛樿鍊煎鑷磋繍琛屾椂閿欒

- **鏃ユ湡**锛?026-02-20  
- **鏂囦欢**锛歚main/libloragw/loragw_i2c.h`銆乣main/packet_forwarder/lora_pkt_fwd.c`銆乣main/board_config.h`锛堟柊寤猴級  
- **涓ラ噸绾у埆**锛氳繍琛屾椂鑷村懡锛圛2C / GPIO 鍒濆鍖栧け璐ワ級

---

## 鐜拌薄

鍥轰欢鐑у綍鍚庝覆鍙ｈ緭鍑哄涓繍琛屾椂閿欒锛?
```
E (xxx) i2c: i2c_set_pin(xxx): invalid GPIO number
E (xxx) gpio: gpio_set_direction(308): GPIO number error
E (xxx) gpio: gpio_pullup_en(78): GPIO number error
```

璁惧闅忓悗鏃犳硶鍚姩 packet forwarder銆?
---

## 鏍规湰鍘熷洜

浠ｇ爜涓悇妯″潡鐨?GPIO 寮曡剼榛樿鍊兼部鐢ㄨ嚜鍘熷 Linux / ESP32锛圕lassic锛夌Щ妞嶇増鏈細

| 妯″潡 | 榛樿寮曡剼 | ESP32-S3 瀛樺湪锛?|
|------|----------|----------------|
| I2C SCL | GPIO 22 | 鉂?涓嶅瓨鍦紙ESP32-S3 GPIO 鑼冨洿 0鈥?1 + 26鈥?8锛?*涓嶅惈 22鈥?5**锛?|
| I2C SDA | GPIO 21 | 鉁咃紙鎭板ソ鏈夋晥锛屼絾寮曡剼琚?SPI Flash 澶嶇敤锛屼笉瀹滀娇鐢級 |
| USER_BUTTON_1 | GPIO 23 | 鉂?涓嶅瓨鍦?|
| USER_BUTTON_2 | GPIO 25 | 鉂?涓嶅瓨鍦?|

`gpio_set_direction(308)` 涓殑 308 鏄?`GPIO_NUM_23`锛堟灇涓惧€?= 23锛屼絾妗嗘灦鍐呴儴妫€楠屾椂鏁板€间笉鍖归厤锛夛紝`gpio_pullup_en(78)` 鍚岀悊銆?
---

## 淇

### 1. 鏂板 `main/board_config.h`

灏嗗叏閮ㄧ‖浠跺紩鑴氬畾涔夐泦涓埌涓€涓枃浠讹紝鏂逛究閽堝涓嶅悓鏉垮崱淇敼锛?
```c
#define I2C_MASTER_SDA_IO   4
#define I2C_MASTER_SCL_IO   5
#define SX1302_RESET_PIN    2
#define BLINK_GPIO          1
#define LED_GREEN_GPIO      7
#define USER_BUTTON_1       0    /* IO0: boot/config button */
#define USER_BUTTON_2       6    /* IO6: reserved (GPIO_NUM_NC 绛夋晥) */
```

鎵€鏈夌浉鍏虫ā鍧楀ご鏂囦欢锛坄loragw_i2c.h`銆乣loragw_gpio.h`銆乣loragw_spi.h`銆乣loragw_gps.h`銆乣led_indication.h`锛夊強 `lora_pkt_fwd.c` 鍧?`#include "board_config.h"`锛屼笉鍐嶅悇鑷‖缂栫爜寮曡剼鍙枫€?
### 2. GPIO_NUM_NC 淇濇姢

瀵逛簬璁捐涓婁笉杩炴帴鐨勬寜閿紩鑴氾紝鍦ㄥ垵濮嬪寲鍓嶅姞 NC 鍒ゆ柇锛?
```c
if (USER_BUTTON_1 != GPIO_NUM_NC) {
    gpio_set_direction(USER_BUTTON_1, GPIO_MODE_INPUT);
    gpio_pullup_en(USER_BUTTON_1);
}
```

---

## 楠岃瘉

淇鍚庡浐浠跺惎鍔ㄦ棤 GPIO 鐩稿叧閿欒锛孖2C OLED 鍒濆鍖栭€氳繃锛孲X1302 姝ｅ父澶嶄綅骞跺惎鍔ㄣ€?
