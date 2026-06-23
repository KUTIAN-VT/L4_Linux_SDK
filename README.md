# L4 Linux SDK

`L4_Linux_SDK` 是面向 Linux 的精简 SDK，用于构建和使用 8030/L4 设备的后台服务、客户端库、应用工具和 API 示例。

本仓库当前维护的是整理后的 Linux SDK 内容。`build/` 和 `install/` 是构建、安装输出，不应作为源码维护。

## 运行链路

SDK 的最小运行链路如下：

```text
+-----------------------------+
| 用户程序 / SDK 示例程序      |
| l4_basic_info 等            |
+--------------+--------------+
               |
               | 调用 SDK API
               v
+-----------------------------+
| libar8030_client.so         |
| 封装客户端接口和 RPC 通信    |
+--------------+--------------+
               |
               | 连接本机或远端 daemon
               v
+-----------------------------+
| l4_daemon                   |
| 设备访问、消息转发、热插拔   |
+--------------+--------------+
               |
               | USB / UART
               v
+-----------------------------+
| 8030 / L4 设备              |
+-----------------------------+
```

通常先启动 `l4_daemon`，再运行示例程序或用户应用。应用侧通过 `libar8030_client.so` 访问 daemon，由 daemon 负责和设备通信。

## 目录结构

```text
L4_Linux_SDK/
  app/              应用侧客户端库和工具
  com/              公共头文件、RPC、日志、缓冲区等基础代码
  daemon/           后台服务和 8030 设备后端
  examples/         API 示例程序
  docs/             开发手册
  script/           编译和清理脚本
  third_package/    第三方依赖源码包或补丁
  toolchain/        ARM64 交叉编译工具链目录
  build/            CMake 构建缓存，生成物
  install/          安装输出，生成物
```

构建和安装输出按架构分目录保存：

```text
build/x86_64/
build/arm64/
install/x86_64/
install/arm64/
```

## 主要产物

| 类型 | 产物 | 说明 |
| --- | --- | --- |
| 客户端库 | `libar8030_client.so` | 用户程序链接的 SDK 动态库 |
| 后台服务 | `l4_daemon` | 访问设备并为客户端程序提供 RPC 服务 |
| 应用工具 | `l4_ota_upgrade` | 固件升级工具 |
| 应用工具 | `l4_tuntap` | 网络透传工具 |
| 示例程序 | `l4_basic_info` | 查询设备基础信息和版本信息 |
| 示例程序 | `l4_pair_manager` | 图传对频和配对管理 |
| 示例程序 | `l4_link_monitor` | 链路状态、质量、吞吐、测距查询 |
| 示例程序 | `l4_link_config` | 链路频段、信道、频宽、MCS 配置 |
| 示例程序 | `l4_config_file` | 配置文件导入、导出和恢复 |
| 示例程序 | `l4_minidb_config` | MiniDB 持久化配置读写 |

`examples/00_common` 是示例公共库，封装连接 daemon、枚举设备和打开设备的公共流程，不是可单独运行的示例程序。

## 本机 x86_64 编译

在 Ubuntu PC 或 Ubuntu 虚拟机上执行：

```bash
cd L4_Linux_SDK
./script/cmk-local.sh
```

脚本会配置 `build/x86_64`，构建本机核心库、daemon、应用工具和全部示例，并安装到：

```text
install/x86_64/
```

常用产物位置：

```text
install/x86_64/bin/
install/x86_64/lib/
install/x86_64/include/
```

## ARM64 交叉编译

在 PC 上为 arm64 Linux 开发板交叉编译：

```bash
cd L4_Linux_SDK
./script/cmk-arm.sh
```

脚本会配置 `build/arm64`，并构建安装当前 SDK 的核心库、daemon、应用工具和示例程序，确保后续 `cmake --install` 能生成完整 `install/arm64/` 目录。

安装输出位于：

```text
install/arm64/
```

ARM64 构建使用 `compiler.arm.cmake`，默认期望工具链位于：

```text
toolchain/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu
```

编译器前缀为：

```text
bin/aarch64-none-linux-gnu-
```

如果工具链放在其他目录，需要修改 `compiler.arm.cmake` 中的 `GCC_PATH`。

## 快速验证

### x86_64 本机验证

连接 8030/L4 设备后，可先确认 USB 设备是否可见：

```bash
lsusb
```

启动 daemon：

```bash
cd L4_Linux_SDK/install/x86_64/bin
sudo ./l4_daemon
```

另开一个终端查询设备版本信息：

```bash
cd L4_Linux_SDK/install/x86_64/bin
./l4_basic_info -V
```

