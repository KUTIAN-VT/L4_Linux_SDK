# 功率强发例程

`l4_power_forced_tx` 用于让本机 8030 设备在指定频点和功率下进入功率强发测试模式。例程固定使用物理用户 `BB_USER_BR_CS` 和 TX 方向。

> 警告：该例程会使基带进入 debug mode，正常业务将停止。程序退出后设备仍保持 debug/功率测试模式，不会自动恢复；请仅在射频测试环境中使用。

## 编译

在 SDK 根目录执行：

```sh
cmake -S . -B build
cmake --build build --target l4_power_forced_tx
```

## 参数

| 参数 | 说明 | 默认值 |
| --- | --- | --- |
| `-f <freq_khz>` | 发射频点，单位 KHz，必须显式指定 | 无 |
| `-P <power_dbm>` | 发射功率，范围 `0-31 dBm`，必须显式指定 | 无 |
| `-a <addr>` | daemon 地址 | `127.0.0.1` |
| `-p <port>` | daemon 端口 | `50000` |
| `-i <index>` | 本机设备索引 | `0` |
| `-h` | 显示帮助 | 无 |

## 使用示例

以 `5100000 KHz`、`27 dBm` 启动功率强发：

```sh
./l4_power_forced_tx -f 5100000 -P 27
```

将发射频点改为 `2.41 GHz`：

```sh
./l4_power_forced_tx -f 2410000 -P 27
```

## 命令执行顺序

例程打开设备后，每条命令之间等待约 200 ms，并依次执行：

1. `BB_SET_DBG_MODE(enable=1)`：进入调试模式。
2. `BB_SET_POWER_AUTO(pwr_auto=0)`：关闭功率自适应。
3. `BB_SET_FREQ(user=BB_USER_BR_CS, dir_bmp=1 << BB_DIR_TX)`：设置发射频点。
4. `BB_SET_POWER(usr=BB_USER_BR_CS)`：设置发射功率。
5. `BB_SET_POWER_TEST_MODE`：进入功率强发测试模式。

任一步骤失败后，程序会停止执行后续命令、释放主机侧资源并返回失败。
