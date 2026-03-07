# BUG-010 学习笔记：ackr 为什么低？怎么找到的？

> 对应 bugfix: [BUG-010](../bugfix/BUG-010_push_data_ack_low_out_of_sync.md)  
> 对应代码: `main/packet_forwarder/lora_pkt_fwd.c`  
> 写法：结合实际代码行号和串口日志，还原当时发生了什么

---

## 第零步：先看懂串口输出在说什么

网关运行时串口会持续打印日志，关键的一行是 30 秒统计：

```
##### 2026-02-21 10:00:30 GMT #####
### [UPSTREAM] ###
# RF packets received by concentrator: 3
# CRC_OK: 100.00%, CRC_FAIL: 0.00%, NO_CRC: 0.00%
# RF packets forwarded: 3 (171 bytes)
# PUSH_DATA datagrams sent: 2 (2154 bytes)
# PUSH_DATA acknowledged: 22.22%        ← 这行是问题所在
### [DOWNSTREAM] ###
# PULL_DATA sent: 3 (100.00% acknowledged)
```

**ackr（acknowledged ratio）就是最后那行的百分比**，计算方式是：

$$\text{ackr} = \frac{\text{收到 PUSH\_ACK 次数}}{\text{发出 PUSH\_DATA 次数}} \times 100\%$$

收 LoRa 包、CRC 检查、上报 JSON 这些都是正常的，
问题专门出在"发出去之后有没有收到 NS 的确认回包"这一步。

---

## 第一步：先搞清楚 PUSH_DATA → PUSH_ACK 的完整流程

代码里负责这件事的是 `thread_up` 函数。
它是一个无限循环，大概每隔 2 秒执行一轮，流程如下：

```
┌─────────────────────────────────────────────────────────┐
│ thread_up 一轮的时间线（约 2 秒）                          │
│                                                         │
│  0ms      收 LoRa 包，组 JSON，填充 buff_up              │
│           生成随机 token（比如 0x1C7F），写入包头         │
│           ↓                                             │
│           drain：清空 sock_up 里可能残留的旧 ACK           │
│           ↓                                             │
│           send(sock_up, buff_up, ...)   → 发出 PUSH_DATA │
│           ↓                                             │
│           第 1 次 recv()，等最多 250ms                   │
│           ↓ 超时或收到                                  │
│           第 2 次 recv()，等最多 250ms                   │
│           ↓                                             │
│           检查收到的包：token 对不对？版本对不对？         │
│           对了 → meas_up_ack_rcv++，打印"PUSH_ACK in Xms" │
│           不对 → 打印"out-of-sync"警告                   │
│                                                         │
│ ~500ms   这轮结束，继续等下一批 LoRa 包                   │
└─────────────────────────────────────────────────────────┘
```

对应代码在 lora_pkt_fwd.c 第 2633–2683 行：

```c
// 第 2633 行：drain 旧 ACK
{
    uint8_t _tmp[4];
    while (recv(sock_up, (void *)_tmp, sizeof _tmp, MSG_DONTWAIT) > 0) {}
}

// 第 2641 行：发出 PUSH_DATA
send(sock_up, (void *)buff_up, buff_index, 0);

// 第 2651 行：等待 ACK，循环两次，每次最多等 push_timeout_half
for (i=0; i<2; ++i) {
    j = recv(sock_up, (void *)buff_ack, sizeof buff_ack, 0);
    if (j == -1) {
        if (errno == EAGAIN) { /* 超时，继续下一次循环 */
            continue;
        } else {
            MSG("WARNING: [up] recv error: %s\n", strerror(errno));
            break;
        }
    } else if ((buff_ack[1] != token_h) || (buff_ack[2] != token_l)) {
        MSG("WARNING: [up] ignored out-of sync ACK packet\n");  // token 不匹配
        continue;
    } else {
        MSG("INFO: [up] PUSH_ACK received in %i ms\n", ...);
        meas_up_ack_rcv += 1;  // 这个计数器就是 ackr 的分子
        break;
    }
}
```

**`push_timeout_half` 控制每次 `recv()` 最多等多久**，它在第 256 行定义：

```c
static struct timeval push_timeout_half = {0, (PUSH_TIMEOUT_MS * 500)};
```

---

## 交叉验证：先确认问题在哪一侧

遇到 ackr 低，最容易犯的错误是直接改代码——但在改之前，要先知道**问题出在哪里**。

用本地 Mock 服务器替代真实 NS：

```bash
python3 scripts/lora_ns_mock.py 1700
```

这个脚本做的事极其简单：收到 PUSH_DATA 就**立刻**回 PUSH_ACK，并打印一行日志。

对比两侧日志：

