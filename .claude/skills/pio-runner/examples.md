# PlatformIO Runner - 使用示例

本文档提供 `pio-runner` skill 的详细使用示例。

## 快速开始

### 基础用法

```bash
# 1. 激活 PlatformIO 环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 2. 运行 PlatformIO 命令
pio run
```

## 实际场景示例

### 场景 1：项目编译

基础编译操作：

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 编译项目（默认环境）
pio run

# 编译指定环境
pio run -e esp32dev

# 详细输出模式
pio run -v

# 静默模式
pio run -s
```

### 场景 2：上传固件

将编译好的固件上传到设备：

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 编译并上传
pio run -t upload

# 指定环境上传
pio run -e esp32dev -t upload

# 指定串口上传
pio run -t upload --upload-port COM3

# 上传后自动打开监视器
pio run -t upload && pio run -t monitor
```

### 场景 3：串口监视器

监控设备输出：

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 打开串口监视器
pio run -t monitor

# 指定波特率
pio device monitor --baud 115200

# 指定串口
pio device monitor --port COM3

# 退出监视器：Ctrl+C
```

### 场景 4：清理构建

清理编译产物：

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 清理构建文件
pio run -t clean

# 清理后重新编译
pio run -t clean && pio run

# 完全清理（包括依赖）
rm -rf .pio
pio run
```

### 场景 5：设备管理

列出和管理连接的设备：

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 列出所有串口设备
pio device list

# 显示详细信息
pio device list --serial

# 监控设备连接状态
watch -n 1 'pio device list'
```

## 高级用法

### 多环境编译

在 `platformio.ini` 中定义多个环境：

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

[env:esp32-release]
platform = espressif32
board = esp32dev
framework = arduino
build_flags = -DRELEASE_MODE
```

编译特定环境：

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 编译所有环境
pio run

# 编译指定环境
pio run -e esp32dev
pio run -e esp32-release

# 编译多个环境
pio run -e esp32dev -e esp32-release
```

### 自定义构建参数

使用构建标志和选项：

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 添加构建标志
pio run --build-flags="-DDEBUG_MODE"

# 设置构建目录
pio run --build-dir custom_build

# 并行构建（加速编译）
pio run -j 4
```

### 依赖管理

管理项目库依赖：

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 安装库
pio lib install "ArduinoJson"

# 搜索库
pio lib search "wifi"

# 列出已安装的库
pio lib list

# 更新库
pio lib update

# 卸载库
pio lib uninstall "ArduinoJson"
```

### 项目初始化

创建新的 PlatformIO 项目：

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 初始化项目
pio project init --board esp32dev

# 指定框架
pio project init --board esp32dev --project-option "framework=arduino"

# 在现有目录初始化
pio project init --board esp32dev --project-dir ./my_project
```

## 在 Claude Code 中的使用

### 示例 1：让 Claude 编译项目

**用户请求**：
```
编译 PlatformIO 项目
```

**Claude 执行**：
```bash
# 激活 PlatformIO 环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 编译项目
pio run
```

### 示例 2：让 Claude 上传固件

**用户请求**：
```
编译并上传固件到 ESP32
```

**Claude 执行**：
```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 编译并上传
pio run -t upload

# 检查结果
if [ $? -eq 0 ]; then
    echo "上传成功！"
else
    echo "上传失败，请检查设备连接"
fi
```

### 示例 3：让 Claude 清理并重新编译

**用户请求**：
```
清理构建文件并重新编译
```

**Claude 执行**：
```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 清理
pio run -t clean

# 重新编译
pio run
```

### 示例 4：让 Claude 打开串口监视器

**用户请求**：
```
打开串口监视器查看输出
```

**Claude 执行**：
```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 列出设备
echo "可用设备："
pio device list

# 打开监视器
pio run -t monitor
```

## 故障排查示例

### 检查环境是否正确激活

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 验证环境
which pio
pio --version
echo $PATH | grep platformio
```

### 检查项目配置

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 检查 platformio.ini 是否存在
if [ -f platformio.ini ]; then
    echo "找到 platformio.ini"
    cat platformio.ini
else
    echo "错误：未找到 platformio.ini，请确认在项目根目录"
fi

# 检查项目配置
pio project config
```

