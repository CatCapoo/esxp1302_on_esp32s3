# STM32F407 开发环境一键配置

## 快速开始

### 前置条件
1. **下载 STM32CubeCLT** 
   - 访问：https://www.st.com/en/development-tools/stm32cubeclt.html
   - 需要 ST 官方账号登录
   - 下载 Linux x86_64 版本（`.sh` 或 `.sh.zip`）
   - 放到 `~/Downloads/` 或 `~/下载/` 目录

### 一键配置
运行脚本自动完成所有配置：

```bash
cd /home/cuckooshan/Projects/esxp1302_on_esp32s3
bash install_toolchain.sh
```

### 脚本会自动做什么？

✅ **第1步** - 找到安装包
- 搜索 `~/Downloads/` 和 `~/下载/` 目录
- 支持 `.sh` 和 `.zip` 两种格式
- 自动解压 ZIP 文件

✅ **第2步** - 安装 STM32CubeCLT
- 使用 `sudo` 权限（需输入密码）
- 安装到 `/opt/st/`
- 自动安装必要的系统依赖库

✅ **第3步** - 配置环境变量
- 写入 `STM32_CLT_PATH` 到 `~/.bashrc`
- 新终端自动生效

✅ **第4步** - 验证工具链
- 检查 `arm-none-eabi-gcc`、`cmake`、`ninja` 可用

✅ **第5步** - CMake 配置与编译
- 执行 `cmake --preset Debug`
- 完整编译项目
- 生成 `.elf` 固件文件

✅ **第6步** - VS Code 自动配置
- 更新 `c_cpp_properties.json`（编译器路径）
- 更新 `settings.json`（CMake 和工具链路径）

---

## 配置完成后

### 新终端使用开发环境
```bash
# 环境变量已自动配置
cd esxp1302_stm32f407
cmake --preset Debug
cmake --build --preset Debug -j8
```

### VS Code 中编译
- 按 `Ctrl+Shift+B` → 选择 "Build" 任务
- 或按 `Ctrl+Shift+P` → 输入 "CMake: Configure"

### 查看编译输出
```
esxp1302_stm32f407/build/Debug/esxp1302_stm32f407.elf
```

---

## 故障排查

### 如果脚本找不到安装包

1. 确保 STM32CubeCLT 已下载到：
   - `~/Downloads/` 或
   - `~/下载/`

2. 文件名格式应为：
   ```
   st-stm32cubeclt_1.21.0_*.sh
   st-stm32cubeclt_1.21.0_*.zip
   ```

3. 手动指定安装包路径：
   ```bash
   # 先解压（如果是 ZIP）
   unzip ~/st-stm32cubeclt_*.zip -d ~/下载/
   
   # 再运行脚本
   bash install_toolchain.sh
   ```

### 如果脚本要求输入密码

这是正常的！脚本需要 `sudo` 权限来安装到 `/opt/st/`：
- 输入你的用户密码
- 按 Enter 继续

### 如果编译失败

检查是否重新打开了终端来加载环境变量：
```bash
# 立即加载环境变量
source ~/.bashrc

# 验证配置
echo $STM32_CLT_PATH
arm-none-eabi-gcc --version
```

---

## 技术背景

本配置基于项目的**跨平台开发方案**：

- **核心思路**：通过 `CMakePresets.json` 中的 `$env{STM32_CLT_PATH}` 环境变量占位符，将工具链路径从版本控制中剥离
- **优势**：每个开发者无需修改版本库文件，只需本机配置一次环境变量
- **支持平台**：Windows / Linux / macOS

详见：[doc/impl/05_cross_platform_config.md](esxp1302_stm32f407/doc/impl/05_cross_platform_config.md)

---

## 脚本详情

| 功能 | 文件 |
|------|------|
| **安装脚本** | `install_toolchain.sh` |
| **诊断脚本** | `diagnose_toolchain.sh`（可选，用于故障排查） |
| **项目文档** | `esxp1302_stm32f407/doc/impl/05_cross_platform_config.md` |

---

**完成时间**：约 5-10 分钟（取决于网络速度和编译时间）
