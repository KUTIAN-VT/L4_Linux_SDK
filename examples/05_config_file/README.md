# 05_config_file 配置文件管理示例

`l4_config_file` 用于演示基带配置文件的导出、导入和恢复默认配置。它覆盖三个命令：

| 命令字 | 作用 |
| --- | --- |
| `BB_GET_CFG` | 分片读取设备配置文件并保存到本地 |
| `BB_SET_CFG` | 读取本地配置文件，计算 CRC 后分片写入设备 |
| `BB_RESET_CFG` | 恢复设备配置文件 |

## 常用命令

### 导出配置文件

```sh
./l4_config_file -g cfg.json
```

默认使用 `BB_GET_CFG` 的 `auto` 模式读取配置。也可以用 `-m` 指定读取来源：

```sh
./l4_config_file -m flash -g cfg_flash.json
```

### 写入配置文件

```sh
./l4_config_file -s cfg.json
```

程序会先读取本地文件，计算 `total_crc16`，再按 `bb_set_cfg_t.data` 的最大长度分片调用 `BB_SET_CFG`。

### 恢复配置文件

```sh
./l4_config_file -r
```

`BB_RESET_CFG` 会改变设备配置状态，示例程序必须显式传入 `-r` 才会执行。

## 参数说明

| 参数 | 作用 |
| --- | --- |
| `-a <addr>` | daemon 地址，默认 `127.0.0.1` |
| `-p <port>` | daemon 端口，默认 `BB_PORT_DEFAULT` |
| `-i <index>` | 设备序号，默认 `0` |
| `-g <file>` | 调用 `BB_GET_CFG`，导出配置到本地文件 |
| `-s <file>` | 调用 `BB_SET_CFG`，把本地文件写入设备 |
| `-r` | 调用 `BB_RESET_CFG`，恢复设备配置 |
| `-m <mode>` | `BB_GET_CFG` 读取模式，支持 `auto`、`memory`、`flash` 或 `0`、`1`、`2` |
| `-h` | 打印帮助信息 |

程序要求 `-g`、`-s`、`-r` 三个动作只能指定一个，避免一次命令同时执行多个配置文件操作。

## 输出日志格式

导出时会打印设备返回的总长度、总 CRC 和每个分片：

```text
[BB_GET_CFG]
mode=auto(0) output=cfg.json
total_length=4096 total_crc16=0x1234
chunk seq=0 offset=0 length=1012
dump config ok, bytes=4096 crc16=0x1234
```

写入时会打印本地文件长度、计算出的 CRC 和每个写入分片：

```text
[BB_SET_CFG]
input=cfg.json total_length=4096 total_crc16=0x1234
chunk seq=0 offset=0 length=1012
set config ok, bytes=4096 crc16=0x1234
```
