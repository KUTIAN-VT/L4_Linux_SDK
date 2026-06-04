# BB API 说明

本文档整理 `com/bb_api.h` 中暴露的 Artosyn 8030 基带 SDK API，按功能分类说明常用函数、`bb_ioctl` 命令字、输入输出结构体和数据通道接口。

## 1. API 使用模型

SDK 主要分为三类接口：

| 类型 | 入口 | 说明 |
| --- | --- | --- |
| 设备与系统控制 | `bb_host_connect`、`bb_dev_getlist`、`bb_dev_open`、`bb_init`、`bb_start` | 连接 daemon/host，枚举并打开 8030 设备，启动或停止基带业务 |
| 统一控制面 | `bb_ioctl`、`bb_ioctl_ex` | 通过 `BB_CFG_*`、`BB_GET_*`、`BB_SET_*` 等命令字配置、查询、控制基带 |
| 数据通道 | `bb_socket_open`、`bb_socket_read`、`bb_socket_write`、`bb_socket_ioctl` | 打开指定 slot/port 的数传 socket，进行业务数据收发 |

典型调用顺序：

```c
bb_host_t *host = NULL;
bb_dev_list_t *list = NULL;

bb_host_connect(&host, "127.0.0.1", BB_PORT_DEFAULT);
int count = bb_dev_getlist(host, &list);
bb_dev_handle_t *dev = bb_dev_open(list[0]);

/* 根据角色调用 BB_CFG_* 配置 */
bb_init(dev);
bb_start(dev);

/* 使用 bb_ioctl 查询状态，或使用 bb_socket_* 收发数据 */

bb_stop(dev);
bb_deinit(dev);
bb_dev_close(dev);
bb_dev_freelist(list);
bb_host_disconnect(host);
```

## 2. 通用类型和命令字

### 2.1 句柄类型

| 类型 | 来源 | 用途 |
| --- | --- | --- |
| `bb_host_t` | `bb_host_connect` | 表示连接到的远程 host/daemon |
| `bb_dev_t` | `bb_dev_getlist` | 表示物理接口上的一个 8030 设备 |
| `bb_dev_handle_t` | `bb_dev_open` | 表示已打开的 8030 设备句柄 |
| `bb_dev_info_t` | `bb_dev_getinfo` | 返回设备 ID、状态、MAC 等信息 |

### 2.2 命令字编码

`bb_ioctl` 的 `request` 由请求大类和序号组成：

```c
#define BB_REQUEST(type, order) ((type) << 24 | (order))
#define BB_REQUEST_TYPE(req)    ((req) >> 24)
```

| 大类 | 值 | 说明 |
| --- | ---: | --- |
| `BB_REQ_CFG` | `0` | 初始化阶段配置类命令 |
| `BB_REQ_GET` | `1` | 状态、信息读取类命令 |
| `BB_REQ_SET` | `2` | 运行时设置和控制类命令 |
| `BB_REQ_CB` | `3` | callback 设置类命令 |
| `BB_REQ_SOCKET` | `4` | socket 控制类命令 |
| `BB_REQ_DBG` | `5` | debug 控制类命令 |
| `BB_REQ_REMOTE` | `6` | 远端 ioctl 分发命令 |
| `BB_REQ_RPC` | `10` | host RPC 命令 |
| `BB_REQ_RPC_IOCTL` | `11` | RPC 内部 ioctl 命令 |
| `BB_REQ_PLAT_CTL` | `12` | 平台控制命令 |

## 3. 设备与系统控制函数

