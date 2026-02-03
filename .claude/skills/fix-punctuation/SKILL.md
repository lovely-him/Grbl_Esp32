---
name: fix-punctuation
description: Format code files by converting Chinese punctuation to English punctuation. Supports .c .cpp .h .py .js .java and all code files
user-invocable: true
allowed-tools: Bash(python *)
argument-hint: [file-path-or-pattern]
---

# 修复代码中的中文标点符号

将代码文件中的中文标点符号（如 `，。；：` 等）转换为对应的英文标点符号（如 `, . ; :` 等）。

## 使用方式

- `/fix-punctuation path/to/file.c` - 修复单个文件
- `/fix-punctuation src/**/*.cpp` - 修复匹配模式的所有文件
- `/fix-punctuation` - 修复当前目录下所有代码文件

## 支持的文件类型

自动识别以下代码文件扩展名：
- C/C++: `.c`, `.cpp`, `.h`, `.hpp`, `.cc`, `.cxx`
- Python: `.py`
- JavaScript/TypeScript: `.js`, `.ts`, `.jsx`, `.tsx`
- Java: `.java`
- C#: `.cs`
- Go: `.go`
- Rust: `.rs`
- 配置文件: `.json`, `.yaml`, `.yml`

## 执行步骤

1. **前置检查: Python 环境**

   按照以下顺序检查 Python 可用性：

   a. 尝试运行 `python --version` 或 `python3 --version`
      - 如果返回版本号（exit code 0），Python 可用，跳到步骤 2

   c. 如果步骤 a 失败，检查 `.claude/skills/` 目录：
      - 使用 Glob 工具查找 `.claude/skills/*/SKILL.md`
      - 读取每个 SKILL.md 的 frontmatter，查找 `allowed-tools` 包含 `python` 的 skill
      - 如果找到，询问用户是否需要先启用该 skill

   b. 如果以上都失败，停止执行并报错：
      ```
      错误：未找到 Python 环境

      此 skill 需要 Python 环境才能运行。请：
      1. 安装 Python (https://www.python.org/downloads/)
      2. 或者检查是否有其他 skill 可以启用 Python 环境
      ```

2. **解析参数**
   - 如果提供了参数 `$ARGUMENTS`, 使用该路径或模式
   - 如果未提供参数, 默认处理当前目录下所有代码文件（使用 `.` 作为参数）

3. **调用 Python 脚本**

   根据步骤 1 确定的 Python 命令，执行：
   ```bash
   <python命令> .claude/skills/fix-punctuation/scripts/fix_chinese_punctuation.py <参数>
   ```

   注意事项：
   - 如果在 Windows 环境下，路径分隔符使用 `/` 而非 `\`
   - 设置超时时间为 120 秒（`timeout: 120000`）
   - 脚本会自动：
     - 查找匹配的文件（支持通配符）
     - 检查是否为代码文件
     - 替换中文标点为英文标点
     - 保持代码结构和缩进不变

4. **报告结果**
   - 脚本会输出处理的文件列表
   - 显示每个文件的修改统计
   - 如果没有找到需要修复的文件，提示用户

## 注意事项

- **仅修改标点符号**：不会修改中文注释或字符串中的中文文字内容
- **保持格式**：保持原有的缩进、换行和代码结构
- **安全操作**：修改前会先读取文件，确保文件存在且可读
- **批量处理**：支持使用通配符批量处理多个文件

## 实现细节

使用 Python 脚本进行批量替换，确保：
1. 只替换标点符号，不影响其他内容
2. 保持原有的空格和缩进
3. 一次性处理所有替换，高效快速
4. 如果文件没有中文标点，跳过不处理
5. 支持通配符模式批量处理多个文件

---

**开始执行任务**：

根据提供的参数 `$ARGUMENTS`，按照上述步骤处理文件。
