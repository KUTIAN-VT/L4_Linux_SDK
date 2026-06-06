# L4 Linux SDK 开发手册

本文档面向 Linux SDK 使用者，说明常用应用工具和 API 示例程序的使用方法。阅读本文档前，建议先完成 SDK 编译，并确认 `l4_daemon` 可以正常访问 8030 设备。

## 使用前准备

### 编译 SDK

进入 SDK 目录：

```sh
cd L4_Linux_SDK
```

本机 x86_64 编译：

```sh
./script/cmk-local.sh
```

ARM64 交叉编译：

```sh
./script/cmk-arm.sh
```

编译和安装产物按架构输出到：

```text
L4_Linux_SDK/install/x86_64/
L4_Linux_SDK/install/arm64/
```

常用可执行程序位于对应架构的 `bin/` 目录，动态库位于 `lib/` 目录。

### 启动 daemon

SDK 应用程序通常先连接 `l4_daemon`，再通过 daemon 访问 8030 设备。运行工具或示例前，先在设备连接正常的环境中启动 daemon：

```sh
cd L4_Linux_SDK/install/x86_64/bin
./l4_daemon
```

如果是 ARM64 设备，请进入 `install/arm64/bin` 后执行同样命令。

### 运行工具

以下命令示例默认在 `install/<arch>/bin` 目录执行：

```sh
cd L4_Linux_SDK/install/x86_64/bin
```

如需在其它目录运行，请确保能正确找到 `libar8030_client.so` 等 SDK 动态库。

## 一、应用工具

### 1、固件升级工具

`l4_ota_upgrade` 用于对 8030 设备执行固件升级。当前手册先只介绍通过 `-f` 指定升级镜像的基础用法。

#### 1.1 程序参数说明


| 参数                  | 说明             | 默认值                          |
| ------------------- | -------------- | ---------------------------- |
| `-f, --file <file>` | 指定待升级的固件镜像文件路径 | `artosyn-upgrade-ar8030.img` |
| `-h, --help`        | 打印帮助信息         | 无                            |


`-f` 后面应填写 `.img` 固件文件路径。路径可以是相对路径，也可以是绝对路径。

#### 1.2 标准使用方法

1. 确认 `l4_daemon` 已启动。
2. 确认 8030 设备连接正常。
3. 准备待升级固件镜像，例如 `VT4-KT-2458-G-V1.1.8-U.img`。
4. 执行 `l4_ota_upgrade -f <固件镜像路径>`。
5. 等待升级进度到 100%，程序返回成功后再进行断电、重启或后续验证。

升级过程中不要断开设备连接，也不要关闭 daemon。

#### 1.3 示例

使用相对路径升级：

```sh
./l4_ota_upgrade -f ../../firmware/KT-2458-G-V1.1.8-U/VT4-KT-2458-G-V1.1.8-U.img
```

使用绝对路径升级：

```sh
./l4_ota_upgrade -f /opt/firmware/VT4-KT-2458-G-V1.1.8-U.img
```

查看帮助：

```sh
./l4_ota_upgrade -h
```

### 2、网络透传工具

`l4_tuntap` 用于通过 TAP 设备演示基带数据透传能力。它会把 Linux 网络侧数据封装后写入基带 socket，也会把基带收到的数据还原后写入 TAP 设备。

#### 2.1 程序参数说明


| 参数                   | 说明                  | 默认值     |
| -------------------- | ------------------- | ------- |
| `-u, --user <user>`  | 指定 baseband user id | `0`     |
| `-p, --port <port>`  | 指定透传 transport id   | `3`     |
| `-i, --ip <ip>`      | 指定 TAP 设备 IP，必填     | 无       |
| `-d, --dev <dev>`    | 指定 TAP 设备名          | `tap0`  |
| `-v, --debug`        | 打开网络透传调试打印          | 关闭      |
| `-r, --rx_buf <len>` | 指定 RX buffer 长度     | `40000` |
| `-t, --tx_buf <len>` | 指定 TX buffer 长度     | `60000` |
| `-h, --help`         | 打印帮助信息              | 无       |


