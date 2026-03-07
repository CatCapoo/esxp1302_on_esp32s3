#!/bin/bash
# =============================================================================
# STM32F407 完整开发环境自动配置脚本
# 
# 功能：
#   1. 检测或安装 STM32CubeCLT 工具链
#   2. 配置 STM32_CLT_PATH 环境变量到 ~/.bashrc
#   3. 验证编译工具链可用性
#   4. 自动配置 VS Code（c_cpp_properties.json 和 settings.json）
#   5. CMake 工程配置和编译验证
#
# 用法：
#   bash install_toolchain.sh
#
# 前提条件：
#   - 已从 ST 官网下载 STM32CubeCLT 安装包（支持 .sh 和 .zip 格式）
#   - 安装包放在 ~/Downloads 或 ~/下载 目录
#
# 运行后效果：
#   - 环境变量已配置到 ~/.bashrc（新终端自动生效）
#   - VS Code 配置已自动更新
#   - 项目已成功编译验证
#
# =============================================================================
set -euo pipefail

BASHRC="$HOME/.bashrc"
CLT_INSTALL_BASE="/opt/ST"
CLT_NAME_PATTERN="STM32CubeCLT*"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok()   { echo -e "${GREEN}[✓]${NC} $*"; }
warn() { echo -e "${YELLOW}[!]${NC} $*"; }
err()  { echo -e "${RED}[✗]${NC} $*"; }
info() { echo -e "    $*"; }

echo "================================================="
echo " STM32F407 工具链安装脚本"
echo "================================================="
echo ""

# ─── 1. 检查 STM32CubeCLT 是否已安装 ─────────────────────────────────────────
echo "▶ 检查 STM32CubeCLT 安装状态..."

# 搜索所有可能的安装位置（包括小写 st 和大写 ST）
INSTALLED_CLT=$(find /opt/st /opt/ST /usr/local "$HOME" 2>/dev/null \
    -maxdepth 3 -name "GNU-tools-for-STM32" -type d 2>/dev/null \
    | head -1 | xargs -I{} dirname {} 2>/dev/null || true)

if [ -n "$INSTALLED_CLT" ] && [ -x "$INSTALLED_CLT/GNU-tools-for-STM32/bin/arm-none-eabi-gcc" ]; then
    ok "已检测到 STM32CubeCLT: $INSTALLED_CLT"
    CLT_PATH="$INSTALLED_CLT"
else
    warn "未检测到 STM32CubeCLT 安装"
    echo ""

    # 搜索安装包（同时支持 .sh 和 .zip）
    echo "▶ 在常见位置搜索安装包..."
    INSTALLER=$(find "$HOME/Downloads" "$HOME/下载" "$HOME/Desktop" "$HOME/桌面" "$HOME" /tmp . 2>/dev/null \
        -maxdepth 3 \( -name "st-stm32cubeclt*.sh" -o -name "st-stm32cubeclt*.zip" \) -type f 2>/dev/null \
        | sort -r | head -1 || true)

    if [ -z "$INSTALLER" ]; then
        err "未找到 STM32CubeCLT 安装包"
        echo ""
        echo "  请手动下载安装包（需要 ST 账号登录）："
        echo "  https://www.st.com/en/development-tools/stm32cubeclt.html"
        echo ""
        echo "  支持以下文件格式："
        echo "    - st-stm32cubeclt_*.sh (shell 脚本)"
        echo "    - st-stm32cubeclt_*.zip (ZIP 压缩包，脚本会自动解压)"
        echo ""
        echo "  下载后重新运行本脚本，或手动安装："
        echo "    sudo sh st-stm32cubeclt_*.sh"
        echo ""
        echo "  如果已安装在非标准路径，请手动设置环境变量："
        echo "    echo 'export STM32_CLT_PATH=\"<你的安装路径>\"' >> ~/.bashrc"
        echo "    source ~/.bashrc"
        exit 1
    fi

    ok "找到安装包: $INSTALLER"
    
    # 处理 ZIP 文件 - 自动解压
    if [[ "$INSTALLER" == *.zip ]]; then
        echo ""
        echo "▶ 解压 ZIP 文件..."
        TEMP_DIR=$(mktemp -d)
        if unzip -q "$INSTALLER" -d "$TEMP_DIR"; then
            # 在临时目录中查找 .sh 文件
            INSTALLER=$(find "$TEMP_DIR" -maxdepth 1 -name "st-stm32cubeclt*.sh" -type f | head -1)
            if [ -z "$INSTALLER" ]; then
                err "ZIP 文件中未找到有效的 .sh 安装脚本"
                rm -rf "$TEMP_DIR"
                exit 1
            fi
            ok "解压完成，已找到安装脚本: $(basename "$INSTALLER")"
        else
            err "解压失败"
            rm -rf "$TEMP_DIR"
            exit 1
        fi
    fi
    
    echo ""
    echo "▶ 安装 STM32CubeCLT (需要 sudo 权限)..."
    echo "  注意: 安装过程中请按提示接受许可协议"
    echo ""

    sudo mkdir -p /opt/st /opt/ST 2>/dev/null || true

    # 尝试静默安装；若失败则以交互方式运行
    if sudo sh "$INSTALLER" --quiet --accept-licenses \
            --root /opt/st 2>/dev/null; then
        ok "静默安装完成（安装到 /opt/st）"
    elif sudo sh "$INSTALLER" --quiet --accept-licenses 2>/dev/null; then
        ok "静默安装完成"
    else
        warn "静默安装失败，切换为交互模式..."
        sudo sh "$INSTALLER" || true
    fi
    
    # 清理临时目录
    [ -n "$TEMP_DIR" ] && [ -d "$TEMP_DIR" ] && rm -rf "$TEMP_DIR"

    # 重新定位安装目录（搜索小写 st 优先）
    INSTALLED_CLT=$(find /opt/st /opt/ST /usr/local 2>/dev/null \
        -maxdepth 3 -name "GNU-tools-for-STM32" -type d \
        | head -1 | xargs -I{} dirname {} 2>/dev/null || true)

    if [ -z "$INSTALLED_CLT" ] || [ ! -x "$INSTALLED_CLT/GNU-tools-for-STM32/bin/arm-none-eabi-gcc" ]; then
        err "安装后仍未找到 arm-none-eabi-gcc，请检查安装日志"
        exit 1
    fi

    ok "安装完成: $INSTALLED_CLT"
    CLT_PATH="$INSTALLED_CLT"
