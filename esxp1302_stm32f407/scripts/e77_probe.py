#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
e77_probe.py – 诊断 E77 串口通信，尝试多种波特率和行尾格式
用法: python scripts/e77_probe.py --port COM18
"""
import serial, time, argparse, sys

BAUDS   = [9600, 115200, 57600, 38400, 19200]
ENDINGS = [b"\r\n", b"\r", b"\n"]

def probe(port, baud, ending, timeout=2.0):
    try:
        s = serial.Serial(port, baud, timeout=0.1,
                          dsrdtr=False, rtscts=False, xonxoff=False)
    except Exception as e:
        print(f"  打开失败: {e}")
        return False
    time.sleep(1.5)
    s.reset_input_buffer()

    cmd = b"AT" + ending
    s.write(cmd)
    print(f"  发送 {cmd!r}")

    buf = b""
    deadline = time.time() + timeout
    while time.time() < deadline:
        chunk = s.read(64)
        if chunk:
            buf += chunk
            deadline = time.time() + 0.5   # 收到数据后再多等 0.5s
    s.close()

    if buf:
        print(f"  收到 ({len(buf)} 字节): {buf!r}")
        return True
    else:
        print(f"  无响应")
        return False

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--baud", type=int, default=0,
                    help="0=自动扫描全部波特率")
    args = ap.parse_args()

    bauds = [args.baud] if args.baud else BAUDS

    for baud in bauds:
        for ending in ENDINGS:
            tag = repr(ending)
            print(f"\n[{baud} bps, ending={tag}]")
            if probe(args.port, baud, ending):
                print(f"\n  ✓ 找到! 波特率={baud}  行尾={tag}")
                return
    print("\n所有组合均无响应，请检查接线和电源。")

if __name__ == "__main__":
    main()
