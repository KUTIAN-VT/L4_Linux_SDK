#!/bin/bash
set -e

SDK_ROOT=$(cd "$(dirname "$0")/.." && pwd)

usage() {
    echo "Usage: $0 [all|x86_64|x86|arm64|arm]"
}

clean_install_dir() {
    local install_dir="$1"

    if [ -d "${install_dir}" ]; then
        if ! rm -rf "${install_dir}" 2>/tmp/l4_linux_sdk_clean.err; then
            echo "Warning: some install files in ${install_dir} could not be removed, likely because they are owned by another user."
            echo "Removing build-installed binaries, libraries, and headers that are writable..."
            rm -f "${install_dir}/bin/l4_linux_mvi"
            rm -f "${install_dir}/bin/L4_Linux_mvi"
            rm -f "${install_dir}/bin/l4_daemon"
            rm -rf "${install_dir}/lib"
            rm -rf "${install_dir}/include"
            find "${install_dir}" -type d -empty -delete 2>/dev/null || true
            echo "Remaining files may need manual removal with elevated permissions."
        fi
    fi
}

clean_arch() {
    local arch="$1"
    local build_dir="${SDK_ROOT}/build/${arch}"
    local install_dir="${SDK_ROOT}/install/${arch}"

    echo "Cleaning ${arch} build outputs..."
    rm -rf "${build_dir}"
    clean_install_dir "${install_dir}"
}

target="${1:-all}"

case "${target}" in
    all)
        clean_arch "x86_64"
        clean_arch "arm64"
        ;;
    x86_64|x86)
        clean_arch "x86_64"
        ;;
    arm64|arm)
        clean_arch "arm64"
        ;;
    -h|--help)
        usage
        exit 0
        ;;
    *)
        usage
        exit 1
        ;;
esac

echo "Clean done."
