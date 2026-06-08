# L4 Linux SDK 开发手册

本文档面向 Linux SDK 使用者，说明常用应用工具和 API 示例程序的使用方法。阅读本文档前，建议先完成 SDK 编译，并确认 `l4_daemon` 可以正常访问 8030 设备。

## 目录

- [使用前准备](#使用前准备)
- [一、应用工具](#一应用工具)
- [二、API 调用示例](#二api-调用示例)
- [常见问题](#常见问题)

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

### 通用参数说明

本文档中的应用工具和 API 示例程序统一支持以下通用参数，后续各章节只说明各程序自身的业务参数。


| 参数           | 说明                 | 默认值               |
| ------------ | ------------------ | ----------------- |
| `-h, --help` | 打印帮助信息             | 无                 |
| `-a <addr>`  | daemon 地址          | `127.0.0.1`       |
| `-p <port>`  | daemon 端口          | `BB_PORT_DEFAULT` |
| `-i <index>` | 选择 daemon 枚举到的设备序号 | `0`               |


查看帮助信息：

```sh
./l4_basic_info -h
```

连接指定 daemon：

```sh
./l4_basic_info -a 192.168.1.10 -p 9000
```

选择第 1 个设备：

```sh
./l4_basic_info -i 1
```

这些通用参数可与各程序自身业务参数组合使用，例如：

```sh
./l4_link_monitor -a 192.168.1.10 -p 9000 -i 1 -S
```

## 一、应用工具

### 1、固件升级工具

`l4_ota_upgrade` 用于对 8030 设备执行固件升级。当前手册先只介绍通过 `-f` 指定升级镜像的基础用法。

#### 1.1 程序参数说明


| 参数                  | 说明             | 默认值 |
| ------------------- | -------------- | --- |
| `-f, --file <file>` | 指定待升级的固件镜像文件路径 | `无` |


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

正常输出示例：

```text
./l4_ota_upgrade compiled at: Jun  6 2026 14:15:56
upgrade file: ../../../../firmware/KT-2458-G-V1.1.8-U/VT4-KT-2458-G-V1.1.8-U.img
file size = 898916

====================upgrade bin infomation====================
magic:            0x4152544f
img size:         0xdb544
partitions:       0x5
segments:         0x2
...
====================upgrade bin infomation done====================

Verify upgrade image...
Verify upgrade image success
update romcode data=0x796e2ca8d230, len=0x8000
ar8030_upgrade_partition addr 0 len 32768 crc b53913ab
update process 100%
firmware_ver: KT-2458-G-V1.1.8-U
running sys is 0
...
update process 100%
upgrade done
```

看到 `update process 100%` 和 `upgrade done`，表示升级流程正常结束。

### 2、网络透传工具

`l4_tuntap` 用于通过 TAP 设备实现基带数据透传能力。它会把 Linux 网络侧数据封装后写入基带 socket，也会把基带收到的数据还原后写入 TAP 设备，从而实现网络透传功能。

`l4_tuntap` 仅适用于网络通过 USB 透传的设备，也就是图传固件版本以 `-U` 结尾的配置。固件版本以 `-E` 结尾时，网络通过设备网口通信，不需要使用 `l4_tuntap`。

#### 2.1 程序参数说明


| 参数                     | 说明                  | 默认值     |
| ---------------------- | ------------------- | ------- |
| `-u, --user <user>`    | 指定 baseband user id | `0`     |
| `-P, --transport <id>` | 指定透传 transport id   | `3`     |
| `-I, --tap-ip <ip>`    | 指定 TAP 设备 IP，必填     | 无       |
| `-d, --dev <dev>`      | 指定 TAP 设备名          | `tap0`  |
| `-v, --debug`          | 打开网络透传调试打印          | 关闭      |
| `-r, --rx_buf <len>`   | 指定 RX buffer 长度     | `40000` |
| `-t, --tx_buf <len>`   | 指定 TX buffer 长度     | `60000` |


`-I` 是网络透传工具的关键参数，用于给 TAP 虚拟网卡配置 IP。两端设备应配置在同一网段，并避免 IP 冲突。

#### 2.2 标准使用方法

1. AP 和 DEV 侧均启动 `l4_daemon`。
2. 确认 AP 和 DEV 已完成配对并建立链路。
3. AP 侧和 DEV 侧分别运行 `l4_tuntap`，并配置不同的 TAP IP。
4. 使用 `ping`、`iperf` 或业务程序通过 TAP IP 验证网络透传。

如果 TAP 设备创建或配置失败，通常需要使用 root 权限运行。

#### 2.3 示例

AP 侧配置 TAP IP：

```sh
sudo ./l4_tuntap -I 192.168.144.55 -d tap0
```

DEV 侧配置 TAP IP：

```sh
sudo ./l4_tuntap -I 192.168.144.66 -d tap0
```

`l4_tuntap` 正常启动输出示例：

```text
l4_tuntap args: -u 0 -P 3 -I 192.168.144.55 -d tap0 -r 40000 -t 60000
dev = tap0,ip = 192.168.144.55,mtu = 4000 , tun_fd = 4
lgeng - 0  ar_bb_socket_fd = 0
```

看到 TAP 设备名、IP、`tun_fd` 和 `ar_bb_socket_fd` 都正常打印，表示 TAP 设备和基带 socket 已创建成功。

`ifconfig` 查看tap0 网卡详情

```
tap0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 4000
        inet 192.168.144.55  netmask 255.255.255.0  broadcast 192.168.144.255
        inet6 fe80::3088:49ff:fe90:c09c  prefixlen 64  scopeid 0x20<link>
        ether f2:a0:f6:85:63:4d  txqueuelen 1000  (以太网)
        RX packets 168  bytes 18353 (18.3 KB)
        RX errors 0  dropped 2  overruns 0  frame 0
        TX packets 50  bytes 6073 (6.0 KB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```

随后 `ping` 能收到回复，表示网络透传链路正常。

```
ping 192.168.144.66
PING 192.168.144.66 (192.168.144.66) 56(84) bytes of data.
64 bytes from 192.168.144.66: icmp_seq=1 ttl=64 time=31.9 ms
64 bytes from 192.168.144.66: icmp_seq=2 ttl=64 time=28.0 ms
64 bytes from 192.168.144.66: icmp_seq=3 ttl=64 time=34.2 ms
64 bytes from 192.168.144.66: icmp_seq=4 ttl=64 time=31.4 ms
64 bytes from 192.168.144.66: icmp_seq=5 ttl=64 time=27.7 ms
^C
--- 192.168.144.66 ping statistics ---
5 packets transmitted, 5 received, 0% packet loss, time 4005ms
rtt min/avg/max/mdev = 27.678/30.619/34.187/2.471 ms
```

### 3、图传调试工具

`l4_cmd_dbg` 是图传调试命令工具。它通过 daemon 连接 8030 设备的 debug 通道，打印内容与图传调试串口输出一致，也可以输入图传调试串口支持的命令来查看图传设备状态。

#### 3.1 程序参数说明


| 参数                | 说明                        | 默认值       |
| ----------------- | ------------------------- | --------- |
| `-m, --mac <mac>` | 按 MAC 地址选择要调试的设备          | 默认打开第一个设备 |
| `-l, --list`      | 列出 daemon 当前枚举到的设备并退出     | 无         |
| `-o, --output`    | 只输出设备 debug 打印，不从标准输入读取命令 | 关闭        |


不加 `-o` 时，程序进入交互模式：用户在终端输入的每一行都会发送到设备 debug 通道，设备返回内容会直接打印到当前终端。该交互命令与图传调试串口命令保持一致。

#### 3.2 标准使用方法

1. 启动 `l4_daemon`。
2. 确认 8030 设备已被 daemon 枚举到。
3. 执行 `l4_cmd_dbg -l` 查看当前设备列表。
4. 执行 `l4_cmd_dbg` 进入交互调试模式。
5. 输入图传调试串口支持的状态查询命令，查看设备状态、链路状态或其它固件调试信息。
6. 如只需要观察设备 debug 打印，使用 `-o` 进入只输出模式。

#### 3.3 示例

进入默认设备的交互调试模式：

```sh
./l4_cmd_dbg
```

随后终端会持续打印与图传调试串口一致的设备 debug 内容。输入相应调试命令后，设备会返回对应信息。

输入 `help` 正常输出示例：

```text
help
help


help:
 Lists all the registered commands


task-stats:
 Displays a table showing the state of each FreeRTOS task

run-time-stats:
 Displays a table showing how much processing time each FreeRTOS task has used

query-heap:
 Displays the free heap space, and minimum ever free heap space.

show_irq:
 show irq
 show_irq all
set - set [--ap_mac xxxxxxx | --save ...]
prj - --prj_pair --timeout [time] --bitmap [bitmap]
    - --featurelog - set log level, log [module] [level]
ll - link level debug command
mac - mac [--tx|--rx] [--user n] --detail
debug - enter/leave debug mode
be - check baseband error
power - set power mode or power dbm, power [--u user] [--d dbm] [--m openloop|closeloop]
chan - display chan frequency and sweep power
bi - display bb task or bi reset (reset history)
mt - msg trace, e.g mt 0x100
reset - reset [trx|tx|rx]
mcs   - mcs --user [n] --mode [auto|manu] --value[-2 ~ 13]
d - display memory, d [addr] [num]
p - display baseband reg page, p [0-15]
m - modify  memory, m [addr] [width] [data]
frame - display super frame information
bl - calc byte length, bl [bw] [tintlv_len] [qam] [cr] [rep] [mimo]
freq - translate freq(khz) <-> reg value (xx-xx-xxxxxx)
irq - enable bb irq trace, irq [0 - 15]
eop - display rx signal statistics
distc - display distance calc result
cfg - display current configure or cfg reset, cfg [--reset]
sleep - 8030 enter sleep mode
echo - internal message test, echo --slot [n]
chipid - chipid R/W
rt -  runtime debug
single_tone - enter single tone mode
recover - Reboot and enter recovery mode
reboot - system reboot
auth
xinfo - dump the info of xds
xstats - dump the stats of xds
x_intf_en - enable/disable intf tx/rx
```

#### 3.4 常用调试命令说明

本节用于整理图传调试串口中的常用命令。通过 `l4_cmd_dbg` 进入交互模式后，输入的命令与调试串口命令一致，设备返回内容会直接打印到终端。

每个命令建议按以下格式说明：


| 项目   | 写法                                                 |
| ---- | -------------------------------------------------- |
| 命令格式 | 写出完整命令和参数形式，例如 `ll`、`frame`、`mac --tx --user 0`    |
| 功能说明 | 说明该命令用于查看或设置什么信息，适合在哪类问题中使用                        |
| 使用方法 | 说明进入 `l4_cmd_dbg` 后如何输入命令，是否需要 AP/DEV 已配对，是否支持重复执行 |
| 输出解析 | 按字段解释输出含义，并说明哪些字段最适合用于判断设备状态                       |
| 注意事项 | 说明固件版本差异、角色差异、单位、枚举值或风险操作                          |


##### 3.4.1 `ll` 命令

**命令格式**

```text
ll
```

**功能说明**

`ll` 用于查看链路层运行时状态。AP 侧会显示已使能 slot 上各 DEV 的状态；DEV 侧通常显示本设备与 AP 的链路状态。该命令适合用于确认配对状态、slot 是否生效、链路收发是否打开、当前 MAC 和链路异常计数。

**使用方法**

进入 `l4_cmd_dbg` 交互模式后输入：

```
ll
```

**输出解析**

```
AP LL state:
----------------
AP time      : 175719458
My    MAC    : A55821C2
Slot Bmp     : 0x01
RT Slot Bmp  : 0x01
State Bmp    : LTX LRX
Frame Time   : 10 ms
CS sel       : 0
psave period : 1
FCH MON-1    : 0:(1,1,111,0) 1:(0,0,0,0) 2:(0,0,0,0) 3:(0,0,0,0) 4:(0,0,0,0)
FCH MON-2    : 5:(0,0,0,0) 6:(0,0,0,0) 7:(0,0,0,0) 8:(0,0,0,0) 9:(0,0,0,0)
ReTX User 8  : 6 810 124
Dwlink Chan  : 2 (2422000KHz)
Uplink Chan  : 4 (2444000KHz)
Updata Chan  : 4 (2444000KHz)

Slot Id      : 0
AP  State    : CONNECT
Dev State    : CONNECT
Dwdata Chan  : 2 (2422000KHz)
State Bmp    :
Dev MAC      : 642D809F
RX  MCS      : 9
Peer Power   : 15
Peer LNAbyss : 0, 0
RF TX (L P)  : 2 2
Req MCS      : 23 | 6
Link unlock  : 0
Token        : 86
BR retx(D)   : 0 | 0x0000
Remote       : 0 1 00(0,0,0,0)
```


| 字段             | 含义                    | 关注点                                       |
| -------------- | --------------------- | ----------------------------------------- |
| `Slot Bmp`     | AP 侧配置使能的 slot bitmap | 判断 AP 是否开放了目标 slot                        |
| `RT Slot Bmp`  | 运行时 slot bitmap       | 动态 slot 模式下用于确认真实超级帧中的 slot 状态            |
| `State Bmp`    | 链路状态 bitmap           | 重点看 `LTX`、`LRX`、`STX`、`SRX`、`PAIR` 是否符合预期 |
| `Frame Time`   | 超级帧时间长度               | 单位按固件输出为准，通常用于确认帧周期是否异常                   |
| `Up Channel`   | 上行信道                  | 用于确认 DEV 到 AP 方向工作信道                      |
| `Down Channel` | 下行信道                  | 用于确认 AP 到 DEV 方向工作信道                      |
| `AP State`     | AP 状态                 | AP 设备以该字段为主，对端状态仅作辅助判断                    |
| `DEV State`    | DEV 状态                | DEV 设备以该字段为主，对端状态仅作辅助判断                   |
| `My MAC`       | 本设备当前使用的 MAC          | 用于确认当前连接的是哪一个设备                           |
| `AP MAC`       | 目标 AP MAC             | DEV 侧用于确认已记录的 AP 地址                       |
| `DEV MAC`      | 对应 slot 的 DEV MAC     | AP 侧用于确认指定 slot 绑定的 DEV 地址                |
| `RX MCS`       | 当前接收 MCS 等级           | 用于初步判断链路质量和速率档位                           |
| `Link unlock`  | 链路层 unlock 计数         | 持续增长通常表示链路不稳定，需要结合信道、MCS、功率继续排查           |


常见判断方式：

- `PAIR` 已置位，通常表示配对关系存在。
- `LTX/LRX` 已置位，通常表示链路层收发已使能。
- `STX/SRX` 已置位，通常表示数据收发路径已使能。
- `Link unlock` 持续增加时，优先检查信号质量、信道干扰、MCS 设置和距离。

##### 3.4.2 `frame` 命令

**命令格式**

```text
frame
```

**功能说明**

`frame` 用于显示当前设备的超级帧信息，包括帧单元类型、方向、时间长度、频点、带宽、payload、MCS、RF 模式、FCH/LTP 信息、可承载字节数和理论吞吐。该命令适合用于确认帧结构、上下行资源分配、频宽、MCS 和理论吞吐是否符合配置。

**使用方法**

进入 `l4_cmd_dbg` 交互模式后输入：

```text
frame
```

建议在以下场景执行：

- 配对完成后，确认当前超级帧结构是否正常。
- 调用 `l4_link_config` 修改频宽、MCS 或帧结构后，确认配置是否反映到设备侧。
- 排查吞吐异常时，对比 `frame` 中的 `BL(B)`、`TP(K)` 与链路监控结果。

**输出解析**

```
type    dir   penc time  freq    bw   pld tintlv MCS TX_Mode RX_Mode   FCH   LTP   BL(B) TP(K)
-------------------------------------------------------------------------------------------
tx_fre  idlet      100                                                                         
br      send  0@6  3000  2472625 2.5  on  Y24x1  0   2TX_STBC          2@96  L:30  281   236   
rx_fre  idler      100                                                                         
sweep_s recv       100   5575000 10                          1T1R            -                 
rx_fre  idler      100                                                                         
slot0   recv       5331  2444000 10   on  Y24x2              2T2R_STBC 2@96  L:30              
rx_fre  idler      100                                                                         
sweep_l recv       674   5575000 10                          2T2R_STBC       L:0               
-------------------------------------------------------------------------------------------
frame time : 9505
```


| 字段        | 含义            | 关注点                                                      |
| --------- | ------------- | -------------------------------------------------------- |
| `type`    | 帧单元类型         | 用于区分不同功能的帧单元                                             |
| `dir`     | 帧单元方向         | 判断该单元属于上行还是下行                                            |
| `prec`    | 预编码位置         | 格式为 `[参考单元]@[8 分位位置]`，`0` 表示本单元，`1` 表示后一个单元，`-1` 表示前一个单元 |
| `time`    | 单元时间长度        | 单位为微秒，用于判断该单元占用时间                                        |
| `freq`    | 单元射频频率        | 单位为 KHz，用于确认实际工作频点                                       |
| `bw`      | 单元频宽          | 单位为 MHz，用于确认频宽配置是否生效                                     |
| `pld`     | 是否有 payload   | `on` 表示该单元承载数据，`off` 表示不承载数据                             |
| `tintlv`  | 是否时域交织        | 用于确认当前帧结构中的交织配置                                          |
| `MCS`     | 发送 MCS 级别     | 用于确认发送速率档位                                               |
| `TX_Mode` | 发送 RF 模式      | 用于确认发送链路使用的 RF 模式                                        |
| `RX_Mode` | 接收 RF 模式      | 用于确认接收链路使用的 RF 模式                                        |
| `FCH`     | FCH 信息        | 格式为 `[OFDM 数]@[总数据 bit 数]`                               |
| `LTP`     | LTP 信息        | 格式为 `[长短模式]:[LTP ID]`                                    |
| `BL(B)`   | 每个交织块最大可携带数据量 | 单位为 Byte，是判断单帧承载能力的重要字段                                  |
| `TP(K)`   | 当前单元理论发送速率    | 单位为 Kbps，可与吞吐测试结果对比                                      |


常见判断方式：

- 修改频宽后，重点查看 `bw` 是否变化。
- 修改 MCS 后，重点查看 `MCS` 和 `TP(K)` 是否变化。
- 排查吞吐低时，重点查看 `pld` 是否为 `on`，以及 `BL(B)`、`TP(K)` 是否符合预期。
- 排查上下行资源问题时，重点查看 `dir`、`time` 和各方向单元数量。

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


| 参数   | 说明                             | 默认值  |
| ---- | ------------------------------ | ---- |
| `-S` | 查询 `BB_GET_STATUS`，打印设备状态和链路状态 | 无    |
| `-V` | 查询 `BB_GET_SYS_INFO`，打印系统和版本信息 | 无    |
| `-A` | 查询全部基础信息                       | 默认动作 |


如果不指定 `-S`、`-V` 或 `-A`，程序默认执行全量查询。

#### 1.2 标准使用方法

1. 启动 `l4_daemon`。
2. 执行 `l4_basic_info` 查询基础状态。
3. 根据输出确认设备角色、工作模式、版本和链路状态。

#### 1.3 示例

##### 1.3.1 查询全部基础信息：

```sh
./l4_basic_info
```

##### 1.3.2 只查询系统版本：

```sh
./l4_basic_info -V
```

正常输出示例:

```
[BB_GET_SYS_INFO]
uptime       : 176706959 ms
compile_time : 01/28/2026 19:57:27
soft_ver     : 1.3.02-21
hardware_ver : Unknown_4.0
firmware_ver : KT-2458-G-V1.1.8-U
```

##### 1.3.3 只查询设备状态：

```sh
./l4_basic_info -S
```

正常输出示例:

```
[BB_GET_STATUS]
role        : AP (0)
mode        : SINGLE_USER (0)
sync_mode   : 0
sync_master : 1
cfg_sbmp    : 0x01
rt_sbmp     : 0x01
local_mac   : a5:58:21:c2

slot link status:
  slot[0]: state=CONNECT(2), rx_mcs_raw=11, rx_mcs_real=9, pair=0, peer_mac=64:2d:80:9f

user phy status:
  RX: user=0 source=rx freq=2477000KHz bw=10MHz(3) tintlv_enable=1 tintlv_len=3 tintlv_num=1
  TX: user=8 source=tx freq=2395625KHz mcs_raw=2 mcs_real=0 bw=2.5MHz(1) tintlv_enable=1 tintlv_len=3 tintlv_num=0
  bw_mode=Y24X2 major_dir=DEV->AP
```

### 2、配对管理示例

`l4_pair_manager` 用于启动配对、停止配对、查询配对结果，以及手动设置对端 MAC。

#### 2.1 程序参数说明


| 参数          | 说明             | 默认值   |
| ----------- | -------------- | ----- |
| `-P`        | 启动配对，并等待配对结果事件 | 无     |
| `-X`        | 停止当前配对         | 无     |
| `-m`        | 查询配对结果和对端 MAC  | 无     |
| `-M <mac>`  | 按当前角色设置对端 MAC  | 无     |
| `-s <slot>` | AP 侧开放的 slot   | `0`   |
| `-t <sec>`  | 设备侧配对超时时间      | `100` |


AP 侧启动配对时，`-s` 表示开放哪个 slot 给 DEV 接入。DEV 侧启动配对时通常不需要指定 slot。

#### 2.2 标准使用方法

1. AP 和 DEV 侧分别启动 `l4_daemon`。
2. AP 侧执行 `l4_pair_manager -P -s <slot>`，开放目标 slot。
3. DEV 侧执行 `l4_pair_manager -P`，进入配对流程。
4. 等待 `BB_EVENT_PAIR_RESULT` 事件返回。
5. 配对成功后执行 `l4_pair_manager -m` 查询对端 MAC。

配对是异步动作，程序会先订阅 `BB_EVENT_PAIR_RESULT`，再下发配对命令，并持续等待设备返回事件；对频成功或设备侧超时都会通过事件回调返回。

#### 2.3 示例

##### 2.3.1 启动对频

AP 侧开放 slot 0 配对：

```sh
./l4_pair_manager -P
```

正常输出示例：

```text
device role: AP (0)
pair callback registered
start pair: role=AP slot=0 bitmap=0x01 timeout=100 asyn=1
waiting for pair result...
waiting for pair result...

[BB_EVENT_PAIR_RESULT]
pair ret=0 slot=0
pair ok
device role: AP (0)

[BB_GET_CANDIDATES]
slot: 0 mac_num: 1
dev_mac[0]: 65:a6:78:c7
```

DEV 侧启动配对：

```sh
./l4_pair_manager -P
```

正常输出示例：

```text
device role: DEV (1)
pair callback registered
start pair: role=DEV slot=0 bitmap=0x00 timeout=100 asyn=1
waiting for pair result...
waiting for pair result...

[BB_EVENT_PAIR_RESULT]
pair ret=0 slot=0
pair ok
device role: DEV (1)

[BB_GET_AP_MAC]
ap_mac: a5:58:21:c2
```

##### 2.3.2 查询配对结果

配对完成后，可查询当前设备已记录的对端 MAC。AP 侧会查询候选 DEV MAC，DEV 侧会查询 AP MAC。

```sh
./l4_pair_manager -m
```

输出中应出现 `[BB_GET_CANDIDATES]` 或 `[BB_GET_AP_MAC]`，用于确认当前角色对应的配对结果。

##### 2.3.3 手动设置配对

假设 AP MAC 地址为 11:22:33:44，DEV MAC 地址为 AA:BB:CC:DD。

AP 侧手动设置 slot 0 的 DEV MAC：

```sh
./l4_pair_manager -M AA:BB:CC:DD
```

DEV 侧手动设置 AP MAC：

```sh
./l4_pair_manager -M 11:22:33:44
```

设置完成后，两边设备会自动对频。

##### 2.3.4 停止配对

如果设备正在配对，可停止当前配对流程：

```sh
./l4_pair_manager -X
```

命令成功后，设备侧会退出当前配对等待状态。

##### 2.3.5 指定配对超时时间

通过 `-t <sec>` 指定设备侧配对超时时间。下面示例将超时时间设置为 60 秒：

```sh
./l4_pair_manager -P -t 60
```

输出中的 `timeout=60` 表示该超时时间已随配对命令下发。

### 3、链路监控示例

`l4_link_monitor` 用于在配对完成后读取链路状态、信号质量、MCS、功率、信道、频段和吞吐信息。

#### 3.1 程序参数说明


| 参数          | 说明                         | 默认值  |
| ----------- | -------------------------- | ---- |
| `-A`        | 查询全部链路信息                   | 默认动作 |
| `-S`        | 查询 `BB_GET_STATUS`         | 无    |
| `-Q`        | 查询 `BB_GET_USER_QUALITY`   | 无    |
| `-q`        | 查询 `BB_GET_PEER_QUALITY`   | 无    |
| `-M`        | 查询 `BB_GET_MCS`            | 无    |
| `-P`        | 查询 `BB_GET_CUR_POWER`      | 无    |
| `-C`        | 查询 `BB_GET_CHAN_INFO`      | 无    |
| `-B`        | 查询 `BB_GET_BAND_INFO`      | 无    |
| `-T`        | 查询 `BB_GET_THROUGHPUT`     | 无    |
| `-V`        | 查询 `BB_GET_1V1_INFO`       | 无    |
| `-s <slot>` | 目标 slot；DEV 侧 slot 0 表示 AP | `0`  |
| `-u <user>` | 物理用户编号                     | `0`  |


链路质量、MCS、功率和吞吐等细节需要在配对完成、链路连接后读取。程序会先查询 `BB_GET_STATUS` 判断目标 slot 是否可用。

#### 3.2 标准使用方法

1. 启动 `l4_daemon`。
2. 使用 `l4_pair_manager` 完成 AP 和 DEV 配对。
3. 执行 `l4_link_monitor` 查询全量链路信息。
4. 根据需要通过 `-S/-Q/-M/-T/-V` 等参数查询指定信息。
5. AP 侧通过 `-s` 指定目标 DEV 所在 slot；DEV 侧通常使用 `-s 0` 查看 AP 方向。

#### 3.3 示例

##### 3.3.1 查询全部链路信息

```sh
./l4_link_monitor
```

##### 3.3.2 查询链路基础信息

```sh
./l4_link_monitor -V
```

正常输出示例：

```text
[BB_GET_1V1_INFO]
  self: snr_raw=4013 snr_db=20.47 dB ldpc_tlv_err_ratio=0 ldpc_num_err_ratio=0 gain_a=33 gain_b=25 tx_mcs=2 tx_chan=10 tx_power=15 tx_freq_khz=5590625
  peer: snr_raw=1010 snr_db=14.48 dB ldpc_tlv_err_ratio=0 ldpc_num_err_ratio=0 gain_a=33 gain_b=37 tx_mcs=11 tx_chan=8 tx_power=15 tx_freq_khz=5110000
```

##### 3.3.3 查询链路信号质量

AP 端：

```sh
./l4_link_monitor -Q -q
```

正常输出示例：

```text
[BB_GET_USER_QUALITY]
user[0]: snr_raw=3900 snr_db=20.35 dB ldpc=0/96 gain_a=34 gain_b=26

[BB_GET_PEER_QUALITY]
slot[0]: snr_raw=1215 snr_db=15.28 dB ldpc=0/2 gain_a=33 gain_b=37
```

DEV 端：

```sh
./l4_link_monitor -Q -u 8 -q
```

正常输出示例：

```text
[BB_GET_USER_QUALITY]
user[8]: snr_raw=1949 snr_db=17.34 dB ldpc=0/2 gain_a=32 gain_b=40

[BB_GET_PEER_QUALITY]
slot[0]: snr_raw=2716 snr_db=18.78 dB ldpc=0/96 gain_a=36 gain_b=34
```

##### 3.3.4 查询 MCS

```sh
./l4_link_monitor -M
```

正常输出示例：

```text
[BB_GET_MCS] slot=0
  TX: mcs_raw=2 mcs_real=0 theory_throughput=236 kbps
  RX: mcs_raw=11 mcs_real=9 theory_throughput=11493 kbps
```

##### 3.3.5 查询天线发射功率

AP 端：

```sh
./l4_link_monitor -P -u 8
```

正常输出示例：

```text
[BB_GET_CUR_POWER]
user[8]: power=15
```

DEV 端：

```sh
./l4_link_monitor -P
```

正常输出示例：

```text
[BB_GET_CUR_POWER]
user[0]: power=15
```

##### 3.3.6 查询信道信息

```sh
./l4_link_monitor -C
```

正常输出示例：

```text
[BB_GET_CHAN_INFO]
chan_num=32 auto_mode=1 acs_chan=26 work_chan=30
  chan[0]: freq=2400000 KHz power=-92 dbm
  chan[1]: freq=2411000 KHz power=-79 dbm
  chan[2]: freq=2422000 KHz power=-73 dbm
  chan[3]: freq=2433000 KHz power=-85 dbm
  chan[4]: freq=2444000 KHz power=-79 dbm
  chan[5]: freq=2455000 KHz power=-78 dbm
  chan[6]: freq=2466000 KHz power=-71 dbm
  chan[7]: freq=2477000 KHz power=-88 dbm
  chan[8]: freq=5110000 KHz power=-95 dbm
  chan[9]: freq=5155000 KHz power=-98 dbm
  chan[10]: freq=5180000 KHz power=-98 dbm
  chan[11]: freq=5225000 KHz power=-80 dbm
  chan[12]: freq=5250000 KHz power=-80 dbm
  chan[13]: freq=5270000 KHz power=-101 dbm
  chan[14]: freq=5295000 KHz power=-100 dbm
  chan[15]: freq=5315000 KHz power=-101 dbm
  chan[16]: freq=5350000 KHz power=-100 dbm
  chan[17]: freq=5380000 KHz power=-98 dbm
  chan[18]: freq=5410000 KHz power=-96 dbm
  chan[19]: freq=5480000 KHz power=-99 dbm
  chan[20]: freq=5495000 KHz power=-99 dbm
  chan[21]: freq=5515000 KHz power=-100 dbm
  chan[22]: freq=5530000 KHz power=-99 dbm
  chan[23]: freq=5555000 KHz power=-98 dbm
  chan[24]: freq=5575000 KHz power=-94 dbm
  chan[25]: freq=5595000 KHz power=-95 dbm
  chan[26]: freq=5730000 KHz power=-100 dbm
  chan[27]: freq=5750000 KHz power=-100 dbm
  chan[28]: freq=5780000 KHz power=-100 dbm
  chan[29]: freq=5800000 KHz power=-100 dbm
  chan[30]: freq=5820000 KHz power=-100 dbm
  chan[31]: freq=5840000 KHz power=-100 dbm
```

##### 3.3.7 查询频段信息

```sh
./l4_link_monitor -B
```

正常输出示例：

```text
[BB_GET_BAND_INFO]
band_mode=AUTO(1) work_band=5G(2)
```

##### 3.3.8 查询传输带宽

```sh
./l4_link_monitor -T
```

正常输出示例：

```text
[BB_GET_THROUGHPUT] slot=0
  TX: phy=232960 real=26112
  RX: phy=11471040 real=53664
```

### 4、链路设置示例

`l4_link_config` 用于设置图传链路的频段、信道、频宽、MCS 和帧结构。它通常与 `l4_link_monitor` 配套使用：先读取当前链路状态，再下发配置。

#### 4.1 程序参数说明


| 参数               | 说明                                               | 默认值  |
| ---------------- | ------------------------------------------------ | ---- |
| `-B <0/1>`       | 调用 `BB_SET_BAND_MODE`，`1` 表示自动，`0` 表示手动          | 不执行  |
| `-b <band>`      | 调用 `BB_SET_BAND`，支持 `1g`、`2g`、`5g` 或 `0`、`1`、`2` | 不执行  |
| `-C <0/1>`       | 调用 `BB_SET_CHAN_MODE`，`1` 表示自动，`0` 表示手动          | 不执行  |
| `-c <index>`     | 调用 `BB_SET_CHAN`，设置信道索引                          | 不执行  |
| `-d <dir>`       | `-c` 和 `-w` 使用的方向，支持 `tx`、`rx` 或 `0`、`1`         | `rx` |
| `-W <0/1>`       | 调用 `BB_SET_BANDWIDTH_MODE`，`1` 表示自动，`0` 表示手动     | 不执行  |
| `-w <bandwidth>` | 调用 `BB_SET_BANDWIDTH`，范围 `0-5`                   | 不执行  |
| `-M <0/1>`       | 调用 `BB_SET_MCS_MODE`，`1` 表示自动，`0` 表示手动           | 不执行  |
| `-m <mcs>`       | 调用 `BB_SET_MCS`，范围 `0-24`                        | 不执行  |
| `-F <0/1>`       | 调用 `BB_SET_FRAME_CHANGE`，仅 `SINGLE_USER` 模式支持    | 不执行  |
| `-s <slot>`      | slot 编号，用于频宽和 MCS 配置                             | `0`  |


如果没有指定任何设置动作，程序只打印帮助并返回错误，避免误操作。

#### 4.2 标准使用方法

1. 启动 `l4_daemon`。
2. 使用 `l4_link_monitor` 查看当前角色、模式、slot 和链路状态。
3. 根据需求设置自动或手动模式。
4. 下发目标频段、信道、频宽或 MCS。
5. 设置后再次使用 `l4_link_monitor` 验证结果。

频段、信道、频宽和 MCS 的设置命令可组合使用。程序会按固定顺序执行：频段模式、频段、信道模式、信道、频宽模式、频宽、MCS 模式、MCS、帧结构。

成功执行的命令会按 `l4_link_monitor` 风格打印命令字标题和参数详情，例如：

```text
[BB_SET_BAND_MODE]
auto_mode=0

[BB_SET_BAND]
target_band=5G(2)
```

如果 `bb_ioctl()` 返回非 0，会打印 `BB_SET_xxx failed, ret=<ret>` 并停止后续设置。

#### 4.3 示例

##### 4.3.1 手动设置频段为 2G

```sh
./l4_link_config -B 0 -b 2g
```

正常输出示例：

```text
[BB_SET_BAND_MODE]
auto_mode=0

[BB_SET_BAND]
target_band=2G(1)
```

设置完成后，可查询当前频段确认结果：

```sh
./l4_link_monitor -B
```

正常输出示例：

```text
[BB_GET_BAND_INFO]
band_mode=MANUAL(0) work_band=2G(1)
```

##### 4.3.2 手动设置信道索引为 0（2400 MHz）

手动设置信道前，应先将频段固定到目标频段。本例前一步已将频段设置为 2G，因此信道索引 0 对应信道信息中的 `2400000 KHz`，也就是 2400 MHz。

```sh
./l4_link_config -C 0 -c 0
```

正常输出示例：

```text
[BB_SET_CHAN_MODE]
auto_mode=0

[BB_SET_CHAN]
dir=RX(1) chan_index=0
```

设置完成后，可查询信道信息确认 `work_chan` 和频点映射：

```sh
./l4_link_monitor -C
```

正常输出示例：

```text
[BB_GET_CHAN_INFO]
chan_num=32 auto_mode=0 acs_chan=26 work_chan=0
  chan[0]: freq=2400000 KHz power=-74 dbm
  chan[1]: freq=2411000 KHz power=-71 dbm
  chan[2]: freq=2422000 KHz power=-78 dbm
  ...
  chan[8]: freq=5110000 KHz power=-99 dbm
  ...
```

其中 `work_chan=0` 表示当前工作信道索引为 0，`chan[0]` 对应 `2400000 KHz`。完整信道索引和频点映射可参考 `3.3.6 查询信道信息` 的 `BB_GET_CHAN_INFO` 输出。

切换到 5G 频点时，也需要先将频段设置为 5G，例如要使用 `5110000 KHz`，应先固定到 5G 频段，再选择对应的信道索引。

##### 4.3.3 设置自动跳频

该命令同时打开频段自动模式和信道自动模式，由设备自动选择工作频段和信道。

```sh
./l4_link_config -B 1 -C 1
```

正常输出示例：

```text
[BB_SET_BAND_MODE]
auto_mode=1

[BB_SET_CHAN_MODE]
auto_mode=1
```

设置完成后，可查询频段和信道信息确认当前自动模式：

```sh
./l4_link_monitor -B
./l4_link_monitor -C
```

##### 4.3.4 设置 RX 方向频宽为 20 MHz

`-W 0` 表示使用手动频宽模式，`-w 4` 对应 `BB_BW_20M`，即 20 MHz。未指定 `-s` 时默认设置 slot 0。

```sh
./l4_link_config -W 0 -d rx -w 4
```

正常输出示例：

```text
[BB_SET_BANDWIDTH_MODE]
slot=0 mode=0

[BB_SET_BANDWIDTH]
slot=0 dir=RX(1) bandwidth=20M(4)
```

设置完成后，可查询设备状态，确认 `user phy status` 中 RX 方向的 `bw=20MHz(4)`：

```sh
./l4_link_monitor -S
```

```text
[BB_GET_STATUS]
...

user phy status:
  RX: user=0 source=rx freq=2400000KHz bw=20MHz(4) tintlv_enable=1 tintlv_len=3 tintlv_num=1
  TX: user=8 source=tx freq=5105625KHz mcs_raw=2 mcs_real=0 bw=10MHz(3) tintlv_enable=1 tintlv_len=3 tintlv_num=0
  bw_mode=Y24X2 major_dir=DEV->AP
```

##### 4.3.5 设置图传调制方式 MCS

`-M 0` 表示使用手动 MCS 模式，`-m 0` 表示将目标 MCS 设置为 0。未指定 `-s` 时默认设置 slot 0。

```sh
./l4_link_config -M 0 -m 0
```

正常输出示例：

```text
[BB_SET_MCS_MODE]
slot=0 auto_mode=0

[BB_SET_MCS]
slot=0 mcs=0
```

设置完成后，可查询设备状态，确认 `slot link status` 中 `rx_mcs_raw=0`：

```sh
./l4_link_monitor -S
```

```text
[BB_GET_STATUS]
...

slot link status:
  slot[0]: state=CONNECT(2), rx_mcs_raw=0, rx_mcs_real=-2, pair=0, peer_mac=65:a6:78:c7

...
```

##### 4.3.6 帧结构切换

默认大带宽方向为 `DEV->AP`。如果需要从 AP 向 DEV 发送大流量数据，例如 AP 上传文件到 DEV，可执行帧结构切换，将大带宽方向切到 `AP->DEV`。

`BB_SET_FRAME_CHANGE` 仅支持 `SINGLE_USER` 模式。示例程序会先读取 `BB_GET_STATUS`，如果当前不是 `SINGLE_USER` 模式，则不会下发帧结构切换命令。

执行帧结构切换：

```sh
./l4_link_config -F 1
```

正常输出示例：

```text
[BB_SET_FRAME_CHANGE]
mode=1
```

设置完成后，可查询设备状态，确认 `major_dir=AP->DEV`：

```sh
./l4_link_monitor -S
```

```text
[BB_GET_STATUS]
...

  bw_mode=Y12X1 major_dir=AP->DEV
```

恢复原始帧结构：

```sh
./l4_link_config -F 0
```

正常输出示例：

```text
[BB_SET_FRAME_CHANGE]
mode=0
```

设置完成后，可查询设备状态，确认 `major_dir=DEV->AP`：

```sh
./l4_link_monitor -S
```

```text
[BB_GET_STATUS]
...

  bw_mode=Y24X2 major_dir=DEV->AP
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

