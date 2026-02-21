# Windows 构建工具进展备忘录

**日期：** 2026-02-22  
**目的：** 在 Windows 上替代 `run_me.sh`，实现一条命令完成"更新配置头文件 + 编译"，
彻底避免因 `global_json.h` 未更新导致固件与 JSON 配置不一致的问题。

---

## 一、背景与问题

### 原有流程（Linux only）

```
修改 global_conf.*.json
    ↓
./run_me.sh make
    ├─ scripts/json_to_hex_array.py → main/global_json.h（hex 字节数组）
    ├─ scripts/dump_html.py         → main/packet_forwarder/webpage.h
    └─ idf.py -DCONFIG_LIBLORAGW_TEST=0 app
```

`global_json.h` 是三个地区 JSON 配置**编译进固件**的字节数组，运行时才解析。  
Windows 无法直接运行 `.sh`，以前只能用 WSL 或手动调用各脚本。

### 发现的具体 Bug

日志显示：
```
INFO: upstream PUSH_DATA time-out is configured to 1800 ms
```
而 `global_conf.cn490.json` 中明明已改为 `"push_timeout_ms": 500`。

根本原因：`global_json.h` 是 `run_me.sh` 的产物，**在 Windows 上直接改 JSON 后若不重新生成该文件并重编译，改动不会生效**。固件内烧录的仍是旧值 `1800`。

---

## 二、解决方案

### 新增脚本

| 文件 | 说明 |
|------|------|
| `run_me.py` | 完整替代 `run_me.sh`，支持 make / make_all / flash / flash_all / run |
| `scripts/gen_global_json.py` | 单独重新生成 `main/global_json.h` 的独立工具 |

### run_me.py 用法

在 **ESP-IDF Terminal** 中执行：

```powershell
# 编译（最常用）
python run_me.py make

# 完整编译（含 bootloader）
python run_me.py make_all

# 烧录 app
python run_me.py flash --port COM3

# 烧录全部（首次烧录或 bootloader 有变动）
python run_me.py flash_all --port COM3

# 烧录后立即打开监视器
python run_me.py flash run --port COM3
```

> `--port` 默认 `COM3`，`--baud` 默认 `921600`。

### gen_global_json.py 用法

只需更新配置头文件而不重新编译时：

```powershell
python scripts/gen_global_json.py
```

输出示例：
```
Generated: D:\...\main\global_json.h
  global_cn_conf: push_timeout_ms": 500,
  global_eu_conf: push_timeout_ms": 100,
  global_us_conf: push_timeout_ms": 100,
```

---

## 三、Flash 参数修正

原 `run_me.sh` 的 flash 参数针对旧 ESP32，对本项目 ESP32-S3 有误，`run_me.py` 已修正：

| 参数 | run_me.sh（错误） | run_me.py（正确） |
|------|-----------------|-----------------|
| `--chip` | `esp32` | `esp32s3` |
| Bootloader 地址 | `0x1000` | `0x0` |
| `--flash_freq` | `40m` | `80m` |
| `--flash_size` | `detect` | `16MB` |

参数来源：`build/flasher_args.json`（CMake 生成，始终与实际硬件配置一致）。

---

## 四、注意事项

1. **必须在 ESP-IDF Terminal 中运行**，普通 PowerShell 没有 `idf.py` / `esptool.py` 环境。
2. **每次修改 JSON 配置后务必重新运行 `python run_me.py make`**，不能只改 JSON 就烧录。
3. `main/packet_forwarder/global_json.h` 已被 `.gitignore` 忽略（正确，属于生成产物）；  
   `main/global_json.h` 需要提交（历史惯例，参与编译）。
4. 若切换分支后出现日志值与配置不符，第一反应是检查 `global_json.h` 是否过期，运行 `gen_global_json.py` 再重编即可。

---

## 五、提交记录

| Commit | 说明 |
|--------|------|
| `2ff4f9b` | `build: add run_me.py to replace run_me.sh on Windows` |

---

## 六、待办 / 后续改进

- [ ] 考虑在 CMakeLists.txt 中添加 cmake custom target，自动在编译前调用 `gen_global_json.py`，彻底防止遗忘
- [ ] `run_me.py flash` 的串口号可从 `sdkconfig` 或环境变量读取，避免每次手动指定