| API | 功能 | 关键参数 | 返回值 |
| --- | --- | --- | --- |
| `bb_host_connect(bb_host_t **phost, const char *addr, int port)` | 连接远程 bb host/daemon | `addr` 为 host IP，`port` 为端口 | `0` 成功，`<0` 失败 |
| `bb_host_disconnect(bb_host_t *phost)` | 断开 host 连接 | `phost` 为连接句柄 | `0` 成功，`<0` 失败 |
| `bb_host_connect_test(const char *addr, int port)` | 测试 host 是否可连接 | host IP 和端口 | `0` 成功，`<0` 失败 |
| `bb_dev_getlist(bb_host_t *phost, bb_dev_list_t **plist)` | 获取 host 侧 8030 设备列表 | `plist` 输出设备列表 | `>0` 为设备数量，`<0` 失败 |
| `bb_dev_freelist(bb_dev_list_t *plist)` | 释放设备列表 | `bb_dev_getlist` 返回的列表 | `0` 成功，`<0` 失败 |
| `bb_dev_open(bb_dev_t *devs)` | 打开指定 8030 设备 | `devs` 来自设备列表 | 非空成功，`NULL` 失败 |
| `bb_dev_close(bb_dev_handle_t *handle)` | 关闭设备 | 已打开设备句柄 | `0` 成功，`<0` 失败 |
| `bb_dev_getinfo(bb_dev_t *pdev, bb_dev_info_t *dev_info)` | 读取设备信息 | 设备对象和输出结构 | `0` 成功，`<0` 失败 |
| `bb_init(bb_dev_handle_t *handle)` | 初始化基带软件 | 应在配置类命令后调用 | `0` 成功，非零失败 |
| `bb_deinit(bb_dev_handle_t *handle)` | 反初始化基带软件 | 已打开设备句柄 | `0` 成功，非零失败 |
| `bb_start(bb_dev_handle_t *handle)` | 启动基带工作 | 必须在 `bb_init` 后调用 | `0` 成功，`<0` 失败，`>0` 表示可用设备数量 |
| `bb_stop(bb_dev_handle_t *handle)` | 停止基带工作 | 已打开设备句柄 | `0` 成功，非零失败 |
| `bb_dev_reg_hotplug_cb(bb_host_t *phost, bb_event_callback cbfun, void *priv)` | 注册 host 侧热插拔回调 | 回调函数和用户私有数据 | `0` 成功，`<0` 失败 |

## 4. `bb_ioctl` 统一控制接口

```c
int bb_ioctl(
    bb_dev_handle_t *dev,
    uint32_t request,
    const void *in,
    void *out
);

int bb_ioctl_ex(
    bb_dev_handle_t *dev,
    uint32_t request,
    const void *input,
    void *output,
    int timeout
);
```

| 参数 | 说明 |
| --- | --- |
| `dev` | 目标 8030 设备句柄；本地调用场景可为 `NULL`，RPC/host 场景一般传 `bb_dev_open` 返回的句柄 |
| `request` | `BB_CFG_*`、`BB_GET_*`、`BB_SET_*` 等命令字 |
| `in` / `input` | 命令对应输入结构体；无输入时可为 `NULL` |
| `out` / `output` | 命令对应输出结构体；配置和设置类命令通常可为 `NULL` |
| `timeout` | `bb_ioctl_ex` 的超时时间，单位 ms |

## 5. 初始化配置类 API

配置类命令通常在 `bb_init` 前调用，用于确定角色、信道、物理用户、MCS、功率等基带初始参数。

| 命令字 | 输入结构 | 输出结构 | 功能 |
| --- | --- | --- | --- |
| `BB_CFG_AP_BASIC` | `bb_conf_ap_t` | 无 | AP 角色基础配置：本机 MAC、时钟、初始 MCS、基带模式、slot 模式、有效 slot bitmap、天线数量 |
| `BB_CFG_DEV_BASIC` | `bb_conf_dev_t` | 无 | DEV 角色基础配置：目标 AP MAC、DEV MAC、时钟、初始 MCS、天线数量 |
| `BB_CFG_CHANNEL` | `bb_conf_chan_t` | 无 | 信道配置：频段模式、初始频段、初始信道、频点表、扫频带宽、跳频策略、子信道和频段自适应参数 |
| `BB_CFG_CANDIDATES` | `bb_conf_candidates_t` | 无 | AP 候选 DEV MAC 表配置，可指定 slot 或允许不固定 slot |
| `BB_CFG_USER_PARA` | `bb_conf_user_para_t` | 无 | 物理用户 TX/RX 参数配置，包括 payload、FCH 长度、交织、带宽、重传、TX/RX 模式 |
| `BB_CFG_SLOT_RX_MCS` | `bb_conf_mcs_t` | 无 | 指定 slot 的 MCS 策略表配置，支持手动/自动切换和自动带宽参数 |
| `BB_CFG_ANY_CHANNEL` | 头文件未定义专用结构 | 无 | 任意工作信道配置，依赖 `BB_CONFIG_ENABLE_ANY_CHANNEL` |
| `BB_CFG_DISTC` | `bb_conf_distc_t` | 无 | 测距功能配置：使能、平均窗口、超时阈值、校准 offset |
| `BB_CFG_AP_SYNC_MODE` | `bb_conf_ap_sync_mode_t` | 无 | AP 同步模式配置：使能、主从身份、同步 pin group/port |
| `BB_CFG_BR_HOP_POLICY` | `bb_conf_br_hop_policy_t` | 无 | BR 跳频策略配置：固定、跟随上行信道、IDLE 跳频等 |
| `BB_CFG_PWR_BASIC` | `bb_phy_pwr_basic_t` | 无 | 发射功率基础配置：开环/闭环、初始功率、自适应开关、功率范围 |
| `BB_CFG_RC_HOP_POLICY` | `bb_conf_rc_hop_policy_t` | 无 | 选择性跳频策略配置 |
| `BB_CFG_POWER_SAVE` | `bb_conf_power_save_t` | 无 | 节能模式配置，支持手动周期和自动策略 |
| `BB_CFG_LNA` | `bb_conf_lna_t` | 无 | LNA 策略配置，依赖 `BB_CONFIG_ENABLE_LNA_POLICY` |
| `BB_CFG_RF_POLICY` | `bb_conf_rf_policy_t` | 无 | RF B 路自适应策略配置 |
| `BB_CFG_SHARE_SLOT` | `bb_conf_share_slot_t` | 无 | 共享时隙配置，依赖 `BB_CONFIG_ENABLE_SHARE_SLOT` |
| `BB_CFG_AGC` | `bb_conf_agc_t` | 无 | 扫频校准参数配置 |
| `BB_CFG_AGIN` | `bb_conf_gain_t` | 无 | RSSI/gain 校准参数配置 |

