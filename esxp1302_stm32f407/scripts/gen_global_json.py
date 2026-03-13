#!/usr/bin/env python3
"""
Regenerates packet_forwarder/global_json.h from the three global_conf JSON files.

The output format understood by parson's json_parse_array_with_comments():
  - Byte 0-1 : big-endian uint16 = length of the JSON text (excluding the
               2-byte header and the null terminator)
  - Byte 2.. : raw JSON text bytes (ASCII)
  - Last byte: 0x00  (null terminator, written by this script but NOT counted
               in the length field, matching json_to_hex_array.py behaviour)

Usage (run from the esxp1302_stm32f407 directory):
    python scripts/gen_global_json.py
"""

import os
import re
import sys

BASE     = os.path.dirname(os.path.abspath(__file__))          # .../esxp1302_stm32f407/scripts
BASE     = os.path.dirname(BASE)                               # .../esxp1302_stm32f407
HD_NAME  = os.path.join(BASE, 'packet_forwarder', 'global_json.h')
CONF_DIR = os.path.join(BASE, 'packet_forwarder', 'global_conf.json')

FILES = [
    ('global_conf.cn490.json', 'global_cn_conf'),
    ('global_conf.eu868.json', 'global_eu_conf'),
    ('global_conf.us915.json', 'global_us_conf'),
]


def json_to_hex(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        chars = f.read()
    n = len(chars)
    if n > 0xFFFF:
        print(f"ERROR: {filepath} exceeds 65535 bytes ({n}). Abort.")
        sys.exit(1)
    # 2-byte big-endian length header
    tokens = [f'0x{n // 256:02X},', f'0x{n % 256:02X},']
    for ch in chars:
        tokens.append(f'0x{ord(ch):02X},')
    tokens.append('0x00')   # null terminator (not counted in length)
    # Format 16 tokens per line with 4-space indent
    lines = []
    for i in range(0, len(tokens), 16):
        lines.append('    ' + ' '.join(tokens[i:i + 16]))
    return '\n'.join(lines)


def main():
    output = []
    for filename, varname in FILES:
        fpath = os.path.join(CONF_DIR, filename)
        if not os.path.exists(fpath):
            print(f"ERROR: {fpath} not found")
            sys.exit(1)
        output.append(f'// dump from {filename} as string array')
        output.append(f'const static uint8_t {varname}[] = {{')
        output.append(json_to_hex(fpath))
        output.append('};\n')

    with open(HD_NAME, 'w', encoding='utf-8') as f:
        f.write('\n'.join(output) + '\n')
    print(f"Generated: {HD_NAME}")

    # Quick sanity-check: verify length header matches actual content
    with open(HD_NAME, 'r') as f:
        content = f.read()
    for filename, varname in FILES:
        block = content.split(varname + '[] = {')[1].split('};')[0]
        hex_values = re.findall(r'0x([0-9A-Fa-f]{2})', block)
        data = bytes(int(h, 16) for h in hex_values)
        declared_len = (data[0] << 8) | data[1]
        actual_len   = len(data) - 3   # minus 2-byte header and 1-byte null terminator
        if declared_len != actual_len:
            print(f"  WARNING: {varname} length mismatch: header={declared_len}, actual={actual_len}")
        else:
            # Show a short JSON snippet for visual confirmation
            text = data[2:].decode('utf-8', errors='replace')
            idx = text.find('"com_type"')
            snippet = text[idx:idx + 30].replace('\n', '\\n') if idx != -1 else '(snippet not found)'
            print(f"  {varname}: len={declared_len}  {snippet}")


if __name__ == '__main__':
    main()
