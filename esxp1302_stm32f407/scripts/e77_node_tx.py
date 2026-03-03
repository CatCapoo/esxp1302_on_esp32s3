#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
e77_node_tx.py — E77-400M22S 节点发包脚本 (配合网关 HAL RX 测试)
================================================================
配合 test_loragw_hal_rx 使用：
  - E77 以 ABP 模式在 CN470 Sub-band 10 (CH80–CH87) 上周期发送上行包
  - SX1302 网关以 HAL RX 模式接收这些包

由于 E77 模块 **不支持 P2P 原始 LoRa 模式**（仅 LoRaWAN），
本脚本使用 ABP + dummy keys 实现无需网络服务器的发包测试。
网关会收到完整的 LoRaWAN MAC 帧（MHDR + FHDR + FPort + Payload + MIC），
作为原始 LoRa 包解码显示。

CN470 Sub-band 10:
  - 上行频段: CH80=486.3 ~ CH87=487.7 MHz (步进 200kHz)
  - 信道掩码: 0000:0000:0000:0000:0000:00FF

用法:
  python scripts/e77_node_tx.py --port COM3 --interval 5 --count 20 --dr 5

  参数说明:
    --port       串口号, Windows: COM3, Linux: /dev/ttyUSB0
    --baud       波特率, 默认 9600
    --interval   发包间隔(秒), 默认 10
    --count      总包数, 0=无限, 默认 0
    --dr         DataRate (0=SF12 .. 5=SF7), 默认 2 (SF10)
    --payload    Payload 十六进制, 默认 DEADBEEF01020304
    --txp        发射功率档 (0=最大), 默认 0
    --verbose    显示 AT 交互细节
