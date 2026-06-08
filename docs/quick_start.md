# L4 Linux SDK 快速开发指南

本文档用于帮助开发者用最短路径把 L4 Linux SDK 跑起来。完成本文档后，应能在 Linux 环境中识别 L4-BOX、编译 SDK、启动 `l4_daemon`，并通过 `l4_linux_mvi -v` 读取设备版本信息。

如果需要完整 API、配对、链路监控、链路配置和工具说明，请继续阅读 `docs/linux_development_manual.md` 和 `docs/api.md`。

## 1. 快速认识 SDK

Linux SDK 的最小运行链路如下：

```text
客户程序 / l4_linux_mvi
  -> libar8030_client.so
  -> l4_daemon
  -> USB / UART backend
  -> L4-BOX
```

最快验证只需要关注以下文件：

| 文件 | 说明 |
|-|-|
| `script/cmk-local.sh` | x86_64 本机编译脚本。 |
| `script/cmk-arm.sh` | arm64 交叉编译脚本。 |
| `install/<arch>/bin/l4_daemon` | 后台通信服务。 |
| `install/<arch>/bin/l4_linux_mvi` | 最小验证程序。 |
| `install/<arch>/lib/libar8030_client.so` | 客户端动态库。 |
| `com/bb_api.h` | SDK 核心 API 头文件。 |

`<arch>` 根据运行平台选择：

| 运行平台 | 架构目录 |
|-|-|
| Ubuntu PC / Ubuntu 虚拟机 | `x86_64` |
| arm64 Linux 开发板 | `arm64` |

## 2. 准备环境和设备

### 2.1 基础环境

Linux 环境需要准备：

- CMake。
- GCC / G++ 编译环境。
- Shell 执行环境。
- 可访问 L4-BOX 的 USB 设备权限，必要时使用 `sudo`。

Ubuntu 上可先确认工具是否存在：

```sh
cmake --version
gcc --version
g++ --version
```

### 2.2 连接 L4-BOX

1. 使用 USB 转 Type-C 数据线连接 Linux 主机或 Linux 开发板与 L4-BOX。
2. 确认 L4-BOX 已上电。
3. 如果使用 Windows 主机中的 Ubuntu 虚拟机，需要在虚拟机菜单中把 USB 设备切换给 Ubuntu。
4. 在 Linux 终端执行：

```sh
lsusb
```

输出中应能看到 `Artosyn`、`Linux Foundation`、`HS Mode` 或类似设备信息。常见设备显示类似：

```text
ID 1d6d:8030 Artosyn in HS Mode
```

如果 `lsusb` 看不到设备，先检查 USB 线、设备供电、虚拟机 USB 直通、开发板 USB Host 模式和设备权限。

## 3. x86_64 本机最快验证

本流程适用于 Ubuntu PC 或 Ubuntu 虚拟机。目标是先把 SDK 在本机编译并完成最小版本读取。

### 3.1 编译 SDK

进入 SDK 根目录：

```sh
cd L4_Linux_SDK
```

如果之前编译过，可先清理历史产物：

```sh
./script/clean.sh all
```

执行 x86_64 本地编译：

```sh
./script/cmk-local.sh
```

编译完成后，确认产物目录存在：

```sh
ls install/x86_64/bin
ls install/x86_64/lib
```

关键产物应包含：

```text
install/x86_64/bin/l4_daemon
install/x86_64/bin/l4_linux_mvi
install/x86_64/lib/libar8030_client.so
```

### 3.2 启动 daemon

打开第一个终端：

```sh
cd L4_Linux_SDK/install/x86_64/bin
sudo ./l4_daemon
```

保持该终端不要关闭。`l4_daemon` 负责访问底层 L4-BOX 设备，后续客户端程序会连接它。

### 3.3 读取设备版本

打开第二个终端：

```sh
cd L4_Linux_SDK/install/x86_64/bin
export LD_LIBRARY_PATH=../lib:$LD_LIBRARY_PATH
./l4_linux_mvi -v
```

如果输出中包含以下字段，说明 x86_64 最小链路已经跑通：

```text
uptime       : ...
compile_time : ...
soft_ver     : ...
hardware_ver : ...
firmware_ver : ...
```

## 4. arm64 开发板最快验证

本流程适用于 arm64 Linux 开发板。PC 上编译出的 `x86_64` 程序不能直接在 arm64 开发板运行，需要先交叉编译 arm64 产物，再部署到开发板。

### 4.1 准备交叉编译工具链