```
[Mock 服务器]  PUSH_DATA from ('192.168.1.50', 52313) token=0x1C7F -> PUSH_ACK sent ✓
[网关串口]     PUSH_DATA acknowledged: 53.00%   ← 53% 没收到
```

**Mock 说它发出了，网关说它没收到。** 结论：问题在网关侧，不是 NS 问题，也不是"ACK 没发出"。
问题在网关"发出 PUSH_DATA 之后能否在时间窗口内收到回包"这一步。

---

## 问题 1：MQTT 客户端的 TLS 握手吃掉了所有时间

### 案发现场（修复前的串口日志）

```
E (15234) MQTT_CLIENT: Error transport connect
E (15234) esp-tls: Failed to open new connection
E (15234) TRANSPORT_SSL: Failed to open a new connection
MQTT_EVENT_DISCONNECTED
... （10 秒后重试）
E (25891) MQTT_CLIENT: Error transport connect
```

这 10 秒里，网关几乎没有打印任何 PUSH_ACK 相关的日志。

### 问题在哪里：FreeRTOS 任务和 MQTT 任务的关系

ESP32 运行 FreeRTOS。程序不是"一行一行顺序执行"的，
而是同时跑多个"任务"（Task），每个任务有独立的执行流。
FreeRTOS 按照优先级和时间片轮流让各任务执行。

WiFi 连上后，第 4125 行启动了两个任务：

```c
// 第 4125 行：
#if ENABLE_MQTT
    xTaskCreatePinnedToCore(
        (TaskFunction_t) mqtt_task,  // 任务函数
        "mqtt",                      // 任务名
        1*4096,                      // 栈大小（字节）
        NULL,
        6,                           // 优先级 6
        &mqtt_handle,
        0                            // 固定在 CPU 核 0
    );
#endif

// 第 4128 行：
xTaskCreatePinnedToCore(
    (TaskFunction_t) pkt_fwd_task,   // 包含 thread_up 的任务
    "pkt_fwd",
    1*4096,
    NULL,
    6,                               // 优先级也是 6
    &pkt_fwd_handle,
    0                                // 也固定在 CPU 核 0 ！
);
```

**关键点：两个任务都固定在 CPU 核 0，优先级相同。**
FreeRTOS 对同优先级任务做时间片轮换，但如果一个任务长时间不主动让出 CPU
（比如卡在 TLS 握手里），另一个任务就得不到充分执行机会。

`mqtt_task` 里调用了 MQTT 客户端连接 `mqtt://192.168.1.202`（第 187–188 行的配置）。
这台机器没有 MQTT Broker，TCP 连接和 TLS 握手会**超时等待长达 10 秒**。
在这 10 秒里，`pkt_fwd_task`（包含 `thread_up`）能抢到的 CPU 时间极少，
导致 `recv()` 的 500ms 窗口内根本没有机会运行，ACK 到了也没人读。

### 修复：把 MQTT 完全编译掉

第 187 行：

```c
// 修复前：
#define ENABLE_MQTT  1

// 修复后：
#define ENABLE_MQTT  0   /* set to 1 to enable MQTT, 0 to disable */
```

C 语言的条件编译（`#if` / `#endif`）：
如果 `ENABLE_MQTT` 为 0，两个 `#if ENABLE_MQTT` … `#endif` 之间的所有代码
**完全不会被编译进固件**，就像这些代码不存在一样。
`xTaskCreatePinnedToCore(mqtt_task, ...)` 那行也就不会运行。

### 修复后效果

ackr 从 22–40% 提升到 53–70%。MQTT 阻塞确实是主要干扰源，但还有其他问题。

---

## 问题 2：tv_usec 填了一个超出合法范围的值，lwIP 把它当 0 处理

### 背景：`struct timeval` 是什么，怎么用

`recv()` 不是永远等下去的。可以预先给 socket 设置一个超时值，
超过时间没收到数据，`recv()` 就返回错误，错误码是 `EAGAIN`（意思是"下次再试"）。

设置超时用的是 `setsockopt()`，超时时间用 `struct timeval` 表示：

```c
// <sys/time.h> 里定义的结构体：
struct timeval {
    long tv_sec;   // 秒数（整数部分）
    long tv_usec;  // 微秒数（小数部分）
                   // 1 秒 = 1,000,000 微秒
                   // ⚠️ 合法范围：0 ~ 999,999（不能 >= 1,000,000）
};

// 例：设置 250ms 超时
struct timeval t;
t.tv_sec  = 0;
t.tv_usec = 250000;  // 250,000 微秒 = 0.25 秒 = 250ms

// 例：设置 1.5 秒超时
t.tv_sec  = 1;       // 1 秒
t.tv_usec = 500000;  // + 0.5 秒

// 然后应用到 socket：
setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof t);
// 参数 1: socket 文件描述符
// 参数 2: SOL_SOCKET = 这是 socket 级别的选项（不是 TCP 级别）
// 参数 3: SO_RCVTIMEO = 设置接收超时（Receive Timeout）
// 参数 4: 指向 timeval 结构体的指针
// 参数 5: 结构体大小
```

