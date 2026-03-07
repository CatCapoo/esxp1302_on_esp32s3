# Windows 构建指南

> 本项目原始构建脚本为 `run_me.sh`，仅适用于 Linux/macOS。  
> `run_me.py` 是 Windows 下的完整等价替代，功能、行为与 `run_me.sh` 一致。

---

## 前提条件

1. 安装 [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/)（推荐通过官方 Windows Installer）。
2. 使用 **ESP-IDF Terminal**（安装后开始菜单可见），该终端已自动导出 `IDF_PATH`、`idf.py`、`esptool.py` 等所有必要环境变量。  
   **普通 PowerShell / cmd 不可用于 `make` / `flash` 命令**。
3. Python 3.10+（ESP-IDF 自带，无需额外安装）。

---

## 命令一览

在 **ESP-IDF Terminal** 中，于项目根目录执行：

| 命令 | 等价 run_me.sh | 说明 |
|------|----------------|------|
| `python run_me.py make` | `./run_me.sh make` | 生成头文件 → 编译 app（跳过测试） |
| `python run_me.py make_all` | `./run_me.sh make_all` | 生成头文件 → 完整编译（含 bootloader） |
| `python run_me.py flash --port COMx` | `./run_me.sh flash` | 仅烧录 app |
| `python run_me.py flash_all --port COMx` | `./run_me.sh flash_all` | 烧录 bootloader + app + 分区表 |
| `python run_me.py run --port COMx` | `./run_me.sh run` | 打开串口监视器 |
| `python run_me.py flash run --port COMx` | `./run_me.sh flash run` | 烧录后立即打开监视器 |

### 可选参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--port COMx` | `COM3` | 串口号，查看设备管理器确认 |
| `--baud N` | `921600` | 烧录波特率 |

---

## 生成的头文件

`make` / `make_all` 执行前，脚本会自动（重新）生成以下两个文件：

| 文件 | 来源 | 说明 |
|------|------|------|
| `main/packet_forwarder/webpage.h` | `webpage.html` | Web UI 页面内容，编译进固件 |
| `main/global_json.h` | `global_conf.cn490.json` / `eu868.json` / `us915.json` | 三个地区的默认配置，编译进固件 |

> **重要**：每次修改 `global_conf.*.json` 或 `webpage.html` 后，**必须重新运行 `python run_me.py make`**，否则改动不会生效（固件烧录的仍是旧 hex 字节数组）。  
> 也可以单独运行 `python scripts/gen_global_json.py` 只更新 `global_json.h`。

---

## Flash 参数说明

针对本项目 ESP32-S3 硬件，`run_me.py` 使用的 flash 参数来自 `build/flasher_args.json`，与原 `run_me.sh` 中针对旧 ESP32 的参数**不同**：

| 参数 | run_me.sh（旧/错） | run_me.py（正确） |
|------|-------------------|-----------------|
| `--chip` | `esp32` | `esp32s3` |
| Bootloader 地址 | `0x1000` | `0x0` |
| `--flash_freq` | `40m` | `80m` |
| `--flash_size` | `detect` | `16MB` |

---

## 常见问题

### `OSError: [WinError 193] %1 不是有效的 Win32 应用程序`
`idf.py` 在 Windows 上是 Python 脚本，不能直接作为可执行文件调用。  
`run_me.py` 已通过 `shell=True` 解决此问题，遇到此错误请确认使用的是最新版 `run_me.py`。

### 日志显示 `push_timeout_ms` 与 JSON 配置不符
`global_json.h` 是编译进固件的 hex 字节数组，修改 JSON 后必须重新生成并重新编译。  
历史上曾出现 `global_conf.cn490.json` 已改为 `500ms`，但固件仍显示 `1800ms` 的问题，原因就是 `global_json.h` 未更新。

### 串口号不知道是哪个
打开 **设备管理器 → 端口 (COM 和 LPT)**，插拔设备观察新增的 COM 口即为目标串口。

### `idf.py` 找不到
未在 ESP-IDF Terminal 中运行。普通 PowerShell 没有 ESP-IDF 环境，需切换到 ESP-IDF Terminal。

---

## 脚本位置

| 脚本 | 用途 |
|------|------|
| `run_me.py` | 主构建脚本，替代 `run_me.sh` |
| `scripts/gen_global_json.py` | 单独重新生成 `global_json.h` |
| `scripts/json_to_hex_array.py` | 底层工具，供 `run_me.sh` / gen 脚本调用 |
| `scripts/dump_html.py` | 底层工具，生成 `webpage.h` |