如果程序提示连接 daemon 失败，先确认 `l4_daemon` 是否正在运行。如果提示没有设备，先确认设备连接、USB 权限和 daemon 日志。

### arm64 开发板验证

先在 PC 侧执行 ARM64 交叉编译，然后将 `install/arm64/` 中的 `bin/`、`lib/`、`include/` 按项目部署方式拷贝到开发板。

在开发板上启动 daemon：

```bash
cd <deploy-dir>/bin
sudo ./l4_daemon
```

ARM64 脚本会构建并安装示例程序，可根据实际设备状态运行：

```bash
./l4_minidb_config -A
```

如果需要在 ARM64 上运行更多示例，可在 `script/cmk-arm.sh` 中增加对应 CMake target 后重新编译。

## 清理构建产物

清理全部架构的构建和安装输出：

```bash
cd L4_Linux_SDK
./script/clean.sh
```

或：

```bash
./script/clean.sh all
```

只清理 x86_64：

```bash
./script/clean.sh x86_64
```

`x86` 也可作为 `x86_64` 的别名。

只清理 arm64：

```bash
./script/clean.sh arm64
```

`arm` 也可作为 `arm64` 的别名。

## 手动 CMake 构建

通常优先使用 `script/` 下的脚本。需要手动控制 CMake 参数时，可参考下面命令。

### x86_64

```bash
cmake -S L4_Linux_SDK -B L4_Linux_SDK/build/x86_64 \
  -DCMAKE_INSTALL_PREFIX=L4_Linux_SDK/install/x86_64 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DAPP_STATIC_LIB=OFF \
  -DUSING_8030USB=ON \
  -DUSING_8030UART=ON \
  -DUSING_8030SDIO=OFF \
  -DUSING_XDS_HDR=ON

cmake --build L4_Linux_SDK/build/x86_64 --target \
  ar8030_client \
  l4_tuntap \
  l4_ota_upgrade \
  l4_basic_info \
  l4_pair_manager \
  l4_link_monitor \
  l4_link_config \
  l4_config_file \
  l4_minidb_config \
  l4_daemon \
  -j

cmake --install L4_Linux_SDK/build/x86_64
```

### arm64

```bash
cmake -S L4_Linux_SDK -B L4_Linux_SDK/build/arm64 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX=L4_Linux_SDK/install/arm64 \
  -DCMAKE_TOOLCHAIN_FILE=L4_Linux_SDK/compiler.arm.cmake \
  -DAPP_STATIC_LIB=OFF \
  -DUSING_8030USB=ON \
  -DUSING_8030UART=ON \
  -DUSING_8030SDIO=OFF \
  -DUSING_XDS_HDR=ON

cmake --build L4_Linux_SDK/build/arm64 --target \
  ar8030_client \
  l4_ota_upgrade \
  l4_minidb_config \
  l4_daemon \
  -j

cmake --install L4_Linux_SDK/build/arm64
```

## 下一步阅读

- 第一次使用 SDK：阅读 `docs/L4 Linux SDK 开发手册.md` 的“SDK 编译使用”和“最快验证”章节。
- 开发自己的应用：先看 `examples/01_basic_info` 和 `examples/00_common`，理解连接 daemon、枚举设备、打开设备和调用 `bb_ioctl()` 的最小流程。
- 查 API 结构体和命令字：查看 `com/bb_api.h`、`com/prj_rpc.h` 和 `app/ar8030/ar_net_api.h`，并结合 `examples/` 中对应示例理解调用方式。
- 使用应用工具：阅读开发手册中的“应用工具”章节。
- 调试链路和配置：阅读 `examples/03_link_monitor`、`examples/04_link_config`、`examples/05_config_file`、`examples/06_minidb_config` 下的 README。

## 注意事项

- USB 后端默认开启，依赖 `third_package/libusb/libusb-cmake.tar`。
- UART 后端默认开启。
- SDIO 后端默认关闭。
- `build/`、`install/`、临时文件和历史构建产物不属于源码维护范围。
- 当前 SDK 面向 Linux，CMake 会拒绝 Windows 和 Android 构建。
- 当前 SDK 不包含原厂 `driver/linux/` 内核模块源码；如需交付 `artosyn_drv.ko`，应作为独立驱动迁移和内核版本适配任务处理。
- `bb_net_dev_create()`、`bb_net_dev_destroy()`、`bb_net_dev_buf_resize()` 使用 `bb_dev_handle_t *` 参数，SDK 内部负责打开和关闭 netdev 设备。