### 代码里的实际计算（出了什么问题）

第 109 行定义超时宏，第 256 行用它初始化结构体：

```c
// 第 109 行（当时的错误值）：
#define PUSH_TIMEOUT_MS  2000

// 第 256 行：
static struct timeval push_timeout_half = {0, (PUSH_TIMEOUT_MS * 500)};
//                             tv_sec=0 ↑   ↑ tv_usec = 2000 × 500 = 1,000,000
//                                          ← 正好等于 1 秒，超出合法上限 999,999！
```

为什么乘以 500？代码设计是"每次等一半的超时，循环两次"：

```
push_timeout_half.tv_usec = PUSH_TIMEOUT_MS × 500
                           = 2000 × 500 = 1,000,000  ← 非法！

设计意图：总超时 2000ms，分两次，每次 1000ms
正确写法：应该用 tv_sec=1, tv_usec=0
但代码只用了 tv_usec 字段，1,000,000 就溢出合法范围了
```

### lwIP 遇到非法值怎么处理

ESP32 的网络库 lwIP 对 `setsockopt()` 里的 `struct timeval` 做合法性检查：

```
如果 tv_usec >= 1,000,000 → 整个 timeval 当作无效，超时设为 0（立刻返回）
```

**效果：`recv()` 的超时变成了 0 毫秒——调用后立刻返回 `EAGAIN`，完全不等待。**

### 实际发生了什么（代码执行路径）

```c
// push_timeout_half = {tv_sec=0, tv_usec=1,000,000}
// lwIP 处理为：超时 = 0ms

// 第 1777 行（socket 初始化时）：
setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &push_timeout_half, sizeof push_timeout_half);
// ↑ 实际设置了 0ms 超时

// 第 2651 行（等 ACK 循环）：
for (i=0; i<2; ++i) {
    j = recv(sock_up, (void *)buff_ack, sizeof buff_ack, 0);
    // ↑ 超时=0ms，立刻返回 -1
    if (errno == EAGAIN) continue;  // ← 每次都走这里
}
// 两次循环都立刻超时，meas_up_ack_rcv 从不增加，ackr 永远是 0%
```

这是典型的"静默 bug"——程序没有报错，日志只是多了很多 EAGAIN，
行为却完全错了。

### 还有一个覆盖关系要注意

代码有两层超时设置，第 1197–1200 行会在启动时用 JSON 配置文件**覆盖** `#define` 的值：

```c
// 第 1197 行（parse_gateway_configuration 函数里）：
val = json_object_get_value(conf_obj, "push_timeout_ms");
if (val != NULL) {
    push_timeout_half.tv_usec = 500 * (long int)json_value_get_number(val);
    //  ↑ 用 JSON 里的值直接覆盖掉第 256 行 #define 算出来的初始值
    MSG("INFO: upstream PUSH_DATA time-out is configured to %u ms\n",
        (unsigned)(push_timeout_half.tv_usec / 500));
}
```

所以 `push_timeout_half` 的实际值：
1. 程序启动时按 `#define PUSH_TIMEOUT_MS` 算出初始值（第 256 行）
2. **然后**调用 `parse_gateway_configuration()`，如果 JSON 里有 `"push_timeout_ms"` 字段，就用它**覆盖**

当时 `global_conf.cn490.json` 里是 `"push_timeout_ms": 100`（原始配置的值）：
```
tv_usec = 500 × 100 = 50,000 微秒 = 50ms/次，总共最多等 100ms
```
50ms 对于任何真实网络都太短（局域网 RTT 约 1–5ms，手机热点约 300ms）。

### 修复

```c
// 第 109 行，修正后：
#define PUSH_TIMEOUT_MS  500  // tv_usec = 500×500 = 250,000 ✓ 合法，250ms/次
#define PULL_TIMEOUT_MS  400  // tv_usec = 400×1000 = 400,000 ✓ 合法，400ms
```

同步修改 `global_conf.cn490.json`（否则启动时会被 JSON 里的旧值覆盖掉代码的修正）：

```json
"push_timeout_ms": 500
```

修改后重新生成 `global_json.h`（这个文件是 JSON 的二进制嵌入版，固件从它读取配置）。

---

## 问题 3：清空旧 ACK 的 drain 循环有一个低概率陷阱

