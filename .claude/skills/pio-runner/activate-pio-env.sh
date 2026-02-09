#!/usr/bin/env bash

# PlatformIO 环境激活脚本 - 用于 Git Bash 环境
# 用途: 在 Claude Code 中运行 PlatformIO 命令前激活环境
# 使用: source .claude/skills/pio-runner/activate-pio-env.sh

# 检测脚本是否通过 source 执行
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "[ERROR] 此脚本必须通过 source 命令执行，而不是直接运行"
    echo "[ERROR] 正确用法:"
    echo "        source ${BASH_SOURCE[0]}"
    echo "        或"
    echo "        . ${BASH_SOURCE[0]}"
    exit 1
fi

# 默认配置（可通过环境变量覆盖）
: "${PIO_PATH:=/c/Users/admin/.platformio/penv/Scripts}"

# 颜色输出
_info() {
    echo "[INFO] $*"
}

_error() {
    echo "[ERROR] $*" >&2
}

_success() {
    echo "[SUCCESS] $*"
}

# 检查 PIO_PATH 是否存在
if [[ ! -d "${PIO_PATH}" ]]; then
    _error "未找到 PlatformIO 目录: ${PIO_PATH}"
    _error "请设置环境变量: export PIO_PATH=/path/to/platformio/Scripts"
    return 1
fi

# 添加到 PATH
export PATH="${PIO_PATH}:$PATH"
_info "已将 PlatformIO 添加到 PATH"

# 验证 pio 命令是否可用
if command -v pio >/dev/null 2>&1; then
    _success "PlatformIO 环境已激活"

    # 显示 PlatformIO 版本信息
    pio_version="$(pio --version 2>&1 || true)"
    if [[ -n "${pio_version}" ]]; then
        echo "  ${pio_version}"
    fi

    # 显示 pio 命令路径
    pio_path="$(command -v pio || true)"
    if [[ -n "${pio_path}" ]]; then
        echo "  PIO 路径: ${pio_path}"
    fi

    return 0
else
    _error "pio 命令不可用，请检查安装"
    _error "预期路径: ${PIO_PATH}/pio.exe"
    return 1
fi
