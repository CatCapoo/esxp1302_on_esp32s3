# impl/ — 工程搭建与代码实现

| 文档 | 说明 |
|------|------|
| [01_cmake_setup.md](01_cmake_setup.md) | CubeMX + CMake + Ninja + VS Code 工程搭建全流程 |
| [02_platform_adapt.md](02_platform_adapt.md) | ESP32 → STM32 所有适配改动速查表 |
| [03_driver_layers.md](03_driver_layers.md) | 驱动层源文件清单、依赖关系、编译修复记录 |

## 阅读顺序

1. **01** — 先搭好工程框架，确保空项目能编译烧录
2. **02** — 了解所有平台差异点，逐一改动
3. **03** — 理清文件层级，解决编译问题
