# SDK 用例脚本

`use_cases/` 用于存放 SDK 用例脚本。

## 固件升级

如果 `l4_ota_upgrade` 和 `l4_config_file` 所在目录已加入 `PATH`，直接执行：

```sh
./use_cases/l4_upgrade.sh <firmware.img>
```

如果可执行文件没有加入 `PATH`，通过 `L4_BIN_DIR` 指定其所在目录：

```sh
L4_BIN_DIR=./install/x86_64/bin ./use_cases/l4_upgrade.sh <firmware.img>
```

`L4_BIN_DIR` 必须同时包含可执行的 `l4_daemon`、`l4_ota_upgrade` 和 `l4_config_file`。未设置该变量时，脚本从 `PATH` 查找这些工具。

## 操作流程及必要性

脚本严格按照以下顺序执行：

| 顺序 | 操作 | 命令 | 必要性 |
| --- | --- | --- | --- |
| 1 | 检查并启动 daemon | `l4_daemon` | `l4_ota_upgrade` 和 `l4_config_file` 都通过 daemon 与设备通信；没有 daemon 时无法访问设备。 |
| 2 | 升级固件 | `l4_ota_upgrade -f <firmware.img>` | 把指定固件镜像写入设备，是升级流程的核心步骤。写入失败时不能继续恢复配置或重启。 |
| 3 | 恢复出厂设置 | `l4_config_file -r` | 清除旧固件遗留且可能与新固件不兼容的配置，保证新固件使用默认配置启动。 |
| 4 | 重启设备 | `l4_config_file -H` | `l4_ota_upgrade` 只写入固件，不会主动重启；重启后新固件和默认配置才正式生效。 |

执行升级前，脚本通过 `/proc` 检查 `l4_daemon` 是否正在运行。未运行时，脚本使用当前用户在后台启动 daemon，等待一秒并确认进程仍然存活。脚本自动启动的 daemon 不保存日志，标准输出和错误输出都会写入 `/dev/null`。如果设备访问需要管理员权限，请使用具备相应权限的用户运行整个脚本。

脚本使用 `set -e`，任一步骤执行失败都会立即停止，不再执行后续操作。设备连接参数使用各工具的默认值，脚本自动启动的 daemon 在升级结束后继续运行，脚本不会停止升级前已经存在的 daemon。
