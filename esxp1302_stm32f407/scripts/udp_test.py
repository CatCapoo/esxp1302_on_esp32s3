#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
W5500 UDP bringup test script

Usage:
    python scripts/udp_test.py [--port PORT] [--target IP]

Arguments:
    --port PORT     Local port to bind (default: 5005)
    --target IP     STM32F407 W5500 IP address (default: 192.168.10.15)

Examples:
    python scripts/udp_test.py                             # defaults
    python scripts/udp_test.py --port 5005 --target 192.168.10.15

This script:
  1. Binds a UDP socket on 0.0.0.0:PORT
  2. Waits for the STM32 "hello" packet and prints it
  3. Sends a series of numbered test messages to the STM32
  4. Receives and displays echo replies from the STM32

The STM32 test (test_w5500_udp) sends an initial hello on boot,
then echoes any received UDP packet back with a prefix.
"""

import socket
import sys
import argparse
import time


def main():
    parser = argparse.ArgumentParser(description="W5500 UDP bringup test")
    parser.add_argument("--port", type=int, default=5005,
                        help="UDP port to bind (default: 5005)")
    parser.add_argument("--target", default="192.168.10.15",
                        help="STM32 W5500 IP (default: 192.168.10.15)")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)
    sock.bind(("0.0.0.0", args.port))
    print(f"[UDP Test] Listening on 0.0.0.0:{args.port}")
    print(f"[UDP Test] Target: {args.target}:{args.port}")
    print()

    # Wait for the STM32 hello packet
    print("[UDP Test] Waiting for STM32 hello packet...")
    try:
        data, addr = sock.recvfrom(1024)
        print(f"[UDP Test] Got from {addr}: {data.decode('utf-8', errors='replace')}")
    except socket.timeout:
        print("[UDP Test] No hello received (timeout). Continuing anyway...")

    # Send test messages
    print()
    num_msgs = 5
    for i in range(1, num_msgs + 1):
        msg = f"Test message #{i} from PC"
        print(f"[UDP Test] Sending: {msg}")
        sock.sendto(msg.encode("utf-8"), (args.target, args.port))

        try:
            data, addr = sock.recvfrom(1024)
            reply = data.decode("utf-8", errors="replace")
            print(f"[UDP Test] Reply from {addr}: {reply}")
        except socket.timeout:
            print(f"[UDP Test] No reply for message #{i} (timeout)")

        time.sleep(0.5)

    print()
    print(f"[UDP Test] Done. Sent {num_msgs} messages.")

    # Interactive mode: keep listening
    print("[UDP Test] Entering interactive mode (Ctrl+C to exit)...")
    print("[UDP Test] Type a message and press Enter to send, or wait for STM32 data.")
    sock.settimeout(1.0)
    try:
        while True:
            try:
                data, addr = sock.recvfrom(1024)
                print(f"[UDP Test] Recv from {addr}: {data.decode('utf-8', errors='replace')}")
            except socket.timeout:
                pass
    except KeyboardInterrupt:
        print("\n[UDP Test] Bye!")

    sock.close()


if __name__ == "__main__":
    main()
