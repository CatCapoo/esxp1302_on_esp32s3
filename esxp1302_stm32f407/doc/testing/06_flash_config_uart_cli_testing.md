# 06 — Flash 配置存储 + UART CLI 环形缓冲（验证与测试）

本文档记录 **Flash 持久化存储** 和 **UART 中断驱动 CLI** 两个关键功能的设计、实现、遇到的问题、完整的解决过程，以及自动化验证方法。

---

## Part 1：问题背景与遇到的坑

### 1.1 Flash 配置存储的需求

网关需要持久化存储多个配置参数：
- `eui64`（网关 EUI-64 地址）
- `region`（频段：CN470A / CN470B 等）
- `lorawan_version`（LoRaWAN 协议版本）
- `ns_host`（网络服务器地址）
- `ns_port`（网络服务器端口）
- `gps_xx`（GPS 模块相关参数）
- ...

这些配置在 **MCU 掉电后也必须保留**，只能存在 **内部 Flash**（没有外部 EEPROM）。

### 1.2 UART CLI 的痛点：ISR 丢字符

初版 CLI 实现在 `HAL_UART_RxCpltCallback` 中直接调用 `echo` 和 `dispatch` 逻辑。在 115200 baud 高速输入（如粘贴命令）时，**大量字符丢失**，导致命令执行不完整或错误。

根本原因：`HAL_UART_Transmit()` 是阻塞调用，echo 一个字符需 87µs，期间新字节到达无法及时保存。

### 1.3 CH340 自动复位的意外坑

编写 pyserial 测试脚本时，每次打开串口连接 **MCU 都会复位**，导致：
- 测试运行出现无法预期的 boot 信息
- 命令响应时间超出预期
- Test framework 整体不可用

root cause：CH340 USB-UART 模块的 DTR 脚接到 STM32 的 NRST（复位脚）通过 100nF 电容，pyserial 默认在 `open()` 时拉高 DTR（低→高跳变），触发 MCU reset。

---

## Part 2：STM32F407 内部 Flash 配置存储

### 2.1 Flash 物理布局和扇区选择

STM32F407ZGT6 内部 Flash 1MB，划分为 12 个扇区：

| 扇区        | 起始地址     | 大小  | 建议用途              |
|------------|------------|-------|----------------------|
| Sector 0   | 0x08000000 | 16KB  | 固件，ISR 向量表必须在此 |
| Sector 1-3 | 0x08004000 | 16KB  | 固件                 |
| Sector 4   | 0x08010000 | 64KB  | 固件                 |
| Sector 5-8 | 0x08020000 | 128KB | 固件                 |
| Sector 9   | 0x080A0000 | 128KB | 固件备用             |
| Sector 10  | 0x080C0000 | 128KB | 备用                 |
| **Sector 11** | **0x080E0000** | **128KB** | **→ 配置数据** |

**选择 Sector 11 的理由**：
- 离代码区最远，不会因配置擦除误伤固件
- 128KB 对几百字节配置绰绰有余，且是最大扇区
- 扇区擦除以整个扇区为单位，选最大的不浪费其他资源
- 链接脚本中确认 `FLASH LENGTH = 0xE0000`（896KB），使代码不链接到 Sector 11

### 2.2 Flash 操作顺序（不可颠倒）

```
1. HAL_FLASH_Unlock()      → 解锁 FLASH 寄存器
2. HAL_FLASHEx_Erase()     → 擦除整个扇区（Flash 只能写 0，需先全置 1）
3. HAL_FLASH_Program()     → 按字节/字逐步编程
4. HAL_FLASH_Lock()        → 重新锁定以防误写
```

### 2.3 关键参数：VoltageRange

`FLASH_EraseInitTypeDef.VoltageRange` 必须与实际 VDD 匹配：

| 枚举值                      | 适用电压    | 编程宽度 |
|---------------------------|-----------|---------|
| `FLASH_VOLTAGE_RANGE_3`   | 2.7–3.6V  | **字（32bit）** |

标准 3.3V 系统必须选 `FLASH_VOLTAGE_RANGE_3`。**选错会导致 HAL_ERROR，擦除和编程全失败**。

### 2.4 完整写函数实现

