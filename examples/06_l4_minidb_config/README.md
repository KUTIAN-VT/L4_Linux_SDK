# 06_l4_minidb_config MiniDB 配置示例

`l4_minidb_config` 用于演示通过 `BB_SET_PRJ_DISPATCH` 和 `BB_GET_PRJ_DISPATCH` 读写设备 MiniDB 持久化配置。

MiniDB 是持久化配置，不等同于当前运行态配置。写入后是否立即生效由设备侧决定，通常建议重启设备后再次查询确认。

## 常用命令

查询全部 MiniDB 信息：

```sh
./l4_minidb_config -A
```

查询 AP MAC：

```sh
./l4_minidb_config -m
```

查询 slot 0 MAC：

```sh
./l4_minidb_config -s
```

设置角色：

```sh
./l4_minidb_config -R ap
./l4_minidb_config -R dev
```

设置 AP MAC：

```sh
./l4_minidb_config -M 11:22:33:44
```

设置 slot MAC：

```sh
./l4_minidb_config -S 11:22:33:44
```

设置频段：

```sh
./l4_minidb_config -B auto
./l4_minidb_config -B 2g
./l4_minidb_config -B 5g
```

设置固定功率：

```sh
./l4_minidb_config -W 20
```

设置功率自适应区间：

```sh
./l4_minidb_config -W 10,27
```

重置 MiniDB：

```sh
./l4_minidb_config -D
```

设置成功后重启设备：

```sh
./l4_minidb_config -B auto -H
```

单独重启设备：

```sh
./l4_minidb_config -H
```

## 参数说明

公共参数：

| 参数 | 作用 | 默认值 |
| --- | --- | --- |
| `-a <addr>` | daemon 地址 | `127.0.0.1` |
| `-p <port>` | daemon 端口 | `BB_PORT_DEFAULT` |
| `-i <index>` | 设备序号 | `0` |
| `-h` | 打印帮助 | 无 |

查询参数：

| 短参数 | 长参数 | 作用 |
| --- | --- | --- |
| `-A` | `--get-all` | 查询 role、AP MAC、slot 0 MAC、band、power |
| `-r` | `--get-role` | 查询 role |
| `-m` | `--get-ap-mac` | 查询 AP MAC |
| `-s [slot]` | `--get-slot-mac[=slot]` | 查询 slot MAC，默认 slot 0 |
| `-b` | `--get-band` | 查询 band |
| `-w` | `--get-pwr` | 查询 power |

设置参数：

| 短参数 | 长参数 | 作用 |
| --- | --- | --- |
| `-R <ap|dev|0|1>` | `--set-role <...>` | 设置 role |
| `-M <mac>` | `--set-ap-mac <mac>` | 设置 AP MAC |
| `-S <mac|slot,mac>` | `--set-slot-mac <mac|slot,mac>` | 设置 slot MAC，默认 slot 0 |
| `-B <auto|2g|5g|0x07>` | `--set-band <...>` | 设置 band bitmap |
| `-W <pwr|min,max>` | `--set-pwr <pwr|min,max>` | 设置固定功率或功率自适应区间 |
| `-D` | `--reset` | 重置 MiniDB |

其它参数：

| 参数 | 作用 |
| --- | --- |
| `-H` / `--reboot` | 单独重启设备，或设置/重置成功后重启设备 |

## 输入格式

MAC 支持两种格式。当前 SDK 中 `BB_MAC_LEN = 4`，因此 AP MAC 和 slot MAC 都按 4 字节输入：

```text
11:22:33:44
11223344
```

功率设置约定：

| 输入 | 行为 |
| --- | --- |
| `-W 20` | 设置固定功率，写入 `pwr_auto=0`、`pwr_init=20` |
| `-W 10,27` | 设置自适应功率区间，写入 `pwr_auto=1`、`pwr_min=10`、`pwr_max=27` |

固定功率和功率区间的合法范围都是 `10-27`。`-W` 只能选择一种格式：单个值表示固定功率，`min,max` 表示功率范围。

band bitmap 约定：

| 输入 | bitmap |
| --- | --- |
| `auto` | `0x07` |
| `2g` | `0x02` |
| `5g` | `0x04` |

每次运行只允许一个主动作，例如不能同时指定 `-A` 和 `-B auto`。`-W` 的固定功率和功率范围格式互斥。`-s` 和 `-S <mac>` 默认使用 slot 0；非 0 slot 可用 `-s 1`、`--get-slot-mac=1` 或 `-S 1,11:22:33:44`。`-H/--reboot` 可以单独作为重启动作，也可以附加在设置或重置动作后。
