# 10_l4_socket_encrypt Socket 实时与持久加密示例

`l4_socket_encrypt` 默认使用 `BB_SET_FORCE_UPD_SOCKET_ENC` 和 `BB_GET_SOCKET_ENCRYPT` 更新或查询运行态 socket；增加 `--minidb` 后，改用项目 RPC 读写 MiniDB 持久配置。两种配置相互独立，`--minidb` 不会立即修改当前 socket，配置在设备重启或相关 socket 重建时生效。默认操作本端，增加 `-R` 后通过 `BB_REMOTE_IOCTL_REQ` 在对端执行。

远程模式只操作对端，不同时修改本端。通信两端必须对同一 port 使用相同的加密模式和密钥，否则数据无法正常解密。

运行前请启动 `l4_daemon`，确认设备和无线链路正常，并确保实际执行端上的目标 socket port 已经创建。

## 常用命令

查询本端 slot 0、port 1 的当前加密配置：

```sh
./l4_socket_encrypt -P 1 -g
```

通过 remote slot 0 查询对端内部 slot 0、port 1：

```sh
./l4_socket_encrypt -R 0 -s 0 -P 1 -g
```

保存本端 slot 0、port 1 的 AES128 持久配置：

```sh
./l4_socket_encrypt --minidb -s 0 -P 1 -e aes128 \
  -k 0102030405060708090a0b0c0d0e0f10
```

查询同一条 MiniDB 配置；AES 模式会输出完整密钥：

```sh
./l4_socket_encrypt --minidb -s 0 -P 1 -g
```

从 AP 经 remote slot 0 保存对端持久配置：

```sh
./l4_socket_encrypt -R 0 --minidb -s 0 -P 1 -e aes128 \
  -k 0102030405060708090a0b0c0d0e0f10
```

为 slot 0、port 1 启用默认加密：

```sh
./l4_socket_encrypt -P 1 -e default
```

为 slot 0、port 1 实时切换为 AES128：

```sh
./l4_socket_encrypt -P 1 -e aes128 \
  -k 0102030405060708090a0b0c0d0e0f10
```

为 slot 1、port 2 实时切换为 AES256：

```sh
./l4_socket_encrypt -s 1 -P 2 -e aes256 \
  -k 0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20
```

从 AP 远程配置 slot 0 对端设备内部的 slot 0、port 1：

```sh
./l4_socket_encrypt -R 0 -s 0 -P 1 -e aes128 \
  -k 0102030405060708090a0b0c0d0e0f10
```

在 1v1 场景中，经 remote slot 0 配置对应 DEV；在 DEV 上，代表 AP 的目标 socket slot 也是 0：

```sh
./l4_socket_encrypt -R 0 -s 0 -P 1 -e aes128 \
  -k 0102030405060708090a0b0c0d0e0f10
```

关闭 slot 0、port 1 的加密：

```sh
./l4_socket_encrypt -P 1 -d
```

## 参数说明

公共参数：

| 短参数 | 长参数 | 作用 | 默认值 |
| --- | --- | --- | --- |
| `-h` | `--help` | 打印帮助 | 无 |
| `-a <addr>` | `--addr <addr>` | daemon 地址 | `127.0.0.1` |
| `-p <port>` | `--port <port>` | daemon 端口 | `BB_PORT_DEFAULT` |
| `-i <index>` | `--index <index>` | 设备序号 | `0` |
| `-R [slot]` | `--remote[=<slot>]` | 通过 remote ioctl 在对端执行，remote slot 默认 `0` | 不启用 |
| `-s <slot>` | `--slot <slot>` | 目标 slot，DEV 侧 `0` 表示 AP | `0` |
| `-P <port>` | `--socket-port <port>` | 目标 socket port，必须指定 | 无 |
| `-g` | `--get` | 查询目标 port 的加密配置 | 不启用 |
| 无 | `--minidb` | 改为读写 MiniDB 持久配置，不更新当前 socket | 不启用 |