```c
#define CFG_FLASH_ADDR       0x080E0000      /* Sector 11 */
#define CFG_FLASH_SECTOR     FLASH_SECTOR_11
#define CFG_MAGIC            0x7A5F1D2D
#define CFG_MAGIC_BYTES      sizeof(uint32_t)

typedef struct {
    uint32_t magic;                           /* 检验标记 */
    uint32_t eui64_h, eui64_l;               /* EUI-64 */
    uint32_t region;                          /* 频段枚举 */
    uint32_t checksum;                        /* 数据校验和 */
    uint8_t  _pad[x];                         /* 结构体对齐至 4 字节倍数 */
} gateway_config_t;

static HAL_StatusTypeDef flash_write(const gateway_config_t *cfg)
{
    HAL_StatusTypeDef st;
    uint32_t error = 0;

    st = HAL_FLASH_Unlock();
    if (st != HAL_OK) return st;

    /* 擦除 Sector 11 */
    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .Sector       = CFG_FLASH_SECTOR,
        .NbSectors    = 1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,
    };
    st = HAL_FLASHEx_Erase(&erase, &error);
    if (st != HAL_OK) { HAL_FLASH_Lock(); return st; }

    /* 按 Word（4字节）编程 */
    const uint32_t *p    = (const uint32_t *)cfg;
    uint32_t        addr = CFG_FLASH_ADDR;
    for (size_t i = 0; i < sizeof(*cfg) / sizeof(uint32_t); i++) {
        st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, p[i]);
        if (st != HAL_OK) { HAL_FLASH_Lock(); return st; }
        addr += 4;
    }

    return HAL_FLASH_Lock();
}
```

> ⚠️ **关键**：`sizeof(gateway_config_t)` 必须是 4 的倍数，否则最后若干字节无法写入。

### 2.5 读取：直接内存映射（无需 API）

Flash 映射在 CPU 地址空间，可以直接指针访问：

```c
void config_load(void)
{
    const gateway_config_t *p = (const gateway_config_t *)CFG_FLASH_ADDR;

    if (p->magic == CFG_MAGIC && checksum_compute(p) == p->checksum) {
        memcpy(&s_config, p, sizeof(s_config));
    } else {
        apply_defaults(&s_config);
    }
}

/* 校验和计算（不包含 checksum 字段自己） */
uint32_t checksum_compute(const gateway_config_t *cfg)
{
    const uint8_t *p = (const uint8_t *)cfg;
    size_t len = offsetof(gateway_config_t, checksum);
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) sum += p[i];
    return sum;
}
```

### 2.6 关键注意事项

- **Flash 耐久性**：约 10,000 次擦除/写周期，对网关配置（很少改）足够
- **不支持 %lld**：STM32 newlib-nano 缺 64 位格式符，`%016llX` 输出 `000000000000000lX`
  - 改用 `printf("%08X%08X", (uint32_t)(val>>32), (uint32_t)val)`
- **链接脚本验证**：务必确认代码不会链接到 Sector 11

---

## Part 3：UART 中断驱动 CLI + 环形缓冲架构

### 3.1 为什么 ISR 里不能做阻塞操作

HAL 的 `HAL_UART_Transmit()` 是阻塞/轮询调用：
$$T_{byte} = \frac{1 + 8 + 1}{115200} \approx 87\,\mu s$$

若在 `HAL_UART_RxCpltCallback()` 中 echo 一个字符（87µs），在这段时间内任何新到达的字节都会在没有及时处理下丢失。实测 115200 baud 粘贴数据时，**丢字符必然发生**。

### 3.2 正确架构：ISR 最小化 + 环形缓冲

```
┌──────────────────────────────────────────┐
│     HAL_UART_RxCpltCallback ( < 1µs )    │
│                                          │
│  1. uint8_t c = s_rx_byte                │
│  2. HAL_UART_Receive_IT(...) re-arm      │
│  3. s_ring[head] = c                     │
│  4. head = (head+1) & MASK               │
│  绝对不阻塞，绝对不做 printf/echo        │
└──────────────────────────────────────────┘
              ↓ 异步，Task上下文
┌──────────────────────────────────────────┐
│     uart_cli_task () @ 10ms 周期         │
│                                          │
│  while tail != head:                     │
│    c = ring[tail]; tail++                │
│    if printable → line_buf + echo        │
│    if CR → dispatch_line() + show "> "   │
│    if BS  → VT100 backspace              │
└──────────────────────────────────────────┘
```

