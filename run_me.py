#!/usr/bin/env python3
"""
Windows replacement for run_me.sh.

Usage:
    python run_me.py make           # generate headers + idf.py app (no tests)
    python run_me.py make_all       # generate headers + idf.py build (full)
    python run_me.py flash          # flash app only
    python run_me.py flash_all      # flash bootloader + app + partition table
    python run_me.py run            # open idf.py monitor
    python run_me.py flash run      # flash app then open monitor

Options:
    --port COM3   Serial port (default: COM3)
    --baud 921600 Baud rate for flash (default: 921600)
"""

import os
import re
import sys
import subprocess

# ── Paths ────────────────────────────────────────────────────────────────────
BASE     = os.path.dirname(os.path.abspath(__file__))
PKT_DIR  = os.path.join(BASE, 'main', 'packet_forwarder')
CONF_DIR = os.path.join(PKT_DIR, 'global_conf.json')
BUILD    = os.path.join(BASE, 'build')

WEBPAGE_HTML = os.path.join(PKT_DIR, 'webpage.html')
WEBPAGE_H    = os.path.join(PKT_DIR, 'webpage.h')
GLOBAL_JSON_H = os.path.join(PKT_DIR, 'global_json.h')  # kept beside webpage.h
# Also keep a copy at main/ for compatibility
GLOBAL_JSON_H_MAIN = os.path.join(BASE, 'main', 'global_json.h')

JSON_FILES = [
    ('global_conf.cn490.json', 'global_cn_conf'),
    ('global_conf.eu868.json', 'global_eu_conf'),
    ('global_conf.us915.json', 'global_us_conf'),
]

# ── Header generation ────────────────────────────────────────────────────────

def gen_webpage_h():
    """Equivalent of: scripts/dump_html.py webpage.html webpage_str > webpage.h"""
    var_name = 'webpage_str'
    with open(WEBPAGE_HTML, 'r', encoding='utf-8') as f:
        chars = f.read()
    result = (
        f"// dump from file {os.path.basename(WEBPAGE_HTML)}\n"
        f"static const char {var_name}[] = {{\n    "
    )
    for i, x in enumerate(chars, 1):
        result += f" 0x{ord(x):02X},"
        if i % 16 == 0:
            result += "\n    "
    result += " 0x00\n};"
    with open(WEBPAGE_H, 'w', encoding='utf-8') as f:
        f.write(result)
    print(f"Generated: {WEBPAGE_H}")


def _json_to_hex_block(filepath):
    """Equivalent of scripts/json_to_hex_array.py output (with 4-space indent)."""
    with open(filepath, 'r', encoding='utf-8') as f:
        chars = f.read()
    n = len(chars)
    tokens = [f'0x{n // 256:02X},', f'0x{n % 256:02X},']
    for ch in chars:
        tokens.append(f'0x{ord(ch):02X},')
    tokens.append('0x00')
    lines = []
    for i in range(0, len(tokens), 16):
        lines.append('    ' + ' '.join(tokens[i:i + 16]))
    return '\n'.join(lines)


def gen_global_json_h():
    """Equivalent of the global_conf -> global_json.h part of prepare_c_head_file_from_files()."""
    output = []
    for filename, varname in JSON_FILES:
        fpath = os.path.join(CONF_DIR, filename)
        output.append(f'// dump from {filename} as string array')
        output.append(f'const static uint8_t {varname}[] = {{')
        output.append(_json_to_hex_block(fpath))
        output.append('};\n')
    content = '\n'.join(output)

    for dest in (GLOBAL_JSON_H, GLOBAL_JSON_H_MAIN):
        with open(dest, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Generated: {dest}")

    # Verify push_timeout_ms values
    for filename, varname in JSON_FILES:
        block = content.split(varname)[1].split('};')[0]
        hex_values = re.findall(r'0x([0-9A-Fa-f]{2})', block)
        data = bytes(int(h, 16) for h in hex_values)
        text = data[2:].decode('utf-8', errors='replace')
        idx = text.find('push_timeout_ms')
        if idx != -1:
            snippet = text[idx:idx + 28].replace('\n', '\\n')
            print(f"  {varname}: {snippet}")


def prepare_headers():
    gen_webpage_h()
    gen_global_json_h()


# ── Build ─────────────────────────────────────────────────────────────────────

def run(cmd, **kwargs):
    print(f"\n>>> {' '.join(cmd)}\n")
    # shell=True is required on Windows so that .py scripts (idf.py, esptool.py)
    # are resolved via PATH/PATHEXT rather than invoked as native executables.
    result = subprocess.run(cmd, cwd=BASE, shell=True, **kwargs)
    if result.returncode != 0:
        sys.exit(result.returncode)


def cmd_make():
    prepare_headers()
    run(['idf.py', '-DCONFIG_LIBLORAGW_TEST=0', 'app'])


def cmd_make_all():
    prepare_headers()
    run(['idf.py', 'build'])


# ── Flash ─────────────────────────────────────────────────────────────────────

def esptool_path():
    idf_path = os.environ.get('IDF_PATH')
    if not idf_path:
        print("ERROR: IDF_PATH not set. Run in ESP-IDF terminal.")
        sys.exit(1)
    return os.path.join(idf_path, 'components', 'esptool_py', 'esptool', 'esptool.py')


def cmd_flash(port, baud, full=False):
    python = sys.executable
    esptool = esptool_path()
    # Parameters from build/flasher_args.json (esp32s3, dio, 80m, 16MB)
    common = [
        python, esptool,
        '--port', port, '--baud', str(baud),
        '--chip', 'esp32s3',
        '--before', 'default_reset', '--after', 'hard_reset',
        'write_flash', '-z',
        '--flash_mode', 'dio', '--flash_freq', '80m', '--flash_size', '16MB',
    ]
    app_bin  = os.path.join(BUILD, 'ESXP1302-Pkt-Fwd.bin')
    boot_bin = os.path.join(BUILD, 'bootloader', 'bootloader.bin')
    part_bin = os.path.join(BUILD, 'partition_table', 'partition-table.bin')

    if full:
        # bootloader at 0x0 (not 0x1000) for esp32s3
        run(common + ['0x0', boot_bin, '0x10000', app_bin, '0x8000', part_bin])
    else:
        run(common + ['0x10000', app_bin])


# ── Monitor ───────────────────────────────────────────────────────────────────

def cmd_run(port):
    run(['idf.py', '--port', port, 'monitor'])


# ── Entry point ───────────────────────────────────────────────────────────────

def usage():
    print(__doc__)
    sys.exit(0)


def main():
    args = sys.argv[1:]
    if not args or '-h' in args or '--help' in args:
        usage()

    # parse optional --port / --baud
    port = 'COM3'
    baud = 921600
    filtered = []
    i = 0
    while i < len(args):
        if args[i] == '--port' and i + 1 < len(args):
            port = args[i + 1]; i += 2
        elif args[i] == '--baud' and i + 1 < len(args):
            baud = int(args[i + 1]); i += 2
        else:
            filtered.append(args[i]); i += 1
    args = filtered

    for cmd in args:
        if cmd == 'make':
            cmd_make()
        elif cmd == 'make_all':
            cmd_make_all()
        elif cmd == 'flash':
            cmd_flash(port, baud, full=False)
        elif cmd == 'flash_all':
            cmd_flash(port, baud, full=True)
        elif cmd == 'run':
            cmd_run(port)
        else:
            print(f"Unknown command: {cmd}")
            usage()


if __name__ == '__main__':
    main()
