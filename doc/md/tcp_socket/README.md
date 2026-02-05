# TCP Socket 文件传输 - 自动化测试系统

## 项目概述

### 背景

Grbl_Esp32 需要通过 WiFi 将 G-code 文件上传到 ESP32 的 SD 卡，以便进行离线加工。本项目实现了一个基于 TCP Socket 的文件传输服务器，支持通过自定义二进制协议进行高效、可靠的文件传输。

### 代码关系

本项目包含三个主要部分：

#### 1. 参考代码（ReferenceCode/）
- **位置**: `doc/md/tcp_socket/ReferenceCode/`
- **文件**: `Jk_tcp.h`, `Jk_tcp.cpp`, `tcp_client.dart`
- **说明**: 原始实现，作为协议参考和对标基准
- **特点**:
  - 功能完整，已在生产环境验证
  - 代码风格较为简洁，但缺少规范化注释
  - 使用中文拼音命名（如 `jie_shou`, `chu_li`）
  - 内存管理采用固定分配（32KB 缓冲区始终占用）

#### 2. 实现代码（Grbl_Esp32/src/WebUI/）
- **位置**: `Grbl_Esp32/src/WebUI/`
- **文件**: `FileTransferServer.h`, `FileTransferServer.cpp`
- **说明**: 基于参考代码重新实现的规范化版本
- **改进点**:
  - ✅ **代码规范化**: 英文命名、完整 Doxygen 注释、清晰的函数职责
  - ✅ **内存优化**: 按需分配接收缓冲区（客户端连接时分配，断开时释放）
  - ✅ **状态机清晰**: 明确的 `TransferState` 枚举和状态转换逻辑
  - ✅ **协议常量化**: 使用 `constexpr` 和 `enum class` 替代魔术数字
  - ✅ **错误处理**: 统一的 `cleanup()` 函数，确保资源正确释放
  - ✅ **日志系统**: 分级日志（ERROR/WARN/INFO/DEBUG），便于调试
  - ✅ **协议兼容**: 与参考代码和 Dart 客户端完全兼容

#### 3. 测试代码（doc/md/tcp_socket/）
- **位置**: `doc/md/tcp_socket/`
- **文件**: `tcp_client.py`, `tcp_auto_test.py`, `verify_checksum.py`
- **说明**: Python 实现的自动化测试工具
- **功能**:
  - 功能测试：小文件 → 大文件（11B → 10MB）
  - 协议异常测试：校验和错误、无效魔术字节、超大包、不完整包
  - 压力测试：连续传输、大文件稳定性
  - 校验和验证工具：协议调试辅助

### 目录结构

```
doc/md/tcp_socket/
├── README.md                    # 本文档
├── tcp_client.py                # Python 测试客户端（协议实现）
├── tcp_auto_test.py             # 自动化测试脚本（14 个测试用例）
├── test_config.json             # 测试配置（IP、端口、超时等）
├── verify_checksum.py           # 校验和验证工具（协议调试）
└── ReferenceCode/               # 参考代码（原始实现）
    ├── Jk_tcp.h                 # 原始服务器实现（头文件）
    ├── Jk_tcp.cpp               # 原始服务器实现（源文件）
    └── tcp_client.dart          # Dart 客户端参考实现

Grbl_Esp32/src/WebUI/
├── FileTransferServer.h         # 新实现（头文件，协议定义）
└── FileTransferServer.cpp       # 新实现（源文件，核心逻辑）
```

### 协议兼容性

**重要**: FileTransferServer 与参考代码（Jk_tcp）和 Dart 客户端完全兼容，使用相同的二进制协议：

- **魔术字节**: `0xAB`
- **数据包类型**: Header(0x01), Data(0x02), Cancel(0x03)
- **响应状态**: Success(0x01), ProtocolError(0x02), ServerError(0x03)
- **校验和算法**: 16 位累加和（大端序）
- **超时时间**: 10 秒
- **最大包大小**: 32KB 数据 + 8 字节头部

### 开发历程

1. **参考阶段**: 分析 Jk_tcp 原始实现，理解协议细节
2. **重构阶段**: 重新设计类结构，规范化命名和注释
3. **优化阶段**: 内存按需分配、状态机清晰化、错误处理完善
4. **测试阶段**: 编写自动化测试，验证协议兼容性和稳定性
5. **文档阶段**: 编写详细注释和使用文档

### 下次对话快速上手

如果你是第一次接触本项目，建议按以下顺序阅读：