当前 SDK 的 `compiler.arm.cmake` 默认期望工具链位于：

```text
L4_Linux_SDK/toolchain/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu
```

其中编译器前缀为：

```text
bin/aarch64-none-linux-gnu-
```

如果工具链放在其他目录，需要先修改 `compiler.arm.cmake` 中的 `GCC_PATH`。

### 4.2 编译 arm64 产物

在 PC 侧进入 SDK 根目录：

```sh
cd L4_Linux_SDK
./script/cmk-arm.sh
```

编译完成后，关键产物应包含：

```text
install/arm64/bin/l4_daemon
install/arm64/bin/l4_linux_mvi
install/arm64/lib/libar8030_client.so
```

### 4.3 部署到开发板

在开发板上准备目录：

```sh
mkdir -p ~/l4box/bin ~/l4box/lib
```

将 PC 侧文件复制到开发板：

| PC 侧文件 | 开发板目标目录 |
|-|-|
| `install/arm64/bin/l4_daemon` | `~/l4box/bin/` |
| `install/arm64/bin/l4_linux_mvi` | `~/l4box/bin/` |
| `install/arm64/lib/libar8030_client.so` | `~/l4box/lib/` |

可使用 `scp`、U 盘、共享目录或项目已有部署方式传输。

### 4.4 在开发板运行

先确认开发板能识别 L4-BOX：

```sh
lsusb
```

打开开发板第一个终端：

```sh
cd ~/l4box/bin
chmod +x l4_daemon l4_linux_mvi
sudo ./l4_daemon
```

打开开发板第二个终端：

```sh
cd ~/l4box/bin
export LD_LIBRARY_PATH=../lib:$LD_LIBRARY_PATH
./l4_linux_mvi -v
```

如果能返回 `uptime`、`compile_time`、`soft_ver`、`hardware_ver`、`firmware_ver`，说明 arm64 开发板链路验证通过。

如需后台启动 daemon，可使用：

```sh
cd ~/l4box/bin
sudo ./l4_daemon > /dev/null 2>&1 &
```

## 5. 从最小验证到业务开发

最小验证通过后，业务程序通常按以下顺序接入：

```text
bb_host_connect("127.0.0.1", BB_PORT_DEFAULT)
  -> bb_dev_getlist(...)
  -> bb_dev_open(...)
  -> bb_ioctl(..., BB_GET_SYS_INFO, ...)
  -> bb_dev_close(...)
  -> bb_dev_freelist(...)
  -> bb_host_disconnect(...)
```

建议从 `app/l4_linux_mvi/l4_linux_mvi.c` 开始阅读最小调用链，再根据需求参考 `examples/` 下的示例：

| 示例 | 用途 |
|-|-|
| `examples/01_basic_info` | 读取基础信息。 |
| `examples/02_pair_manager` | 配对管理。 |
| `examples/03_link_monitor` | 链路状态、信号质量、MCS、功率、吞吐查询。 |
| `examples/04_link_config` | 频段、信道、频宽、MCS 等链路配置。 |

## 6. 常见问题速查

| 现象 | 优先检查 |
|-|-|
| `lsusb` 看不到设备 | L4-BOX 是否上电，USB 数据线是否正常，虚拟机是否已接管 USB，开发板是否处于 USB Host 模式。 |
| `l4_daemon` 启动失败 | 是否使用 `sudo`，文件是否有执行权限，设备是否被其他程序占用。 |
| `l4_linux_mvi` 提示找不到 `.so` | 是否执行 `export LD_LIBRARY_PATH=../lib:$LD_LIBRARY_PATH`，`libar8030_client.so` 是否与程序架构一致。 |
| 连接 daemon 失败 | `l4_daemon` 是否仍在运行，端口是否被占用，客户端是否连接默认地址 `127.0.0.1` 和默认端口。 |
| 提示没有设备 | `lsusb` 是否能看到 L4-BOX，`l4_daemon` 是否有设备访问权限。 |
| arm64 程序无法运行 | 是否误用了 `install/x86_64` 产物，开发板上应使用 `install/arm64` 产物。 |

## 7. 最小验收标准

完成快速接入后，至少应满足：

- Linux 系统可以通过 `lsusb` 识别 L4-BOX。
- `./script/cmk-local.sh` 或 `./script/cmk-arm.sh` 能生成目标架构产物。
- `l4_daemon` 可以启动并保持运行。
- `l4_linux_mvi -v` 可以读取版本信息。
- 客户程序可以加载同架构的 `libar8030_client.so`。
