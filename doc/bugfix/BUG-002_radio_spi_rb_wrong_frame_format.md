# BUG-002：radio_spi_rb SPI 读帧格式错误导致 SX1250 寄存器回读全零

- **日期**：2026-02-19  
- **文件**：`main/libloragw/loragw_spi.c`，函数 `radio_spi_rb()`  
- **严重级别**：功能性 bug（SX1250 所有寄存器读操作数据错误）

---

## 问题现象

运行 `test_loragw_spi_sx1250` 测试，SX1250 状态读取正常（`get_status: 0x32`），  
但频率寄存器的读写压力测试在第 0 次就失败：

```
Cycle 0 > error during the buffer comparison
Written value: 2DCF4629
Read value:    00000000
```

---

## 调用链

```
sx1250_reg_r(op_code, data, size, rf_chain)
  └→ sx1250_com_r(com_type, com_target, spi_mux_target, op_code, data, size)
       └→ sx1250_spi_r(spi, spi_mux_target, op_code, data, size)
            └→ radio_spi_rb(spi, spi_mux_target, op_code, data, size)  ← 根源
```

---

## 根本原因

ESP-IDF SPI 的 `spi_transaction_ext_t` 把一次事务分为三段发送：

```
[CMD 段: command_bits 位] [ADDR 段: address_bits 位] [DATA 段: length 位]
```

### 写函数 `radio_spi_wb` 的配置（正确）

```c
et.command_bits = 8;          // CMD  = 1 字节：SPI MUX target
et.address_bits = 8;          // ADDR = 1 字节：opcode
et.base.tx_buffer = data;     // DATA = 用户数据
```

实际 SPI 帧：
```
[mux] [opcode] [data[0]] [data[1]] [data[2]] [data[3]]
```

### 读函数 `radio_spi_rb` 的配置（原始—错误）

```c
et.command_bits = 8;
et.address_bits = 8 * 2;      // ← ADDR = 2 字节！多了 1 字节
et.base.addr = ((READ_ACCESS | op_code) << 8) | 0x00;  // ← 把 0x00 塞进 ADDR 低字节
et.base.tx_buffer = tbuf;     // ← tbuf 是全零缓冲区！不发用户数据
```

实际 SPI 帧：
```
[mux] [opcode] [0x00] [0x00] [0x00] [0x00] [0x00] [0x00] [0x00]
               ^^^^^^^^^^^^  ← ADDR段多出1字节，DATA段全为零
```

### 对 SX1250 READ_REGISTER 命令的影响

SX1250 `READ_REGISTER(0x1D)` 期望的帧格式：
```
[0x1D(opcode)] [addr_h] [addr_l] [nop] [nop] [nop] [nop]
                ↑data[0] ↑data[1]
```

由于：
1. `tx_buffer = tbuf`（全零）→ `addr_h=0x00`, `addr_l=0x00`，寄存器地址变成 `0x0000` 而非 `0x088B`
2. `address_bits=16` → ADDR 段多出1字节 `0x00`，数据进一步错位

两者叠加，回读数据全零。

---

## 定位过程

**第一步**：读回全零，排除 SPI 连接失败（`get_status` 正常），怀疑底层读帧格式有误，顺调用链找到 `radio_spi_rb`。

**第二步**：对比 `radio_spi_wb` 和 `radio_spi_rb`，发现三处不一致：

| 字段 | `radio_spi_wb` | `radio_spi_rb`（原始）|
|------|--------------|----------------------|
| `address_bits` | `8` | `16` |
| `et.base.addr` | `WRITE_ACCESS \| op_code` | `(READ_ACCESS \| op_code) << 8 \| 0x00` |
| `tx_buffer` | `data`（用户数据）| `tbuf`（全零）|

**第三步**：先只修复 `tx_buffer = data`，重测，读回从 `00000000` 变为 `00010000`——有进展但仍错误，确认 `address_bits=16` 的错位问题也必须修复。

**第四步**：将 `address_bits` 改回 `8`，`addr` 计算对齐写函数，读回数据正确，测试全部通过。

---

## 具体修改

**文件**：`main/libloragw/loragw_spi.c`，`radio_spi_rb()` 函数

```c
// 修改前
int radio_spi_rb(...) {
    ...
    uint8_t tbuf[LGW_BURST_CHUNK] = {0x00};  // ← 删除
    ...
    memset(&et, 0, sizeof(et));
    et.command_bits = 8;
    et.address_bits = 8 * 2;                              // ← 错误：16位
    et.base.cmd = spi_mux_target;
    et.base.addr = ((READ_ACCESS | (op_code & ADDR_MASK)) << 8) | 0x00;  // ← 错误：移位+填0
    et.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    et.base.tx_buffer = tbuf;                             // ← 错误：全零

    et.base.rx_buffer = (unsigned long *)data;
    ...
}

// 修改后
int radio_spi_rb(...) {
    ...
    // tbuf 已删除
    ...
    memset(&et, 0, sizeof(et));
    et.command_bits = 8;
    et.address_bits = 8;                                  // ← 修正：8位，与写一致
    et.base.cmd = spi_mux_target;
    et.base.addr = READ_ACCESS | (op_code & ADDR_MASK);   // ← 修正：与 radio_spi_wb 一致
    et.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
    et.base.tx_buffer = (unsigned long *)data;            // ← 修正：发用户数据（含寄存器地址）

    et.base.rx_buffer = (unsigned long *)data;
    ...
}
```

修复后实际 SPI 帧：
```
[mux] [opcode] [data[0]=0x08] [data[1]=0x8B] [data[2]=0x00] ...
SX1250 收到: READ_REGISTER, addr=0x088B → 读取正确寄存器
```

---

## 影响范围

`radio_spi_rb` 被以下两处调用：

| 调用者 | 芯片 | 影响 |
|--------|------|------|
| `sx1250_spi.c` → `sx1250_spi_r()` | SX1250 | **直接修复** |
| `sx125x_spi.c` → `sx125x_spi_r()` | SX1255/SX1257 | 同样受益（地址只有1字节，修复后帧格式更紧凑）|

---

## 验证结果

修复后 20 次读写压力测试全部通过：

```
Cycle 0  > did a 4-byte R/W on a register with no error
Cycle 1  > did a 4-byte R/W on a register with no error
...
Cycle 19 > did a 4-byte R/W on a register with no error
End of test for loragw_spi_sx1250.c
```
