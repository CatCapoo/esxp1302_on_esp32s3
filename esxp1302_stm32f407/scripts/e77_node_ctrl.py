#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
e77_node_ctrl.py — E77-xxxM22S LoRaWAN 节点 AT 指令控制脚本
=============================================================
适用模块：成都亿佰特 E77-400M22S (CN470) / E77-900M22S
串口：默认 9600 bps 8N1（可通过 --baud 修改）
功能：
  otaa      — OTAA 方式入网并周期性发送上行数据
  abp       — ABP 方式入网并周期性发送上行数据
  query     — 查询模块当前参数（不入网）
  send      — 仅发送一包数据（需已入网前提）
  restore   — 恢复出厂配置

使用示例（Linux 用 /dev/ttyUSBx，Windows 用 COMx）：

  # 查询模块参数
  python3 scripts/e77_node_ctrl.py query --port /dev/ttyUSB0        # Linux
  python  scripts/e77_node_ctrl.py query --port COM4                 # Windows

  # 恢复出厂（每次新测试前必须执行）
  python3 scripts/e77_node_ctrl.py restore --port /dev/ttyUSB0

  # OTAA 入网 + 持续发包（CN470-10，CH80~CH87）
  python3 scripts/e77_node_ctrl.py otaa \\
      --port /dev/ttyUSB0 \\
      --region 2 \\
      --deveui AABBCCDD11223344 \\
      --appeui 0000000000000000 \\
      --appkey 00112233445566778899AABBCCDDEEFF \\
      --chanmask 0000:0000:0000:0000:0000:00FF \\
      --adr 1 --interval 30 \\
      --payload DEADBEEF01020304

  # OTAA + Confirmed uplink（验证双向链路）
  python3 scripts/e77_node_ctrl.py otaa \\
      --port /dev/ttyUSB0 \\
      --region 2 --deveui AABBCCDD11223344 \\
      --appeui 0000000000000000 \\
      --appkey 00112233445566778899AABBCCDDEEFF \\
      --chanmask 0000:0000:0000:0000:0000:00FF \\
      --adr 1 --interval 10 --ack 1

  # OTAA + 发固定次数后退出
  python3 scripts/e77_node_ctrl.py otaa \\
      --port /dev/ttyUSB0 --region 2 \\
      --deveui AABBCCDD11223344 --appeui 0000000000000000 \\
      --appkey 00112233445566778899AABBCCDDEEFF \\
      --chanmask 0000:0000:0000:0000:0000:00FF \\
      --interval 20 --count 5 --ack 1

  # ABP 入网（SubBand1 mock NS 测试用）
  python3 scripts/e77_node_ctrl.py abp \\
      --port /dev/ttyUSB0 \\
      --region 2 \\
      --devaddr 26011234 \\
      --nwkskey 00112233445566778899AABBCCDDEEFF \\
      --appskey FFEEDDCCBBAA99887766554433221100 \\
      --chanmask 0001:0000:0000:0000:0000:0000 \\
      --adr 0 --dr 2 --interval 5

  # 仅发一包（已入网前提下）
  python3 scripts/e77_node_ctrl.py send --port /dev/ttyUSB0 --payload CAFEBABE

注意：
  - CN470 region=2，信道掩码默认 SubBand1（mask=0001:0000:0000:0000:0000:0000）
    若 ChirpStack 网关配置了其他子带，用 --chanmask 指定，如 0002:0000:...
  - EU868 region=5，US915 region=8
  - AT 指令结尾自动追加 \\r\\n，响应用 +EVT 或 OK/ERROR 判断
  - 查询 E77 出厂 DevEUI：serial 发送 AT+CDEVEUI=?
