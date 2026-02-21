#!/usr/bin/env python3
"""
Windows equivalent of run_me.sh's prepare_c_head_file_from_files().
Regenerates main/global_json.h from the three global_conf JSON files.

Usage:
    python scripts/gen_global_json.py
"""

import os
import re

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HD_NAME = os.path.join(BASE, 'main', 'global_json.h')
CONF_DIR = os.path.join(BASE, 'main', 'packet_forwarder', 'global_conf.json')

FILES = [
    ('global_conf.cn490.json', 'global_cn_conf'),
    ('global_conf.eu868.json', 'global_eu_conf'),
    ('global_conf.us915.json', 'global_us_conf'),
]


def json_to_hex(filepath):
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


def main():
    output = []
    for filename, varname in FILES:
        fpath = os.path.join(CONF_DIR, filename)
        output.append(f'// dump from {filename} as string array')
        output.append(f'const static uint8_t {varname}[] = {{')
        output.append(json_to_hex(fpath))
        output.append('};\n')

    with open(HD_NAME, 'w', encoding='utf-8') as f:
        f.write('\n'.join(output))
    print(f"Generated: {HD_NAME}")

    # Verify key values
    with open(HD_NAME, 'r') as f:
        content = f.read()
    for filename, varname in FILES:
        block = content.split(varname)[1].split('};')[0]
        hex_values = re.findall(r'0x([0-9A-Fa-f]{2})', block)
        data = bytes(int(h, 16) for h in hex_values)
        text = data[2:].decode('utf-8', errors='replace')
        idx = text.find('push_timeout_ms')
        if idx != -1:
            snippet = text[idx:idx + 28].replace('\n', '\\n')
            print(f"  {varname}: {snippet}")


if __name__ == '__main__':
    main()
