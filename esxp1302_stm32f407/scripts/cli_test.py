#!/usr/bin/env python3
"""
cli_test.py – Automated UART CLI test for STM32F407 gateway
Usage:
    python cli_test.py COM5          # auto-run all tests
    python cli_test.py COM5 --shell  # interactive shell mode
"""

import sys
import time
import serial

# ── config ─────────────────────────────────────────────────────────────────
BAUD        = 115200
TIMEOUT_S   = 3.0   # read timeout per command

# ── helpers ────────────────────────────────────────────────────────────────

def send(ser: serial.Serial, cmd: str) -> str:
    """Send one command and collect response until next '> ' prompt."""
    full = cmd.strip() + "\r\n"
    # Send slowly (10ms/char) to avoid overwhelming single-byte ISR
    for ch in full:
        ser.write(ch.encode())
        time.sleep(0.01)

    buf = b""
    deadline = time.time() + TIMEOUT_S
    while time.time() < deadline:
        chunk = ser.read(ser.in_waiting or 1)
        if chunk:
            buf += chunk
            if b"> " in buf:
                break
        time.sleep(0.02)

    text = buf.decode(errors="replace")
    print(f"\n>>> {cmd}")
    print(text.strip())
    return text


def wait_prompt(ser: serial.Serial, timeout: float = 5.0) -> bool:
    """Wait until the CLI '> ' prompt appears (e.g. after reboot)."""
    buf = b""
    deadline = time.time() + timeout
    while time.time() < deadline:
        chunk = ser.read(ser.in_waiting or 1)
        if chunk:
            buf += chunk
            decoded = buf.decode(errors="replace")
            print(decoded, end="", flush=True)
            if "> " in decoded:
                return True
        time.sleep(0.05)
    return False

# ── automated test suite ────────────────────────────────────────────────────

def run_tests(ser: serial.Serial):
    PASS = "\033[32mPASS\033[0m"
    FAIL = "\033[31mFAIL\033[0m"
    results = []

    def check(name, response, expected):
        ok = expected.lower() in response.lower()
        results.append((name, ok))
        print(f"  {'✓' if ok else '✗'} [{PASS if ok else FAIL}] {name}")
        return ok

    print("\n========== Automated CLI Tests ==========")

    # 1. help
    r = send(ser, "help")
    check("help lists commands", r, "config show")

    # 2. config show (default state)
    r = send(ser, "config show")
    check("config show – ns_host",      r, "ns_host")
    check("config show – gateway_eui",  r, "gateway_eui")
    check("config show – eth_ip",       r, "eth_ip")

    # 3. set ns_host
    r = send(ser, "config set ns_host 172.16.0.1")
    check("set ns_host accepted", r, "172.16.0.1")

    # 4. set ns_port_up
    r = send(ser, "config set ns_port_up 1701")
    check("set ns_port_up accepted", r, "1701")

    # 5. set ns_port_down
    r = send(ser, "config set ns_port_down 1702")
    check("set ns_port_down accepted", r, "1702")

    # 6. set gw_eui
    r = send(ser, "config set gw_eui AABB0000CCDD0011")
    check("set gw_eui accepted", r, "AABB0000CCDD0011")

    # 7. set eth_ip
    r = send(ser, "config set eth_ip 10.1.2.3")
    check("set eth_ip accepted", r, "10.1.2.3")

    # 8. verify with show
    r = send(ser, "config show")
    check("show – ns_host updated",       r, "172.16.0.1")
    check("show – ns_port_up updated",    r, "1701")
    check("show – ns_port_down updated",  r, "1702")
    check("show – gw_eui updated",        r, "AABB0000CCDD0011")
    check("show – eth_ip updated",        r, "10.1.2.3")

    # 9. save
    r = send(ser, "config save")
    check("config save OK", r, "Saved to Flash OK")

    # 10. reboot and wait for CLI to come back
    print("\n>>> reboot  (waiting for reboot...)")
    ser.write(b"reboot\r\n")
    time.sleep(2.5)
    ser.reset_input_buffer()
    if not wait_prompt(ser, timeout=10):
        print(f"  ✗ [{FAIL}] reboot – CLI did not come back")
        results.append(("reboot – CLI restart", False))
    else:
        results.append(("reboot – CLI restart", True))
        print(f"  ✓ [{PASS}] reboot – CLI restarted")

    # 11. verify persistence after reboot
    r = send(ser, "config show")
    check("persist – ns_host across reboot",   r, "172.16.0.1")
    check("persist – ns_port_up across reboot", r, "1701")
    check("persist – eth_ip across reboot",    r, "10.1.2.3")

    # 12. reset to defaults
    r = send(ser, "config reset")
    check("config reset replied", r, "Defaults applied")

    r = send(ser, "config show")
    check("reset – ns_host back to default", r, "192.168.10.1")

    # ── summary ────────────────────────────────────────────────────────────
    passed = sum(1 for _, ok in results if ok)
    total  = len(results)
    print(f"\n=========================================")
    print(f"  Result: {passed}/{total} passed")
    if passed == total:
        print(f"  \033[32mALL TESTS PASSED\033[0m")
    else:
        print(f"  \033[31mFAIL – see ✗ above\033[0m")
    print(f"=========================================\n")

# ── interactive shell ───────────────────────────────────────────────────────

def interactive_shell(ser: serial.Serial):
    import threading

    def reader():
        while True:
            try:
                data = ser.read(ser.in_waiting or 1)
                if data:
                    print(data.decode(errors="replace"), end="", flush=True)
            except Exception:
                break

    t = threading.Thread(target=reader, daemon=True)
    t.start()

    print("=== Interactive shell (Ctrl+C to quit) ===")
    try:
        while True:
            line = input()
            ser.write((line.strip() + "\r\n").encode())
    except (KeyboardInterrupt, EOFError):
        print("\n[Exiting]")

# ── main ────────────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} <COM_PORT> [--shell]")
        print(f"  e.g. python {sys.argv[0]} COM5")
        print(f"  e.g. python {sys.argv[0]} COM5 --shell")
        sys.exit(1)

    port    = sys.argv[1]
    shell   = "--shell" in sys.argv

    try:
        ser = serial.Serial(port, BAUD, timeout=1)
        # ST-Link VCP needs DTR=True to relay TX data back to host
        ser.dtr = True
        ser.rts = True
    except serial.SerialException as e:
        print(f"[ERROR] Cannot open {port}: {e}")
        sys.exit(1)

    print(f"[+] Opened {port} @ {BAUD} baud")
    print("[+] Waiting for boot + auto-test (12s) – showing live output:")
    deadline = time.time() + 12.0
    while time.time() < deadline:
        chunk = ser.read(ser.in_waiting or 1)
        if chunk:
            print(chunk.decode(errors="replace"), end="", flush=True)
        time.sleep(0.02)
    ser.reset_input_buffer()
    print("\n[+] Boot wait done.")

    # Send a blank line to get a fresh prompt, then discard it
    ser.write(b"\r\n")
    time.sleep(0.5)
    ser.reset_input_buffer()

    if shell:
        interactive_shell(ser)
    else:
        run_tests(ser)

    ser.close()

if __name__ == "__main__":
    main()
