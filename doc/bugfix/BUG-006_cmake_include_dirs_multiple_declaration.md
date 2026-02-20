# BUG-006：main/CMakeLists.txt 多次声明 INCLUDE_DIRS，仅最后一条生效

- **日期**：2026-02-20  
- **文件**：`main/CMakeLists.txt`  
- **严重级别**：编译失败（关键头文件无法找到）

---

## 现象

使用 ESP-IDF 构建时报错：

```
fatal error: global_json.h: No such file or directory
```

`global_json.h` 位于 `main/` 目录下，但 `packet_forwarder/` 子目录中的源文件无法找到它。

---

## 根本原因

`idf_component_register()` 同一调用中多次出现 `INCLUDE_DIRS` 关键字时，CMake 只保留**最后一条**，前面的全部被静默丢弃。

原始 `main/CMakeLists.txt`：

```cmake
idf_component_register(
    SRCS ...
    INCLUDE_DIRS "libloragw"
    INCLUDE_DIRS "libtools"
    INCLUDE_DIRS "libloragw-test"
    INCLUDE_DIRS "packet_forwarder"
    ...
)
```

实际生效的只有 `"packet_forwarder"`，`main/` 目录（含 `global_json.h`）以及其余子目录均未被包含。

---

## 修复

将所有路径合并到单个 `INCLUDE_DIRS` 条目，并补充 `"."` 以暴露 `main/` 根目录：

```cmake
idf_component_register(
    SRCS ...
    INCLUDE_DIRS "." "libloragw" "libtools" "libloragw-test" "packet_forwarder"
    ...
)
```

---

## 验证

修复后 `idf.py build` 通过，`global_json.h` 以及所有子目录头文件均可正常索引。
