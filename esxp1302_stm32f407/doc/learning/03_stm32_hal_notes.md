# 03 — STM32 HAL / FreeRTOS 使用要点

## CubeMX 生成代码结构

```
Core/
├── Inc/          # 头文件 (main.h, stm32f4xx_hal_conf.h, ...)
├── Src/          # 源文件 (main.c, freertos.c, stm32f4xx_it.c, ...)
Drivers/
├── CMSIS/        # ARM CMSIS 标准头 + RTOS2 wrapper
├── STM32F4xx_HAL_Driver/  # ST HAL 驱动库
Middlewares/
├── Third_Party/FreeRTOS/  # FreeRTOS 内核
```

### 用户代码区域

CubeMX 重新生成时会保留 `USER CODE BEGIN/END` 之间的代码:
```c
/* USER CODE BEGIN 2 */
// 这里的代码在重新生成后保留
/* USER CODE END 2 */
```

⚠️ 在 `USER CODE` 区域之外写的代码会在重新生成时被覆盖。

---

## SPI 使用要点

### HAL SPI 基本接口

```c
/* 全双工传输 */
HAL_SPI_TransmitReceive(&hspi3, tx_buf, rx_buf, len, timeout);

/* 仅发送 */
HAL_SPI_Transmit(&hspi3, tx_buf, len, timeout);

/* 仅接收 */
HAL_SPI_Receive(&hspi3, rx_buf, len, timeout);
```

### 手动片选 (本项目方式)

CubeMX 配置 SPI NSS 为 **Software** 模式，手动控制片选线:

```c
/* NSS = PB12 */
HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);  // CS Low
HAL_SPI_TransmitReceive(&hspi3, ...);
HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);     // CS High
```

为什么不用硬件 NSS？
- SX1302 的 SPI 协议要求在一次多字节事务中 CS 保持 Low
- STM32 硬件 NSS 在某些模式下会在每字节间自动拉高拉低
- 手动控制最可靠

### SPI 配置参数

| 参数 | 值 |
|------|-----|
| 实例 | SPI3 |
| 引脚 | SCK=PC10, MISO=PC11, MOSI=PC12 |
| NSS | PB12 (GPIO Output, 软件控制) |
| 模式 | Master, Full-Duplex |
| CPOL/CPHA | Mode 0 (CPOL=Low, CPHA=1Edge) |
| 数据大小 | 8-bit |
| 字节序 | MSB First |
| 分频 | /8 (实际时钟取决于 APB 频率) |

---

## I2C 使用要点

### HAL I2C 基本接口

```c
/* 内存式写入 (寄存器地址 + 数据) */
HAL_I2C_Mem_Write(&hi2c2, dev_addr << 1, reg, I2C_MEMADD_SIZE_8BIT,
                  data, len, timeout);

/* 内存式读取 */
HAL_I2C_Mem_Read(&hi2c2, dev_addr << 1, reg, I2C_MEMADD_SIZE_8BIT,
                 data, len, timeout);
```

### 地址左移

STM32 HAL I2C 函数 **要求 7 位地址左移 1 位** (变成 8 位格式):
- LM75A 7-bit 地址: `0x48`
- 传给 HAL: `0x48 << 1 = 0x90`

⚠️ 常见错误: 忘记左移导致通信失败

---

## GPIO 使用要点

### 读写

```c
/* 写 */
HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);

/* 读 */
GPIO_PinState state = HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
```

### SX1302 Reset 时序

```c
void lgw_reset(void) {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_SET);    // RESET = High
    HAL_Delay(100);                                         // 100ms
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_RESET);  // RESET = Low
    HAL_Delay(500);                                         // 500ms (等待 PLL 稳定)
}
```

---

## 定时器 (TIM2) 微秒计数器

### 配置

TIM2 是 32 位定时器，配置为 1 MHz 自由运行计数器:
- APB1 Timer Clock = 84 MHz
- Prescaler = 83 → 84 MHz / (83+1) = 1 MHz
- Period = 0xFFFFFFFF (最大值)
- 计数器每 µs 加 1
- 溢出周期 ≈ 71.6 分钟

### 使用

```c
uint32_t timestamp_us(void) {
    return __HAL_TIM_GET_COUNTER(&htim2);
}

void wait_us(uint32_t us) {
    uint32_t start = timestamp_us();
    while ((timestamp_us() - start) < us);  // 忙等待
}
```

---

## FreeRTOS (CMSIS-RTOS2) 要点

### 任务创建

CubeMX 中创建的默认任务在 `freertos.c`:

```c
void StartDefaultTask(void *argument) {
    /* USER CODE BEGIN 5 */
    extern void app_main(void);
    app_main();        // 调用测试入口
    for(;;) {
        osDelay(1000);
    }
    /* USER CODE END 5 */
}
```

### 延时函数

| 函数 | 说明 |
|------|------|
| `osDelay(ms)` | RTOS tick 级延时，释放 CPU |
| `HAL_Delay(ms)` | SysTick 忙等待，不释放 CPU |
| `wait_ms(ms)` | 封装 `osDelay(ms)` |
| `wait_us(us)` | 封装 TIM2 忙等待 |

> ⚠️ 在 FreeRTOS 中使用 `HAL_Delay()` 可能导致低优先级任务饿死。
> 优先使用 `osDelay()` 进行毫秒级延时。

### 堆栈大小

ARM Cortex-M4 堆栈是向下增长的。如果任务堆栈太小，会导致:
- 局部变量覆盖
- HardFault

CubeMX 中 `defaultTask` 堆栈设为 **4096 words** (16 KB) 以容纳 HAL 测试的大数组。

---

## Printf 重定向

### USART1 输出

通过重写 `_write()` 或 `__io_putchar()` 将 `printf` 输出到 USART1:

```c
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
```

### 浮点 printf

newlib-nano 默认不支持 `%f`，需要在链接选项中添加:

```cmake
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -u_printf_float)
```

无此选项时 `printf("%.1f", 22.5)` 不会输出任何内容（不报错，只是静默跳过）。

---

## 中断优先级

使用 FreeRTOS 时，所有调用 FreeRTOS API 的中断优先级必须 ≥ `configMAX_SYSCALL_INTERRUPT_PRIORITY` (默认 5):

| 中断 | 优先级 | 说明 |
|------|--------|------|
| SysTick | 15 | FreeRTOS tick |
| SVC | 0 | RTOS 服务调用 |
| PendSV | 15 | 上下文切换 |
| USART1 | 5 | printf 输出 |
| SPI3 | 5 | (如果使用中断模式) |
