# BUG-009：网关 Web 配置页面返回 HTTP 431 Request Header Fields Too Large

- **日期**：2026-02-20  
- **文件**：`sdkconfig`、`sdkconfig.defaults`  
- **严重级别**：功能性错误（Web 配置界面无法访问）

---

## 现象

在浏览器中访问网关 Web 配置页面（`http://<设备IP>/`）时，服务器返回：

```
431 Request Header Fields Too Large
```

现代浏览器发送的 HTTP 请求头（包含 Host、User-Agent、Accept、Authorization 等）总长度通常超过 512 字节。

---

## 根本原因

ESP-IDF `esp_http_server` 组件的最大请求头长度由 Kconfig 选项 `CONFIG_HTTPD_MAX_REQ_HDR_LEN` 控制，默认值为 **512 字节**。

`httpd_config_t` 结构体**没有** `max_req_hdr_len` 运行时字段，该限制只能通过 Kconfig 在编译时配置。

---

## 错误尝试

尝试在 `http_server.c` 中通过结构体字段覆盖：

```c
httpd_config_t config = HTTPD_DEFAULT_CONFIG();
config.max_req_hdr_len = 2048;   // ← 编译报错：struct 无此成员
```

该字段不存在，此方案不可行。

---

## 修复

在 `sdkconfig.defaults` 中增加：

```
CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048
```

同时在 `sdkconfig` 中将对应项从 512 改为 2048，重新编译烧录后 Web 配置页面可正常访问。

---

## 验证

修复后浏览器访问 `http://<设备IP>/`，正常显示登录框及配置页面，HTTP Basic Auth（`iot` / `lora`）通过，配置保存功能正常。