1. **协议理解**: 阅读 `ReferenceCode/tcp_client.dart`（协议最清晰）
2. **实现代码**: 阅读 `FileTransferServer.h`（协议常量和状态定义）
3. **核心逻辑**: 阅读 `FileTransferServer.cpp`（数据包解析和处理）
4. **测试验证**: 运行 `tcp_auto_test.py`（验证功能正确性）

---

## 快速开始

### 1. 配置测试参数

编辑 `test_config.json`：

```json
{
  "server": {
    "ip": "192.168.8.177",        // ESP32 的 IP 地址
    "port": 8081,                 // TCP Socket 端口
    "timeout": 20,
    "http_port": 80               // HTTP 端口
  },
  "http_command": {                // HTTP 命令配置
    "enabled": true,               // 是否启用 HTTP 命令测试
    "url": "http://192.168.8.177/command",  // HTTP 接口 URL
    "command": "$H",               // 默认命令（归零）
    "timeout": 30,                 // HTTP 请求超时（秒）
    "delay_after_upload": 1.0,     // 文件传输后延迟（秒）
    "auto_send_after_upload": false // 是否在每次文件传输后自动发送
  }
}
```

### 2. 运行测试

```bash
# 激活 Python 环境并运行测试
source .claude/skills/python-runner/activate-python-env.sh
python doc/md/tcp_socket/tcp_auto_test.py
```

或者使用 python-runner skill：

```bash
# 在项目根目录执行
python-runner doc/md/tcp_socket/tcp_auto_test.py
```

## 测试脚本

当前包含 1 个测试脚本：

### 1. tcp_auto_test.py - 功能测试
测试基本功能和不同文件大小：
- 小文件传输 (Hello World, 11 字节)
- 1KB 文件传输
- 4KB 文件传输
- 8KB 文件传输
- 16KB 文件传输
- 32KB 文件传输（单包最大）
- 100KB 文件传输（多包）
- 10MB 文件传输（大文件压力测试）
- 校验和错误处理
- 无效魔术字节处理
- 超大包处理
- 不完整包处理
- 取消传输
- HTTP 命令接口（测试 Grbl HTTP 接口）

#### HTTP 命令测试

HTTP 命令测试用于验证文件传输后发送 Grbl 命令的功能，常见应用场景：
- 文件上传后自动发送 `$H` 归零命令
- 文件上传后发送 `$X` 解除报警
- 测试 HTTP 接口的连通性和响应

**配置选项**:
- `enabled`: 是否启用 HTTP 命令测试
- `url`: HTTP 接口 URL（默认 `http://192.168.8.177/command`）
- `command`: 要发送的 Grbl 命令（如 `$H`, `$X`, `?`）
- `timeout`: HTTP 请求超时时间（秒，建议 30 秒）
- `delay_after_upload`: 文件传输完成后延迟时间（秒）
- `auto_send_after_upload`: 是否在每次文件传输后自动发送命令

**命令行参数**:
```bash
# 启用文件传输后自动发送 HTTP 命令
python tcp_auto_test.py --enable-http-after-upload

# 禁用文件传输后的 HTTP 命令
python tcp_auto_test.py --disable-http-after-upload

# 循环测试 HTTP 命令（10 次，间隔 1 秒）
python tcp_auto_test.py -l http -c 10 -i 1000

# 循环测试文件传输 + HTTP 命令
python tcp_auto_test.py -l 1kb -c 5 --enable-http-after-upload
```

**注意事项**:
- `$H` 归零命令可能需要 5-30 秒完成（取决于机器尺寸）
- HTTP 请求失败只记录警告，不影响测试结果
- 建议在循环测试时设置足够的间隔时间（≥ 5 秒）

## 测试前准备

### ESP32 端
1. 确保固件已烧录（包含 FileTransferServer）
2. 确保设备已连接到 WiFi
3. 记录设备的 IP 地址（从串口日志或路由器获取）
4. 确认 SD 卡已插入并格式化为 FAT32

### PC 端
1. 确保 PC 和 ESP32 在同一网络
2. 确认可以 ping 通 ESP32 的 IP
3. 确认配置文件中的 IP 地址正确

## 测试结果

测试完成后会显示：
- 每个测试的通过/失败状态
- 传输速度（KB/s）
- 总通过率

### 示例输出