### 调试编译问题

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 详细模式编译
pio run -v

# 显示编译器版本
pio run -t compiledb

# 检查平台和工具链
pio platform list
pio platform show espressif32
```

### 调试上传问题

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 列出所有串口
pio device list

# 检查设备权限（Linux/Mac）
ls -l /dev/ttyUSB* /dev/ttyACM*

# 手动指定串口上传
pio run -t upload --upload-port /dev/ttyUSB0

# 使用详细模式查看上传过程
pio run -t upload -v
```

## 性能优化示例

### 并行编译

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 使用多核编译（4 个并行任务）
pio run -j 4

# 自动检测 CPU 核心数
pio run -j $(nproc)
```

### 增量编译

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 默认就是增量编译，只编译修改的文件
pio run

# 强制完全重新编译
pio run -t clean && pio run
```

### 缓存管理

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 查看缓存大小
du -sh .pio

# 清理缓存
pio run -t clean

# 完全清理（包括下载的工具链）
rm -rf .pio
```

## 集成示例

### 与 Git 工作流集成

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 编译前检查
pio run -t clean
pio run

# 如果编译成功，提交代码
if [ $? -eq 0 ]; then
    git add .
    git commit -m "feat: add new feature"
    echo "编译通过，代码已提交"
else
    echo "编译失败，请修复后再提交"
fi
```

### 自动化测试

```bash
# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 运行单元测试
pio test

# 运行指定测试
pio test -f test_basic

# 详细测试输出
pio test -v
```

### CI/CD 集成

```bash
#!/bin/bash
# ci-build.sh

set -e  # 遇到错误立即退出

# 激活环境
source .claude/skills/pio-runner/activate-pio-env.sh

# 清理
echo "清理构建..."
pio run -t clean

# 编译
echo "编译项目..."
pio run

# 运行测试
echo "运行测试..."
pio test

echo "CI 构建成功！"
```

## 常见问题处理

### 问题 1：pio 命令不可用

**错误信息**：
```
[ERROR] pio 命令不可用，请检查安装
```

**解决方案**：
1. 检查 `PIO_PATH` 是否正确
2. 确认 PlatformIO 已安装：`ls -l $PIO_PATH/pio.exe`
3. 手动测试：`$PIO_PATH/pio.exe --version`

### 问题 2：编译失败

**常见原因**：
- 缺少依赖库
- 平台或框架版本不兼容
- 代码语法错误

**解决方案**：
```bash
# 更新平台
pio platform update

# 重新安装依赖
pio lib install

# 清理后重新编译
pio run -t clean && pio run -v
```

### 问题 3：上传失败

**常见原因**：
- 设备未连接
- 串口被占用
- 驱动未安装

**解决方案**：
```bash
# 检查设备连接
pio device list

# 关闭占用串口的程序（如串口监视器）
# 然后重新上传
pio run -t upload

# Windows 下可能需要安装 CH340/CP2102 驱动
```

### 问题 4：串口监视器乱码

**解决方案**：
```bash
# 检查波特率是否匹配
pio device monitor --baud 115200

# 在 platformio.ini 中设置
# monitor_speed = 115200
```

## 注意事项

1. **工作目录**：必须在包含 `platformio.ini` 的项目根目录执行命令
2. **串口占用**：上传前确保串口监视器已关闭
3. **权限问题**：Linux/Mac 下可能需要将用户添加到 dialout 组
4. **路径空格**：避免项目路径包含空格或中文
5. **环境隔离**：每次打开新的 Git Bash 窗口都需要重新激活环境

## 参考资源

- **PlatformIO Core CLI 文档**：https://docs.platformio.org/en/latest/core/index.html
- **platformio.ini 配置**：https://docs.platformio.org/en/latest/projectconf/index.html
- **库管理**：https://docs.platformio.org/en/latest/librarymanager/index.html
- **调试指南**：https://docs.platformio.org/en/latest/plus/debugging.html

---

**更新时间**: 2026-01-29
**维护者**: 建豪 (JianHao)