### 为什么需要 drain（清空旧 ACK）

设想这个场景：

```
轮次 1：
  t=0ms    发出 PUSH_DATA（token = 0x1C7F）
  t=250ms  第 1 次 recv() 超时，没收到（网络抖动）
  t=500ms  第 2 次 recv() 超时，没收到
           ackr 分子不增加，这轮失败

轮次 2：
  t=550ms  准备发新 PUSH_DATA（token 换成 0x4D21）
           ↓ 此刻 socket 缓冲区里有什么？
  t=580ms  上一轮的 PUSH_ACK（token=0x1C7F）姗姗来迟，进入 sock_up 缓冲区
  t=581ms  发出新 PUSH_DATA（token = 0x4D21）
  t=582ms  recv() 读到缓冲区里的旧 ACK（token = 0x1C7F）
           代码检查：0x1C7F ≠ 0x4D21
           → MSG("WARNING: [up] ignored out-of sync ACK packet\n")
           → 这轮也失败！

轮次 3：
  t=600ms  NS 真正回应 0x4D21 的 ACK 到来
           但 recv() 已经在轮次 2 里消耗掉了两次机会（一次读旧ACK，一次超时）
           → 这个 ACK 没人读，又留在缓冲区等下一轮...恶性循环
```

所以每次发新 PUSH_DATA **之前**，必须先把 socket 缓冲区里积压的旧 ACK 清空。

### 旧代码的做法（修复前）

```c
// 旧代码（有风险）：
struct timeval drain_tv = {0, 1000}; // 1ms 超时（几乎等于"立刻返回"）
uint8_t _tmp[4];

// ① 把 sock_up 的接收超时改为 1ms
setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &drain_tv, sizeof drain_tv);

// ② 反复 recv() 直到缓冲区空了（返回 -1 表示 EAGAIN，没东西了）
while (recv(sock_up, (void *)_tmp, sizeof _tmp, 0) > 0) {}

// ③ 把超时恢复回 250ms，后面真正等 ACK 的 recv() 才能有时间等
setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, (void *)&push_timeout_half,
           sizeof push_timeout_half);
```

逻辑上没问题，但第 ③ 步的 `setsockopt()` 没有检查返回值。
**`setsockopt()` 在 ESP32 lwIP 里，在内存压力大时可能返回错误（-1）。**

如果第 ③ 步悄悄失败了：

```
修改失败 → sock_up 的超时永远停在 1ms
→ 后面所有 recv() 等 1ms 就放弃
→ ACK 不管什么时候到都收不到
→ ackr 跌到接近 0%
→ 重启才能恢复
```

这种 bug 不是每次都出现，只有在内存压力大的特定时刻才偶发，很难稳定复现。

### 修复：用 `MSG_DONTWAIT` 标志，完全不碰 `SO_RCVTIMEO`

修复后的第 2637–2639 行：

```c
// 新代码：
uint8_t _tmp[4];
while (recv(sock_up, (void *)_tmp, sizeof _tmp, MSG_DONTWAIT) > 0) {}
```

`recv()` 的完整函数签名：

```c
ssize_t recv(int sockfd,   // socket 文件描述符
             void *buf,    // 接收缓冲区
             size_t len,   // 最多读多少字节
             int flags);   // 标志位（这里是关键）
```

`MSG_DONTWAIT` 是一个标志位，含义是：
"这**一次** `recv()` 调用，不管 socket 有没有设置超时，
如果缓冲区里没有数据，立刻返回 `EAGAIN`，不要等。"

它只影响这一次调用，**不修改 socket 的 `SO_RCVTIMEO` 设置**。
所以后面真正等 ACK 的 `recv()`（第 2655 行，没有 `MSG_DONTWAIT` 标志）
还是按照之前 `setsockopt` 设置的 250ms 超时来等。

用这种方式，整个 drain 过程完全不需要碰 `SO_RCVTIMEO`，
也就不存在"恢复失败"的风险。

---

## 问题 4：WiFi 在打盹，路由器替它扣押了 ACK 包

### 这是 ackr 从 70% 提到 86–100% 的关键修复

解决前面三个问题后，ackr 还是只有 60–80%。
Mock 日志显示 ACK 100% 发出了，网关就是收不到，说明 ACK 在"路由器到网关"这段消失了。
这跟代码逻辑无关，是硬件/驱动层的行为。

### WiFi Modem Sleep 是什么

ESP32 WiFi 有几种省电模式，默认是 `WIFI_PS_MIN_MODEM`（最小调制解调器睡眠）。