## 6. 状态与信息查询 API

查询类命令通常通过 `bb_ioctl(dev, BB_GET_*, in, out)` 调用。只读命令的输出结构必须由调用方分配。

### 6.1 运行状态与链路质量

| 命令字 | 输入结构 | 输出结构 | 功能 |
| --- | --- | --- | --- |
| `BB_GET_STATUS` | `bb_get_status_in_t` | `bb_get_status_out_t` | 读取基带整体状态，包括角色、模式、同步状态、MAC、物理用户状态和 slot 链路状态 |
| `BB_GET_PAIR_RESULT` | 无 | `bb_get_pair_out_t` | 读取配对结果、成功 slot bitmap、对端 MAC、配对时无线质量 |
| `BB_GET_USER_QUALITY` | `bb_get_user_quality_in_t` | `bb_get_user_quality_out_t` | 按物理用户 bitmap 获取信号质量，支持实时或平均值 |
| `BB_GET_PEER_QUALITY` | `bb_get_peer_quality_in_t` | `bb_get_peer_quality_out_t` | 获取对端 slot 的数据通道信号质量 |
| `BB_GET_DISTC_RESULT` | `bb_get_distc_result_in_t` | `bb_get_distc_result_out_t` | 获取指定 slot 的测距结果 |
| `BB_GET_MCS` | `bb_get_mcs_in_t` | `bb_get_mcs_out_t` | 获取指定 slot 和方向的当前 MCS 及理论吞吐率 |
| `BB_GET_THROUGHPUT` | `bb_get_throughput_in_t` | `bb_get_throughput_out_t` | 获取指定 slot、方向的实时物理吞吐率和实际承载吞吐率 |

`BB_GET_STATUS` 重点字段：

| 字段 | 说明 |
| --- | --- |
| `role` | 当前角色，取值参考 `bb_role_e` |
| `mode` | 当前基带模式，取值参考 `bb_mode_e` |
| `cfg_sbmp` | 配置阶段有效 slot bitmap |
| `rt_sbmp` | 运行时有效 slot bitmap |
| `mac` | 本机 MAC |
| `user_status[]` | 每个物理用户的 TX/RX 物理状态 |
| `link_status[]` | 每个 slot 的链路状态、RX MCS、pair 状态和对端 MAC |

### 6.2 信道、频段、功率和 RF 信息

| 命令字 | 输入结构 | 输出结构 | 功能 |
| --- | --- | --- | --- |
| `BB_GET_CHAN_INFO` | 无 | `bb_get_chan_info_out_t` | 获取信道数量、自适应模式、ACS 信道、当前工作信道、频点表和扫频能量 |
| `BB_GET_BAND_INFO` | `bb_get_band_info_in_t` | `bb_get_band_info_out_t` | 获取频段模式和当前工作频段 |
| `BB_GET_POWER_MODE` | 无 | `bb_get_pwr_mode_out_t` | 获取发射功率开环/闭环模式 |
| `BB_GET_CUR_POWER` | `bb_get_cur_pwr_in_t` | `bb_get_cur_pwr_out_t` | 获取指定用户当前发射功率 |
| `BB_GET_POWER_AUTO` | 无 | `bb_get_pwr_auto_out_t` | 获取功率自适应是否开启 |
| `BB_GET_RF` | 无 | `bb_get_rf_out_t` | 获取 A/B 路 RF TX/RX 开关状态 |
| `BB_GET_POWER_OFFSET_SAVE` | `bb_get_power_offset_in_t` | `bb_get_power_offset_out_t` | 读取功率补偿，用于功率测试校验 |
| `BB_GET_FACTORY_POWER_OFFSET_SAVE` | `bb_get_power_offset_in_t` | `bb_get_power_offset_out_t` | 读取 flash 中的工厂功率补偿 |
| `BB_GET_POWER_OFFSET2` | `bb_get_power_offset2_in_t` | `bb_get_power_offset2_out_t` | 获取 A/B 路功率补偿值 |
| `BB_GET_POWER_SAVE_MODE` | 无 | `bb_get_power_save_mode_t` | 获取 1V1 低功耗策略模式 |
| `BB_GET_POWER_SAVE` | 无 | `bb_get_power_save_t` | 获取 1V1 低功耗手动周期 |