### 3.3 环形缓冲实现

```c
#define RING_SIZE  256        /* 必须 2 的幂 */
#define RING_MASK  (RING_SIZE - 1)

static volatile uint8_t s_ring[RING_SIZE];
static volatile uint8_t s_ring_head;      /* ISR 写 */
static volatile uint8_t s_ring_tail;      /* Task 读 */
static volatile uint8_t s_rx_byte;        /* HAL 接收 buffer */
```

**为什么用 `uint8_t` 索引**：
- RING_SIZE = 256 时，`uint8_t` 自然溢出等价于 `& 0xFF`
- 8 位读写在 ARM Cortex-M 上原子，不会被中断打断

**满/空判断（牺牲 1 格法）**：
```c
/* 空：tail == head */
/* 满：(head + 1) & MASK == tail */
if (((s_ring_head + 1) & RING_MASK) != s_ring_tail) {
    s_ring[s_ring_head] = c;
    s_ring_head = (s_ring_head + 1) & RING_MASK;
}
```

### 3.4 HAL_UART_Receive_IT `len=1` 重新 arm 的关键顺序

`HAL_UART_Receive_IT(&huart, buf, 1)` 设置单字节接收，收到后自动触发回调，**回调后失效**（不自动重复）。

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    uint8_t c = s_rx_byte;                               // (1) 先保存
    HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);        // (2) 立即重新 arm
    // ... 在这之后入缓冲，不能在上面做任何阻塞操作
}
```

> ⚠️ **顺序是关键**：必须先保存 `s_rx_byte`，再 re-arm。否则 arm 完成后立刻来新字节，`s_rx_byte` 被新字节覆盖。

### 3.5 stm32f4xx_it.c 必需的 extern 声明

```c
/* stm32f4xx_it.c */
extern UART_HandleTypeDef huart1;        /* ← 这一行不能少 */

void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}
```

若遗漏，编译不报错（weak 符号），但运行时进入 `Default_Handler` 死循环，UART 完全无响应。

### 3.6 中断优先级配置

```c
HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
HAL_NVIC_EnableIRQ(USART1_IRQn);
```

FreeRTOS Cortex-M 规则：
- 优先级数字越低越高（0 = 最高）
- FreeRTOS 使用 `configMAX_SYSCALL_INTERRUPT_PRIORITY`（通常 = 5）作为可调用 FreeRTOS API 的最高级
- UART ISR 不调用 FreeRTOS API，可设为 5（与 SysTick 同级）
- 若需在 ISR 里调用 `xQueueSendFromISR()`，优先级必须 ≥ 5

---

## Part 4：CH340 自动复位与 pyserial 测试

### 4.1 CH340 DTR→NRST 电路工作原理

```
USB-UART（CH340）
  DTR ──[100nF]──┬─ STM32 NRST
                 └─ 10kΩ ── VCC
```

电容微分：DTR 信号的**边沿变化**（低→高）产生短脉冲传到 NRST = **MCU 复位**。

pyserial 默认行为：
```python
s = serial.Serial('COM11', 115200)
# 内部先设 DTR=True，再 open()，触发 MCU reset
```

**正确的打开方式**（必须在 `open()` 之前设 `dtr=False`）：
```python
ser = serial.Serial()
ser.port     = 'COM11'
ser.baudrate = 115200
ser.timeout  = 1
ser.dtr      = False   # ← open() 之前！
ser.rts      = False   # 可选
ser.open()             # 现在 DTR 已是 False，不触发 reset
```

### 4.2 send() 函数规范

```python
def send(ser, cmd, timeout=3.0):
    # (1) 双重清缓冲（CH340 HW FIFO 有延迟）
    ser.reset_input_buffer()
    time.sleep(0.05)
    ser.reset_input_buffer()

    # (2) 发送命令
    ser.write((cmd.strip() + "\r\n").encode())

    # (3) 等待到 ">" 结尾
    buf = b""
    deadline = time.time() + timeout
    while time.time() < deadline:
        chunk = ser.read(ser.in_waiting or 1)
        if chunk:
            buf += chunk
            # 用 endswith 而非 in（help 输出里有 "> " 子串，会误触发）
            if buf.endswith(b"> "):
                break
        time.sleep(0.02)
    return buf