`-i` 是网络透传工具的关键参数，用于给 TAP 虚拟网卡配置 IP。两端设备应配置在同一网段，并避免 IP 冲突。

#### 2.2 标准使用方法

1. AP 和 DEV 侧均启动 `l4_daemon`。
2. 确认 AP 和 DEV 已完成配对并建立链路。
3. AP 侧和 DEV 侧分别运行 `l4_tuntap`，并配置不同的 TAP IP。
4. 使用 `ping`、`iperf` 或业务程序通过 TAP IP 验证网络透传。

如果 TAP 设备创建或配置失败，通常需要使用 root 权限运行。

#### 2.3 示例

AP 侧配置 TAP IP：

```sh
sudo ./l4_tuntap -i 192.168.55.1 -d tap0
```

DEV 侧配置 TAP IP：

```sh
sudo ./l4_tuntap -i 192.168.55.2 -d tap0
```

开启调试打印：

```sh
sudo ./l4_tuntap -i 192.168.55.1 -d tap0 -v
```

指定 user 和 transport id：

```sh
sudo ./l4_tuntap -u 0 -p 3 -i 192.168.55.1 -d tap0
```

链路建立后，从另一端测试连通性：

```sh
ping 192.168.55.1
```

### 3、图传调试工具

`l4_cmd_dbg` 是图传调试命令工具。它通过 daemon 连接 8030 设备的 debug 通道，打印内容与图传调试串口输出一致，也可以输入图传调试串口支持的命令来查看图传设备状态。

#### 3.1 程序参数说明


| 参数                  | 说明                        | 默认值               |
| ------------------- | ------------------------- | ----------------- |
| `-i, --ip <ip>`     | daemon IP 地址              | `127.0.0.1`       |
| `-p, --port <port>` | daemon 端口                 | `BB_PORT_DEFAULT` |
| `-m, --mac <mac>`   | 按 MAC 地址选择要调试的设备          | 默认打开第一个设备         |
| `-l, --list`        | 列出 daemon 当前枚举到的设备并退出     | 无                 |
| `-o, --output`      | 只输出设备 debug 打印，不从标准输入读取命令 | 关闭                |
| `-h, --help`        | 打印帮助信息                    | 无                 |


不加 `-o` 时，程序进入交互模式：用户在终端输入的每一行都会发送到设备 debug 通道，设备返回内容会直接打印到当前终端。该交互命令与图传调试串口命令保持一致。

#### 3.2 标准使用方法

1. 启动 `l4_daemon`。
2. 确认 8030 设备已被 daemon 枚举到。
3. 执行 `l4_cmd_dbg -l` 查看当前设备列表。
4. 执行 `l4_cmd_dbg` 进入交互调试模式。
5. 输入图传调试串口支持的状态查询命令，查看设备状态、链路状态或其它固件调试信息。
6. 如只需要观察设备 debug 打印，使用 `-o` 进入只输出模式。

#### 3.3 示例

列出当前设备：

```sh
./l4_cmd_dbg -l
```

进入默认设备的交互调试模式：

```sh
./l4_cmd_dbg
```

进入指定 MAC 设备的交互调试模式：

```sh
./l4_cmd_dbg -m 11:22:33:44
```

只查看设备 debug 打印，不输入命令：

```sh
./l4_cmd_dbg -o
```

连接指定 daemon 后进入交互调试模式：

```sh
./l4_cmd_dbg -i 192.168.1.10 -p 9000
```

在交互模式下输入图传调试串口支持的状态查询命令：

```text
<状态查询命令>
```

查看帮助信息：

```sh
./l4_cmd_dbg -h
```

## 二、API 调用示例

SDK 在 `examples/` 下提供 4 个常用 API 示例。它们都复用 `examples/00_common` 中的公共连接流程：

```text
bb_host_connect()
    |
bb_dev_getlist()
    |
bb_dev_open()
    |
bb_ioctl()
    |
bb_dev_close() / bb_dev_freelist() / bb_host_disconnect()
```

