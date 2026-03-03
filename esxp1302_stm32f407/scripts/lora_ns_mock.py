#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
lora_ns_mock.py — LoRa 网关协议调试服务器

模拟 LoRaWAN Network Server 的 UDP 端点，正确响应 Semtech UDP 协议，
并将收到的所有数据包详细打印出来，用于排查网关与 NS 的连接问题。

协议参考：Semtech UDP packet forwarder protocol
  PUSH_DATA (0x00): 网关 -> NS，上行数据 / 统计
  PUSH_ACK  (0x01): NS   -> 网关，确认 PUSH_DATA
  PULL_DATA (0x02): 网关 -> NS，心跳 / 请求下行
  PULL_ACK  (0x04): NS   -> 网关，确认 PULL_DATA
  PULL_RESP (0x03): NS   -> 网关，下行数据
  TX_ACK    (0x05): 网关 -> NS，告知下行发送结果

用法:
  python3 scripts/lora_ns_mock.py [port]
  默认端口 1680（CN470 packet forwarder 标准端口）

示例:
  python3 scripts/lora_ns_mock.py 1680

提示:
  如果网关配置的 NS 地址是本机，把 server_address / server_port 改成本机 IP/端口即可。
  用 Ctrl+C 退出。
"""

import socket
import json
import sys
import datetime
import base64

# ── 协议常量 ──────────────────────────────────────────────────────────────────
PROTOCOL_VERSION = 2

PKT_PUSH_DATA = 0x00
PKT_PUSH_ACK  = 0x01
PKT_PULL_DATA = 0x02
PKT_PULL_RESP = 0x03
PKT_PULL_ACK  = 0x04
PKT_TX_ACK    = 0x05

PKT_NAMES = {
    PKT_PUSH_DATA: "PUSH_DATA",
    PKT_PUSH_ACK:  "PUSH_ACK",
    PKT_PULL_DATA: "PULL_DATA",
    PKT_PULL_RESP: "PULL_RESP",
    PKT_PULL_ACK:  "PULL_ACK",
    PKT_TX_ACK:    "TX_ACK",
}

# ── 工具函数 ──────────────────────────────────────────────────────────────────
def ts():
    """当前时间戳字符串，精确到毫秒"""
    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]


def eui64_str(raw: bytes) -> str:
    """将 8 字节 EUI-64 格式化为 XX:XX:XX:XX:XX:XX:XX:XX"""
    return ":".join(f"{b:02X}" for b in raw)


def pretty_json(raw: bytes) -> str:
    """尝试将字节解析为 JSON 后美化输出；失败则原样返回"""
    try:
        obj = json.loads(raw)
        return json.dumps(obj, indent=2, ensure_ascii=False)
    except Exception:
        return raw.decode("utf-8", errors="replace")


def decode_rxpk(rxpk: dict) -> str:
    """将单个 rxpk 对象格式化为多行字符串"""
    lines = []
    for key in ["tmst", "time", "chan", "rfch", "freq", "stat", "modu",
                "datr", "codr", "rssi", "lsnr", "size"]:
        if key in rxpk:
            lines.append(f"    {key}: {rxpk[key]}")
    if "data" in rxpk:
        try:
            raw_phy = base64.b64decode(rxpk["data"])
            lines.append(f"    data (hex): {raw_phy.hex()}")
            lines.append(f"    data (b64): {rxpk['data']}")
        except Exception:
            lines.append(f"    data: {rxpk['data']}")
    return "\n".join(lines)


# ── 主逻辑 ────────────────────────────────────────────────────────────────────
def run(port: int):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", port))
    print(f"[{ts()}] LoRa NS Mock 已启动，监听 0.0.0.0:{port}")
    print("─" * 60)

    # 记录网关的 PULL_DATA 来源地址，供将来发 PULL_RESP 用
    pull_addr = None

    while True:
        try:
            data, addr = sock.recvfrom(4096)
        except KeyboardInterrupt:
            print("\n退出。")
            break

        if len(data) < 4:
            print(f"[{ts()}] 来自 {addr} 的数据太短，已忽略: {data.hex()}")
            continue

        ver      = data[0]
        token    = data[1:3]          # 2 字节 token
        pkt_type = data[3]
        pkt_name = PKT_NAMES.get(pkt_type, f"UNKNOWN(0x{pkt_type:02X})")

        print(f"\n[{ts()}] ← {addr[0]}:{addr[1]}  {pkt_name}  "
              f"ver={ver}  token={token.hex()}")

        # ── PUSH_DATA ──────────────────────────────────────────────────────────
        if pkt_type == PKT_PUSH_DATA:
            if len(data) >= 12:
                gw_eui = data[4:12]
                print(f"  网关 EUI : {eui64_str(gw_eui)}")
            payload = data[12:] if len(data) > 12 else b""
            if payload:
                try:
                    obj = json.loads(payload)
                    # 上行帧
                    if "rxpk" in obj:
                        print(f"  上行帧数量: {len(obj['rxpk'])}")
                        for i, rxpk in enumerate(obj["rxpk"]):
                            print(f"  [rxpk {i}]")
                            print(decode_rxpk(rxpk))
                    # 统计信息
                    if "stat" in obj:
                        stat = obj["stat"]
                        print(f"  统计信息:")
                        for k, v in stat.items():
                            print(f"    {k}: {v}")
                except Exception as e:
                    print(f"  JSON 解析失败: {e}")
                    print(f"  原始: {payload!r}")
            # 回复 PUSH_ACK
            ack = bytes([PROTOCOL_VERSION]) + token + bytes([PKT_PUSH_ACK])
            sock.sendto(ack, addr)
            print(f"  → 已回复 PUSH_ACK")

        # ── PULL_DATA ──────────────────────────────────────────────────────────
        elif pkt_type == PKT_PULL_DATA:
            if len(data) >= 12:
                gw_eui = data[4:12]
                print(f"  网关 EUI : {eui64_str(gw_eui)}")
            pull_addr = addr
            # 回复 PULL_ACK
            ack = bytes([PROTOCOL_VERSION]) + token + bytes([PKT_PULL_ACK])
            sock.sendto(ack, addr)
            print(f"  → 已回复 PULL_ACK  (pull_addr 已记录: {addr})")

        # ── TX_ACK ─────────────────────────────────────────────────────────────
        elif pkt_type == PKT_TX_ACK:
            if len(data) >= 12:
                gw_eui = data[4:12]
                print(f"  网关 EUI : {eui64_str(gw_eui)}")
            payload = data[12:] if len(data) > 12 else b""
            if payload:
                print(f"  TX 结果: {pretty_json(payload)}")
            else:
                print(f"  TX 结果: (无 payload，表示成功)")

        # ── 其他 ───────────────────────────────────────────────────────────────
        else:
            print(f"  原始 hex: {data.hex()}")


if __name__ == "__main__":
    default_port = 1680   # CN470 packet forwarder 默认端口
    port = int(sys.argv[1]) if len(sys.argv) > 1 else default_port
    run(port)
