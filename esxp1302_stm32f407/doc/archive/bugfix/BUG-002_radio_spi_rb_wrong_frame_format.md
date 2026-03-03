# BUG-002锛歳adio_spi_rb SPI 璇诲抚鏍煎紡閿欒瀵艰嚧 SX1250 瀵勫瓨鍣ㄥ洖璇诲叏闆?
- **鏃ユ湡**锛?026-02-19  
- **鏂囦欢**锛歚main/libloragw/loragw_spi.c`锛屽嚱鏁?`radio_spi_rb()`  
- **涓ラ噸绾у埆**锛氬姛鑳芥€?bug锛圫X1250 鎵€鏈夊瘎瀛樺櫒璇绘搷浣滄暟鎹敊璇級

---

## 闂鐜拌薄

杩愯 `test_loragw_spi_sx1250` 娴嬭瘯锛孲X1250 鐘舵€佽鍙栨甯革紙`get_status: 0x32`锛夛紝  
浣嗛鐜囧瘎瀛樺櫒鐨勮鍐欏帇鍔涙祴璇曞湪绗?0 娆″氨澶辫触锛?
```
Cycle 0 > error during the buffer comparison
Written value: 2DCF4629
Read value:    00000000
```

---

## 璋冪敤閾?
```
sx1250_reg_r(op_code, data, size, rf_chain)
  鈹斺啋 sx1250_com_r(com_type, com_target, spi_mux_target, op_code, data, size)
       鈹斺啋 sx1250_spi_r(spi, spi_mux_target, op_code, data, size)
            鈹斺啋 radio_spi_rb(spi, spi_mux_target, op_code, data, size)  鈫?鏍规簮
