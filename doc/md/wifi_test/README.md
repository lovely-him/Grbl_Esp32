# Grbl_ESP32 网络服务测试工具

## 概述

本目录包含用于测试 Grbl_ESP32 所有网络服务的 Python 测试脚本。

## 测试脚本列表

| 脚本文件 | 测试服务 | 端口 | 说明 |
|---------|---------|------|------|
| `test_all_services.py` | 全部服务 | - | 一键测试所有服务 |
| `test_http.py` | HTTP Web Server | 80 | Web 界面和 REST API |
| `test_websocket.py` | WebSocket Server | 81 | 实时双向通信 |
| `test_telnet.py` | Telnet Server | 8080 | 命令行控制 |
| `test_file_transfer.py` | File Transfer | 8081 | 文件传输 |
| `test_mdns.py` | mDNS | 5353 | .local 域名解析 |
| `test_ssdp.py` | SSDP/UPnP | 1900 | 设备发现 |

## 环境要求

### Python 版本
- Python 3.6 或更高版本

### 依赖库
所有脚本使用 Python 标准库，无需额外安装依赖。

```bash
# 验证 Python 版本
python3 --version
```

## 快速开始

### 1. 一键测试所有服务

```bash
# 测试所有服务（使用默认 IP）
python3 test_all_services.py 192.168.8.177

# 详细输出
python3 test_all_services.py 192.168.8.177 --verbose

# 跳过某些服务
python3 test_all_services.py 192.168.8.177 --skip http,websocket
```

**示例输出**:
```
========================================
Grbl_ESP32 网络服务测试工具
========================================
目标设备: 192.168.8.177
测试时间: 2026-02-04 10:30:00

[1/7] 测试 HTTP Web Server (端口 80)...
✓ HTTP 服务正常

[2/7] 测试 WebSocket Server (端口 81)...
✓ WebSocket 服务正常

...

========================================
测试结果汇总
========================================
✓ 成功: 6 个服务
✗ 失败: 1 个服务
- 跳过: 0 个服务

总体状态: 部分成功
========================================
```

### 2. 测试单个服务

#### HTTP Web Server
```bash
python3 test_http.py 192.168.8.177

# 测试特定端口
python3 test_http.py 192.168.8.177 --port 8080
```

#### Telnet Server
```bash
python3 test_telnet.py 192.168.8.177

# 测试特定命令
python3 test_telnet.py 192.168.8.177 --command "?"
```

#### WebSocket Server
```bash
python3 test_websocket.py 192.168.8.177

# 详细输出
python3 test_websocket.py 192.168.8.177 --verbose
```

#### File Transfer Server
```bash
# 基本连接测试
python3 test_file_transfer.py 192.168.8.177

# 上传文件
python3 test_file_transfer.py 192.168.8.177 --file test.gcode

# 跳过 SD 卡检查（测试模式）
python3 test_file_transfer.py 192.168.8.177 --file test.gcode --test-mode
```

#### mDNS
```bash
# 测试默认主机名
python3 test_mdns.py

# 测试自定义主机名
python3 test_mdns.py --hostname mydevice.local
```

#### SSDP/UPnP
```bash
# 发现设备
python3 test_ssdp.py

# 测试特定 IP 的描述文件
python3 test_ssdp.py --ip 192.168.8.177
```

## 使用场景

### 场景 1: 首次配置验证

设备首次启动后，运行全面测试确保所有服务正常：

```bash
python3 test_all_services.py 192.168.8.177 --verbose
```

### 场景 2: 故障诊断

某个服务异常时，单独测试该服务：

```bash
# Telnet 无法连接
python3 test_telnet.py 192.168.8.177 --verbose

# File Transfer 传输失败
python3 test_file_transfer.py 192.168.8.177 --file test.gcode --verbose
```

### 场景 3: 性能测试

测试文件传输性能：

```bash
# 创建 1MB 测试文件
dd if=/dev/zero of=test_1mb.gcode bs=1M count=1

# 上传并计时
time python3 test_file_transfer.py 192.168.8.177 --file test_1mb.gcode
```

### 场景 4: 自动化测试

将测试集成到 CI/CD 流程：