"""

import serial
import time
import argparse
import sys
import datetime

# ──────────────────────────────────────────────
#  全局常量
# ──────────────────────────────────────────────
DEFAULT_TIMEOUT = 5.0
SEND_TIMEOUT    = 12.0

# ABP 虚拟密钥 (仅用于触发 MAC 帧发送，不经过真实 NS)
DUMMY_DEVADDR  = "26011234"
DUMMY_NWKSKEY  = "00112233445566778899AABBCCDDEEFF"
DUMMY_APPSKEY  = "FFEEDDCCBBAA99887766554433221100"

# CN470 Sub-band 10 信道掩码
CN470_SB10_CHANMASK = "0000:0000:0000:0000:0000:00FF"


# ──────────────────────────────────────────────
#  工具
# ──────────────────────────────────────────────
def ts():
    return datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]


def log(msg, tag="INFO"):
    print(f"[{ts()}] [{tag}] {msg}")


# ──────────────────────────────────────────────
#  E77 串口封装
# ──────────────────────────────────────────────
class E77Node:
    def __init__(self, port: str, baud: int = 9600, verbose: bool = False):
        self.verbose = verbose
        log(f"打开串口 {port} @ {baud} bps")
        self.ser = serial.Serial(
            port=port,
            baudrate=baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.2,
            dsrdtr=False,
            rtscts=False,
            xonxoff=False,
        )
        time.sleep(1.5)
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()

    def close(self):
        if self.ser and self.ser.is_open:
            self.ser.close()

    def _write(self, cmd: str):
        raw = (cmd.strip() + "\r\n").encode("ascii")
        if self.verbose:
            log(f">> {cmd.strip()}", "SEND")
        self.ser.write(raw)

    def _readline(self) -> str:
        line = self.ser.readline()
        if line:
            text = line.decode("ascii", errors="replace").strip()
            if self.verbose and text:
                log(f"<< {text}", "RECV")
            return text
        return ""

    def _read_until(self, keywords, timeout=DEFAULT_TIMEOUT) -> list[str]:
        lines = []
        deadline = time.time() + timeout
        while time.time() < deadline:
            line = self._readline()
            if line:
                lines.append(line)
                for kw in keywords:
                    if kw.upper() in line.upper():
                        return lines
        return lines

    def send_cmd(self, cmd: str, expect="OK",
                 timeout=DEFAULT_TIMEOUT) -> tuple[bool, list[str]]:
        echo_str = cmd.strip().upper()
        self._write(cmd)
        raw_lines = self._read_until([expect, "ERROR", "AT_ERROR"],
                                     timeout=timeout)
        lines = [l for l in raw_lines if l.upper() != echo_str]
        ok = any(expect.upper() in l.upper() for l in lines)
        if not ok:
            log(f"指令失败: {cmd}  响应: {lines}", "WARN")
        return ok, lines

    def at_test(self, retries: int = 3) -> bool:
        for attempt in range(1, retries + 1):
            self.ser.reset_input_buffer()
            ok, resp = self.send_cmd("AT", timeout=2.0)
            if ok:
                return True
            log(f"AT 测试第 {attempt}/{retries} 次失败，1s 后重试...", "WARN")
            time.sleep(1.0)
        return False


# ──────────────────────────────────────────────
#  ABP 配置 + 发包
# ──────────────────────────────────────────────
def run_tx(args):
    node = E77Node(args.port, args.baud, args.verbose)

    # AT 通信测试
    if not node.at_test():
        log("AT 通信失败，请检查串口和模块", "ERROR")
        node.close()
        return

    log("AT 通信正常 ✓")

    # ---- ABP 配置 ----
    devaddr = args.devaddr if args.devaddr else DUMMY_DEVADDR
    nwkskey = args.nwkskey if args.nwkskey else DUMMY_NWKSKEY
    appskey = args.appskey if args.appskey else DUMMY_APPSKEY

    # 自动格式化 devaddr 为 XX:XX:XX:XX
    if ":" not in devaddr and len(devaddr) == 8:
        devaddr = ":".join(devaddr[i:i + 2] for i in range(0, 8, 2))

    steps = [
        (f"AT+REGION=2",                            "设置频段 CN470"),
        (f"AT+CDEVADDR={devaddr.upper()}",           "设置 DevAddr"),
        (f"AT+CNWKSKEY={nwkskey.upper()}",           "设置 NwkSKey"),
        (f"AT+CAPPSKEY={appskey.upper()}",           "设置 AppSKey"),
        (f"AT+CTXP={args.txp}",                     "发射功率"),
        (f"AT+CADR=0",                               "ADR 关闭"),
        (f"AT+CDATARATE={args.dr}",                  f"固定 DR{args.dr}"),
        (f"AT+CMANUALMASK=1",                        "手动掩码使能"),
        (f"AT+CFREQBANDMASK={CN470_SB10_CHANMASK}",  "信道掩码 SB10"),
    ]

    log("=" * 55)
    log("  配置 E77 ABP 模式 (CN470 Sub-band 10)")
    log("=" * 55)

    for cmd, desc in steps:
        ok, resp = node.send_cmd(cmd)
        status = "✓" if ok else "✗"
        log(f"{status} {desc}: {cmd}")
        if not ok:
            log(f"  响应: {resp}", "WARN")
        time.sleep(0.15)

    # 触发 ABP 本地入网
    log("─" * 55)
    log("触发 ABP 本地入网 ...")
    node._write("AT+CJOIN=0:0")
    lines = node._read_until(["+EVT:JOINED", "ERROR", "OK"], timeout=5)
    joined = any("+EVT:JOINED" in l or "OK" in l for l in lines)
    if joined:
        log("✓ ABP 入网成功!", "OK")
    else:
        log(f"✗ ABP 入网失败！响应: {lines}", "ERROR")
        node.close()
        return

    # 等待入网稳定
    time.sleep(1.0)

    # ---- 周期发包 ----
    log("─" * 55)
    cn470_dr_sf = {0: 12, 1: 11, 2: 10, 3: 9, 4: 8, 5: 7}
    sf = cn470_dr_sf.get(args.dr, "?")
    log(f"开始发包: DR{args.dr}(SF{sf}) 间隔={args.interval}s "
        f"payload={args.payload}")
    log(f"{'无限循环' if args.count == 0 else f'共 {args.count} 包'}")
    log("按 Ctrl+C 停止\n")

    sent = 0
    try:
        while True:
            sent += 1
            payload = args.payload.upper().replace(" ", "")
            cmd = f"AT+SEND=2:1:0:{payload}"
            log(f"── 第 {sent} 包 ──")
            node._write(cmd)

            # 等待发送结果
            dl = time.time() + SEND_TIMEOUT
            got_ok = False
            while time.time() < dl:
                line = node._readline()
                if not line:
                    continue
                if any(k in line.upper() for k in
                       ["SEND_OK", "SEND_CONFIRMED", "OK+SENT"]):
                    got_ok = True
                    log(f"  TX 确认: {line}", "OK")
                elif "+EVT:RX" in line:
                    log(f"  下行: {line}", "DOWN")
                elif "ERROR" in line.upper():
                    log(f"  错误: {line}", "ERROR")
                    break

            if not got_ok:
                log("  发送超时或失败", "WARN")

            if args.count > 0 and sent >= args.count:
                break

            log(f"等待 {args.interval}s ...")
            time.sleep(args.interval)

    except KeyboardInterrupt:
        log(f"\n用户终止，已发送 {sent} 包", "INFO")

    node.close()
    log("串口关闭")


# ──────────────────────────────────────────────
#  CLI
# ──────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(
        description="E77 ABP 发包脚本 (配合 SX1302 网关 HAL RX 测试)")
    parser.add_argument("--port", required=True,
                        help="串口, 如 COM3 或 /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=9600, help="波特率")
    parser.add_argument("--interval", type=int, default=10,
                        help="发包间隔(秒)")
    parser.add_argument("--count", type=int, default=0,
                        help="总包数, 0=无限")
    parser.add_argument("--dr", type=int, default=2, choices=range(6),
                        help="DataRate 0=SF12..5=SF7, 默认 2(SF10)")
    parser.add_argument("--payload", default="DEADBEEF01020304",
                        help="Payload 十六进制")
    parser.add_argument("--txp", type=int, default=0,
                        help="发射功率档 (0=最大)")
    parser.add_argument("--devaddr", default=None,
                        help="DevAddr, 默认 26011234")
    parser.add_argument("--nwkskey", default=None,
                        help="NwkSKey 32 hex, 默认 dummy")
    parser.add_argument("--appskey", default=None,
                        help="AppSKey 32 hex, 默认 dummy")
    parser.add_argument("--verbose", action="store_true",
                        help="显示 AT 交互细节")

    args = parser.parse_args()
    run_tx(args)


if __name__ == "__main__":
    main()