`BB_GET_CHAN_INFO` 重点字段：

| 字段 | 说明 |
| --- | --- |
| `chan_num` | 当前可用信道数量 |
| `auto_mode` | `1` 表示信道自适应，`0` 表示手动 |
| `acs_chan` | ACS 启动选频得到的信道 |
| `work_chan` | 当前工作信道；不同角色/模式下含义可能不同，通常表示当前收信号信道 |
| `freq[]` | 频点表，单位 KHz |
| `power[]` | 扫频均化能量，单位 dbm |

### 6.3 设备、系统、版本和配置文件

| 命令字 | 输入结构 | 输出结构 | 功能 |
| --- | --- | --- | --- |
| `BB_GET_AP_MAC` | 无 | `bb_get_ap_mac_out_t` | DEV 读取目标 AP MAC |
| `BB_GET_CANDIDATES` | `bb_get_candidates_in_t` | `bb_get_candidates_out_t` | AP 读取指定 slot 的候选人 MAC 表 |
| `BB_GET_AP_TIME` | 无 | `bb_get_ap_time_out_t` | 获取 AP 时间戳，单位 ms |
| `BB_GET_REMOTE` | `bb_get_remote_in_t` | `bb_get_remote_out_t` | 获取通讯对端配置 |
| `BB_GET_SOCK_INFO` | `bb_get_sock_info_in_t` | `bb_get_sock_info_out_t` | 获取指定 slot/port 或全部 socket 的 buffer 和统计信息 |
| `BB_GET_HARDWARE_VERSION` | 头文件未定义专用结构 | 头文件未定义专用结构 | 获取硬件版本 |
| `BB_GET_FIRMWARE_VERSION` | 头文件未定义专用结构 | 头文件未定义专用结构 | 获取固件版本 |
| `BB_GET_SYS_INFO` | 无 | `bb_get_sys_info_out_t` | 获取系统运行时间、编译时间、软件/硬件/固件版本 |
| `BB_GET_USER_INFO` | `bb_get_user_info_in_t` | `bb_get_user_info_out_t` | 获取指定物理用户信息，如频偏 |
| `BB_GET_1V1_INFO` | `bb_get_1v1_info_in_t` | `bb_get_1v1_info_out_t` | 获取 1V1 模式 self/peer 信息 |
| `BB_GET_RUN_SYS` | 无 | `bb_get_runsys_out_t` | 获取系统当前运行 app，取值参考 `bb_runsys_t` |
| `BB_GET_CUSTOMER_KEY` | 无 | `bb_get_customer_key_out_t` | 读取 customer key |
| `BB_GET_BOOT_REASON` | 无 | `bb_get_boot_reason_out_t` | 获取重启原因、PC、RA、额外异常信息和描述 |
| `BB_GET_CFG` | `bb_get_cfg_in_t` | `bb_get_cfg_out_t` | 分页读取基带配置文件 |
| `BB_GET_REG` | `bb_get_reg_in_t` | `bb_get_reg_out_t` | 读取基带寄存器，主要用于调试诊断 |
| `BB_GET_DBG_MODE` | 无 | `bb_get_dbg_mode_out_t` | 获取 SDK debug mode 状态 |
| `BB_GET_PRJ_DISPATCH` | `bb_get_prj_dispatch_in_t` | `bb_get_prj_dispatch_out_t` | 项目二级 GET 命令分发 |

## 7. 运行时设置与控制 API

### 7.1 事件、配对和接入控制

| 命令字 | 输入结构 | 输出结构 | 功能 |
| --- | --- | --- | --- |
| `BB_SET_EVENT_SUBSCRIBE` | `bb_set_event_callback_t` | 无 | 订阅指定事件，并设置事件回调 |
| `BB_SET_EVENT_UNSUBSCRIBE` | `bb_set_event_callback_t` | 无 | 反订阅指定事件 |
| `BB_SET_PAIR_MODE` | `bb_set_pair_mode_t` | 无 | 设置指定 slot 进入或退出配对模式 |
| `BB_SET_AP_MAC` | `bb_set_ap_mac_t` | 无 | DEV 设置目标 AP MAC |
| `BB_SET_LOCAL_MAC` | `bb_set_local_mac_t` | 无 | 设置本机运行时 MAC |
| `BB_SET_CANDIDATES` | `bb_set_candidate_t` | 无 | AP 设置候选人 MAC 表 |
| `BB_SET_MASTER_DEV` | `bb_set_master_dev_t` | 无 | 设置导演模式下的主 DEV，仅 AP 侧支持 |

