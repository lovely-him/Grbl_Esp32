#!/bin/bash

# 设置 UTF-8 编码
export LANG=zh_CN.UTF-8
export LC_ALL=zh_CN.UTF-8

echo "Loading Claude Code environment configuration..."
echo ""

# 获取脚本所在目录和相关路径
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPT_NAME="$(basename "${BASH_SOURCE[0]}" .sh)"
SWAP_FILE="${SCRIPT_DIR}/${SCRIPT_NAME}.swp"

# Windows 路径转 Unix 路径的函数
windows_to_unix_path() {
    local win_path="$1"
    # 将反斜杠转为正斜杠
    local unix_path="${win_path//\\//}"
    # 转换盘符 (D: -> /d)
    if [[ "$unix_path" =~ ^([A-Za-z]):(.*)$ ]]; then
        local drive="${BASH_REMATCH[1]}"
        local path="${BASH_REMATCH[2]}"
        unix_path="/${drive,,}${path}"
    fi
    echo "$unix_path"
}

# 验证路径是否存在
verify_git_bash() {
    local git_bash_win="$1"
    local git_bash_unix=$(windows_to_unix_path "$git_bash_win")

    if [ -f "$git_bash_unix" ]; then
        return 0
    else
        return 1
    fi
}

# 读取配置
GIT_BASH_PATH=""

# 优先使用 .bat 传入的参数 (向后兼容)
if [ -n "$1" ]; then
    echo "Using Git Bash path from parameter..."
    GIT_BASH_PATH="$1"
# 否则从 .swp 文件读取
elif [ -f "$SWAP_FILE" ]; then
    echo "Reading Git Bash path from config file..."
    while IFS='=' read -r key value; do
        if [ "$key" = "GIT_ROOT" ]; then
            GIT_ROOT="$value"
            GIT_BASH_PATH="${GIT_ROOT}\\bin\\bash.exe"
            break
        fi
    done < "$SWAP_FILE"
fi

# 验证路径有效性, 无效则提示用户输入
while true; do
    if [ -n "$GIT_BASH_PATH" ] && verify_git_bash "$GIT_BASH_PATH"; then
        echo "Git Bash found: $GIT_BASH_PATH"
        break
    fi

    # 路径无效或不存在, 提示用户输入
    echo ""
    if [ -n "$GIT_BASH_PATH" ]; then
        echo "Error: Git Bash not found at $GIT_BASH_PATH"
    fi
    echo "Please enter Git installation path (e.g. D:\\JianHao\\Git):"
    read -r GIT_ROOT

    if [ -z "$GIT_ROOT" ]; then
        echo "Error: Git path is required"
        continue
    fi

    # 移除路径末尾的反斜杠
    GIT_ROOT="${GIT_ROOT%\\}"
    GIT_BASH_PATH="${GIT_ROOT}\\bin\\bash.exe"
done

# 保存配置到 .swp 文件
echo "GIT_ROOT=${GIT_ROOT:-${GIT_BASH_PATH%\\bin\\bash.exe}}" > "$SWAP_FILE"
echo "Configuration saved to: $SWAP_FILE"
echo ""

# 设置环境变量
export CLAUDE_CODE_GIT_BASH_PATH="$GIT_BASH_PATH"

echo "Environment variables set:"
echo "  CLAUDE_CODE_GIT_BASH_PATH=$CLAUDE_CODE_GIT_BASH_PATH"
echo "  Note: API keys configured in .claude/settings.json"
echo ""

echo "========================================"
echo "Environment configuration completed!"
echo "========================================"
echo ""

echo "Starting claude command..."
echo ""

# 执行 claude 命令
claude

# claude 执行完毕后的提示
echo ""
echo "========================================"
echo "Claude command completed"
echo "========================================"
echo ""
echo "Terminal remains open. Type 'exit' to close."
echo ""

# 启动交互式 shell 保持终端打开
exec bash -i

