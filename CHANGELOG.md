# Changelog

本文件记录 L4 Linux SDK 的版本变更，内容根据 Git tag 和对应提交历史整理。

## [v0.2.0] - 2026-06-09

Git tag: `v0.2.0`

### Added

- 新增 `docs/quick_start.md` 快速开发指南。
- 新增 `docs/linux_development_manual.md` Linux SDK 开发手册。
- 新增并完善 `docs/api.md`，整理 SDK API、命令字、结构体和数据通道说明。
- 新增 `l4_tuntap` 应用程序及 `libtuntap` 第三方依赖补丁。
- 新增 `l4_ota_upgrade` 固件升级工具及 CRC、镜像校验相关实现。
- 新增 `l4_cmd_dbg` 命令行调试工具及构建配置。
- 新增示例程序：`l4_basic_info`、`l4_pair_manager`、`l4_link_monitor`、`l4_link_config`。
- 新增示例公共模块和各示例 README 文档。

### Changed

- 将 daemon 可执行程序命名调整为 `l4_daemon`，并同步更新构建脚本。
- 将 OTA 升级工具可执行程序命名调整为 `l4_ota_upgrade`，并同步更新构建配置。
- 增强命令行参数处理和设备序号选择逻辑。
- 增强 `l4_basic_info`、`l4_link_monitor`、`l4_link_config` 的 user phy 状态、频段、链路配置等输出。
- 优化 `l4_pair_manager` 配对流程，移除等待时间参数。
- 优化 `l4_tuntap` 参数配置显示。
- 更新 `README.md`、清理脚本和本机/交叉编译脚本。

## [v0.1.0] - 2026-06-08

Git tag: `v0.1.0`

### Added

- 初始化 L4 Linux SDK 项目基础框架。
- 新增 CMake 构建入口、x86_64 本机构建脚本、arm64 交叉编译脚本和清理脚本。
- 新增 `com/` 公共库、核心 API 头文件和 RPC/日志/缓冲区等基础模块。
- 新增 `libar8030_client` 客户端库相关源码。
- 新增 `l4_daemon` 基础通信服务框架，支持 USB 和 UART 后端。
- 新增 `l4_linux_mvi` 最小验证工具。
- 新增 ARM64 toolchain 配置和 libusb 第三方包。
- 新增根目录 `README.md`，说明 SDK 目录结构、构建方式和产物位置。

### Notes

- 该版本是第一次发布。
