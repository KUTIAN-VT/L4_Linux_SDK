#!/bin/bash
# 任一步骤失败时立即退出，避免升级失败后继续恢复配置或重启设备。
set -e

# 打印脚本用法。
usage()
{
    echo "Usage: $0 <firmware.img>"
}

# 通过 /proc 中的进程名判断 l4_daemon 是否已经运行，避免重复启动。
is_l4_daemon_running()
{
    local comm_file
    local process_name

    for comm_file in /proc/[0-9]*/comm; do
        [ -r "${comm_file}" ] || continue
        IFS= read -r process_name < "${comm_file}" || continue
        if [ "${process_name}" = "l4_daemon" ]; then
            return 0
        fi
    done

    return 1
}

# 只接受一个固件镜像路径参数。
if [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

firmware_path=$1

# 在调用升级工具前确认固件镜像存在且当前用户可读。
if [ ! -f "${firmware_path}" ] || [ ! -r "${firmware_path}" ]; then
    echo "Firmware image is not a readable file: ${firmware_path}" >&2
    exit 1
fi

# 优先使用 L4_BIN_DIR 中的工具；未设置时从 PATH 查找。
if [ -n "${L4_BIN_DIR:-}" ]; then
    l4_ota_upgrade_cmd=${L4_BIN_DIR%/}/l4_ota_upgrade
    l4_config_file_cmd=${L4_BIN_DIR%/}/l4_config_file

    if [ ! -x "${l4_ota_upgrade_cmd}" ]; then
        echo "Executable not found in L4_BIN_DIR: ${l4_ota_upgrade_cmd}" >&2
        exit 1
    fi

    if [ ! -x "${l4_config_file_cmd}" ]; then
        echo "Executable not found in L4_BIN_DIR: ${l4_config_file_cmd}" >&2
        exit 1
    fi
else
    if ! command -v l4_ota_upgrade >/dev/null 2>&1; then
        echo "Command not found in PATH: l4_ota_upgrade" >&2
        exit 1
    fi

    if ! command -v l4_config_file >/dev/null 2>&1; then
        echo "Command not found in PATH: l4_config_file" >&2
        exit 1
    fi

    l4_ota_upgrade_cmd=$(command -v l4_ota_upgrade)
    l4_config_file_cmd=$(command -v l4_config_file)
fi

# 操作 1：确保 l4_daemon 正在运行。
# 必要性：升级和配置工具都通过 daemon 与设备通信，没有 daemon 时无法访问设备。
echo "[1/4] Checking l4_daemon..."
if ! is_l4_daemon_running; then
    if [ -n "${L4_BIN_DIR:-}" ]; then
        l4_daemon_cmd=${L4_BIN_DIR%/}/l4_daemon
        if [ ! -x "${l4_daemon_cmd}" ]; then
            echo "Executable not found in L4_BIN_DIR: ${l4_daemon_cmd}" >&2
            exit 1
        fi
    else
        if ! command -v l4_daemon >/dev/null 2>&1; then
            echo "Command not found in PATH: l4_daemon" >&2
            exit 1
        fi
        l4_daemon_cmd=$(command -v l4_daemon)
    fi

    # 使用 nohup 保证脚本退出后 daemon 仍继续运行，并丢弃 daemon 的全部输出。
    echo "l4_daemon is not running, starting it..."
    nohup "${l4_daemon_cmd}" > /dev/null 2>&1 &
    daemon_pid=$!
    sleep 1

    # daemon 若在启动后一秒内退出，则停止升级。
    if ! kill -0 "${daemon_pid}" 2>/dev/null; then
        echo "Failed to start l4_daemon" >&2
        exit 1
    fi

    echo "l4_daemon started, pid=${daemon_pid}"
else
    echo "l4_daemon is already running"
fi

# 操作 2：把指定固件镜像写入设备。
# 必要性：这是升级流程的核心步骤；失败时必须立即停止，不能继续修改配置或重启。
echo "[2/4] Upgrading firmware..."
"${l4_ota_upgrade_cmd}" -f "${firmware_path}"

# 操作 3：恢复出厂设置，清除旧固件遗留且可能与新固件不兼容的配置。
# 必要性：保证新固件使用默认配置启动，避免旧配置影响升级后的设备行为。
echo "[3/4] Restoring factory settings..."
"${l4_config_file_cmd}" -r

# 操作 4：重启设备，使新固件和恢复后的默认配置正式生效。
# 必要性：l4_ota_upgrade 只完成固件写入，不会主动重启设备。
echo "[4/4] Rebooting device..."
"${l4_config_file_cmd}" -H
