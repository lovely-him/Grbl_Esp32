# ESP-IDF Build & Flash Wrapper for Claude Code
# 从 MINGW64 Bash 通过 PowerShell 调用 ESP-IDF 编译和烧录

param(
    [Parameter(Mandatory=$false)]
    [string]$ProjectPath = (Get-Location).Path,

    [Parameter(Mandatory=$false)]
    [ValidateSet("BuildOnly", "FlashOnly", "BuildAndFlash")]
    [string]$Action = "BuildAndFlash"
)

# 移除 MSYSTEM 环境变量以避免 ESP-IDF 检测到 MSys 环境并跳过操作
Remove-Item Env:\MSYSTEM -ErrorAction SilentlyContinue

# 设置 ESP-IDF 环境变量 (硬编码路径)
$env:IDF_PATH = "D:\JianHao\Espressif\frameworks\esp-idf-v5.5.1"
$env:IDF_PYTHON = "D:\JianHao\Espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe"
$env:IDF_TOOLS_PATH = "D:\JianHao\Espressif"

# 添加 ESP-IDF 工具链到 PATH (cmake, ninja, esptool等)
$env:PATH = "D:\JianHao\Espressif\tools\cmake\3.30.2\bin;" + `
            "D:\JianHao\Espressif\tools\ninja\1.12.1;" + `
            "D:\JianHao\Espressif\python_env\idf5.5_py3.11_env\Scripts;" + `
            "D:\JianHao\Espressif\frameworks\esp-idf-v5.5.1\tools;" + `
            $env:PATH

# 切换到项目目录
Set-Location $ProjectPath

# 根据 Action 参数执行对应操作
switch ($Action) {
    "FlashOnly" {
        Write-Host "============================================" -ForegroundColor Cyan
        Write-Host "ESP-IDF Flash Only (Skip Build)" -ForegroundColor Yellow
        Write-Host "============================================" -ForegroundColor Cyan
        Write-Host "Project Path : $ProjectPath"
        Write-Host ""

        # 执行 idf.py flash
        & $env:IDF_PYTHON "$env:IDF_PATH\tools\idf.py" flash

        $exitCode = $LASTEXITCODE
        Write-Host ""
        Write-Host "============================================" -ForegroundColor Cyan
        if ($exitCode -eq 0) {
            Write-Host "Flash Completed Successfully" -ForegroundColor Green
        } else {
            Write-Host "Flash Failed with exit code: $exitCode" -ForegroundColor Red
        }
        Write-Host "============================================" -ForegroundColor Cyan
        exit $exitCode
    }

    "BuildOnly" {
        Write-Host "============================================" -ForegroundColor Cyan
        Write-Host "ESP-IDF Build Only (Skip Flash)" -ForegroundColor Green
        Write-Host "============================================" -ForegroundColor Cyan
        Write-Host "Project Path : $ProjectPath"
        Write-Host "IDF Path     : $env:IDF_PATH"
        Write-Host "IDF Python   : $env:IDF_PYTHON"
        Write-Host ""

        # 执行 idf.py build
        & $env:IDF_PYTHON "$env:IDF_PATH\tools\idf.py" build

        $exitCode = $LASTEXITCODE
        Write-Host ""
        Write-Host "============================================" -ForegroundColor Cyan
        if ($exitCode -eq 0) {
            Write-Host "Build Completed Successfully" -ForegroundColor Green
        } else {
            Write-Host "Build Failed with exit code: $exitCode" -ForegroundColor Red
        }
        Write-Host "============================================" -ForegroundColor Cyan
        exit $exitCode
    }

    "BuildAndFlash" {
        Write-Host "============================================" -ForegroundColor Cyan
        Write-Host "ESP-IDF Build + Flash" -ForegroundColor Green
        Write-Host "============================================" -ForegroundColor Cyan
        Write-Host "Project Path : $ProjectPath"
        Write-Host "IDF Path     : $env:IDF_PATH"
        Write-Host "IDF Python   : $env:IDF_PYTHON"
        Write-Host ""

        # 执行 idf.py build
        & $env:IDF_PYTHON "$env:IDF_PATH\tools\idf.py" build

        $buildExitCode = $LASTEXITCODE
        Write-Host ""
        Write-Host "============================================" -ForegroundColor Cyan
        if ($buildExitCode -eq 0) {
            Write-Host "Build Completed Successfully" -ForegroundColor Green
        } else {
            Write-Host "Build Failed with exit code: $buildExitCode" -ForegroundColor Red
        }
        Write-Host "============================================" -ForegroundColor Cyan

        # 如果编译失败，直接退出
        if ($buildExitCode -ne 0) {
            exit $buildExitCode
        }

        # 编译成功，执行烧录
        Write-Host ""
        Write-Host "============================================" -ForegroundColor Cyan
        Write-Host "ESP-IDF Flash Starting..." -ForegroundColor Yellow
        Write-Host "============================================" -ForegroundColor Cyan
        Write-Host ""

        & $env:IDF_PYTHON "$env:IDF_PATH\tools\idf.py" flash

        $flashExitCode = $LASTEXITCODE
        Write-Host ""
        Write-Host "============================================" -ForegroundColor Cyan
        if ($flashExitCode -eq 0) {
            Write-Host "Flash Completed Successfully" -ForegroundColor Green
        } else {
            Write-Host "Flash Failed with exit code: $flashExitCode" -ForegroundColor Red
        }
        Write-Host "============================================" -ForegroundColor Cyan
        exit $flashExitCode
    }
}