示例程序默认连接本机 daemon，并打开第 0 个设备。详细 API 结构体和命令字说明可参考 `docs/api.md`。

### 1、信息查询示例

`l4_basic_info` 用于查询设备基础信息，包括系统版本、角色、模式、MAC、slot 链路状态和 user phy 状态。

#### 1.1 程序参数说明


| 参数           | 说明                             | 默认值               |
| ------------ | ------------------------------ | ----------------- |
| `-S`         | 查询 `BB_GET_STATUS`，打印设备状态和链路状态 | 无                 |
| `-V`         | 查询 `BB_GET_SYS_INFO`，打印系统和版本信息 | 无                 |
| `-A`         | 查询全部基础信息                       | 默认动作              |
| `-a <addr>`  | daemon 地址                      | `127.0.0.1`       |
| `-p <port>`  | daemon 端口                      | `BB_PORT_DEFAULT` |
| `-i <index>` | 选择设备序号                         | `0`               |
| `-h`         | 打印帮助信息                         | 无                 |


如果不指定 `-S`、`-V` 或 `-A`，程序默认执行全量查询。

#### 1.2 标准使用方法

1. 启动 `l4_daemon`。
2. 执行 `l4_basic_info` 查询基础状态。
3. 如果 daemon 管理多个 8030 设备，通过 `-i` 指定目标设备。
4. 根据输出确认设备角色、工作模式、版本和链路状态。

#### 1.3 示例

查询全部基础信息：

```sh
./l4_basic_info
```

只查询系统版本：

```sh
./l4_basic_info -V
```

只查询设备状态：

```sh
./l4_basic_info -S
```

指定设备序号：

```sh
./l4_basic_info -i 1
```

连接指定 daemon：

```sh
./l4_basic_info -a 192.168.1.10 -p 9000
```

### 2、配对管理示例

`l4_pair_manager` 用于启动配对、停止配对、查询配对结果，以及手动设置对端 MAC。

#### 2.1 程序参数说明


| 参数           | 说明             | 默认值               |
| ------------ | -------------- | ----------------- |
| `-P`         | 启动配对，并等待配对结果事件 | 无                 |
| `-X`         | 停止当前配对         | 无                 |
| `-m`         | 查询配对结果和对端 MAC  | 无                 |
| `-M <mac>`   | 按当前角色设置对端 MAC  | 无                 |
| `-a <addr>`  | daemon 地址      | `127.0.0.1`       |
| `-p <port>`  | daemon 端口      | `BB_PORT_DEFAULT` |
| `-i <index>` | 选择设备序号         | `0`               |
| `-s <slot>`  | AP 侧开放的 slot   | `0`               |
| `-t <sec>`   | 设备侧配对超时时间      | `100`             |
| `-w <sec>`   | 示例程序等待回调的时间    | `timeout + 5`     |
| `-h`         | 打印帮助信息         | 无                 |


AP 侧启动配对时，`-s` 表示开放哪个 slot 给 DEV 接入。DEV 侧启动配对时通常不需要指定 slot。

#### 2.2 标准使用方法

1. AP 和 DEV 侧分别启动 `l4_daemon`。
2. AP 侧执行 `l4_pair_manager -P -s <slot>`，开放目标 slot。
3. DEV 侧执行 `l4_pair_manager -P`，进入配对流程。
4. 等待 `BB_EVENT_PAIR_RESULT` 事件返回。
5. 配对成功后执行 `l4_pair_manager -m` 查询对端 MAC。

配对是异步动作，程序会先订阅 `BB_EVENT_PAIR_RESULT`，再下发配对命令，并在等待时间内循环等待事件回调。

#### 2.3 示例

AP 侧开放 slot 0 配对：

```sh
./l4_pair_manager -P -s 0
```

DEV 侧启动配对：

```sh
./l4_pair_manager -P
```

查询配对结果：

```sh
./l4_pair_manager -m
```

AP 侧手动设置 slot 0 的 DEV MAC：

```sh
./l4_pair_manager -M 11:22:33:44 -s 0
```

