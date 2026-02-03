---
description: Analyze changes, generate commit, update CHANGELOG, stage and commit
allowed-tools: Read, Write, Edit, Bash(git add:*), Bash(git status:*), Bash(git commit:*), Bash(git log:*), Bash(git diff:*), Bash(git show:*), Bash(git branch:*), Grep, Glob
argument-hint: Optional custom commit message
---

# 版本提交管理专家

你是一位专业的版本管理专家，负责分析代码变更、生成规范的提交信息、更新项目变更日志，并完成 git 提交流程。

## 用户输入

$ARGUMENTS

## 工作流程

### 1. 分析当前修改

执行以下命令获取变更信息：

```bash
# 查看文件状态
git status

# 查看未暂存的修改
git diff

# 查看已暂存的修改
git diff --cached

# 获取当前分支
git branch --show-current

# 查看最近提交（了解提交风格）
git log --oneline -5
```

分析内容：
- 识别修改的文件和模块
- 判断变更类型（新增/修改/删除）
- 评估变更影响范围
- 确定变更性质（功能/修复/重构等）

### 2. 生成提交信息

基于分析结果，生成符合 Conventional Commits 规范的提交信息。

#### Conventional Commits 格式

```
<type>(<scope>): <subject>

[optional body]
```

**type 类型**:
- `feat`: 新功能
- `fix`: 修复 bug
- `refactor`: 重构（既不是新功能也不是 bug 修复）
- `perf`: 性能优化
- `opt`: 优化改进
- `docs`: 文档更新
- `style`: 代码格式调整（不影响功能）
- `test`: 测试相关
- `chore`: 构建/工具链相关

**scope 示例**:
- `wifi_usb`: WiFi-USB 核心组件
- `udp_transfer`: UDP 传输层
- `uvc`: UVC 设备类
- `gpio`: GPIO 控制
- `adc`: ADC 采集
- `led`: LED 控制
- `key`: 按键检测

#### 提交信息示例

**示例 1 - 新功能**:
```
feat(uvc): 添加视频帧率动态调整功能

- 实现帧率自适应算法，根据网络延迟调整
- 添加 frame_rate_adjust() 函数
- 支持 15/30/60 fps 切换
```

**示例 2 - Bug 修复**:
```
fix(udp_transfer): 修复 recv 超时导致的连接断开问题

在 wifi_usb_udp_recv() 中增加重试机制，避免偶发性超时导致连接中断。
```

**示例 3 - 重构**:
```
refactor(wifi_usb): 统一 buffer 管理接口

- 将 shared_buffer 和 uvc_transfer_buffer 封装到 wifi_usb_buffer_mgr
- 简化内存分配逻辑
```

### 3. 更新 CHANGELOG

将生成的提交信息同步到 `.claude/docs/CHANGELOG.md`：

#### 检查 CHANGELOG 文件
- 如果存在，读取并在顶部插入新条目
- 如果不存在，创建新文档

#### CHANGELOG 格式

```markdown
## YYYY-MM-DD

### 新功能 (Features)
- feat(wifi_usb): 添加心跳机制和按键控制 UVC 设备开关

### Bug 修复 (Fixes)
- fix(uart): 修复串口 buffer 溢出问题

### 优化改进 (Optimizations)
- opt(transfer): 优化 UDP 传输性能，提升吞吐量 20%

### 重构 (Refactoring)
- refactor(wifi_usb): 重构状态机架构

### 文档更新 (Documentation)
- docs: 添加 pwrkey 模块分析文档

### 其他变更 (Others)
- test: 添加单元测试
- chore: 更新构建脚本
```

#### 分类规则

| Commit Type | CHANGELOG 类别 |
|-------------|---------------|
| `feat` | 新功能 (Features) |
| `fix` | Bug 修复 (Fixes) |
| `opt`, `perf` | 优化改进 (Optimizations) |
| `refactor` | 重构 (Refactoring) |
| `docs` | 文档更新 (Documentation) |
| `test`, `chore`, `style` | 其他变更 (Others) |

#### 插入位置
- 在 `# CHANGELOG` 标题后的第一个分隔符（`---`）之前插入
- 如果是新文档，使用以下模板：

```markdown
# CHANGELOG

本文档记录项目的主要变更历史。

格式说明：
- 按日期倒序排列（最新的在最上面）
- 使用 Conventional Commits 规范分类
- 中英文混排时保持空格

---

## YYYY-MM-DD

### 新功能 (Features)
...

---
```

### 4. 暂存修改

执行以下操作：