```

---

## 鏍规湰鍘熷洜

ESP-IDF SPI 鐨?`spi_transaction_ext_t` 鎶婁竴娆′簨鍔″垎涓轰笁娈靛彂閫侊細

```
[CMD 娈? command_bits 浣峕 [ADDR 娈? address_bits 浣峕 [DATA 娈? length 浣峕
```

### 鍐欏嚱鏁?`radio_spi_wb` 鐨勯厤缃紙姝ｇ‘锛?
```c
et.command_bits = 8;          // CMD  = 1 瀛楄妭锛歋PI MUX target
et.address_bits = 8;          // ADDR = 1 瀛楄妭锛歰pcode
et.base.tx_buffer = data;     // DATA = 鐢ㄦ埛鏁版嵁
```

瀹為檯 SPI 甯э細
```
[mux] [opcode] [data[0]] [data[1]] [data[2]] [data[3]]
```

### 璇诲嚱鏁?`radio_spi_rb` 鐨勯厤缃紙鍘熷鈥旈敊璇級

```c
et.command_bits = 8;
et.address_bits = 8 * 2;      // 鈫?ADDR = 2 瀛楄妭锛佸浜?1 瀛楄妭
et.base.addr = ((READ_ACCESS | op_code) << 8) | 0x00;  // 鈫?鎶?0x00 濉炶繘 ADDR 浣庡瓧鑺?et.base.tx_buffer = tbuf;     // 鈫?tbuf 鏄叏闆剁紦鍐插尯锛佷笉鍙戠敤鎴锋暟鎹?```

瀹為檯 SPI 甯э細
```
[mux] [opcode] [0x00] [0x00] [0x00] [0x00] [0x00] [0x00] [0x00]
               ^^^^^^^^^^^^  鈫?ADDR娈靛鍑?瀛楄妭锛孌ATA娈靛叏涓洪浂
```

### 瀵?SX1250 READ_REGISTER 鍛戒护鐨勫奖鍝?
SX1250 `READ_REGISTER(0x1D)` 鏈熸湜鐨勫抚鏍煎紡锛?```
[0x1D(opcode)] [addr_h] [addr_l] [nop] [nop] [nop] [nop]
                鈫慸ata[0] 鈫慸ata[1]
```

鐢变簬锛?1. `tx_buffer = tbuf`锛堝叏闆讹級鈫?`addr_h=0x00`, `addr_l=0x00`锛屽瘎瀛樺櫒鍦板潃鍙樻垚 `0x0000` 鑰岄潪 `0x088B`
2. `address_bits=16` 鈫?ADDR 娈靛鍑?瀛楄妭 `0x00`锛屾暟鎹繘涓€姝ラ敊浣?
涓よ€呭彔鍔狅紝鍥炶鏁版嵁鍏ㄩ浂銆?
---

## 瀹氫綅杩囩▼

**绗竴姝?*锛氳鍥炲叏闆讹紝鎺掗櫎 SPI 杩炴帴澶辫触锛坄get_status` 姝ｅ父锛夛紝鎬€鐤戝簳灞傝甯ф牸寮忔湁璇紝椤鸿皟鐢ㄩ摼鎵惧埌 `radio_spi_rb`銆?
**绗簩姝?*锛氬姣?`radio_spi_wb` 鍜?`radio_spi_rb`锛屽彂鐜颁笁澶勪笉涓€鑷达細

| 瀛楁 | `radio_spi_wb` | `radio_spi_rb`锛堝師濮嬶級|
|------|--------------|----------------------|
| `address_bits` | `8` | `16` |
| `et.base.addr` | `WRITE_ACCESS \| op_code` | `(READ_ACCESS \| op_code) << 8 \| 0x00` |
| `tx_buffer` | `data`锛堢敤鎴锋暟鎹級| `tbuf`锛堝叏闆讹級|

**绗笁姝?*锛氬厛鍙慨澶?`tx_buffer = data`锛岄噸娴嬶紝璇诲洖浠?`00000000` 鍙樹负 `00010000`鈥斺€旀湁杩涘睍浣嗕粛閿欒锛岀‘璁?`address_bits=16` 鐨勯敊浣嶉棶棰樹篃蹇呴』淇銆?
**绗洓姝?*锛氬皢 `address_bits` 鏀瑰洖 `8`锛宍addr` 璁＄畻瀵归綈鍐欏嚱鏁帮紝璇诲洖鏁版嵁姝ｇ‘锛屾祴璇曞叏閮ㄩ€氳繃銆?
---

## 鍏蜂綋淇敼

**鏂囦欢**锛歚main/libloragw/loragw_spi.c`锛宍radio_spi_rb()` 鍑芥暟

```c
// 淇敼鍓?int radio_spi_rb(...) {
    ...
    uint8_t tbuf[LGW_BURST_CHUNK] = {0x00};  // 鈫?鍒犻櫎
    ...
    memset(&et, 0, sizeof(et));
    et.command_bits = 8;
    et.address_bits = 8 * 2;                              // 鈫?閿欒锛?6浣?    et.base.cmd = spi_mux_target;
    et.base.addr = ((READ_ACCESS | (op_code & ADDR_MASK)) << 8) | 0x00;  // 鈫?閿欒锛氱Щ浣?濉?
    et.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    et.base.tx_buffer = tbuf;                             // 鈫?閿欒锛氬叏闆?
    et.base.rx_buffer = (unsigned long *)data;
    ...
}

// 淇敼鍚?int radio_spi_rb(...) {
    ...
    // tbuf 宸插垹闄?    ...
    memset(&et, 0, sizeof(et));
    et.command_bits = 8;
    et.address_bits = 8;                                  // 鈫?淇锛?浣嶏紝涓庡啓涓€鑷?    et.base.cmd = spi_mux_target;
    et.base.addr = READ_ACCESS | (op_code & ADDR_MASK);   // 鈫?淇锛氫笌 radio_spi_wb 涓€鑷?    et.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    et.base.tx_buffer = (unsigned long *)data;            // 鈫?淇锛氬彂鐢ㄦ埛鏁版嵁锛堝惈瀵勫瓨鍣ㄥ湴鍧€锛?
    et.base.rx_buffer = (unsigned long *)data;
    ...
}
```

淇鍚庡疄闄?SPI 甯э細
```
[mux] [opcode] [data[0]=0x08] [data[1]=0x8B] [data[2]=0x00] ...
SX1250 鏀跺埌: READ_REGISTER, addr=0x088B 鈫?璇诲彇姝ｇ‘瀵勫瓨鍣?```

---

## 褰卞搷鑼冨洿

`radio_spi_rb` 琚互涓嬩袱澶勮皟鐢細

| 璋冪敤鑰?| 鑺墖 | 褰卞搷 |
|--------|------|------|
| `sx1250_spi.c` 鈫?`sx1250_spi_r()` | SX1250 | **鐩存帴淇** |
| `sx125x_spi.c` 鈫?`sx125x_spi_r()` | SX1255/SX1257 | 鍚屾牱鍙楃泭锛堝湴鍧€鍙湁1瀛楄妭锛屼慨澶嶅悗甯ф牸寮忔洿绱у噾锛墊

---

## 楠岃瘉缁撴灉

淇鍚?20 娆¤鍐欏帇鍔涙祴璇曞叏閮ㄩ€氳繃锛?
```
Cycle 0  > did a 4-byte R/W on a register with no error
Cycle 1  > did a 4-byte R/W on a register with no error
...
Cycle 19 > did a 4-byte R/W on a register with no error
End of test for loragw_spi_sx1250.c
```