在这个模式下，ESP32 WiFi 无线电会周期性关闭，只在路由器发送 Beacon 信标帧的时候醒来。
Beacon 的间隔由路由器配置，典型值是 100ms（DTIM=1）到 300ms（DTIM=3）。

**当 ESP32 无线电休眠时，路由器发给它的单播包怎么办？**
路由器不会丢弃，会**缓存**这些包，等 ESP32 在下一个 Beacon 周期醒来后再发送。

### 时间线：ACK 是怎么被卡住的

```
假设路由器 DTIM Beacon 间隔 = 200ms

t=0ms    网关发出 PUSH_DATA
t=0ms    ESP32 WiFi 恰好进入休眠（刚刚发完包就休眠了）
t=2ms    NS 收到，立刻回 PUSH_ACK（局域网 RTT < 5ms）
t=2ms    PUSH_ACK 到达路由器

         ┌──────────────────────────────────────────────┐
         │  ESP32 WiFi 无线电正在休眠（约 198ms）          │
         │  路由器：这个包目标是 ESP32，它在睡，先缓存着   │
         └──────────────────────────────────────────────┘

t=200ms  ESP32 WiFi 醒来，接收到路由器的 Beacon
t=200ms  路由器："你醒了，把之前缓存的包给你"
t=200ms  PUSH_ACK 终于到达 ESP32 recv() 缓冲区

此时 thread_up 的 recv() 窗口状态：
  第 1 次 recv()：t=0 → t=250ms（250ms 超时）
  t=200ms 收到，距超时还剩 50ms，能收到 ✓

  但如果 DTIM 间隔是 300ms：
  t=300ms 才到，超过 250ms 窗口 → recv() 已经超时退出 ✗
```

这解释了为什么 ackr 不是 0%（不是完全收不到），而是 60–80%：
大多数情况 ACK 在窗口内到达，但当 WiFi 睡眠周期刚好让 ACK 延迟到窗口末尾时就丢掉了。

### 修复：第 4168–4172 行

```c
// wifi_init_sta() 函数里：
ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
ESP_ERROR_CHECK(esp_wifi_start());

// ↓ 新增这一行，紧接在 esp_wifi_start() 之后：
ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
// WIFI_PS_NONE = 完全禁用省电模式
// WiFi 无线电始终保持开启，路由器发来的包立刻送达，不再缓存
```

关于 `ESP_ERROR_CHECK()`：这是 ESP-IDF 提供的宏，展开后大概是：

```c
// 等价于：
esp_err_t err = esp_wifi_set_ps(WIFI_PS_NONE);
if (err != ESP_OK) {
    ESP_LOGE(..., "esp_wifi_set_ps failed: %s", esp_err_to_name(err));
    abort();  // 打印错误并重启
}
```

用它包裹关键初始化操作，确保设置失败时不会静默地继续运行（避免产生难以追查的后续问题）。

### 修复后实测数据

```
修复前（有 WiFi PS）：
  INFO: [up] PUSH_ACK received in 2 ms
  INFO: [up] PUSH_ACK received in 237 ms   ← 快超时了
  （某些轮次静默超时，没有打印）
  PUSH_DATA acknowledged: 70%

修复后（WIFI_PS_NONE）：
  INFO: [up] PUSH_ACK received in 2 ms
  INFO: [up] PUSH_ACK received in 3 ms
  INFO: [up] PUSH_ACK received in 47 ms    ← 远端 NS，延迟正常
  INFO: [up] PUSH_ACK received in 94 ms
  PUSH_DATA acknowledged: 100.00%
```

---

## 全程 ackr 变化回顾

```
初始状态：22–40%
  ↓ 关掉 MQTT（ENABLE_MQTT=0，消除 10s TLS 阻塞）
  53–70%
  ↓ 修正超时值（PUSH_TIMEOUT_MS=500，消除 tv_usec 溢出）
    同步更新 JSON + 重新生成 global_json.h
  60–80%
  ↓ 禁用 WiFi Modem Sleep（WIFI_PS_NONE，消除 AP 缓冲延迟）
  86–100% ✓

（drain 改 MSG_DONTWAIT 是防御性修复，正常情况看不出差别，
  消除了内存压力下偶发的"超时永久卡 1ms"极端情况）
```

---

## 调试思路总结

**1. 交叉验证找断点，不要猜**
让两端都有日志：Mock 发了 + 网关没收到 → 问题在传输/接收层。
这个方法能迅速把"可能范围"缩到某一侧，省去大量猜测时间。

**2. 静默 bug：改了参数没效果时，先验证参数是否真的生效**
`tv_usec = 1,000,000` 没有任何报错。
遇到"改了参数完全没效果"的情况，在关键位置打 log 验证：
```c
MSG("DBG: push_timeout_half.tv_usec = %ld\n", push_timeout_half.tv_usec);
```

