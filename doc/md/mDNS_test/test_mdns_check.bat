@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul 2>&1

set SCORE=0
set MAX_SCORE=4

echo ========================================
echo mDNS 环境检查工具
echo ========================================
echo.

REM ============================================
REM 检查 1: DNS Client 服务
REM ============================================
echo [检查 1/5] DNS Client 服务状态
echo 说明: Windows mDNS 依赖此服务
echo.

sc query Dnscache | findstr "RUNNING" >nul 2>&1
if %errorlevel% equ 0 (
    echo [✓ 通过] DNS Client 服务正在运行
    set /a SCORE+=1
) else (
    echo [✗ 失败] DNS Client 服务未运行
    echo 解决: 运行 'net start Dnscache' ^(需要管理员权限^)
)
echo.

REM ============================================
REM 检查 2: UDP 5353 端口
REM ============================================
echo [检查 2/5] UDP 5353 端口监听状态
echo 说明: mDNS 使用此端口进行通信
echo.

netstat -an | findstr ":5353" >nul 2>&1
if %errorlevel% equ 0 (
    echo [✓ 通过] 检测到进程监听 UDP 5353 端口
    for /f "tokens=*" %%a in ('netstat -an ^| findstr ":5353" ^| find /c ":5353"') do set PORT_COUNT=%%a
    echo 详情: 发现 !PORT_COUNT! 个监听实例
    set /a SCORE+=1
) else (
    echo [✗ 失败] 没有进程监听 UDP 5353 端口
    echo 说明: 这可能是正常的, 取决于系统配置
)
echo.

REM ============================================
REM 检查 3: 防火墙规则
REM ============================================
echo [检查 3/5] 防火墙 mDNS 规则
echo 说明: 防火墙需要允许 UDP 5353 通信
echo.

netsh advfirewall firewall show rule name=all | findstr /C:"5353" >nul 2>&1
if %errorlevel% equ 0 (
    echo [✓ 通过] 检测到 mDNS 相关防火墙规则
    set /a SCORE+=1
) else (
    echo [✗ 失败] 未检测到 mDNS 防火墙规则
    echo 解决: 运行以下命令添加规则 ^(需要管理员权限^):
    echo   netsh advfirewall firewall add rule name="mDNS UDP 5353 In" dir=in action=allow protocol=UDP localport=5353
    echo   netsh advfirewall firewall add rule name="mDNS UDP 5353 Out" dir=out action=allow protocol=UDP localport=5353
)
echo.

REM ============================================
REM 检查 4: Bonjour 服务 (可选)
REM ============================================
echo [检查 4/5] Bonjour 服务 ^(可选^)
echo 说明: Apple 的 mDNS 实现, 非必需
echo.

sc query "Bonjour Service" 2>nul | findstr "RUNNING" >nul 2>&1
if %errorlevel% equ 0 (
    echo [✓ 已安装] Bonjour 服务正在运行
) else (
    echo [- 跳过] Bonjour 服务未安装 ^(这是正常的^)
    echo 说明: Windows 10/11 自带 mDNS 支持, 不需要 Bonjour
)
echo.

REM ============================================
REM 检查 5: 组播连通性 (参考)
REM ============================================
echo [检查 5/5] 组播地址连通性 ^(参考^)
echo 说明: 测试组播地址 224.0.0.251 ^(mDNS 使用^)
echo.

ping -n 1 -w 1000 224.0.0.251 | findstr "TTL=" >nul 2>&1
if %errorlevel% equ 0 (
    echo [✓ 通过] 组播地址可达
    set /a SCORE+=1
) else (
    echo [! 警告] 组播 ping 失败
    echo.
    echo 重要说明:
    echo   - 这不一定意味着 mDNS 不工作! 
    echo   - Ping 使用 ICMP 协议, mDNS 使用 UDP 协议
    echo   - 很多系统不响应组播 ICMP, 但 UDP 组播正常
    echo   - 需要运行实际的 mDNS 测试来确认
)
echo.

REM ============================================
REM 综合判断
REM ============================================
echo ========================================
echo 检查结果汇总
echo ========================================
echo 得分: %SCORE% / %MAX_SCORE% 项通过
echo.

if %SCORE% geq 3 (
    echo [✓ 结论] 你的环境很可能支持 mDNS
    echo.
    echo 下一步:
    echo   1. 运行实际测试验证:
    echo      - 终端 1: python test_mdns_provider.py
    echo      - 终端 2: python test_mdns_browser.py
    echo   2. 如果实际测试成功, 说明环境完全支持 mDNS
    echo   3. 如果实际测试失败, 检查:
    echo      - 是否在同一局域网
    echo      - 路由器是否禁用组播
    echo      - 临时关闭防火墙测试
) else if %SCORE% geq 2 (
    echo [! 警告] 你的环境可能部分支持 mDNS
    echo.
    echo 建议:
    echo   1. 检查上述失败项并尝试修复
    echo   2. 运行实际测试验证功能
    echo   3. 如果测试失败, 考虑更换网络环境
) else (
    echo [✗ 结论] 你的环境可能不支持 mDNS
    echo.
    echo 建议:
    echo   1. 启动 DNS Client 服务
    echo   2. 配置防火墙规则
    echo   3. 检查网络设置
    echo   4. 考虑更换网络环境测试
)

echo.
echo ========================================
echo 按任意键退出...
pause >nul
