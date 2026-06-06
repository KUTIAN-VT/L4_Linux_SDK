#!/bin/bash
set -e

SDK_ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR="${SDK_ROOT}/build/arm64"
INSTALL_DIR="${SDK_ROOT}/install/arm64"
TOOLCHAIN_FILE="${SDK_ROOT}/compiler.arm.cmake"

cmake -S "${SDK_ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
    -DAPP_STATIC_LIB=OFF \
    -DUSING_8030USB=ON \
    -DUSING_8030UART=ON \
    -DUSING_8030SDIO=OFF \
    -DUSING_XDS_HDR=ON

num=$(grep -c processor /proc/cpuinfo)
cmake --build "${BUILD_DIR}" --target ar8030_client l4_cmd_dbg l4_linux_mvi l4_ota_upgrade l4_daemon -j${num}
cmake --install "${BUILD_DIR}"