"""

import serial
import time
import argparse
import sys
import datetime

# ─────────────────────────────────────────────────────────────────────────────
#  全局常量
# ─────────────────────────────────────────────────────────────────────────────
REGION_MAP = {
    0: "AS923",
    1: "AU915",
    2: "CN470",
    4: "EU433",
    5: "EU868",
    6: "KR920",
    7: "IN865",
    8: "US915",
    9: "RU864",
}

DEFAULT_TIMEOUT = 5.0   # 普通指令超时（秒）
JOIN_TIMEOUT    = 30.0  # 等待入网超时（秒）
SEND_TIMEOUT    = 12.0  # 等待发送确认超时（秒）

# ─────────────────────────────────────────────────────────────────────────────
#  工具函数
# ─────────────────────────────────────────────────────────────────────────────
def ts():
    return datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]


def log(msg, tag="INFO"):
    print(f"[{ts()}] [{tag}] {msg}")


# ─────────────────────────────────────────────────────────────────────────────
#  串口封装
# ─────────────────────────────────────────────────────────────────────────────
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
            # 禁止 pyserial 拉动 DTR/RTS，防止模块意外复位
            dsrdtr=False,
            rtscts=False,
            xonxoff=False,
        )
        # 等待模块就绪（复位或上电约需 1s），再排空启动消息
        time.sleep(1.5)
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()

    def close(self):
        if self.ser and self.ser.is_open:
            self.ser.close()

    # ── 低层读写 ─────────────────────────────────────────────────────────────
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

    def _read_until(self, keywords, timeout=DEFAULT_TIMEOUT) -> list:
        """
        持续读行直到收到包含任意 keyword 的行，或超时。
        返回收到的所有非空行列表。
        """
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

    # ── 高层指令 ─────────────────────────────────────────────────────────────
    def send_cmd(self, cmd: str, expect="OK", timeout=DEFAULT_TIMEOUT):
        """发送 AT 指令，等待 expect 关键字。返回 (成功, 响应行列表)。"""
        echo_str = cmd.strip().upper()  # 用于识别并丢弃回显行
        self._write(cmd)
        raw_lines = self._read_until([expect, "ERROR", "AT_ERROR"], timeout=timeout)
        # 过滤掉模块回显的命令行（模块 echo on 时会原样返回发送的命令）
        lines = [l for l in raw_lines if l.upper() != echo_str]
        ok = any(expect.upper() in l.upper() for l in lines)
        if not ok:
            log(f"指令失败: {cmd}  响应: {lines}", "WARN")
        return ok, lines

    def at_test(self, retries: int = 3) -> bool:
        """测试 AT 通信是否正常，最多重试 retries 次（模块上电/复位后可能短暂返回 AT_ERROR）。"""
        for attempt in range(1, retries + 1):
            self.ser.reset_input_buffer()
            ok, resp = self.send_cmd("AT", expect="OK", timeout=2)
            if ok:
                return True
            log(f"AT 测试第 {attempt}/{retries} 次失败，响应: {resp}，1s 后重试...", "WARN")
            time.sleep(1.0)
        return False

    # ── 参数查询 ─────────────────────────────────────────────────────────────
    def query_all(self):
        cmds = [
            ("AT+VER=?",          "版本信息"),
            ("AT+DEVTYPE=?",      "设备型号"),
            ("AT+REGION=?",       "当前频段"),
            ("AT+CDEVEUI=?",      "DevEUI"),
            ("AT+CAPPEUI=?",      "AppEUI"),
            ("AT+CDEVADDR=?",     "DevAddr(ABP)"),
            ("AT+CCLASS=?",       "设备类型"),
            ("AT+CADR=?",         "ADR 状态"),
            ("AT+CDATARATE=?",    "数据速率"),
            ("AT+CTXP=?",         "发射功率"),
            ("AT+LINKC",          "链路检查触发"),
            ("AT+CLSTCHANNEL=?",  "当前信道"),
        ]
        print("\n" + "=" * 60)
        print("  E77 模块当前参数")
        print("=" * 60)
        for cmd, desc in cmds:
            self._write(cmd)
            time.sleep(0.1)
            lines = []
            deadline = time.time() + 2.0
            while time.time() < deadline:
                l = self._readline()
                if l:
                    lines.append(l)
            result = "  ".join(l for l in lines if l not in ("OK", ""))
            print(f"  {desc:18s}: {result}")
        print("=" * 60 + "\n")

    # ── 恢复出厂 ─────────────────────────────────────────────────────────────
    def restore(self):
        log("恢复出厂配置 ...")
        ok, lines = self.send_cmd("AT+RESTORE", expect="OK", timeout=5)
        if ok:
            log("出厂配置已恢复，等待重启 ...")
            time.sleep(2.0)
        return ok

    # ── OTAA 配置 ─────────────────────────────────────────────────────────────
    def config_otaa(self, region: int, deveui: str, appeui: str, appkey: str,
                    chanmask: str = None, adr: int = 1, dr: int = None,
                    txp: int = 0):
        """
        配置并执行 OTAA 入网。
        region:   见 REGION_MAP
        chanmask: CN470/US915/AU915 用，如 "0001:0000:0000:0000:0000:0000"
        dr:       None = 不强制设置（ADR 管理）；0~5 = 强制 SF

        注：E77 在入网状态下不允许修改 REGION/CDEVEUI 等参数（AT_PARAM_ERROR/AT_ERROR），
            因此每次配置前先 restore 恢复出厂状态，确保参数设置成功。
        """
        # 先恢复出厂，解除入网锁定状态，使参数可写
        self.restore()
        time.sleep(0.5)   # restore 内已等 2s，额外 0.5s 确保启动稳定

        steps = [
            (f"AT+REGION={region}",          "设置频段"),
            (f"AT+CDEVEUI={deveui.upper()}", "设置 DevEUI"),
            (f"AT+CAPPEUI={appeui.upper()}", "设置 AppEUI"),
            (f"AT+CAPPKEY={appkey.upper()}", "设置 AppKey"),
            # 注：Class A 是入网默认模式，入网前设置 AT+CCLASS 会报 AT_NO_NETWORK_JOINED
            (f"AT+CTXP={txp}",               "发射功率"),
            (f"AT+CADR={adr}",               "ADR"),
        ]
        if adr == 0 and dr is not None:
            steps.append((f"AT+CDATARATE={dr}", f"固定 DR{dr}"))
        if chanmask:
            steps.append(("AT+CMANUALMASK=1",              "手动掩码使能"))
            steps.append((f"AT+CFREQBANDMASK={chanmask}",  "信道掩码"))

        for cmd, desc in steps:
            ok, resp = self.send_cmd(cmd)
            status = "✓" if ok else "✗"
            log(f"{status} {desc}: {cmd}")
            if not ok:
                log(f"  响应: {resp}", "WARN")
            time.sleep(0.15)

        # 触发 OTAA 入网（不自动上电入网）
        log("─" * 50)
        log(f"触发 OTAA 入网 (region={REGION_MAP.get(region, region)}) ...")
        self._write("AT+CJOIN=1:0")
        lines = self._read_until(["+EVT:JOINED", "JOIN FAILED", "ERROR"],
                                 timeout=JOIN_TIMEOUT)
        joined = any("+EVT:JOINED" in l for l in lines)
        if joined:
            log("✓ OTAA 入网成功！", "OK")
        else:
            log(f"✗ OTAA 入网失败！响应: {lines}", "ERROR")
        return joined

    # ── ABP 配置 ─────────────────────────────────────────────────────────────
    def config_abp(self, region: int, devaddr: str, nwkskey: str, appskey: str,
                   chanmask: str = None, adr: int = 0, dr: int = 2, txp: int = 0):
        """
        配置并执行 ABP 本地入网。
        devaddr: 8 位 hex，如 "26011234"（无冒号时自动加冒号）

        注：同 OTAA，先 restore 解除入网锁定，确保参数可写。
        """
        # 先恢复出厂，解除入网锁定状态
        self.restore()
        time.sleep(0.5)

        # 自动格式化 devaddr 为 XX:XX:XX:XX
        if ":" not in devaddr and len(devaddr) == 8:
            devaddr = ":".join(devaddr[i:i+2] for i in range(0, 8, 2))

        steps = [
            (f"AT+REGION={region}",            "设置频段"),
            (f"AT+CDEVADDR={devaddr.upper()}", "设置 DevAddr"),
            (f"AT+CNWKSKEY={nwkskey.upper()}", "设置 NwkSKey"),
            (f"AT+CAPPSKEY={appskey.upper()}", "设置 AppSKey"),
            # 注：Class A 是入网默认模式，入网前不能设置 AT+CCLASS
            (f"AT+CTXP={txp}",                 "发射功率"),
            (f"AT+CADR={adr}",                 "ADR"),
        ]
        if adr == 0:
            steps.append((f"AT+CDATARATE={dr}", f"固定 DR{dr}"))
        if chanmask:
            steps.append(("AT+CMANUALMASK=1",             "手动掩码使能"))
            steps.append((f"AT+CFREQBANDMASK={chanmask}", "信道掩码"))

        for cmd, desc in steps:
            ok, resp = self.send_cmd(cmd)
            status = "✓" if ok else "✗"
            log(f"{status} {desc}: {cmd}")
            if not ok:
                log(f"  响应: {resp}", "WARN")
            time.sleep(0.15)

        log("─" * 50)
        log("触发 ABP 本地入网 ...")
        self._write("AT+CJOIN=0:0")
        lines = self._read_until(["+EVT:JOINED", "ERROR", "OK"], timeout=5)
        joined = any("+EVT:JOINED" in l or "OK" in l for l in lines)
        if joined:
            log("✓ ABP 入网成功！", "OK")
        else:
            log(f"✗ ABP 入网失败！响应: {lines}", "ERROR")
        return joined

    # ── 发送数据 ─────────────────────────────────────────────────────────────
    def send_data(self, payload_hex: str, port: int = 2, ack: int = 0,
                  retries: int = 1) -> bool:
        """
        发送上行数据。
        payload_hex: 偶数位十六进制字符串，如 "DEADBEEF"
        ack: 0=unconfirmed, 1=confirmed
        """
        payload_hex = payload_hex.upper().replace(" ", "")
        cmd = f"AT+SEND={port}:{retries}:{ack}:{payload_hex}"
        log(f"发送上行 → port={port} ack={ack} payload={payload_hex}")
        self._write(cmd)

        # 等待发送结果和下行
        keywords_ok  = ["+EVT:SEND_CONFIRMED", "+EVT:SEND_OK", "OK"]
        keywords_rx  = ["+EVT:RX_1", "+EVT:RX_2"]
        keywords_err = ["AT_NO_NETWORK", "AT_DUTYCYCLE", "ERROR"]

        lines = []
        deadline = time.time() + SEND_TIMEOUT
        got_tx_ok = False
        got_rx    = False

        while time.time() < deadline:
            line = self._readline()
            if not line:
                continue
            lines.append(line)
            if any(kw in line.upper() for kw in [k.upper() for k in keywords_ok]):
                got_tx_ok = True
                log(f"  TX 确认: {line}", "OK")
            if any(kw in line for kw in keywords_rx):
                got_rx = True
                log(f"  下行收到: {line}", "DOWN")
            if any(kw in line.upper() for kw in [k.upper() for k in keywords_err]):
                log(f"  发送错误: {line}", "ERROR")
                return False
            # 收到 TX 确认后再多等 3 秒等下行
            if got_tx_ok and (time.time() > deadline - SEND_TIMEOUT + 3):
                break

        if not got_tx_ok:
            log(f"  发送超时，响应: {lines}", "WARN")
        if not got_rx:
            log("  本次无下行（Class A 正常现象）", "INFO")
        return got_tx_ok

    # ── 周期发送循环 ─────────────────────────────────────────────────────────
    def loop_send(self, interval: int, payload_hex: str,
                  port: int = 2, ack: int = 0, count: int = 0):
        """
        周期发送循环。
        interval: 发送间隔秒数
        count:    发送次数，0=无限循环
        """
        sent = 0
        log(f"开始周期发送：间隔={interval}s  payload={payload_hex}  "
            f"{'无限循环' if count == 0 else f'共{count}次'}")
        log("按 Ctrl+C 停止")
        try:
            while True:
                sent += 1
                log(f"─── 第 {sent} 包 " + "─" * 40)
                self.send_data(payload_hex, port=port, ack=ack)
                if count > 0 and sent >= count:
                    break
                log(f"等待 {interval} 秒...")
                time.sleep(interval)
        except KeyboardInterrupt:
            log("用户中断，退出循环。")
        log(f"共发送 {sent} 包，结束。")


# ─────────────────────────────────────────────────────────────────────────────
#  CLI 入口
# ─────────────────────────────────────────────────────────────────────────────
def build_parser():
    p = argparse.ArgumentParser(
        description="E77-xxxM22S LoRaWAN 节点控制脚本",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument("command", choices=["otaa", "abp", "query", "send", "restore"],
                   help="执行的动作")
    p.add_argument("--port",     required=True,
                   help="串口设备：Linux 用 /dev/ttyUSBx，Windows 用 COMx")
    p.add_argument("--baud",     type=int, default=9600,
                   help="波特率（默认 9600）")
    p.add_argument("--verbose",  action="store_true",
                   help="打印所有串口收发原始内容")

    # LoRaWAN 公共参数
    p.add_argument("--region",   type=int, default=2,
                   help="频段 ID：0=AS923 1=AU915 2=CN470 4=EU433 5=EU868 8=US915（默认 2=CN470）")
    p.add_argument("--adr",      type=int, default=1,
                   help="ADR 0=关 1=开（默认 1）")
    p.add_argument("--dr",       type=int, default=None,
                   help="固定 DR（ADR=0 时有效）：0=SF12 1=SF11 2=SF10 3=SF9 4=SF8 5=SF7")
    p.add_argument("--txp",      type=int, default=0,
                   help="发射功率档位：0=最大(CN470 20dBm) ... 7=3dBm（默认 0）")
    p.add_argument("--chanmask", default=None,
                   help="CN470/US915 信道掩码，如 0001:0000:0000:0000:0000:0000（对应 SubBand1）")

    # OTAA 参数
    p.add_argument("--deveui",   default=None,
                   help="OTAA: DevEUI，16位hex（E77 出厂固化，AT+CDEVEUI=? 查询）")
    p.add_argument("--appeui",   default="0000000000000000",
                   help="OTAA: AppEUI（默认全0）")
    p.add_argument("--appkey",   default=None,
                   help="OTAA: AppKey，32位hex（ChirpStack 中配置）")

    # ABP 参数
    p.add_argument("--devaddr",  default=None,
                   help="ABP: DevAddr，8位hex，如 26011234")
    p.add_argument("--nwkskey",  default=None,
                   help="ABP: NwkSKey，32位hex")
    p.add_argument("--appskey",  default=None,
                   help="ABP: AppSKey，32位hex")

    # 发送参数
    p.add_argument("--payload",  default="DEADBEEF",
                   help="上行 payload 十六进制字符串（默认 DEADBEEF）")
    p.add_argument("--port-fwd", type=int, default=2, dest="fport",
                   help="LoRaWAN FPort（默认 2）")
    p.add_argument("--ack",      type=int, default=0,
                   help="Confirmed uplink 0=否 1=是（默认 0）")
    p.add_argument("--interval", type=int, default=30,
                   help="周期发送间隔秒数（默认 30）")
    p.add_argument("--count",    type=int, default=0,
                   help="发送总包数，0=无限（默认 0）")
    return p


def main():
    parser = build_parser()
    args = parser.parse_args()

    node = E77Node(args.port, baud=args.baud, verbose=args.verbose)

    # 测试 AT 通信
    if not node.at_test():
        log("AT 指令无响应，请检查串口连接和波特率！", "ERROR")
        node.close()
        sys.exit(1)
    log("AT 通信正常")

    try:
        # ── restore ──────────────────────────────────────────────────────────
        if args.command == "restore":
            node.restore()

        # ── query ─────────────────────────────────────────────────────────────
        elif args.command == "query":
            node.query_all()

        # ── send（仅发包，已入网前提）────────────────────────────────────────
        elif args.command == "send":
            node.loop_send(
                interval=args.interval,
                payload_hex=args.payload,
                port=args.fport,
                ack=args.ack,
                count=args.count if args.count > 0 else 1,
            )

        # ── OTAA ──────────────────────────────────────────────────────────────
        elif args.command == "otaa":
            if not args.deveui:
                log("OTAA 模式需要 --deveui", "ERROR"); sys.exit(1)
            if not args.appkey:
                log("OTAA 模式需要 --appkey", "ERROR"); sys.exit(1)

            joined = node.config_otaa(
                region=args.region,
                deveui=args.deveui,
                appeui=args.appeui,
                appkey=args.appkey,
                chanmask=args.chanmask,
                adr=args.adr,
                dr=args.dr,
                txp=args.txp,
            )
            if joined:
                node.loop_send(
                    interval=args.interval,
                    payload_hex=args.payload,
                    port=args.fport,
                    ack=args.ack,
                    count=args.count,
                )

        # ── ABP ───────────────────────────────────────────────────────────────
        elif args.command == "abp":
            if not args.devaddr:
                log("ABP 模式需要 --devaddr", "ERROR"); sys.exit(1)
            if not args.nwkskey:
                log("ABP 模式需要 --nwkskey", "ERROR"); sys.exit(1)
            if not args.appskey:
                log("ABP 模式需要 --appskey", "ERROR"); sys.exit(1)

            dr = args.dr if args.dr is not None else 2  # ABP 默认 DR2 (SF10)
            joined = node.config_abp(
                region=args.region,
                devaddr=args.devaddr,
                nwkskey=args.nwkskey,
                appskey=args.appskey,
                chanmask=args.chanmask,
                adr=args.adr,
                dr=dr,
                txp=args.txp,
            )
            if joined:
                node.loop_send(
                    interval=args.interval,
                    payload_hex=args.payload,
                    port=args.fport,
                    ack=args.ack,
                    count=args.count,
                )

    except KeyboardInterrupt:
        log("用户中断。")
    finally:
        node.close()


if __name__ == "__main__":
    main()