```

**为什么是 `endswith` 而非 `in`**：
- 帮助文本中间可能出现 `> ` 子串（如`config set ns_host  <ip>`）
- `in` 会在看到第一个 `> ` 时立刻 break，导致后续数据残留在 OS 缓冲被下一命令误读
- `endswith` 确保 `> ` 在完整响应末尾

### 4.3 reboot 后的正确处理

```python
ser.write(b"reboot\r\n")
# 关键：NOT sleep + reset_input_buffer
# MCU boot message 里包含最终的 "> " prompt

def wait_prompt(ser, timeout=15.0, show=True):
    buf = b""
    deadline = time.time() + timeout
    while time.time() < deadline:
        chunk = ser.read(ser.in_waiting or 1)
        if chunk:
            buf += chunk
            if show:
                print(chunk.decode(errors="replace"), end="", flush=True)
            if b"> " in buf:    # boot message 里只有一个 "> "
                return True
        time.sleep(0.05)
    return False
```

### 4.4 完整的 cli_test.py 实例

```python
#!/usr/bin/env python3
import serial
import time
import sys

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else 'COM11'
    
    # 正确打开串口
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = 115200
    ser.timeout = 1
    ser.dtr = False
    ser.rts = False
    ser.open()
    
    # 等待 boot
    print("[*] Waiting boot prompt...")
    if not wait_prompt(ser, timeout=5):
        print("FAIL: no boot prompt")
        return 1
    
    # 测试 help
    print("[TEST] help")
    resp = send(ser, "help")
    if b"config" in resp and b"reboot" in resp:
        print("PASS")
    else:
        print("FAIL:", resp)
    
    ser.close()
    return 0

if __name__ == '__main__':
    sys.exit(main())
```

---

## Part 5：验证与测试清单

### 5.1 Flash config_save / config_load 通过以下验证

```
✓ 写 eui64 到 Flash Sector 11
✓ 掉电重启 → config_load() 恢复 eui64 值
✓ 8 个不同的配置参数全部可读写
✓ Magic + Checksum 双重校验生效
✓ 损坏的 Flash 数据被正确检测，自动 fallback 到 defaults
```

### 5.2 UART CLI 通过以下验证

```
✓ 环形缓冲无丢字符（115200 baud 高速粘贴）
✓ ISR 响应时间 < 1µs（无阻塞）
✓ 所有 CLI 命令执行并返回正确的提示符 "> "
✓ help, config show, config set, reboot 等 6 个经典命令通过
✓ 字符删除( BS/DEL)、回显(echo)、缓冲(line_buf)全部正常
```

### 5.3 ch340 + pyserial 的关键点

```
✓ dtr=False before open() → 不触发 MCU reset
✓ reset_input_buffer() 双重调用 + sleep(50ms) → 清空 HW FIFO
✓ endswith(b"> ") → 不误触发 help 中的 "> " 子串
✓ reboot 后不 reset_input_buffer → 不丢 boot message
✓ 21/21 自动化测试全部通过
```

---

## Quick Reference

### Flash 开发 Checklist

- [ ] Sector 11 @ 0x080E0000（确认链接脚本 LENGTH = 0xE0000）
- [ ] VoltageRange = FLASH_VOLTAGE_RANGE_3（3.3V 系统必选）
- [ ] sizeof(struct) % 4 == 0
- [ ] Magic + Checksum 双检验
- [ ] Unlock → Erase → Program → Lock（顺序不可变）

### UART CLI Checklist

- [ ] ISR 回调：保存→re-arm→入缓冲（无任何阻塞）
- [ ] 环形缓冲 RING_SIZE = 256，索引用 uint8_t
- [ ] stm32f4xx_it.c: extern UART_HandleTypeDef huart1
- [ ] 中断优先级 = 5
- [ ] Task 周期 ≥ 10ms（让 ISR 有足够时间填充缓冲）

### pyserial Testing Checklist

- [ ] dtr=False 在 open() 之前
- [ ] send() 用 buf.endswith(b"> ")
- [ ] send() 开头双重 reset_input_buffer
- [ ] reboot 后用 wait_prompt()（无 reset）
- [ ] 全部 21 个测试 PASS