```
======================================================================
TCP Socket 文件传输 - 自动化测试
======================================================================

服务器地址: 192.168.8.177:8081
停止策略: 遇到失败立即停止

[INFO] 已连接到 192.168.8.177:8081

======================================================================
测试 1: 小文件传输 (Hello World)
======================================================================
[INFO] 发送文件信息: test.txt
[INFO] 开始传输 11 字节，共 1 个数据包
[INFO] 数据包 1/1 已发送
[INFO] 文件传输完成
[PASS] 小文件传输成功

======================================================================
测试结果汇总
======================================================================
[PASS] 小文件传输
[PASS] 1KB 文件传输
[PASS] 100KB 文件传输
[PASS] 校验和错误处理
[PASS] 取消传输
[PASS] HTTP 命令接口

总计: 6/6 通过

[SUCCESS] 所有测试通过！
```

#### HTTP 命令测试示例输出

```
======================================================================
测试 14: HTTP 命令接口
======================================================================
[INFO] 发送 HTTP 命令: $H
[INFO] URL: http://192.168.8.177/command
[PASS] HTTP 命令成功, 状态码: 200
[INFO] 响应内容: ok
```

#### 文件传输后自动发送 HTTP 命令示例

```
======================================================================
测试 1: 小文件传输 (Hello World)
======================================================================
[INFO] 发送文件信息: test.txt
[INFO] 开始传输 11 字节，共 1 个数据包
[INFO] 数据包 1/1 已发送
[INFO] 文件传输完成
[PASS] 小文件传输成功
[INFO] 等待 1.0 秒后发送 HTTP 命令...
[INFO] 发送 HTTP 命令: $H
[INFO] HTTP 命令成功
```

## 故障排查

### 连接失败

**问题**: 无法连接到服务器

**解决方案**:
1. 检查 ESP32 是否已启动并连接 WiFi
2. 检查 IP 地址是否正确
3. 使用 `ping <ESP32_IP>` 测试网络连通性
4. 检查防火墙是否阻止端口 8081

### 传输失败

**问题**: 收到校验和错误或超时错误

**解决方案**:
1. 检查 SD 卡是否插入
2. 确认 SD 卡格式化为 FAT32
3. 检查 SD 卡是否有足够空间
4. 查看 ESP32 串口日志了解详细错误

### 校验和不匹配

**问题**: 服务器返回校验和错误

**解决方案**:
1. 这通常表示 FileTransferServer 代码有问题
2. 检查 FileTransferServer.cpp 中的校验和计算逻辑
3. 使用 verify_checksum.py 验证数据包的校验和

## 文件说明

- `tcp_client.py` - TCP 客户端库，实现协议通信
- `tcp_auto_test.py` - 功能测试脚本
- `test_config.json` - 测试配置文件
- `verify_checksum.py` - 校验和验证工具（协议调试）
- `tcp_client.dart` - Dart 客户端参考实现

## 下一步

测试通过后，可以：
1. 使用 Dart 客户端进行集成测试
2. 运行压力测试验证大文件传输稳定性
3. 测试网络不稳定情况（弱信号、高延迟）
4. 进行长时间稳定性测试

## 开发和调试

### 修改测试用例

编辑 `tcp_auto_test.py`，添加新的测试函数：

```python
def test_my_feature(client: FileTransferClient) -> bool:
    """测试自定义功能"""
    print("\n测试: 我的功能")

    # 你的测试代码

    return True  # 或 False
```

### 查看详细日志

TCP 客户端会输出详细的调试信息，包括：
- 连接状态
- 数据包发送/接收
- 错误信息

### 单独测试某个功能

```python
from tcp_client import FileTransferClient

client = FileTransferClient('192.168.8.177', 8081)
if client.connect():
    # 测试代码
    client.upload_file('test.txt', b'Hello')
    client.disconnect()
```

## 协议规范

详细的协议规范请参考 Dart 客户端实现：`tcp_client.dart`

### 数据包格式

**客户端发送：**
- 文件信息包: `[0xAB][0x01][长度2B][序号2B][文件名UTF-8][校验2B]`
- 数据包: `[0xAB][0x02][长度2B][序号2B][数据内容][校验2B]`
- 取消包: `[0xAB][0x03][0x00][0x06][校验2B]`

**服务器响应：**
- 响应包: `[0xAB][STATUS][长度2B][下一包序号2B][校验2B]`
  - STATUS: 0x01=成功, 0x02=校验错误, 0x03=超时/错误
  - 固定 8 字节长度

### 校验和算法

累加所有字节（除了最后 2 字节校验和），取低 16 位，大端序存储。
