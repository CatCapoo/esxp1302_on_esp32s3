# impl/ — 工程搭建与代码实现

| 文档 | 说明 |
|------|------|
| [01_cmake_setup.md](01_cmake_setup.md) | CubeMX + CMake + Ninja + VS Code 工程搭建全流程 |
| [02_platform_adapt.md](02_platform_adapt.md) | ESP32 → STM32 所有适配改动速查表 |
| [03_driver_layers.md](03_driver_layers.md) | 驱动层源文件清单、依赖关系、编译修复记录 |
| [04_w5500_ethernet.md](04_w5500_ethernet.md) | W5500 以太网驱动集成：引脚适配、ioLibrary 架构、pkt_fwd 移植准备 |
| [05_cross_platform_config.md](05_cross_platform_config.md) | 跨平台配置：ESP32 / STM32 分支管理 |
| [06_freq_plan_flash_config_v4.md](06_freq_plan_flash_config_v4.md) | CN470 频率计划 v4、Flash 扇区配置存储 |
| [07_sntp_implementation.md](07_sntp_implementation.md) | SNTP 实现剖析：软件时钟、后台任务、Newlib 桥接 |
| [07_sntp_configuration.md](07_sntp_configuration.md) | SNTP 配置速查 |
| [08_oled_display_bug.md](08_oled_display_bug.md) | OLED 第 6 行永久空白 Bug 分析与修复 |
| [09_ccmram_optimization.md](09_ccmram_optimization.md) | CCMRAM 优化：大型静态变量迁移，SRAM 95% → 74% |
| [10_config_cleanup.md](10_config_cleanup.md) | 配置整理：global_conf.json 构建管道、gateway_defaults.h 清理、版本号修复 |
| [11_pin_remap_spi1_i2c1.md](11_pin_remap_spi1_i2c1.md) | 引脚重映射：SX1302 切换至 SPI1，OLED/LM75A 切换至 I2C1 |
| [12_oled_boot_screen.md](12_oled_boot_screen.md) | OLED 启动画面：上电即显示加载进度，消除黑屏；完整显示布局与刷新逻辑 |

## 阅读顺序

1. **01** — 先搭好工程框架，确保空项目能编译烧录
2. **02** — 了解所有平台差异点，逐一改动
3. **03** — 理清文件层级，解决编译问题