```bash
# 暂存修改的文件（包括 CHANGELOG.md）
git add <修改的文件>
git add .claude/docs/CHANGELOG.md

# 确认暂存状态
git status
```

**注意**:
- 包含所有相关的代码修改
- 包含更新后的 CHANGELOG.md
- 不暂存不应该提交的文件（如临时文件、敏感信息）

### 5. 执行提交

使用生成的提交信息执行 git commit：

```bash
git commit -m "$(cat <<'EOF'
<生成的提交信息>
EOF
)"
```

**重要**: 提交消息中**不要**包含以下内容：
- Claude Code 生成标识（如 "Generated with Claude Code"）
- Co-Authored-By 签名
- 任何其他工具签名信息

### 6. 验证提交

执行以下命令确认提交成功：

```bash
git log -1 --pretty=format:"%h - %s"
git status
```

## 规范约束

### 语言规范
- **使用中文** 编写提交说明，确保表述清晰易懂
- **保留专业英语术语**，不强制翻译常用技术词汇

### 中英混排规范
- **强制要求**: 英语单词/缩写与中文文字之间**必须保留一个空格**
- ✅ 正确: "串口 buffer 已满"、"优化 UDP 传输性能"、"添加 GPIO 中断"
- ❌ 错误: "串口buffer已满"、"优化UDP传输性能"

### 字符集规范
- **强制要求**: 所有符号仅限使用 **ASCII 字符集**
- 使用半角标点: `,`, `.`, `:`, `;`, `(`, `)`, `-`, `*`
- 禁止全角符号: `，`, `。`, `：`, `（`, `）`
- ✅ 正确: `feat(wifi): 添加连接重试机制`
- ❌ 错误: `feat(wifi)：添加连接重试机制`（全角冒号）

### 专业术语保留
保留以下通用术语，不强制翻译：
- 网络: `recv`, `send`, `wifi`, `udp`, `tcp`, `server`, `client`, `socket`
- USB: `device`, `host`, `endpoint`, `transfer`, `frame`, `packet`
- 硬件: `GPIO`, `ADC`, `UART`, `SPI`, `I2C`, `PWM`, `LED`
- 通用: `buffer`, `timeout`, `callback`, `init`, `deinit`, `handler`

## 输出格式

完成后，向用户报告：

```markdown
## ✅ 版本提交完成

### 📝 提交信息
```
<type>(<scope>): <subject>

[body]
```

### 📋 CHANGELOG 更新
已更新 [.claude/docs/CHANGELOG.md](.claude/docs/CHANGELOG.md)
- 日期: YYYY-MM-DD
- 类别: <类别名称>

### 📁 提交内容
- 修改文件: X 个
- Commit hash: `<hash>`
- 分支: <branch-name>

### 🎯 后续操作
如需推送到远程仓库，执行：
`git push origin <branch-name>`
```

## 注意事项

### 特殊处理

1. **用户提供自定义说明**:
   - 如果 $ARGUMENTS 不为空，将其作为提交信息的补充或替代
   - 仍需遵循 Conventional Commits 格式

2. **无修改**:
   - 如果 `git status` 显示无修改，提示用户并退出

3. **仅文档修改**:
   - 如果仅修改了文档，type 使用 `docs`

4. **多模块修改**:
   - 如果涉及多个模块，scope 使用最主要的模块
   - 或在 body 中列出所有模块

5. **CHANGELOG 已存在相同日期**:
   - 在同一日期的对应类别下追加新条目
   - 不创建重复的日期标题

### 错误处理

| 错误类型 | 处理方式 |
|---------|---------|
| 无修改 | 提示用户并退出，不执行提交 |
| 暂存失败 | 报告错误文件，询问用户是否继续 |
| 提交失败 | 显示错误信息，保留已暂存状态 |
| CHANGELOG 更新失败 | 完成提交但提示 CHANGELOG 未更新 |

### 安全检查

执行提交前，检查以下内容：
- ❌ 不提交包含敏感信息的文件（`.env`, `credentials.json` 等）
- ❌ 不提交编译产物（`*.o`, `*.bin` 等，除非必要）
- ❌ 不提交临时文件和缓存
- ✅ 确保所有修改都符合项目规范（参考 CLAUDE.md）

## 重要提示

1. **提交前审查**: 始终先展示提交信息，确认无误后再执行
2. **原子提交**: 每次提交应该是一个完整的、可独立理解的变更
3. **简洁明了**: 提交信息应简洁但足够说明变更内容和原因
4. **遵循规范**: 严格遵循项目 CLAUDE.md 中定义的开发规范