操作参数：

| 短参数 | 长参数 | 作用 |
| --- | --- | --- |
| `-e <mode>` | `--encrypt-mode <mode>` | 启用或更新加密，可选 `default`、`aes128`、`aes256` |
| `-d` | `--disable` | 关闭目标 port 的加密 |
| `-k <hex>` | `--encrypt-key <hex>` | AES 密钥；AES128 为 16 字节，AES256 为 32 字节 |

查询时单独使用 `-g`；设置时 `-e` 和 `-d` 必须且只能指定一个。`-g` 不能与 `-e`、`-d` 或 `-k` 同时使用。`default` 模式和 `-d` 不使用 key；AES128 和 AES256 必须提供长度完全匹配的十六进制 key。key 支持连续十六进制字符，也支持空格、冒号或逗号分隔的字节。

`--minidb` 是配置后端选择器，必须与 `-g`、`-e` 或 `-d` 之一组合。MiniDB 只支持 port `0` 到 `BB_CONFIG_MAX_TRANSPORT_PER_SLOT - 1`，当前配置为 `0-3`；未保存过的项显示 `state=unset` 并正常退出。持久设置成功只表示已经写入 MiniDB，不表示当前 socket 已立即更新。

`-R` 只操作对端，默认 remote slot 为 0；1v1 场景统一写成 `-R 0 -s 0`。其中 `-R 0` 写入外层 `BB_REMOTE_IOCTL_REQ.slot`，`-s 0` 写入命令实际执行端内部的目标 socket slot，两者语义独立。

## 工作方式

例程组装以下参数并执行一次 `bb_ioctl()`：

```c
bb_set_force_upd_socket_enc_t request = {0};

request.slot = slot;
request.port = socket_port;
request.sock_upd_enc.encrypt_en = 1;
request.sock_upd_enc.encrypt_mode = BB_SOCK_ENCRYPT_MODE_AES128;
memcpy(request.sock_upd_enc.key, key, 16);

bb_ioctl(handle, BB_SET_FORCE_UPD_SOCKET_ENC, &request, NULL);
```

查询操作使用独立的输入输出结构，不返回或打印密钥：

```c
bb_get_socket_enc_in_t input = { .slot = slot, .port = socket_port };
bb_get_socket_enc_out_t output = {0};

bb_ioctl(handle, BB_GET_SOCKET_ENCRYPT, &input, &output);
```

设置操作的远程模式将同一个 `request` 封装进 `BB_REMOTE_IOCTL_REQ`：

```c
bb_remote_ioctl_in_t remote_in = {0};

bb_remote_ioctl_out_t remote_out = {0};
remote_in.slot = remote_slot;
remote_in.len = sizeof(request);
remote_in.msg_id = BB_SET_FORCE_UPD_SOCKET_ENC;
memcpy(remote_in.data, &request, sizeof(request));

bb_ioctl(handle, BB_REMOTE_IOCTL_REQ, &remote_in, &remote_out);
```

查询操作采用相同的远程封装，并从 `remote_out.data` 读取 `bb_get_socket_enc_out_t`。

MiniDB 模式把 `prj_cmd_set_sock_encrypt_t` 或 `prj_cmd_get_sock_encrypt_in_t` 放入 `prj_rpc_hdr_t`，分别通过 `BB_SET_PRJ_DISPATCH` 和 `BB_GET_PRJ_DISPATCH` 发送。AES128 查询输出 16 字节密钥，AES256 查询输出 32 字节密钥；default 或关闭状态显示 `key=none`。

不带 `-R` 时只操作本端；带 `-R` 时只操作对端，不会同时修改本端。两端需要分别使用相同的 port、mode 和 key。运行态查询不返回密钥，MiniDB 查询按用户选择打印完整有效密钥；所有请求和响应中的密钥缓存会在使用后清零。完整密钥会进入终端、录屏或日志，生产环境应限制输出访问。
