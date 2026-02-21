# BUG-010：PUSH_DATA acknowledged 低 & out-of-sync ACK

- **日期**：2026-02-21  
- **文件**：`main/packet_forwarder/lora_pkt_fwd.c`  
- **严重级别**：功能性错误（NS 无法正常确认上行包，网关统计 ACK 率极低）

---

## 现象

网关启动并连接 NS 后，串口持续出现：

```
WARNING: [up] ignored out-of sync ACK packet
INFO: [up] PUSH_ACK received in 0 ms      ← 立刻收到（实为上轮过期 ACK）
INFO: [down] PULL_ACK received in 272 ms
```

30 秒统计周期内：

```
PUSH_DATA acknowledged: 22.22%
PULL_DATA sent: 5 (100.00% acknowledged)
```

PULL_ACK 正常（100%），但 PUSH_ACK 极低（20%~33%），反复告警。

---

## 根本原因（两个独立 bug）

### Bug A：socket 路由错误（代码缺陷）

`sock_up` 和 `sock_down` 在初始化时 `connect()` 被注释在 `#if 0` 块中，
未向 OS 注册对端地址。

**后果 1：** `recvfrom(..., (struct sockaddr *)&dest_addr, &socklen)` 将**回包发送方地址写入全局变量 `dest_addr`**。两个线程交替调用后，`dest_addr` 被 PUSH_ACK 发送方地址或 PULL_ACK 发送方地址覆盖，后续所有 `sendto(..., &dest_addr, ...)` 的目标地址均被破坏。

**后果 2：** 无 `connect()` 时，OS 按目的端口分发回包。`sock_up` 和 `sock_down` 目的端口相同（都是 NS:1700），OS 可能将 PULL_ACK 交给 `sock_up` 读取，token 不匹配 → out-of-sync。

### Bug B：ACK 超时窗口远小于 NS 实际 RTT

```c
#define PUSH_TIMEOUT_MS  100   // push_timeout_half = 50 ms/次，共等 2×50 = 100 ms
```

NS 实际 UDP RTT（手机热点）= **270–400 ms**，远超 100 ms 窗口。
每轮 PUSH_ACK 均超时，真正的 ACK 留在 socket 缓冲区。
下一轮发出新 PUSH_DATA（新 token）后，`recv()` 读到旧 ACK → token 不匹配 → out-of-sync。

---

## 修复方案

### 修复 A：启用 `connect()`，替换 `sendto/recvfrom` 为 `send/recv`

在 `pkt_fwd_main()` socket 初始化段，移除 `#if 0 ... #endif`，启用：

```c
i = connect(sock_up, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
i = connect(sock_down, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
```

将以下位置的 `sendto/recvfrom` 全部替换：

| 函数 | 修改前 | 修改后 |
|------|--------|--------|
| `send_tx_ack()` | `sendto(sock_down, ..., &dest_addr, ...)` | `send(sock_down, ...)` |
| `thread_up` 发送 | `sendto(sock_up, ..., &dest_addr, ...)` | `send(sock_up, ...)` |
| `thread_up` 接收 | `recvfrom(sock_up, ..., &dest_addr, &socklen)` | `recv(sock_up, ...)` |
| `thread_down` 发送 | `sendto(sock_down, ..., &dest_addr, ...)` | `send(sock_down, ...)` |
| `thread_down` 接收 | `recvfrom(sock_down, ..., &dest_addr, &socklen)` | `recv(sock_down, ...)` |

### 修复 B：增大超时阈值

```c
// 修改前
#define PUSH_TIMEOUT_MS  100
#define PULL_TIMEOUT_MS  200

// 修改后
#define PUSH_TIMEOUT_MS  2000   /* tolerate WiFi hotspot jitter */
#define PULL_TIMEOUT_MS  1000   /* tolerate WiFi hotspot jitter */
```

### 修复 C：发送前 drain stale ACK

即使超时窗口扩大，若某轮因偶发丢包超时，上一轮的 ACK 仍留在缓冲区。
在每次发出新 PUSH_DATA **之前**，用 1 ms 非阻塞超时清空 `sock_up` 缓冲区：

```c
{
    struct timeval drain_tv = {0, 1000}; /* 1 ms */
    uint8_t _tmp[4];
    setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &drain_tv, sizeof drain_tv);
    while (recv(sock_up, _tmp, sizeof _tmp, 0) > 0) {}
    setsockopt(sock_up, SOL_SOCKET, SO_RCVTIMEO, &push_timeout_half, sizeof push_timeout_half);
}
```

---

## 说明：修复 A 与网络质量无关

Bug A（socket 路由错误）在**任何网络环境**下均存在，换成低延迟有线网后依然会偶发 out-of-sync。修复 B/C 是针对高延迟网络（手机热点）的额外保护，低延迟环境也无副作用。

---

## 验证方法

修复并烧录后，观察串口：

1. `WARNING: [up] ignored out-of sync ACK packet` 不再出现
2. 统计行显示：
   ```
   PUSH_DATA acknowledged: 100.00%
   PULL_DATA sent: N (100.00% acknowledged)
   ```
3. 可用本地 Mock 脚本在局域网内验证（详见 [test_pkt_fwd_ns_uplink_memo.md](../test_notes/test_pkt_fwd_ns_uplink_memo.md)）

---

## 相关文件

- `main/packet_forwarder/lora_pkt_fwd.c`
- `scripts/lora_ns_mock.py`（新增，本地验证用）
- commit: `50370e9` fix: socket routing and ACK timeout for reliable NS communication
