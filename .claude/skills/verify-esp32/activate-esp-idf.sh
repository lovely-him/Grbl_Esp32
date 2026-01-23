#!/bin/bash
# ESP-IDF 环境激活脚本 (MINGW64 Bash 适配版)
# 通过 PowerShell 跨环境调用 ESP-IDF 编译和烧录

# 获取脚本所在目录 (用于定位 PowerShell 脚本)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PS_SCRIPT="$(cygpath -w "$SCRIPT_DIR/esp_idf_build.ps1")"

# 验证 PowerShell 脚本存在
if [ ! -f "$SCRIPT_DIR/esp_idf_build.ps1" ]; then
    echo "❌ 错误: 找不到 PowerShell 构建脚本"
    echo "   预期位置: $SCRIPT_DIR/esp_idf_build.ps1"
    return 1
fi

# 定义 esp_idf_build 函数 - 仅编译，不烧录
esp_idf_build() {
    local project_dir="${1:-.}"

    # 转换为绝对路径
    project_dir="$(cd "$project_dir" && pwd)"

    # 转换 Unix 路径为 Windows 路径
    local win_project_dir
    win_project_dir=$(cygpath -w "$project_dir" 2>/dev/null)
    if [ $? -ne 0 ]; then
        # cygpath 不可用时的备用方案 (简单替换)
        win_project_dir=$(echo "$project_dir" | sed 's|^/\([a-z]\)/|\U\1:/|')
    fi

    echo "================================================"
    echo "ESP-IDF Build Only (Skip Flash)"
    echo "================================================"
    echo "Bash Project Dir  : $project_dir"
    echo "Windows Path      : $win_project_dir"
    echo "PowerShell Script : $PS_SCRIPT"
    echo ""

    # 调用 PowerShell 脚本，传递 -Action BuildOnly
    powershell.exe -NoProfile -ExecutionPolicy Bypass \
        -File "$PS_SCRIPT" \
        -ProjectPath "$win_project_dir" \
        -Action "BuildOnly"

    local exit_code=$?

    echo ""
    echo "================================================"
    echo "Build process completed with exit code: $exit_code"
    echo "================================================"

    return $exit_code
}

# 定义 esp_idf_build_flash 函数 - 编译 + 烧录（推荐）
esp_idf_build_flash() {
    local project_dir="${1:-.}"

    # 转换为绝对路径
    project_dir="$(cd "$project_dir" && pwd)"

    # 转换 Unix 路径为 Windows 路径
    local win_project_dir
    win_project_dir=$(cygpath -w "$project_dir" 2>/dev/null)
    if [ $? -ne 0 ]; then
        # cygpath 不可用时的备用方案 (简单替换)
        win_project_dir=$(echo "$project_dir" | sed 's|^/\([a-z]\)/|\U\1:/|')
    fi

    echo "================================================"
    echo "ESP-IDF Build + Flash"
    echo "================================================"
    echo "Bash Project Dir  : $project_dir"
    echo "Windows Path      : $win_project_dir"
    echo "PowerShell Script : $PS_SCRIPT"
    echo ""

    # 调用 PowerShell 脚本，传递 -Action BuildAndFlash
    powershell.exe -NoProfile -ExecutionPolicy Bypass \
        -File "$PS_SCRIPT" \
        -ProjectPath "$win_project_dir" \
        -Action "BuildAndFlash"

    local exit_code=$?

    echo ""
    echo "================================================"
    echo "Build + Flash process completed with exit code: $exit_code"
    echo "================================================"

    return $exit_code
}

# 定义 esp_idf_flash 函数 - 仅烧录，不编译
esp_idf_flash() {
    local project_dir="${1:-.}"

    # 转换为绝对路径
    project_dir="$(cd "$project_dir" && pwd)"

    # 转换 Unix 路径为 Windows 路径
    local win_project_dir
    win_project_dir=$(cygpath -w "$project_dir" 2>/dev/null)
    if [ $? -ne 0 ]; then
        # cygpath 不可用时的备用方案 (简单替换)
        win_project_dir=$(echo "$project_dir" | sed 's|^/\([a-z]\)/|\U\1:/|')
    fi

    echo "================================================"
    echo "ESP-IDF Flash Only (Skip Build)"
    echo "================================================"
    echo "Bash Project Dir  : $project_dir"
    echo "Windows Path      : $win_project_dir"
    echo "PowerShell Script : $PS_SCRIPT"
    echo ""

    # 调用 PowerShell 脚本，传递 -Action FlashOnly
    powershell.exe -NoProfile -ExecutionPolicy Bypass \
        -File "$PS_SCRIPT" \
        -ProjectPath "$win_project_dir" \
        -Action "FlashOnly"

    local exit_code=$?

    echo ""
    echo "================================================"
    echo "Flash process completed with exit code: $exit_code"
    echo "================================================"

    return $exit_code
}

# 显示激活信息
echo "✅ ESP-IDF 构建环境已准备"
echo "   调用方式: PowerShell 脚本 (绕过 MINGW64 限制)"
echo ""
echo "可用命令:"
echo "  esp_idf_build [项目目录]             # 仅编译，不烧录"
echo "  esp_idf_build_flash [项目目录]       # 编译 + 烧录（推荐）"
echo "  esp_idf_flash [项目目录]             # 仅烧录，不编译"
echo ""
echo "示例:"
echo "  esp_idf_build                        # 当前目录，仅编译"
echo "  esp_idf_build_flash                  # 当前目录，编译+烧录"
echo "  esp_idf_flash                        # 当前目录，仅烧录"
echo ""
