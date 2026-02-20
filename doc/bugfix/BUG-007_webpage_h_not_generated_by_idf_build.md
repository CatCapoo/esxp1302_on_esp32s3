# BUG-007：webpage.h 在 ESP-IDF 构建中不会自动生成

- **日期**：2026-02-20  
- **文件**：`main/packet_forwarder/webpage.h`（生成产物）、`scripts/dump_html.py`  
- **严重级别**：编译失败（缺少生成文件）

---

## 现象

使用 `idf.py build` 构建时报错：

```
fatal error: webpage.h: No such file or directory
```

`http_server.c` 中 `#include "webpage.h"`，但该文件在代码仓库中不存在。

---

## 根本原因

`webpage.h` 是由 `scripts/dump_html.py` 从 `main/packet_forwarder/webpage.html` 生成的 C 字节数组头文件。

**PlatformIO** 工程通过 `platformio_pre.py` 预构建钩子自动运行该脚本；但 **ESP-IDF CMake** 构建不调用 PlatformIO 钩子，因此每次 first-build 或 clean 后都需要手动生成。

---

## 修复（临时）

在首次构建或清理后，在项目根目录手动执行：

```bash
cd main/packet_forwarder
python ../../scripts/dump_html.py webpage.html webpage_str > webpage.h
```

或在 Windows PowerShell：

```powershell
cd main\packet_forwarder
python ..\..\scripts\dump_html.py webpage.html webpage_str > webpage.h
```

---

## 建议（长期）

在 `main/CMakeLists.txt` 中通过 `add_custom_command` / `add_custom_target` 将生成步骤集成到 CMake 构建：

```cmake
add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_SOURCE_DIR}/packet_forwarder/webpage.h
    COMMAND python ${PROJECT_SOURCE_DIR}/scripts/dump_html.py
                   ${CMAKE_CURRENT_SOURCE_DIR}/packet_forwarder/webpage.html
                   webpage_str
                   > ${CMAKE_CURRENT_SOURCE_DIR}/packet_forwarder/webpage.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/packet_forwarder/webpage.html
    COMMENT "Generating webpage.h from webpage.html"
)
```

---

## 注意

`webpage.h` 已加入 `.gitignore`，不需要提交到版本库。