### 7.2 信道、频段、带宽和 DFS

| 命令字 | 输入结构 | 输出结构 | 功能 |
| --- | --- | --- | --- |
| `BB_SET_CHAN_MODE` | `bb_set_chan_mode_t` | 无 | 设置信道自适应或手动模式 |
| `BB_SET_CHAN` | `bb_set_chan_t` | 无 | 手动设置信道 |
| `BB_SET_COMPLIANCE_MODE` | `bb_set_compliance_mode_t` | 无 | 设置合规模式 |
| `BB_SET_BAND_MODE` | `bb_set_band_mode_t` | 无 | 设置频段自动/手动切换模式 |
| `BB_SET_BAND` | `bb_set_band_t` | 无 | 设置目标工作频段 |
| `BB_SET_BANDWIDTH` | `bb_set_bandwidth_t` | 无 | 1V1 模式下手动改变 bandwidth |
| `BB_SET_BANDWIDTH_MODE` | `bb_set_bandwidth_mode_t` | 无 | 设置频宽自动/手动模式 |
| `BB_SET_DFS` | `bb_set_dfs_t` | 无 | 设置 DFS 检测配置或触发 DFS 动作 |
| `BB_SET_FRAME_CHANGE` | `bb_set_frame_change_t` | 无 | 运行中改变帧结构，仅 1V1 模式 |

### 7.3 MCS、功率、RF、LNA 和低功耗

| 命令字 | 输入结构 | 输出结构 | 功能 |
| --- | --- | --- | --- |
| `BB_SET_MCS_MODE` | `bb_set_mcs_mode_t` | 无 | 设置指定 slot 的 MCS 自动/手动模式 |
| `BB_SET_MCS` | `bb_set_mcs_t` | 无 | 手动设置指定 slot 的 MCS 挡位 |
| `BB_SET_MCS_RANGE` | `bb_set_mcs_range_in_t` | 无 | 设置 MCS 生效范围 |
| `BB_SET_POWER_MODE` | `bb_set_pwr_mode_in_t` | 无 | 设置发射功率开环/闭环模式 |
| `BB_SET_POWER` | `bb_set_pwr_in_t` | 无 | 设置指定用户发射功率，功率范围按头文件注释为 `[0,31dbm]` |
| `BB_SET_POWER_AUTO` | `bb_set_pwr_auto_in_t` | 无 | 使能或关闭功率自适应 |
| `BB_SET_POWER_RANGE` | `bb_set_power_range_in_t` | 无 | 设置功率范围和方向 mask |
| `BB_SET_RF` | `bb_set_rf_t` | 无 | 动态控制 A/B 路 RF TX/RX 开关 |
| `BB_SET_LNA_MODE` | `bb_set_lna_mode_t` | 无 | 设置 LNA 自动/手动模式 |
| `BB_SET_LNA` | `bb_set_lna_t` | 无 | 设置 LNA bypass 状态 |
| `BB_SET_POWER_SAVE_MODE` | `bb_set_power_save_mode_t` | 无 | 设置 1V1 低功耗策略模式 |
| `BB_SET_POWER_SAVE` | `bb_set_power_save_t` | 无 | 设置 1V1 低功耗手动周期 |
| `BB_SET_REMOTE` | `bb_set_remote_t` | 无 | 设置通讯对端配置，如频段、信道、合规模式、功率 |

### 7.4 系统、升级和客户数据

| 命令字 | 输入结构 | 输出结构 | 功能 |
| --- | --- | --- | --- |
| `BB_SET_HOT_UPGRADE_WRITE` | `bb_set_hot_upgrade_write_in_t` | `bb_set_hot_upgrade_write_out_t` | 热升级数据写入 |
| `BB_SET_HOT_UPGRADE_CRC32` | `bb_set_hot_upgrade_crc32_in_t` | `bb_set_hot_upgrade_crc32_out_t` | 热升级 CRC32 校验 |
| `BB_SET_SYS_REBOOT` | `bb_set_reboot_t` | 无 | 系统重启，可指定延迟时间 |
| `BB_SET_CUSTOMER_KEY` | `bb_set_customer_key_in_t` | 无 | 写入 customer key，可用空间 8 字节 |
| `BB_FORCE_CLS_SOCKET_ALL` | 无 | 无 | 强制关闭所有 socket，可能造成 socket 信息不同步 |
| `BB_FORCE_CLS_SOCKET` | `bb_force_close_socket_t` | 无 | 强制关闭指定 slot/port 的 socket |