**3. 偶发 bug：改设计比加重试更有效**
setsockopt 失败是低概率事件，用 `MSG_DONTWAIT` 从根本上消除那个步骤，
让 bug 没有机会出现，比事后处理更可靠。

**4. 硬件特性也是 bug 来源**
WiFi Modem Sleep 是驱动行为，代码逻辑完全正确也会导致数据延迟。
以后遇到"代码没问题但数据还是丢失/延迟"，要考虑：
- 硬件层有没有省电/缓冲机制？
- 驱动层有没有 DMA、中断延迟？
- OS 层有没有任务调度影响？

---

## 扩展知识

---

### Q1：`struct timeval` 的 `tv_sec` 和 `tv_usec` 怎么组合？

**答：对，`{1, 500000}` 就是 1.5 秒。** 规则很简单：

$$\text{实际超时} = \text{tv\_sec} \times 1\text{s} + \text{tv\_usec} \times 1\mu\text{s}$$

```c
// 常见例子：
{0, 250000}   // 0 + 250,000μs = 250ms
{0, 500000}   // 0 + 500,000μs = 500ms
{1, 0}        // 1s + 0 = 1000ms
{1, 500000}   // 1s + 500,000μs = 1500ms
{2, 0}        // 2s
```

**关键约束：`tv_usec` 必须在 `[0, 999999]` 范围内。**
`tv_usec` 表示不足 1 秒的"余数"部分，如果写成 `1,000,000` 就相当于多了整整 1 秒，
标准规范要求进位到 `tv_sec`，不允许 `tv_usec >= 1,000,000`。

Linux 和 lwIP 对此的处理不同：
- Linux glibc：**静默进位**（`1,000,000μs` → `tv_sec+1, tv_usec=0`），不报错
- **lwIP（ESP32）：直接拒绝，把整个 timeval 当 `{0,0}` 处理**（超时=0，立刻返回）

这就是 BUG-010 里"改了 `#define` 但完全没效果"的根本原因——lwIP 比 Linux 更严格。

---

### Q2：DTIM Beacon 机制具体怎么工作的？

**三个概念要区分清楚：Beacon、TIM、DTIM。**

**Beacon 帧**是 WiFi AP（路由器）定期广播的管理帧，告诉附近所有设备"我在这里，这是我的参数"。
默认间隔 `beacon_interval = 100ms`（即每秒 10 次），这是 802.11 协议规定的基础节拍。

**TIM（Traffic Indication Map）** 是 Beacon 帧里的一个字段，AP 用它通告"哪些处于省电模式的 STA 有数据包在等待"。STA 收到 Beacon，看自己的 AID 在 TIM 里有没有标记，有的话再发 PS-Poll 帧请求 AP 把缓存的包发过来。

**DTIM（Delivery TIM）** 是 TIM 的超集，每隔 `DTIM Period` 个 Beacon 才出现一次。
DTIM Beacon 比普通 Beacon 更重要：AP 在 DTIM Beacon 时才会广播缓存的**多播/广播**包；对于**单播**包，STA 在任意 Beacon 醒来后都可以用 PS-Poll 取回来。

```
时间线（DTIM Period = 3，Beacon 间隔 100ms）：

  t=0ms    Beacon（普通 TIM）
  t=100ms  Beacon（普通 TIM）
  t=200ms  Beacon（普通 TIM）
  t=300ms  Beacon（DTIM） ← AP 释放广播包，STA 也在此醒来
  t=400ms  Beacon（普通 TIM）
  ...
```

**ESP32 的 `WIFI_PS_MIN_MODEM` 默认行为：**
1. ESP32 只在每个 DTIM Beacon 时唤醒无线电
2. 醒来后接收 Beacon，看 TIM 里有没有自己的单播数据
3. 有 → 发 PS-Poll → AP 回数据；没有 → 无线电继续休眠
4. 休眠期间 AP 将发给 ESP32 的单播包**缓存在发送队列**里

所以延迟 = DTIM Period × Beacon 间隔（最坏情况）。
手机热点 DTIM=1 → 最大延迟 100ms；家用路由器 DTIM=3 → 最大延迟 300ms。
这就是 BUG-010 里 ACK 延迟 0–300ms 的来源。

`WIFI_PS_NONE` 彻底关掉这个机制，无线电常驻，收到包立刻处理，没有缓存延迟。
代价是功耗增加（约 +20–30mA），对于有电源供电的网关完全可以接受。

---

### Q3：`MSG_DONTWAIT` 和 `O_NONBLOCK` 有什么区别？

