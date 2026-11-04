# Changelog

本文件记录 L4 Linux SDK 的版本变更。当前 SDK 版本以根目录 [`VERSION`](VERSION) 为唯一依据；本文件只记录各版本的功能和兼容性变化。

## [Unreleased]

### Changed

- 调整 `l4_link_monitor` 的 DEV 吞吐查询：未指定 `-s` 时通过 `BB_GET_DEV_CONNECT_SLOT` 自动获取连接 slot，显式指定 `-s` 时仍使用给定值；本端和远端查询行为保持一致。

### Fixed

- 修复 `l4_basic_info` 和 `l4_link_monitor` 在 DEV 角色下固定使用 `BB_USER_0` 打印 TX 物理状态的问题：查询 `BB_GET_STATUS` 时先通过 `BB_GET_DEV_CONNECT_SLOT` 获取本机连接 slot，再读取同编号 user 的 TX 状态；本端和远端查询行为保持一致，并增加无效 slot 检查。

## [v1.1.1] - 2026-10-23

### Added

- 为 `l4_basic_info` 增加本端及远端设备重启原因查询，输出重启类型和相关诊断信息。

### Changed

- 将 `l4_socket_transfer` 的默认 Socket buffer 调整为 RX 4000 字节、TX 6000 字节，并增加 `--rx-buf`、`--tx-buf` 参数用于自定义大小。
- **不兼容变更：** 统一示例程序的远程调用参数语义：`-R [slot]` 指定外层远程路由，`-s <slot>` 表示命令执行端内部目标 slot；同步移除旧的重复远程 slot 参数，并加强 slot 范围、重复参数和非法参数校验。

## [v1.1.0] - 2026-09-08

### Added

- 新增 `l4_socket_encrypt` 示例，支持查询、启用、更新和关闭本端运行态 Socket 加密配置，并支持通过 `-R [slot]` 对远端执行相同操作。
- 新增 Socket 加密持久化能力，通过 `--minidb` 查询、写入或关闭本端及远端 MiniDB 配置；持久配置在设备重启或相关 Socket 重建后生效。
- 同步 SDK `1.3.02-30` 的 `BB_GET_SOCKET_ENCRYPT` 查询接口、持久加密项目 RPC 命令及参数结构。
- 为 `l4_socket_transfer` 增加 `-e`/`--encrypt-mode` 和 `-k`/`--encrypt-key`，支持 default、AES128 和 AES256 加密 Socket 收发。
- 为 `l4_tuntap` 增加 default、AES128 和 AES256 加密 Socket，并在设备断线重连时复用启动时的加密参数。
- 新增《L4 通信加密应用手册》，补充 TUN/TAP、SBUS 和 MAVLink 的本端/远端配置、持久化、验收及故障排查流程。

### Changed

- **不兼容变更：** `l4_tuntap` 移除 `-k`/`--force-close-on-open-fail` 开关，`-k` 改为加密密钥参数；Socket 打开失败时默认清理同 slot/port 的残留 Socket 并重试一次。
- 统一使用 `BB_SOCK_ENCRYPT_KEY_LEN` 定义 Socket 加密密钥长度，并适配现有调用点。
- 将 `prj_rpc.h` 纳入 SDK 公开头文件安装，并将 `l4_socket_encrypt` 加入本机和 ARM64 构建安装流程。

### Fixed

- 完善 Socket 端口范围、加密模式、密钥长度、参数组合及持久配置未设置状态的校验与错误提示。
- 在加密配置请求完成后清零临时密钥缓存，避免密钥在进程内存中无必要保留。

### Notes

- 运行态配置与 MiniDB 持久配置相互独立；如需立即生效并在重启后保留，需要分别配置两者。
- 加密通信两端必须使用相同的 Socket port、加密模式和密钥。

## [v1.0.0] - 2026-08-19

### Added