### 7.5 调试诊断与产测

| 命令字 | 输入结构 | 输出结构 | 功能 |
| --- | --- | --- | --- |
| `BB_SET_REG` | `bb_set_reg_t` | 无 | 写入基带寄存器 |
| `BB_SET_CFG` | `bb_set_cfg_t` | 无 | 分页写入基带配置文件 |
| `BB_RESET_CFG` | 头文件未定义专用结构 | 无 | reset 基带配置文件 |
| `BB_SET_PLOT` | `bb_set_plot_t` | 无 | 设置 plot debug 参数，配合 `BB_EVENT_PLOT_DATA` |
| `BB_SET_DBG_MODE` | `bb_set_dbg_mode_t` | 无 | 设置 SDK 进入或退出 debug mode |
| `BB_SET_FREQ` | `bb_set_freq_t` | 无 | 设置物理用户 TX/RX 频率 |
| `BB_SET_TX_MCS` | `bb_set_tx_mcs_t` | 无 | 设置物理用户发送 MCS |
| `BB_SET_RESET` | `bb_set_reset_t` | 无 | 复位物理用户或基带 |
| `BB_SET_TX_PATH` | `bb_set_tx_path_t` | 无 | 设置无线发射通道，用于功率测试 |
| `BB_SET_RX_PATH` | `bb_set_rx_path_t` | 无 | 设置无线接收通道，用于灵敏度测试 |
| `BB_SET_POWER_OFFSET` | `bb_set_power_offset_t` | 无 | 设置功率补偿 |
| `BB_SET_POWER_TEST_MODE` | 头文件未定义专用结构 | 无 | 进入产测功率测试模式 |
| `BB_SET_SENSE_TEST_MODE` | 头文件未定义专用结构 | 无 | 进入产测灵敏度测试模式 |
| `BB_SET_ORIG_CFG` | `bb_set_orig_cfg_t` | 无 | 加载原始镜像配置，仅基带 IDLE 状态支持 |
| `BB_SET_SINGLE_TONE` | 头文件未定义专用结构 | 无 | 单音信号，需要先进入 debug mode |
| `BB_SET_PURE_SLOT` | 头文件未定义专用结构 | 无 | 纯图传模式，头文件注明暂不支持退出 |
| `BB_SET_FACTORY_POWER_OFFSET_SAVE` | 头文件未定义专用结构 | 无 | 产测功率校准保存 |
| `BB_SET_TR_SWITCH` | `bb_tr_switch_t` | 无 | 项目注册 TR switch 回调钩子 |

### 7.6 项目分发与远程控制

| 命令字 | 输入结构 | 输出结构 | 功能 |
| --- | --- | --- | --- |
| `BB_REMOTE_IOCTL_REQ` | `bb_remote_ioctl_in_t` | `bb_remote_ioctl_out_t` | 远端 ioctl 命令分发，携带 slot、msg_id 和 data |
| `BB_SET_PRJ_DISPATCH` | `bb_set_prj_dispatch_in_t` | 无 | 项目二级 SET 命令分发 |
| `BB_SET_PRJ_DISPATCH2` | `bb_set_prj_dispatch2_in_t` | 无 | 项目二级 SET 命令分发，USB，1K 缓存 |
| `BB_SET_PRJ_DISPATCH2_UART` | `bb_set_prj_dispatch2_in_t` | 无 | 项目二级 SET 命令分发，UART，1K 缓存 |
| `BB_SET_PRJ_DISPATCH2_SDIO` | `bb_set_prj_dispatch2_in_t` | 无 | 项目二级 SET 命令分发，SDIO，1K 缓存 |
| `BB_GET_PRJ_DISPATCH` | `bb_get_prj_dispatch_in_t` | `bb_get_prj_dispatch_out_t` | 项目二级 GET 命令分发 |

## 8. 数据通道 Socket API

### 8.1 Socket 创建标志

| 标志 | 说明 |
| --- | --- |
| `BB_SOCK_FLAG_RX` | socket 具备接收方向 |
| `BB_SOCK_FLAG_TX` | socket 具备发送方向 |
| `BB_SOCK_FLAG_TROC` | 基带连接时清空 TX buffer 历史数据，仅芯片侧支持 |
| `BB_SOCK_FLAG_DATAGRAM` | 数据包模式，仅 host driver 侧支持 |
| `BB_SOCK_FLAG_SBUS` | SBUS 模式，先清除发送队列再写入数据，芯片侧支持且要求整包写入 |

