# 04_link_config 图传链路配置示例

`l4_link_config` 用于设置图传链路的频段和信道配置。它和 `03_link_monitor` 配套使用：`03_link_monitor` 负责读取链路状态，`04_link_config` 负责下发链路配置。

这个示例当前实现四个设置命令：

| 命令字 | 作用 |
| --- | --- |
| `BB_SET_BAND_MODE` | 设置频段自动/手动模式 |
| `BB_SET_BAND` | 手动设置目标工作频段 |
| `BB_SET_CHAN_MODE` | 设置信道自动/手动模式 |
| `BB_SET_CHAN` | 手动设置目标信道索引 |

## 常用命令

### 频段和信道都设置为自动

```sh
./l4_link_config -B 1 -C 1
```

这里 `-B 1` 表示频段自动，`-C 1` 表示信道自动。

### 手动设置目标频段

```sh
./l4_link_config -B 0 -b 2g
```

这里先把频段模式设置为手动，再把目标频段设置为 2G。

### 手动设置目标信道

```sh
./l4_link_config -C 0 -d rx -c 3
```

这里先把信道模式设置为手动，再把 RX 方向的目标信道索引设置为 `3`。

### 同时设置频段和信道

```sh
./l4_link_config -B 0 -b 2g -C 0 -d rx -c 3
```

程序会按固定顺序执行：先设置频段模式，再设置频段，然后设置信道模式，最后设置信道。

## 参数说明

| 参数 | 作用 |
| --- | --- |
| `-a <addr>` | daemon 地址，默认 `127.0.0.1` |
| `-p <port>` | daemon 端口，默认 `BB_PORT_DEFAULT` |
| `-i <index>` | 设备序号，默认 `0` |
| `-B <0|1>` | 调用 `BB_SET_BAND_MODE`，`1` 自动，`0` 手动 |
| `-b <band>` | 调用 `BB_SET_BAND`，支持 `1g`、`2g`、`5g` 或 `0`、`1`、`2` |
| `-C <0|1>` | 调用 `BB_SET_CHAN_MODE`，`1` 自动，`0` 手动 |
| `-c <index>` | 调用 `BB_SET_CHAN`，设置信道索引，范围 `0-255` |
| `-d <dir>` | `-c` 使用的信道方向，支持 `tx`、`rx` 或 `0`、`1`，默认 `rx` |
| `-h` | 打印帮助信息 |

如果没有指定任何设置动作，程序只打印帮助并返回错误，避免误操作。

## 参数和结构体字段的对应关系

### `BB_SET_BAND_MODE`

```c
bb_set_band_mode_t input;
input.auto_mode = auto_mode;
bb_ioctl(handle, BB_SET_BAND_MODE, &input, NULL);
```

| 参数 | 字段 | 含义 |
| --- | --- | --- |
| `-B 1` | `auto_mode = 1` | 频段自动 |
| `-B 0` | `auto_mode = 0` | 频段手动 |

### `BB_SET_BAND`

```c
bb_set_band_t input;
input.target_band = target_band;
bb_ioctl(handle, BB_SET_BAND, &input, NULL);
```

| 参数 | 字段值 | 含义 |
| --- | --- | --- |
| `-b 1g` 或 `-b 0` | `BB_BAND_1G` | 1G 频段 |
| `-b 2g` 或 `-b 1` | `BB_BAND_2G` | 2G 频段 |
| `-b 5g` 或 `-b 2` | `BB_BAND_5G` | 5G 频段 |

### `BB_SET_CHAN_MODE`

```c
bb_set_chan_mode_t input;
input.auto_mode = auto_mode;
bb_ioctl(handle, BB_SET_CHAN_MODE, &input, NULL);
```

| 参数 | 字段 | 含义 |
| --- | --- | --- |
| `-C 1` | `auto_mode = 1` | 信道自适应 |
| `-C 0` | `auto_mode = 0` | 信道手动 |

### `BB_SET_CHAN`

```c
bb_set_chan_t input;
input.chan_dir = chan_dir;
input.chan_index = chan_index;
bb_ioctl(handle, BB_SET_CHAN, &input, NULL);
```

| 参数 | 字段 | 含义 |
| --- | --- | --- |
| `-d tx` 或 `-d 0` | `chan_dir = BB_DIR_TX` | TX 方向 |
| `-d rx` 或 `-d 1` | `chan_dir = BB_DIR_RX` | RX 方向 |
| `-c <index>` | `chan_index = index` | 目标信道索引 |

`chan_index` 是否是设备支持的预置信道，由设备侧判断。示例程序只检查它是否在 `0-255` 范围内。

## 执行流程

执行：

```sh
./l4_link_config -B 0 -b 2g -C 0 -d rx -c 3
```

程序内部大致按下面流程运行：

```text
解析命令行参数
    |
检查参数范围
    |
连接 daemon 和指定设备
    |
BB_SET_BAND_MODE
    |
BB_SET_BAND
    |
BB_SET_CHAN_MODE
    |
BB_SET_CHAN
    |
关闭设备连接
```

任意一步 `bb_ioctl()` 返回非 0 时，程序会停止后续设置，关闭连接并返回错误。

## 和 `03_link_monitor` 配合使用

设置完成后，可以用 `03_link_monitor` 读取当前信道信息：

```sh
./l4_link_monitor -C
```

它会调用 `BB_GET_CHAN_INFO`，打印信道数量、自动模式、ACS 信道、工作信道和预置信道列表。

## 新手最容易混淆的点

### 自动模式和手动设置不是一回事

`-B 1` 或 `-C 1` 只是把设备切到自动模式。手动指定频段或信道时，通常要先设置 `-B 0` 或 `-C 0`。

### `-c` 是信道索引，不是频率值

`BB_SET_CHAN` 使用的是设备预置信道索引，不是 KHz 或 MHz 频率。可以先用 `l4_link_monitor -C` 查看设备返回的信道列表。

### `-d` 只影响 `BB_SET_CHAN`

`-d` 用于填写 `bb_set_chan_t.chan_dir`。它不会影响 `BB_SET_BAND` 或 `BB_SET_BAND_MODE`。

### 示例程序不会自动下发远端配置

这个示例只实现 `BB_SET_CHAN_MODE`、`BB_SET_CHAN`、`BB_SET_BAND_MODE`、`BB_SET_BAND`。它不会额外调用 `BB_SET_REMOTE`。

## 一句话总结

`l4_link_config` 做的事情就是：连接 daemon，打开指定 8030 设备，然后按参数下发频段模式、目标频段、信道模式和目标信道配置。
