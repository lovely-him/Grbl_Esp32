# Claude Code 自定义斜杠指令模板

本文档提供标准化的指令编写模板，用于创建 `.claude/commands/` 下的自定义斜杠指令。

---

## 📐 标准模板结构（混合策略）

```markdown
---
description: 简短描述
allowed-tools: 工具列表
argument-hint: [参数提示]
---

# [可选] 角色定义（仅复杂任务）
简短角色描述

## [必需] 上下文/输入
- 环境信息
- $ARGUMENTS（如需要）

## [必需] 规范/约束
关键规则和约束

## [必需] 任务/流程
- 简单任务：直接描述任务
- 复杂任务：分步骤工作流程

## [可选] 输出格式
仅在需要统一输出时添加

## [可选] 注意事项
边界情况和特殊处理
```

---

## 📝 章节说明

### Frontmatter（必需）

```yaml
---
description: 动词+名词形式，简洁明了（如：生成测试用例）
allowed-tools: Read, Write, Edit, Bash, Grep, Glob, WebSearch
argument-hint: [参数说明]  # 可选，帮助用户理解如何使用
model: sonnet  # 可选，默认 sonnet，简单任务用 haiku，复杂任务用 opus
---
```

**说明**:
- `description`: 显示在 `/help` 中，应简洁明了
- `allowed-tools`: 限制指令可用的工具，遵循最小权限原则
- `argument-hint`: 提供参数提示，改善用户体验
- `model`: 指定使用的 Claude 模型

---

### 角色定义（可选，仅复杂任务）

```markdown
# 角色名称

你是一位 [领域] 专家，负责 [职责描述]。
```

**何时使用**:
- ✅ 复杂的多步骤任务
- ✅ 需要专业领域知识
- ✅ 跨项目通用指令

**何时省略**:
- ❌ 简单的单步操作
- ❌ 高频使用的快捷指令
- ❌ 项目特定的约定任务

---

### 上下文/输入（必需）

```markdown
## 上下文

- 环境信息: !`command`  # Bash 命令执行，结果插入提示词
- 当前状态: !`git status`
- 用户输入: $ARGUMENTS  # 接收用户参数

## 用户输入

$ARGUMENTS
```

**说明**:
- 使用 `!`backtick`command`backtick 执行 Bash 命令并插入输出
- 使用 `$ARGUMENTS` 接收所有用户输入参数
- 使用 `$1`, `$2`, `$3` 接收位置参数
- 使用 `@file.txt` 自动读取文件内容

---

### 规范/约束（必需）

```markdown
## 规范

关键规则和约束：
- 规则 1
- 规则 2
- 规则 3

### [可选] 格式规范
具体格式要求

### [可选] 质量标准
输出质量标准
```

**说明**:
- 明确指令执行时必须遵守的规则
- 可包含格式规范、命名约定、代码风格等
- 建议将通用规范（如中英混排、ASCII 字符）单独列出

---

### 任务/流程（必需）

**简单任务**（直接描述）:
```markdown
## 任务

基于以上信息，执行 [具体任务描述]。

**重要**: 注意事项。
```

**复杂任务**（分步骤）:
```markdown
## 工作流程

### 1. 步骤一名称
- 子步骤 1
- 子步骤 2

### 2. 步骤二名称
- 子步骤 1
- 子步骤 2

### 3. 步骤三名称
- 子步骤 1
- 子步骤 2
```

**说明**:
- 简单任务用一段话描述即可
- 复杂任务用编号步骤，确保执行顺序
- 每个步骤应该清晰、可执行、不遗漏

---

### 输出格式（可选）

```markdown
## 输出格式

完成后，报告：

```markdown
## 标题

### 摘要
- 关键信息 1
- 关键信息 2

### 详细信息
具体内容
```
```

**何时使用**:
- ✅ 需要统一输出格式的场景
- ✅ 输出会被其他工具解析
- ✅ 多次执行需要对比结果

**何时省略**:
- ❌ 输出格式灵活多变
- ❌ Claude 可以自行决定如何报告

---

### 注意事项（可选）

```markdown
## 注意事项

- 边界情况 1: 处理方式
- 边界情况 2: 处理方式
- 特殊处理: 说明

## 错误处理

| 错误类型 | 处理方式 |
|---------|---------|
| 错误 1 | 处理策略 |
| 错误 2 | 处理策略 |
```

**说明**:
- 记录边界情况和特殊处理逻辑
- 避免 Claude 在异常情况下执行错误操作
- 提供错误恢复建议

---

## 🎯 实践建议

### 1. 命令命名

| 推荐 | 避免 |
|------|------|
| `/test`, `/fix`, `/opt` | `/generate-test-cases-for-all-files` |
| `/commit`, `/review` | `/do-something` |
| `/refactor` | `/refactor-code-with-best-practices` |

**原则**: 短小精悍，动词或动词短语

---

### 2. 参数处理

```markdown
# 接收所有参数
$ARGUMENTS

# 位置参数
$1, $2, $3, ...

# 推荐用法
## 用户输入
$ARGUMENTS
```

---

### 3. Bash 命令执行

```markdown
# 内联执行（自动插入输出）
- 当前分支: !`git branch --show-current`
- 文件状态: !`git status`

# 需要在 allowed-tools 授权
allowed-tools: Bash(git status:*), Bash(git log:*)
```

---

## 📚 参考文档

### 官方文档
- [Claude Code - Slash Commands](https://code.claude.com/docs/en/slash-commands)
- [Claude Agent SDK - Slash Commands](https://platform.claude.com/docs/en/agent-sdk/slash-commands)
- [Conventional Commits](https://www.conventionalcommits.org/)

### 实践案例
- [GitHub: wshobson/commands](https://github.com/wshobson/commands) - 生产级指令集合
- [GitHub: hesreallyhim/awesome-claude-code](https://github.com/hesreallyhim/awesome-claude-code) - 精选资源

---

**最后更新**: 2026-01-09
**维护者**: 建豪 (JianHao)