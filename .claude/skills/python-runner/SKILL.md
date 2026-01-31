---
name: python-runner
description: Run Python scripts in Git Bash environment with conda activation. Use when you need to execute Python scripts, test Python code, or run Python-based tools.
allowed-tools: Bash, Read, Write
disable-model-invocation: false
argument-hint: [script-path] [script-arguments]
---

# Python Runner Skill

在 Git Bash 环境下运行 Python 脚本，自动处理 conda 环境激活。

## 环境要求

- **Git Bash** 环境
- **Conda** 已安装
- **Python 环境** 已创建

## 配置

**默认配置**：见 [activate-python-env.sh](activate-python-env.sh) 脚本头部

**自定义配置**（通过环境变量覆盖）：
```bash
export CONDA_ROOT=/path/to/your/conda
export PYTHON_ENV_NAME=your_env_name
```

## 执行流程

### 1. 激活 Python 环境

```bash
source .claude/skills/python-runner/activate-python-env.sh
```

**成功标志**：输出 `[SUCCESS] Python 环境已激活: <环境名>`

### 2. 运行 Python 脚本

```bash
python /path/to/script.py [arguments]
```

### 3. 检查执行结果

检查脚本输出和退出码（`$?`），如有错误分析并修复。

## 快速示例

```bash
# 基础用法
source .claude/skills/python-runner/activate-python-env.sh
python test_script.py

# 带参数
python data_processor.py --input data.csv --output result.json

# 捕获输出
python analysis.py > output.log 2>&1
```

## 故障排查

遇到问题时，查阅 [examples.md](examples.md) 获取：
- 详细使用示例（数据分析、测试、代码检查等）
- 常见错误处理（conda 不可用、环境激活失败等）
- 高级用法（自定义环境、并行执行、超时控制）
- 集成场景（Git 工作流、构建流程）

## 参考资源

- **详细示例和故障排查**：[examples.md](examples.md)
- **环境激活脚本**：[activate-python-env.sh](activate-python-env.sh)

---

**版本**: 1.0 | **维护者**: 建豪 (JianHao) | **创建**: 2026-01-28