- 为 `l4_basic_info` 增加 `-l` 设备列表查询选项，支持连接 daemon 后列出设备并按 index 选择。
- 为 `l4_tuntap` 增加 `-k`/`--force-close-on-open-fail`，Socket 打开失败时可强制清理残留 Socket 后重试。
- 新增 `l4_socket_transfer` Socket 收发示例，支持文本、十六进制数据发送和持续接收。
- 重新引入 `l4_cmd_dbg` 命令行调试工具，支持按设备序号或 MAC 选择设备、列出设备及纯输出模式。
- 同步原厂 `1.3.02-28-release-20260708` 的扩展功率和频点功率限制等控制接口。
- 为 `l4_link_monitor` 增加 MCS 模式和频宽模式的本机、远端及全量查询，并输出手动或自动控制状态。
- 为 `l4_minidb_config` 增加扩展功率配置查询；设置固定功率时同步写入扩展功率配置。
- 新增 `l4_project_event` 跨端自定义项目事件例程及实现原理文档，并加入本机和 ARM64 构建目标。
- 新增 `use_cases/l4_upgrade.sh`，可自动启动 `l4_daemon`，并串联固件升级、恢复出厂设置和设备重启流程。
- 为 `l4_config_file` 增加 `-H` 设备重启功能，并支持在写入或恢复配置成功后重启。

### Changed

- 重新划分 README 与开发手册职责：README 保留快速编译、验证和阅读入口，高级构建及完整工具说明统一收敛到开发手册。
- **不兼容变更：** 公共 Socket、信道和 netdev 结构尺寸已变化，应用需要与客户端库和 daemon 整体清理重编，且外部 Linux 内核驱动必须使用匹配版本。
- 更新本机与 ARM64 构建脚本，使新增工具和例程纳入多架构构建、安装流程。

### Fixed

- 修复 `l4_tuntap` 在设备断开后无法恢复的问题：按设备 MAC 重新获取 working ID、恢复 Socket 收发，并增加并发保护、解析状态重置和逐次重连日志。

### Notes

- 加密相关代码仍处于开发和验证阶段，不作为 `v1.0.0` 的正式发布能力。

## [v0.4.0] - 2026-06-24

### Added

- 为 `l4_basic_info` 增加远端设备基础信息查询能力。
- 为 `l4_link_config` 增加远端链路配置和发射功率设置能力。
- 为 `l4_link_monitor` 增加远端链路状态查询能力，并完善质量与功率查询。
- 为 `l4_minidb_config` 增加频点列表和 UART 波特率配置能力。
- 新增 `l4_uart_config` 示例，支持 UART 配置读取和写入。
- 为配置类示例增加远程 `ioctl` 支持。

### Changed

- 将 MiniDB 示例目录规范为 `examples/06_minidb_config`。
- 同步新原厂 SDK 的 `bb_api.h`、`bb_config.h`、`ioctl_tab.c` 和 `prj_rpc.h`，新增恢复模式、纯净模式消息检查、连接 slot、扩展功率、RF 校准、UART 配置和频点功率偏移等命令定义。
- 同步 `bb_net_dev_*` API，netdev 创建、销毁和缓冲区调整接口改为接收 `bb_dev_handle_t *`。
- 调整链路监控质量与功率查询的默认用户，并同步更新示例文档。
- 完善 Linux 本机与 ARM64 交叉编译、安装目标和开发文档。

### Fixed

- 修复 Linux socket 连接超时处理和 USB/UART 后端控制结构初始化问题。
- 修复 `ar_net_api.h` 安装路径，并调整 ARM64 脚本目标列表，保证全量安装阶段可找到所有 install 目标。

## [v0.3.0] - 2026-06-13

### Added

- 新增 `l4_config_file` 示例，支持配置文件导入、导出和恢复。
- 新增 `l4_minidb_config` 示例，支持 MiniDB 持久化配置读取和写入。
- 扩展 `l4_link_monitor` 的链路状态和统计信息查询能力。

### Changed

- 合并快速指南与 Linux SDK 开发手册，补充配置文件示例、构建和使用说明。
- 精简 SDK 文档结构，移除独立的 `docs/api.md` 和 `docs/quick_start.md`。
- 同步构建与清理脚本，移除已删除目标的引用。

### Removed

- 移除 `l4_cmd_dbg` 工具及其 CMake 构建配置。
- 移除 `l4_linux_mvi` 最小验证工具。

## [v0.2.0] - 2026-06-09

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