```bash
#!/bin/bash
# ci_test.sh

DEVICE_IP="192.168.8.177"
RESULT=$(python3 test_all_services.py $DEVICE_IP)

if echo "$RESULT" | grep -q "总体状态: 成功"; then
    echo "✓ 所有服务测试通过"
    exit 0
else
    echo "✗ 服务测试失败"
    echo "$RESULT"
    exit 1
fi
```

## 常见问题

### Q1: `test_all_services.py` 报错 "No module named 'test_http'"

**原因**: Python 无法找到子模块。

**解决方法**:
```bash
# 确保在 wifi_test 目录下运行
cd doc/md/wifi_test
python3 test_all_services.py 192.168.8.177
```

### Q2: mDNS 测试失败

**原因**:
- Windows 可能需要安装 Bonjour
- 防火墙阻止 UDP 5353 端口
- 设备和电脑不在同一局域网

**解决方法**:
```bash
# 先测试基本连接
ping 192.168.8.177

# 使用 IP 直接测试 HTTP
python3 test_http.py 192.168.8.177
```

### Q3: SSDP 测试超时

**原因**:
- 仅 STA 模式启用
- 防火墙阻止 UDP 1900 端口
- 设备未启用 SSDP (`ENABLE_SSDP` 未定义)

**解决方法**:
```bash
# 跳过 SSDP 测试
python3 test_all_services.py 192.168.8.177 --skip ssdp

# 或直接测试描述文件
curl http://192.168.8.177/description.xml
```

### Q4: File Transfer 测试失败

**原因**:
- SD 卡未插入
- 文件系统错误
- 设备未启用服务

**解决方法**:
```bash
# 使用测试模式（跳过 SD 卡）
python3 test_file_transfer.py 192.168.8.177 --test-mode

# 检查端口是否开放
nc -zv 192.168.8.177 8081
```

## 高级用法

### 自定义超时时间

所有脚本支持 `--timeout` 参数：

```bash
python3 test_http.py 192.168.8.177 --timeout 10
python3 test_telnet.py 192.168.8.177 --timeout 5
```

### 批量测试多个设备

```bash
#!/bin/bash
# test_multiple_devices.sh

DEVICES=(
    "192.168.8.177"
    "192.168.8.178"
    "192.168.8.179"
)

for IP in "${DEVICES[@]}"; do
    echo "测试设备: $IP"
    python3 test_all_services.py $IP
    echo "---"
done
```

### 输出 JSON 格式结果

```bash
# 将结果保存为 JSON（需要修改脚本支持）
python3 test_all_services.py 192.168.8.177 --json > results.json
```

## 脚本详解

### test_all_services.py

**功能**:
- 自动检测设备 IP
- 顺序测试所有服务
- 生成汇总报告

**参数**:
```
positional arguments:
  ip                    设备 IP 地址

optional arguments:
  -h, --help            显示帮助信息
  -v, --verbose         详细输出
  --skip SERVICES       跳过的服务（逗号分隔）
  --timeout SECONDS     超时时间（秒）
```

### test_file_transfer.py

**功能**:
- 测试连接
- 上传文件（可选）
- 验证协议实现

**协议格式**:
```
请求包: [0xAB][CMD][LEN_H][LEN_L][IDX_H][IDX_L][DATA...][CHK_H][CHK_L]
响应包: [0xAB][STATUS][LEN_H][LEN_L][IDX_H][IDX_L][CHK_H][CHK_L]
```

**参数**:
```
positional arguments:
  ip                    设备 IP 地址

optional arguments:
  -h, --help            显示帮助信息
  -f FILE, --file FILE  要上传的文件
  --port PORT           端口号（默认 8081）
  --test-mode           测试模式（跳过 SD 卡）
  -v, --verbose         详细输出
```

## 贡献

如需添加新的测试功能或修复 bug，请参考现有脚本的实现模式：

1. 创建独立的 `test_xxx.py` 文件
2. 实现 `test_xxx_service()` 函数
3. 添加命令行参数解析
4. 更新 `test_all_services.py` 导入
5. 更新本 README 文档

## 参考文档

- [网络服务完全指南](../功能解析/网络服务完全指南.md) - 详细的服务功能说明
- [CLAUDE.md](../../../CLAUDE.md) - 项目开发指南

## 许可证

与 Grbl_ESP32 项目保持一致 (GPL-3.0)

---

**最后更新**: 2026-02-04