两个都能让 `recv()` 在没有数据时立刻返回 `EAGAIN`，而不是阻塞等待。
区别在于**作用范围**：

| | `MSG_DONTWAIT` | `O_NONBLOCK`（`fcntl` 设置） |
|---|---|---|
| 作用范围 | **仅这一次** `recv()` 调用 | **这个 fd 之后所有调用** |
| 设置方式 | 作为 `flags` 参数传入 | `fcntl(fd, F_SETFL, O_NONBLOCK)` |
| 是否修改 socket 状态 | 否 | **是**，持久生效直到主动清除 |
| 清除方式 | 不需要 | `fcntl(fd, F_SETFL, flags & ~O_NONBLOCK)` |
| 影响 `send()` 吗 | 有 `MSG_DONTWAIT` 版本 | **影响所有 I/O**（send/recv/accept…）|

```c
// 方式 A：MSG_DONTWAIT（本项目用这个）
while (recv(sock_up, _tmp, sizeof _tmp, MSG_DONTWAIT) > 0) {}
// ↑ 只有这个 while 循环里的 recv 是非阻塞的
// ↓ 后面这个 recv 不受影响，还是按 SO_RCVTIMEO 超时等待
j = recv(sock_up, buff_ack, sizeof buff_ack, 0);

// 方式 B：O_NONBLOCK
int flags = fcntl(sock_up, F_GETFL, 0);
fcntl(sock_up, F_SETFL, flags | O_NONBLOCK);   // 设为非阻塞
while (recv(sock_up, _tmp, sizeof _tmp, 0) > 0) {}   // drain
fcntl(sock_up, F_SETFL, flags);                 // ← 必须手动恢复！
// 恢复失败 → 后面所有 recv 都变成非阻塞 → SO_RCVTIMEO 完全失效
// 这和旧代码双 setsockopt 有同样的"恢复失败"风险
```

**结论：在"只想对某一次调用非阻塞"的场景里，`MSG_DONTWAIT` 比 `O_NONBLOCK` 更安全，
不需要 set/restore，也没有"忘记恢复"或"恢复失败"的可能。**

`O_NONBLOCK` 更适合"整个 socket 生命周期都要非阻塞"的架构，
比如 event loop（epoll/select 驱动的异步 I/O 服务器）。

---

### Q4：`ESP_ERROR_CHECK` 里的 `abort()` 会做什么？会打印 backtrace 吗？

**会打印很多信息，然后重启。** 展开看宏定义（ESP-IDF `esp_err.h`）：

```c
#define ESP_ERROR_CHECK(x) do {                                 \
    esp_err_t err_rc_ = (x);                                    \
    if (unlikely(err_rc_ != ESP_OK)) {                          \
        _esp_error_check_failed(err_rc_, __FILE__, __LINE__,    \
                                __ASSERT_FUNC, #x);             \
    }                                                           \
} while(0)
```

`_esp_error_check_failed()` 函数里调用 `abort()`，ESP32 的 `abort()` 会触发 FreeRTOS panic handler，实际输出大概是：

```
assert failed: esp_wifi_set_ps wifi_init.c:87 (ESP_OK)

Backtrace: 0x40082b40:0x3ffb1c10 0x40082b73:0x3ffb1c30 0x4008594e:0x3ffb1c50
  0x400d1a52:0x3ffb1ca0 0x400d2bf0:0x3ffb1cc0 ...

ELF file SHA256: ...

Rebooting...
```

**Backtrace 里每个 `0x地址:0x栈帧` 是一个函数调用层级。** 用 `idf.py monitor` 可以自动把地址翻译成函数名+文件+行号（符号化）：

```bash
# 或者手动解析：
xtensa-esp32s3-elf-addr2line -pfiaC -e build/ESXP1302-Pkt-Fwd.elf 0x400d1a52
# 输出：wifi_init_sta at main/packet_forwarder/lora_pkt_fwd.c:4170
```

**为什么要用 `ESP_ERROR_CHECK` 而不是 `if (err != ESP_OK) return err`？**
初始化阶段的错误通常是不可恢复的（硬件不存在、内存不足等），
继续运行只会产生更难定位的后续问题。`abort()` + 重启比"带着错误跑"更安全。
生产环境有时用 `ESP_ERROR_CHECK_WITHOUT_ABORT`，只打印 log 不重启，
但调试阶段始终建议用 `ESP_ERROR_CHECK` 以便快速发现问题。

---

### Q5：lwIP 和标准 BSD socket API 的关系？ESP32 和 Linux 上有哪些不同？

**lwIP（lightweight IP）是一个开源的嵌入式 TCP/IP 协议栈**，
专为没有操作系统或资源极少的嵌入式系统设计（内存占用 <100KB）。
ESP32 用的是 lwIP 移植版，由乐鑫维护。

