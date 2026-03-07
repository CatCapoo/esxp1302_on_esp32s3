# 05 — STM32 跨平台开发配置方案

## 概述

本文记录当前工程的跨平台开发配置方案。核心思路是：通过 `CMakePresets.json` 中的 **环境变量占位符**，将 STM32CubeCLT 的安装路径从版本控制中剥离，各开发者只需在本机配置一次 `STM32_CLT_PATH` 环境变量，即可直接构建，无需修改任何代码文件。

---

## 1. 背景：为什么需要跨平台配置

STM32CubeCLT（包含 arm-none-eabi-gcc / CMake / Ninja / STM32CubeProgrammer）在不同机器上的安装路径可能不同，例如：

| 场景 | 典型路径 |
|------|----------|
| Windows 默认安装 | `C:\ST\STM32CubeCLT_1.21.0` |
| Windows 非默认盘 | `D:\ST\STM32CubeCLT_1.21.0` |
| Windows 不同版本 | `C:\ST\STM32CubeCLT_1.22.0` |
| Linux / WSL | `/opt/ST/STM32CubeCLT_1.21.0` |

如果在 `CMakePresets.json` 中**硬编码**安装路径，其他机器 clone 后必须手动修改文件，容易产生不必要的 git diff，也难以维护。

---

## 2. 当前方案：`STM32_CLT_PATH` 环境变量

### 2.1 CMakePresets.json 中的配置

`CMakePresets.json` 的 `default` preset 使用 `$env{STM32_CLT_PATH}` 引用环境变量：

```json
"environment": {
    "PATH": "$env{STM32_CLT_PATH}/GNU-tools-for-STM32/bin;$env{STM32_CLT_PATH}/CMake/bin;$penv{PATH}"
}
```

| 占位符 | 含义 |
|--------|------|
| `$env{STM32_CLT_PATH}` | 读取本机环境变量 `STM32_CLT_PATH` |
| `$penv{PATH}` | 保留系统原有 `PATH`，不覆盖 |

该配置会在 CMake 运行期间将以下两个目录**前置**到 PATH：
- `{STM32_CLT_PATH}/GNU-tools-for-STM32/bin` — arm-none-eabi-gcc 等编译工具
- `{STM32_CLT_PATH}/CMake/bin` — CubeCLT 内置的 CMake

### 2.2 工具链文件

工具链文件路径通过 preset 相对路径指定，无需修改：

```json
"toolchainFile": "${sourceDir}/cmake/gcc-arm-none-eabi.cmake"
```

`gcc-arm-none-eabi.cmake` 内部使用 `CMAKE_FIND_ROOT_PATH` 等机制，不包含硬编码的安装路径。

---

## 3. 如何使用（首次配置）

### 3.1 安装 STM32CubeCLT

从 ST 官网下载并安装 [STM32CubeCLT](https://www.st.com/en/development-tools/stm32cubeclt.html)。  
安装完成后记录安装根目录，例如：

```
C:\ST\STM32CubeCLT_1.21.0
```

### 3.2 设置环境变量

#### Windows（推荐：系统级永久配置）

在 PowerShell（管理员）中执行（路径改为实际安装路径）：

```powershell
[System.Environment]::SetEnvironmentVariable(
    "STM32_CLT_PATH",
    "C:\ST\STM32CubeCLT_1.21.0",
    "Machine"   # "User" 只对当前用户生效，"Machine" 对全部用户生效
)
```

或通过图形界面：`Win + R` → `sysdm.cpl` → "高级" → "环境变量" → 新建：

| 变量名 | 变量值 |
|--------|--------|
| `STM32_CLT_PATH` | `C:\ST\STM32CubeCLT_1.21.0` |

> **注意**：设置后需要**重新打开终端和 VS Code** 才能生效。

#### Linux / macOS

在 `~/.bashrc`（bash）或 `~/.zshrc`（zsh）中添加：

```bash
export STM32_CLT_PATH="/opt/ST/STM32CubeCLT_1.21.0"
```

然后执行 `source ~/.bashrc` 或重新打开终端。

### 3.3 验证配置

打开新终端，执行：

```powershell
# Windows PowerShell
$env:STM32_CLT_PATH
# 应输出类似：C:\ST\STM32CubeCLT_1.21.0

# 验证编译器可访问
& "$env:STM32_CLT_PATH\GNU-tools-for-STM32\bin\arm-none-eabi-gcc.exe" --version
```

```bash
# Linux / macOS
echo $STM32_CLT_PATH
arm-none-eabi-gcc --version
```

---

## 4. 构建流程

环境变量配置完成后，标准构建流程如下（在工程根目录 `esxp1302_stm32f407/` 下执行）：

```powershell
# Configure（首次或 CMakeLists.txt 修改后）
cmake --preset Debug

# Build
cmake --build --preset Debug -j8

# Clean
cmake --build --preset Debug --target clean
```

也可直接使用 VS Code 任务（`Ctrl+Shift+B`）触发默认 Build 任务。

---

## 5. 说明与注意事项

### 5.1 Ninja 的位置

CubeCLT 自带 Ninja，但其路径未加入 PATH 配置。CMake 在配置阶段会通过 `PATH` 中的 CMake 自动定位同级目录的 Ninja，或者 CMakePresets 的 `generator = "Ninja"` 会让 CMake 在环境中搜索。如果出现 `ninja: not found` 错误，可手动追加：

```json
"PATH": "$env{STM32_CLT_PATH}/GNU-tools-for-STM32/bin;$env{STM32_CLT_PATH}/CMake/bin;$env{STM32_CLT_PATH}/Ninja/bin;$penv{PATH}"
```

### 5.2 不需要将 STM32_CLT_PATH 加入版本控制

`.gitignore` 中无需特殊处理——该变量由各开发者在各自机器上配置，不出现在任何被追踪的文件中。

### 5.3 CI 环境

在 CI（如 GitHub Actions / GitLab CI）中，通过 Secret / Variable 注入 `STM32_CLT_PATH` 即可，无需修改仓库文件。

---

## 6. 与 ESP32 (ESP-IDF) 的对比

| 项目 | STM32 (本方案) | ESP32 (ESP-IDF) |
|------|---------------|-----------------|
| 工具链管理 | 手动安装 CubeCLT，设置环境变量 | `idf_tools.py install` 自动下载管理 |
| 配置入口 | `STM32_CLT_PATH` 环境变量 | `IDF_PATH` + `idf_tools.py export` |
| 初始化脚本 | 无（手动设置一次即可） | `get_idf` / `. $IDF_PATH/export.sh` |
| 跨平台支持 | Windows / Linux / macOS 均可 | Windows / Linux / macOS 均可 |
| 版本锁定 | 变量值含版本号，更换版本需改变量 | `idf_tools.py` 按 `idf_tools.json` 锁定 |