`bb_sock_opt_t` 可指定 `tx_buf_size`、`rx_buf_size`。`tx_buf` 和 `rx_buf` 是芯片侧应用指定 buffer 地址时使用，host 侧通常只关注 buffer size。

### 8.2 Socket 函数

| API | 功能 | 说明 |
| --- | --- | --- |
| `bb_socket_open(dev, slot, port, flag, opt)` | 打开数传 socket | `slot` 指目标逻辑 slot；DEV 通常使用 `BB_SLOT_AP`；`port` 为逻辑端口 |
| `bb_socket_close(sockfd)` | 关闭 socket | `sockfd` 为打开时返回的 ID |
| `bb_socket_read(sockfd, buf, len, timeout)` | 读取数据 | 有数据即返回，不要求填满 buffer；`timeout < 0` 表示无限等待 |
| `bb_socket_write(sockfd, buf, len, timeout)` | 写入数据 | 写入表示进入基带发送队列，不代表无线侧已发送完成 |
| `bb_socket_ioctl(sockfd, cmd, value)` | 控制 socket 行为 | `cmd` 取值见 `bb_sock_cmd_e` |
| `bb_socket_query(sockfd, len, mem, timeout)` | 零拷贝方式请求数据 | 仅芯片侧支持 |
| `bb_socket_release(sockfd, mem)` | 释放 `bb_socket_query` 得到的内存 | 仅芯片侧支持，必须与 query 配对 |
| `bb_socket_ioctl_tx_len_get(...)` | 获取发送字节统计 | 使用 input/output buffer 传参 |
| `bb_socket_ioctl_tx_len_rst(...)` | 重置发送字节统计 | 使用 input/output buffer 传参 |

### 8.3 Socket 控制命令

| 命令 | 说明 | 关联结构 |
| --- | --- | --- |
| `BB_SOCK_QUERY_TX_BUFF_LEN` | 查询 TX buffer 状态 | `QUERY_TX_IN`、`QUERY_TX_OUT` |
| `BB_SOCK_QUERY_RX_BUFF_LEN` | 查询 RX buffer 状态 | `QUERY_RX_OUT` |
| `BB_SOCK_READ_INV_DATA` | 读取无效数据相关控制 | 头文件未定义专用结构 |
| `BB_SOCK_SET_TX_LIMIT` | 设置 TX 限制 | `BUFF_LIMIT` |
| `BB_SOCK_GET_TX_LIMIT` | 获取 TX 限制 | `BUFF_LIMIT` |
| `BB_SOCK_IOCTL_ECHO` | echo 控制命令 | 用户自定义 |
| `BB_SOCK_TX_LEN_GET` | 获取 TX 字节数 | 通过 `bb_socket_ioctl_tx_len_get` |
| `BB_SOCK_TX_LEN_RESET` | 重置 TX 字节数 | 通过 `bb_socket_ioctl_tx_len_rst` |

## 9. 事件与回调

事件通过 `BB_SET_EVENT_SUBSCRIBE` 订阅，回调类型为：

```c
typedef void (*bb_event_callback)(void *arg, void *user);
```

`arg` 由事件类型决定，`user` 是订阅时传入的用户私有数据。本地事件回调是同步的，RPC 场景是异步的。

| 事件 | 回调数据结构 | 说明 |
| --- | --- | --- |
| `BB_EVENT_LINK_STATE` | `bb_event_link_state_t` | 链路状态变化 |
| `BB_EVENT_MCS_CHANGE` | `bb_event_mcs_change_t` | MCS 等级变化 |
| `BB_EVENT_CHAN_CHANGE` | `bb_event_chan_change_t` | 工作信道变化 |
| `BB_EVENT_PLOT_DATA` | `bb_event_plot_data_t` | debug plot 数据 |
| `BB_EVENT_FRAME_START` | `bb_event_frame_start_t` | 基带帧开始 |
| `BB_EVENT_OFFLINE` | 头文件未定义专用结构 | 设备离线通知，仅 host 侧有效 |
| `BB_EVENT_PRJ_DISPATCH` | `bb_event_prj_dispatch_t` | 项目自定义事件分发 |
| `BB_EVENT_PAIR_RESULT` | `bb_event_pair_result_t` | 配对结果事件 |
| `BB_EVENT_PRJ_DISPATCH2` | `bb_event_prj_dispatch2_t` | 项目自定义事件分发 2，USB RPC |
| `BB_EVENT_MCS_CHANGE_END` | `bb_event_mcs_change_end_t` | MCS 变化结束事件 |
| `BB_EVENT_PRJ_DISPATCH2_UART` | `bb_event_prj_dispatch2_t` | 项目自定义事件分发 2，UART RPC |
| `BB_EVENT_PRJ_DISPATCH2_SDIO` | `bb_event_prj_dispatch2_t` | 项目自定义事件分发 2，SDIO RPC |
| `BB_EVENT_BW_CHANGE` | `bb_event_bw_change_t` | 带宽变化事件 |