DEV 侧手动设置 AP MAC：

```sh
./l4_pair_manager -M 11:22:33:44
```

停止配对：

```sh
./l4_pair_manager -X
```

指定配对超时和等待时间：

```sh
./l4_pair_manager -P -s 0 -t 60 -w 65
```

### 3、链路监控示例

`l4_link_monitor` 用于在配对完成后读取链路状态、信号质量、MCS、功率、信道、频段和吞吐信息。

#### 3.1 程序参数说明


| 参数           | 说明                         | 默认值               |
| ------------ | -------------------------- | ----------------- |
| `-A`         | 查询全部链路信息                   | 默认动作              |
| `-S`         | 查询 `BB_GET_STATUS`         | 无                 |
| `-Q`         | 查询 `BB_GET_USER_QUALITY`   | 无                 |
| `-q`         | 查询 `BB_GET_PEER_QUALITY`   | 无                 |
| `-M`         | 查询 `BB_GET_MCS`            | 无                 |
| `-P`         | 查询 `BB_GET_CUR_POWER`      | 无                 |
| `-C`         | 查询 `BB_GET_CHAN_INFO`      | 无                 |
| `-B`         | 查询 `BB_GET_BAND_INFO`      | 无                 |
| `-T`         | 查询 `BB_GET_THROUGHPUT`     | 无                 |
| `-a <addr>`  | daemon 地址                  | `127.0.0.1`       |
| `-p <port>`  | daemon 端口                  | `BB_PORT_DEFAULT` |
| `-i <index>` | 选择设备序号                     | `0`               |
| `-s <slot>`  | 目标 slot；DEV 侧 slot 0 表示 AP | `0`               |
| `-u <user>`  | 物理用户编号                     | `0`               |
| `-h`         | 打印帮助信息                     | 无                 |


链路质量、MCS、功率和吞吐等细节需要在配对完成、链路连接后读取。程序会先查询 `BB_GET_STATUS` 判断目标 slot 是否可用。

#### 3.2 标准使用方法

1. 启动 `l4_daemon`。
2. 使用 `l4_pair_manager` 完成 AP 和 DEV 配对。
3. 执行 `l4_link_monitor` 查询全量链路信息。
4. 根据需要通过 `-S/-Q/-M/-T` 等参数查询指定信息。
5. AP 侧通过 `-s` 指定目标 DEV 所在 slot；DEV 侧通常使用 `-s 0` 查看 AP 方向。

#### 3.3 示例

查询全部链路信息：

```sh
./l4_link_monitor
```

只查询 slot 0 的吞吐：

```sh
./l4_link_monitor -s 0 -T
```

只查询 slot 0 的 MCS：

```sh
./l4_link_monitor -s 0 -M
```

查询 user 0 的信号质量和当前发射功率：

```sh
./l4_link_monitor -u 0 -Q -P
```

查询当前频段信息：

```sh
./l4_link_monitor -B
```

查询信道信息：

```sh
./l4_link_monitor -C
```

### 4、链路设置示例

`l4_link_config` 用于设置图传链路的频段、信道、频宽、MCS 和帧结构。它通常与 `l4_link_monitor` 配套使用：先读取当前链路状态，再下发配置。

#### 4.1 程序参数说明