lwIP 实现了 BSD socket API（`socket()`、`bind()`、`recv()`、`setsockopt()` 等），
函数名和参数和 Linux 完全一样——这就是为什么可以把一份网络代码稍加修改同时跑在 Linux 和 ESP32 上。

**但有几个已知的行为差异需要注意：**

| 行为 | Linux | ESP32 lwIP |
|---|---|---|
| `tv_usec >= 1,000,000` | 静默进位 | **整个 timeval 当 0 处理** ← 本 bug 来源 |
| `errno` 的线程隔离 | glibc TLS，每线程独立 | FreeRTOS 任务本地存储，效果相同 |
| `SO_REUSEPORT` | 支持 | **不支持**（仅支持 `SO_REUSEADDR`）|
| `MSG_PEEK` | 支持 | 部分支持，有已知 bug |
| `fcntl(F_DUPFD)` | 支持 | **不支持** |
| IPv6 | 完整支持 | 需要 menuconfig 开启，默认关 |
| socket 缓冲区默认大小 | 几百 KB | 几 KB，吞吐密集场景需手动调大 |
| `getaddrinfo()` | 调用 OS DNS | lwIP 内置 DNS 解析器，受 lwIP DNS 缓存影响 |

**`errno` 在 ESP32 上怎么工作？**
lwIP socket 函数出错时设置 `errno`。ESP-IDF 里 `errno` 是宏，展开为 `(*__errno())`，
`__errno()` 返回当前 FreeRTOS 任务的 TLS（Task Local Storage）里的 errno 槽位指针。
所以不同任务的 `errno` 相互独立，和 Linux 多线程行为一致。

**实际移植经验：** 代码从 Linux 移到 ESP32，90% 的 socket 代码不用改。
要注意的是：① 上面表里的已知差异；② `select()`/`poll()` 的 fd 上限（lwIP 默认 FD_SETSIZE=64）；③ socket 缓冲区大小默认比 Linux 小很多（影响吞吐量密集场景）。

---

### Q6：FreeRTOS 时间片轮换具体怎么工作？同优先级任务怎么分 CPU 时间？

**FreeRTOS 调度器的两个核心机制：抢占式调度 + 时间片轮换。**

**抢占式（Preemptive）：** 高优先级任务就绪时，立刻打断低优先级任务。
只要有更高优先级的任务从阻塞态变成就绪态（比如等待的信号量被释放），
调度器立刻切换，低优先级任务被暂停。

**时间片（Time Slicing）：** 对于**同优先级**任务，每隔一个 tick 自动轮换。

```
ESP32 的 tick 配置（sdkconfig 里）：
CONFIG_FREERTOS_HZ = 100   → tick 间隔 = 1/100s = 10ms

时间线（mqtt_task 和 pkt_fwd_task，同优先级 6，绑 core 0）：
  0ms     mqtt_task 运行
  10ms    tick → 切换到 pkt_fwd_task
  20ms    tick → 切换到 mqtt_task
  ...
```

**但"时间片 10ms 轮换"只是最坏情况。** 任务一旦主动进入阻塞态，立刻让出 CPU：

```c
// 这些调用会让任务立刻挂起，不等 tick：
recv(sock, buf, len, 0);            // 等数据到，或超时
vTaskDelay(pdMS_TO_TICKS(100));     // 主动睡 100ms
xSemaphoreTake(sem, portMAX_DELAY); // 等信号量
xQueueReceive(q, &item, 100);       // 等队列
```

**BUG-010 里 MQTT 阻塞的本质：**

表面上 `mqtt_task` 在 TCP connect 里阻塞等待，应该让出 CPU 才对。
但 MQTT 客户端库在连接失败后有**忙等重试逻辑**——不是纯粹阻塞，而是调用 `vTaskDelay` 后立刻重试（retry loop），
期间每次重试都执行 TLS 握手，`mbedTLS` 的加解密计算比较耗 CPU。
加上 10 秒周期内多次重试，合计下来 `pkt_fwd_task` 能拿到的 CPU 时间很少。

即便 `pkt_fwd_task` 在 `recv()` 的 250ms 窗口内阻塞等待，`mqtt_task` 的 TLS 重试占满了这段时间；
等 `recv()` 超时回调触发、`pkt_fwd_task` 变成就绪态，还要等下一个 10ms tick 才能真正运行——ACK 窗口已经错过。

**一句话总结调度行为：**
优先级决定"谁能跑"，阻塞调用决定"让不让跑"，时间片决定"让多久"。
同优先级任务之间，谁能多拿 CPU，看谁阻塞的时间少。