fi

echo ""

# ─── 2. 检查并安装系统依赖（部分 Linux 发行版缺少运行库） ─────────────────────
echo "▶ 检查系统运行库..."

MISSING_LIBS=()
# CubeCLT 工具在部分系统上需要 lib32 / libncurses / libusb
for pkg in libncurses5 libusb-1.0-0 libc6-i386 unzip; do
    if ! dpkg -l "$pkg" &>/dev/null; then
        MISSING_LIBS+=("$pkg")
    fi
done

if [ ${#MISSING_LIBS[@]} -gt 0 ]; then
    warn "缺少以下系统库: ${MISSING_LIBS[*]}"
    echo "  正在安装..."
    sudo apt-get update >/dev/null 2>&1 || true
    sudo apt-get install -y "${MISSING_LIBS[@]}" 2>/dev/null \
        || warn "部分库安装失败，如遇运行问题再手动安装"
else
    ok "系统运行库齐全"
fi

echo ""

# ─── 3. 配置环境变量 ────────────────────────────────────────────────────────
echo "▶ 配置 STM32_CLT_PATH 环境变量..."

if grep -q "STM32_CLT_PATH" "$BASHRC" 2>/dev/null; then
    CURRENT=$(grep "export STM32_CLT_PATH" "$BASHRC" | tail -1 | sed 's/.*="\?//;s/"\?$//')
    if [ "$CURRENT" = "$CLT_PATH" ]; then
        ok "STM32_CLT_PATH 已正确配置: $CLT_PATH"
    else
        warn "STM32_CLT_PATH 已存在旧值: $CURRENT"
        warn "更新为: $CLT_PATH"
        sed -i.bak '/export STM32_CLT_PATH/d' "$BASHRC"
        printf '\n# STM32CubeCLT (auto-configured by install_toolchain.sh)\nexport STM32_CLT_PATH="%s"\n' "$CLT_PATH" >> "$BASHRC"
        ok "已更新 $BASHRC"
    fi
else
    printf '\n# STM32CubeCLT (auto-configured by install_toolchain.sh)\nexport STM32_CLT_PATH="%s"\n' "$CLT_PATH" >> "$BASHRC"
    ok "已写入 $BASHRC"
    # 同时写入 ~/.profile，GUI 启动的程序（如 VS Code 桌面图标）也能读到
    if ! grep -q "STM32_CLT_PATH" "$HOME/.profile" 2>/dev/null; then
        printf '\n# STM32CubeCLT\nexport STM32_CLT_PATH="%s"\n' "$CLT_PATH" >> "$HOME/.profile"
        ok "已写入 ~/.profile（GUI 应用生效）"
    fi
fi

# 在当前 shell 中立即生效
export STM32_CLT_PATH="$CLT_PATH"
export PATH="$STM32_CLT_PATH/GNU-tools-for-STM32/bin:$STM32_CLT_PATH/CMake/bin:$STM32_CLT_PATH/Ninja/bin:$PATH"

# ─── 4. 验证所有工具 ────────────────────────────────────────────────────────
echo "▶ 验证工具链..."

ALL_OK=true

check_tool() {
    local bin="$STM32_CLT_PATH/$1"
    local label="$2"
    if [ -x "$bin" ]; then
        local ver
        ver=$("$bin" --version 2>&1 | head -1)
        ok "$label: $ver"
    else
        err "$label 不可用: $bin"
        ALL_OK=false
    fi
}

check_tool "GNU-tools-for-STM32/bin/arm-none-eabi-gcc" "arm-none-eabi-gcc"
check_tool "CMake/bin/cmake"                            "cmake"

# Ninja 路径在不同 CubeCLT 版本可能不同
NINJA_BIN=""
for p in "$STM32_CLT_PATH/Ninja/bin/ninja" "$STM32_CLT_PATH/CMake/bin/ninja"; do
    [ -x "$p" ] && NINJA_BIN="$p" && break
done
if [ -n "$NINJA_BIN" ]; then
    ok "ninja: $($NINJA_BIN --version)"
else
    # 尝试系统 ninja
    if command -v ninja &>/dev/null; then
        ok "ninja (系统): $(ninja --version)"
    else
        warn "ninja 未找到，尝试安装..."
        sudo apt-get install -y ninja-build && ok "ninja 已通过 apt 安装" || { err "ninja 安装失败"; ALL_OK=false; }
    fi
fi

echo ""

# ─── 5. 尝试 CMake 配置构建 ─────────────────────────────────────────────────
echo "▶ 尝试 cmake --preset Debug..."

PROJ_DIR="$(dirname "$0")/esxp1302_stm32f407"
cd "$PROJ_DIR" || exit 1

if cmake --preset Debug 2>&1; then
    ok "CMake 配置成功"
    echo ""
    echo "▶ 开始编译..."
    if cmake --build --preset Debug -j"$(nproc)"; then
        ok "编译成功！"
        ELF="build/Debug/esxp1302_stm32f407.elf"
        [ -f "$ELF" ] && info "输出文件: $PROJ_DIR/$ELF"
    else
        err "编译失败，请查看上方错误信息"
    fi
else
    err "CMake 配置失败，请查看上方错误信息"
    ALL_OK=false
fi

# ─── 6. 配置 VS Code（如果在 VS Code 中打开） ──────────────────────────────
VSCODE_DIR="$PROJ_DIR/../.vscode"
if [ -d "$VSCODE_DIR" ]; then
    echo "▶ 更新 VS Code 配置..."
    
    # 更新 c_cpp_properties.json 中的编译器路径
    C_CPP_PROPS="$VSCODE_DIR/c_cpp_properties.json"
    if [ -f "$C_CPP_PROPS" ]; then
        # 使用 sed 更新编译器路径（支持跨平台格式）
        sed -i.bak \
            "s|\"compilerPath\": \"[^\"]*arm-none-eabi-gcc[^\"]*\"|\"compilerPath\": \"$STM32_CLT_PATH/GNU-tools-for-STM32/bin/arm-none-eabi-gcc\"|g" \
            "$C_CPP_PROPS" 2>/dev/null || true
        
        ok "已更新 c_cpp_properties.json"
    fi
    
    # 更新 settings.json 中的 CMake 可执行文件路径
    SETTINGS="$VSCODE_DIR/settings.json"
    if [ -f "$SETTINGS" ]; then
        # 创建临时 Python 脚本来更新 JSON 配置
        TEMP_PY=$(mktemp)
        cat > "$TEMP_PY" << 'PYTHON_EOF'
import json
import sys

settings_path = sys.argv[1]
clt_path = sys.argv[2]

try:
    with open(settings_path, 'r') as f:
        settings = json.load(f)
    
    # 写入核心路径配置（launch.json 通过 ${config:stm32.cltPath} 引用）
    settings["stm32.cltPath"] = clt_path
    # Cortex-Debug 全局工具链路径（无需在 launch.json 重复写）
    settings["cortex-debug.armToolchainPath"] = f"{clt_path}/GNU-tools-for-STM32/bin"
    settings["cortex-debug.gdbPath"] = f"{clt_path}/GNU-tools-for-STM32/bin/arm-none-eabi-gdb"
    # CMake
    settings["cmake.cmakePath"] = f"{clt_path}/CMake/bin/cmake"
    settings["cmake.executable"] = f"{clt_path}/CMake/bin/cmake"
    # C++ IntelliSense
    settings["C_Cpp_Runner.cCompilerPath"] = f"{clt_path}/GNU-tools-for-STM32/bin/arm-none-eabi-gcc"
    settings["C_Cpp_Runner.cppCompilerPath"] = f"{clt_path}/GNU-tools-for-STM32/bin/arm-none-eabi-g++"
    
    with open(settings_path, 'w') as f:
        json.dump(settings, f, indent=2)
    
    print("OK")
except Exception as e:
    print(f"ERROR: {e}", file=sys.stderr)
    sys.exit(1)
PYTHON_EOF
        
        if command -v python3 &>/dev/null; then
            python3 "$TEMP_PY" "$SETTINGS" "$STM32_CLT_PATH" >/dev/null 2>&1 && ok "已更新 settings.json" || true
        fi
        rm -f "$TEMP_PY"
    fi
else
    warn "VS Code 配置目录不存在，跳过 VS Code 配置"
fi
