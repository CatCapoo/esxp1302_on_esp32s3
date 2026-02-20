#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# License: Revised BSD License, see LICENSE.TXT file including in the project
#

# UDP 测试服务器
#
# 用法:
#     python scripts/udp_srv.py [port]
#
# 参数:
#     port    监听端口号，默认为 5005
#
# 示例:
#     python scripts/udp_srv.py 5005      # 监听端口 5005
#
# 功能说明:
#     监听指定 UDP 端口，收到数据后在终端打印，
#     并回复 "OK: <原始内容>" 给发送方。
#     可配合 ESP32 test_network_connection 命令使用:
#     test_network_connection -u 222 -p 0987654321 --host 10.184.141.238 --port 5005

import socket
import sys

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

srv_addr = '0.0.0.0'
srv_port = 5005

if len(sys.argv) > 1:
    srv_port = int(sys.argv[1])
print(f'port: {srv_port}')

server = (srv_addr, srv_port)
sock.bind(server)

print(f"Listening on {srv_addr}:{srv_port}")

while True:
    payload, client_addr = sock.recvfrom(1024)
    print(f"Got: {payload}")
    print(f"Echoing data back to {client_addr}")
    sent = sock.sendto(b'OK: ' + payload, client_addr)