## 10. RPC 和平台控制 API

### 10.1 RPC 命令字

| 命令字 | 功能 |
| --- | --- |
| `BB_START_REQ` | RPC 内部启动请求 |
| `BB_STOP_REQ` | RPC 内部停止请求 |
| `BB_INIT_REQ` | RPC 内部初始化请求 |
| `BB_DEINIT_REQ` | RPC 内部反初始化请求 |
| `BB_RPC_GET_LIST` | 获取 8030 可用列表 |
| `BB_RPC_SEL_ID` | 选择指定 8030 通讯 |
| `BB_RPC_GET_MAC` | 获取指定 8030 MAC |
| `BB_RPC_GET_HOTPLUG_EVENT` | 获取设备上下线通知 |
| `BB_RPC_SOCK_BUF_STA` | 查询 socket buffer 状态 |
| `BB_RPC_TEST` | 测试服务器连通性 |

### 10.2 平台控制函数和命令

| API / 命令 | 功能 |
| --- | --- |
| `BB_RPC_SERIAL_LIST` / `bb_uart_get_list` | 获取串口列表 |
| `BB_RPC_SERIAL_SETUP` / `bb_uart_setup` | 设置串口参数 |
| `BB_RPC_SET_DEBUG_LV` / `bb_daemon_set_dbg_level` | 设置 daemon 打印等级 |
| `BB_RPC_GET_DEBUG_LV` / `bb_daemon_get_dbg_level` | 获取 daemon 打印等级 |
| `BB_RPC_HOST_EXEC` / `bb_daemon_exec` | 在 host 侧执行命令 |
| `bb_debug_cmd` | 发送 debug channel 命令 |
| `bb_ioctl_set_hook2` | 设置项目 callback hook |
| `bb_ioctl_get_hook2` | 获取项目 callback hook |

## 11. 常用枚举和字段含义

| 类型 | 说明 |
| --- | --- |
| `bb_role_e` | 基带角色：`BB_ROLE_AP`、`BB_ROLE_DEV` |
| `bb_mode_e` | 基带模式：单用户、多用户、中继、导演模式 |
| `bb_slot_e` | 逻辑 slot；DEV 侧使用 `BB_SLOT_AP` 标识 AP |
| `bb_user_e` | 物理用户 ID，包含数据用户、BR/CS、扫频用户 |
| `bb_phy_mcs_e` | MCS 档位，包含负 MCS、单流、双流 |
| `bb_dir_e` | 方向：`BB_DIR_TX`、`BB_DIR_RX` |
| `bb_link_state_e` | 链路状态：IDLE、LOCK、CONNECT |
| `bb_band_e` | 工作频段：1G、2G、5G |
| `bb_bandwidth_e` | 工作带宽：1.25M 到 40M |
| `bb_rf_path_e` | RF 通道：A 路、B 路 |

## 12. 示例：查询状态和信道信息

```c
bb_get_status_in_t status_in = {0};
bb_get_status_out_t status_out = {0};

status_in.user_bmp = 0; /* 不关心物理层用户状态时可填 0 */
int ret = bb_ioctl(dev, BB_GET_STATUS, &status_in, &status_out);
if (ret == 0) {
    /* status_out.role / mode / link_status 等字段可用 */
}
```

```c
bb_get_chan_info_out_t chan = {0};

int ret = bb_ioctl(dev, BB_GET_CHAN_INFO, NULL, &chan);
if (ret == 0) {
    /* chan.work_chan、chan.freq[]、chan.power[] 等字段可用 */
}
```

## 13. 注意事项

- 本文档只根据 `bb_api.h` 中的宏、结构体和注释整理；头文件未定义专用输入/输出结构的命令，文档中明确标记为“头文件未定义专用结构”。
- `BB_CFG_*` 多数用于初始化阶段，建议在 `bb_init` 前完成。
- `BB_GET_*` 输出结构由调用方分配并传入 `out`。
- `BB_SET_*` 通常只需要输入结构，`out` 可为 `NULL`，但热升级等少数命令定义了输出结构。
- `bb_socket_write` 返回成功只表示数据进入基带发送队列，不等同于无线发送完成。
- 强制关闭 socket 类命令可能造成 socket 信息不同步，应只在恢复异常状态时使用。
