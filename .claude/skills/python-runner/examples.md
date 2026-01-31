# Python Runner - 使用示例

本文档提供 `python-runner` skill 的详细使用示例。

## 快速开始

### 基础用法

```bash
# 1. 激活 Python 环境
source .claude/skills/python-runner/activate-python-env.sh

# 2. 运行 Python 脚本
python your_script.py
```

## 实际场景示例

### 场景 1：数据分析脚本

假设您有一个数据分析脚本 `analyze_data.py`：

```python
# analyze_data.py
import pandas as pd
import sys

def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_data.py <csv_file>")
        sys.exit(1)

    csv_file = sys.argv[1]
    df = pd.read_csv(csv_file)

    print(f"数据行数: {len(df)}")
    print(f"数据列数: {len(df.columns)}")
    print("\n数据摘要:")
    print(df.describe())

if __name__ == "__main__":
    main()
```

**运行方式**：

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 运行分析
python analyze_data.py data/sales.csv
```

### 场景 2：代码格式化工具

使用 Python 工具格式化代码：

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 使用 black 格式化代码
python -m black src/

# 使用 autopep8 格式化单个文件
python -m autopep8 --in-place --aggressive script.py
```

### 场景 3：自动化测试

运行 Python 测试套件：

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 运行 pytest
python -m pytest tests/ -v

# 运行特定测试文件
python -m pytest tests/test_utils.py

# 运行并生成覆盖率报告
python -m pytest --cov=src tests/
```

### 场景 4：文件处理脚本

批量处理文件：

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 运行文件转换脚本
python convert_files.py --input-dir ./raw --output-dir ./processed --format json
```

### 场景 5：Web 爬虫脚本

运行数据采集脚本：

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 运行爬虫（带日志输出）
python scraper.py --url "https://example.com" --output data.json 2>&1 | tee scraper.log
```

## 高级用法

### 使用不同的 Python 环境

```bash
# 使用 Python 3.8 环境
export PYTHON_ENV_NAME=py38
source .claude/skills/python-runner/activate-python-env.sh
python modern_script.py

# 使用其他环境
export PYTHON_ENV_NAME=your_env_name
source .claude/skills/python-runner/activate-python-env.sh
python legacy_script.py
```

### 自定义 Conda 路径

```bash
# 使用自定义 conda 安装路径
export CONDA_ROOT=/D/Tools/Anaconda3
export PYTHON_ENV_NAME=myenv
source .claude/skills/python-runner/activate-python-env.sh
python script.py
```

### 捕获和分析输出

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 运行脚本并保存输出
python script.py > output.txt 2>&1

# 检查退出码
if [ $? -eq 0 ]; then
    echo "脚本执行成功"
    cat output.txt
else
    echo "脚本执行失败"
    cat output.txt
fi
```

### 管道和重定向

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 使用管道处理数据
cat input.txt | python process.py | python filter.py > output.txt

# 分离标准输出和错误输出
python script.py 1>stdout.log 2>stderr.log
```

## 在 Claude Code 中的使用

### 示例 1：让 Claude 运行测试脚本

**用户请求**：
```
请运行 test_connection.py 脚本测试网络连接
```

**Claude 执行**：
```bash
# 激活 Python 环境
source .claude/skills/python-runner/activate-python-env.sh

# 运行测试脚本
python test_connection.py
```

### 示例 2：让 Claude 执行数据处理

**用户请求**：
```
用 Python 脚本处理 data.csv，生成统计报告
```

**Claude 执行**：
```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 运行数据处理脚本
python process_data.py --input data.csv --output report.txt

# 显示结果
cat report.txt
```

### 示例 3：让 Claude 运行代码检查

**用户请求**：
```
用 pylint 检查 src/ 目录下的代码质量
```

**Claude 执行**：
```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 运行 pylint
python -m pylint src/ --output-format=text > pylint_report.txt

# 显示报告
cat pylint_report.txt
```

## 故障排查示例

### 检查环境是否正确激活

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 验证环境
echo "当前 conda 环境: $CONDA_DEFAULT_ENV"
which python
python --version
python -c "import sys; print(sys.executable)"
```

### 检查 Python 包是否安装

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 列出已安装的包
pip list

# 检查特定包
python -c "import pandas; print(pandas.__version__)"
```

### 调试脚本执行问题

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 使用详细模式运行
python -v script.py

# 使用调试模式
python -m pdb script.py

# 检查语法错误
python -m py_compile script.py
```

## 性能优化示例

### 并行执行多个脚本

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 后台运行多个脚本
python script1.py > log1.txt 2>&1 &
python script2.py > log2.txt 2>&1 &
python script3.py > log3.txt 2>&1 &

# 等待所有脚本完成
wait

# 检查结果
cat log1.txt log2.txt log3.txt
```

### 使用超时控制

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 限制脚本运行时间（60秒）
timeout 60 python long_running_script.py

# 检查退出码
if [ $? -eq 124 ]; then
    echo "脚本超时被终止"
else
    echo "脚本正常完成"
fi
```

## 集成示例

### 与 Git 工作流集成

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 运行预提交检查
python pre_commit_check.py

# 如果检查通过，提交代码
if [ $? -eq 0 ]; then
    git add .
    git commit -m "feat: add new feature"
else
    echo "预提交检查失败，请修复问题后再提交"
fi
```

### 与构建流程集成

```bash
# 激活环境
source .claude/skills/python-runner/activate-python-env.sh

# 运行构建前脚本
python prebuild.py

# 执行构建
./build.sh

# 运行构建后脚本
python postbuild.py
```

## 注意事项

1. **环境隔离**：每次打开新的 Git Bash 窗口都需要重新激活环境
2. **路径问题**：使用绝对路径或确保在正确的工作目录
3. **依赖管理**：确保 Python 环境中已安装脚本所需的所有依赖
4. **权限问题**：某些脚本可能需要管理员权限
5. **编码问题**：注意文件编码，建议使用 UTF-8

---

**更新时间**: 2026-01-28
**维护者**: 建豪 (JianHao)