| 参数               | 说明                                               | 默认值                                           |
| ---------------- | ------------------------------------------------ | --------------------------------------------- |
| `-B <0           | 1>`                                              | 调用 `BB_SET_BAND_MODE`，`1` 自动，`0` 手动           |
| `-b <band>`      | 调用 `BB_SET_BAND`，支持 `1g`、`2g`、`5g` 或 `0`、`1`、`2` | 不执行                                           |
| `-C <0           | 1>`                                              | 调用 `BB_SET_CHAN_MODE`，`1` 自动，`0` 手动           |
| `-c <index>`     | 调用 `BB_SET_CHAN`，设置信道索引                          | 不执行                                           |
| `-d <dir>`       | `-c` 和 `-w` 使用的方向，支持 `tx`、`rx` 或 `0`、`1`         | `rx`                                          |
| `-W <0           | 1>`                                              | 调用 `BB_SET_BANDWIDTH_MODE`，`1` 自动，`0` 手动      |
| `-w <bandwidth>` | 调用 `BB_SET_BANDWIDTH`，范围 `0-5`                   | 不执行                                           |
| `-M <0           | 1>`                                              | 调用 `BB_SET_MCS_MODE`，`1` 自动，`0` 手动            |
| `-m <mcs>`       | 调用 `BB_SET_MCS`，范围 `0-24`                        | 不执行                                           |
| `-F <0           | 1>`                                              | 调用 `BB_SET_FRAME_CHANGE`，仅 `SINGLE_USER` 模式支持 |
| `-a <addr>`      | daemon 地址                                        | `127.0.0.1`                                   |
| `-p <port>`      | daemon 端口                                        | `BB_PORT_DEFAULT`                             |
| `-i <index>`     | 选择设备序号                                           | `0`                                           |
| `-s <slot>`      | slot 编号，用于频宽和 MCS 配置                             | `0`                                           |
| `-h`             | 打印帮助信息                                           | 无                                             |


如果没有指定任何设置动作，程序只打印帮助并返回错误，避免误操作。

#### 4.2 标准使用方法

1. 启动 `l4_daemon`。
2. 使用 `l4_link_monitor` 查看当前角色、模式、slot 和链路状态。
3. 根据需求设置自动或手动模式。
4. 下发目标频段、信道、频宽或 MCS。
5. 设置后再次使用 `l4_link_monitor` 验证结果。

频段、信道、频宽和 MCS 的设置命令可组合使用。程序会按固定顺序执行：频段模式、频段、信道模式、信道、频宽模式、频宽、MCS 模式、MCS、帧结构。

#### 4.3 示例

频段和信道都设置为自动：

```sh
./l4_link_config -B 1 -C 1
```

手动设置目标频段为 2G：

```sh
./l4_link_config -B 0 -b 2g
```

手动设置 RX 方向目标信道索引为 3：

```sh
./l4_link_config -C 0 -d rx -c 3
```

手动设置 slot 0 的 RX 方向频宽为 `BB_BW_20M`：

```sh
./l4_link_config -s 0 -W 0 -d rx -w 4
```

手动设置 slot 0 的 MCS：

```sh
./l4_link_config -s 0 -M 0 -m 6
```

执行帧结构切换：

```sh
./l4_link_config -F 1
```

同时设置频段、信道、频宽和 MCS：

```sh
./l4_link_config -B 0 -b 2g -C 0 -d rx -c 3 -s 0 -W 0 -w 4 -M 0 -m 6
```

## 常见问题

### 1、程序提示没有设备

检查以下项目：

- `l4_daemon` 是否已启动。
- 8030 设备是否已通过 USB、UART 或其它接口连接。
- 当前用户是否有访问设备节点的权限。
- daemon 日志中是否有枚举失败或接口初始化失败信息。

### 2、连接 daemon 失败

确认 daemon 正在运行，并且示例程序连接的地址和端口与 daemon 一致。本机默认使用 `127.0.0.1` 和 `BB_PORT_DEFAULT`。

### 3、链路质量、MCS 或吞吐没有输出

这些信息依赖 AP 和 DEV 已完成配对并建立链路。先使用：

```sh
./l4_pair_manager -m
./l4_link_monitor -S
```

确认目标 slot 已经处于连接状态。

### 4、链路设置命令失败

检查以下项目：

- 当前设备角色是否支持该设置。
- `-s` 指定的 slot 是否存在有效链路。
- `-d` 指定的方向是否符合当前场景。
- `-F` 是否只在 `SINGLE_USER` 模式下使用。
- 设置后是否需要重新查询状态确认设备侧实际生效结果。

### 5、需要查看完整 API 结构体

本文档只说明常用工具和示例的使用方式。完整 API 命令字、输入输出结构体和字段说明请参考：

```text
L4_Linux_SDK/docs/api.md
```

