#!/usr/bin/env bash

# Python 环境激活脚本 - 用于 Git Bash 环境
# 用途: 在 Claude Code 中运行 Python 脚本前激活 conda 环境
# 使用: source .claude/skills/python-runner/activate-python-env.sh

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
: "${CONDA_ROOT:=/D/JianHao/miniconda3}"
: "${PYTHON_ENV_NAME:=py3139uart}"

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

# 检查 CONDA_ROOT 是否存在
if [[ ! -d "${CONDA_ROOT}" ]]; then
    _error "未找到 conda 根目录: ${CONDA_ROOT}"
    _error "请设置环境变量: export CONDA_ROOT=/path/to/conda"
    return 1
fi

# 初始化 conda
CONDA_PROFILE="${CONDA_ROOT}/etc/profile.d/conda.sh"
if [[ -f "${CONDA_PROFILE}" ]]; then
    # shellcheck disable=SC1090
    source "${CONDA_PROFILE}"
    _info "已加载 conda 初始化脚本"
else
    _error "未找到 conda.sh: ${CONDA_PROFILE}"
    return 1
fi

# 检查 conda 命令是否可用
if ! command -v conda >/dev/null 2>&1; then
    _error "conda 命令不可用，请检查安装"
    return 1
fi

# 激活指定的 Python 环境
_info "正在激活 Python 环境: ${PYTHON_ENV_NAME}"
conda activate "${PYTHON_ENV_NAME}" 2>/dev/null

# 验证激活结果
if [[ "${CONDA_DEFAULT_ENV}" == "${PYTHON_ENV_NAME}" ]]; then
    _success "Python 环境已激活: ${PYTHON_ENV_NAME}"

    # 显示 Python 版本信息
    python_path="$(command -v python || true)"
    python_version="$(python --version 2>&1 || true)"

    if [[ -n "${python_path}" ]]; then
        echo "  Python 路径: ${python_path}"
    fi
    if [[ -n "${python_version}" ]]; then
        echo "  Python 版本: ${python_version}"
    fi

    return 0
else
    _error "激活 Python 环境失败: ${PYTHON_ENV_NAME}"
    _error "当前环境: ${CONDA_DEFAULT_ENV:-未激活}"
    return 1
fi
