---
name: pio-runner
description: Run PlatformIO commands in Git Bash environment. Use when you need to build, upload, clean, or monitor PlatformIO projects.
allowed-tools: Bash, Read, Write
disable-model-invocation: false
argument-hint: [pio-command] [arguments]
---

# PlatformIO Runner Skill

在 Git Bash 环境下运行 PlatformIO 命令，自动处理环境配置。

## 环境要求

- **Git Bash** 环境
- **PlatformIO Core** 已安装
- **项目目录** 包含 `platformio.ini` 配置文件

## 配置

**默认配置**：见 [activate-pio-env.sh](activate-pio-env.sh) 脚本头部

**自定义配置**（通过环境变量覆盖）：
```bash
export PIO_PATH=/path/to/platformio/Scripts
```

## 执行流程

### 1. 激活 PlatformIO 环境

```bash
source .claude/skills/pio-runner/activate-pio-env.sh
```

**成功标志**：输出 `[SUCCESS] PlatformIO 环境已激活` 和版本信息

### 2. 运行 PlatformIO 命令

```bash
pio [command] [options]
```

### 3. 检查执行结果

检查命令输出和退出码（`$?`），如有错误分析并修复。

## 常用命令快速参考

```bash
# 编译项目
pio run

# 上传到设备
pio run -t upload

# 清理构建
pio run -t clean

# 串口监视器
pio run -t monitor

# 列出设备
pio device list
```

## 快速示例

```bash
# 基础编译
source .claude/skills/pio-runner/activate-pio-env.sh
pio run

# 编译并上传
pio run -t upload

# 清理后重新编译
pio run -t clean && pio run

# 上传后打开串口监视器
pio run -t upload && pio run -t monitor
```

## 故障排查

遇到问题时，查阅 [examples.md](examples.md) 获取：
- 详细使用示例（项目管理、编译、上传、调试）
- 常见错误处理（pio 不可用、编译失败、上传失败）
- 高级用法（多环境编译、自定义参数、并行构建）
- 集成场景（CI/CD、自动化测试）

## 参考资源

- **详细示例和故障排查**：[examples.md](examples.md)
- **环境激活脚本**：[activate-pio-env.sh](activate-pio-env.sh)
- **PlatformIO 官方文档**：https://docs.platformio.org

---

**版本**: 1.0 | **维护者**: 建豪 (JianHao) | **创建**: 2026-01-29
