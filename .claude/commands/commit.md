---
description: Create git commit following Conventional Commits specification
allowed-tools: Bash(git add:*), Bash(git status:*), Bash(git commit:*)
argument-hint: Commit message
---

## 上下文

- 当前 git 状态: !`git status`
- 当前 git 差异 (已暂存): !`git diff --cached`
- 当前 git 差异 (未暂存): !`git diff`
- 当前分支: !`git branch --show-current`
<!-- 近期提交: !`git log --oneline -10` -->
<!-- 注: 上行在 Git Bash 中有兼容性问题，暂时禁用 -->

## 提交规范

创建 git 提交时，必须遵循以下规范：

### 1. Conventional Commits 格式

提交消息采用 Conventional Commits 规范，格式为：

```
<type>(<scope>): <subject>

[optional body]

[optional footer]
```

**常用 type:**
- `feat`: 新功能
- `fix`: 修复 bug
- `refactor`: 重构（既不是新功能也不是 bug 修复）
- `perf`: 性能优化
- `docs`: 文档更新
- `style`: 代码格式调整（不影响功能）
- `test`: 测试相关
- `chore`: 构建/工具链相关

**scope 示例:**
- `wifi_usb`: WiFi-USB 核心组件
- `udp_transfer`: UDP 传输层
- `uvc`: UVC 设备类
- `ap_host`: AP 主机端
- `sta_device`: STA 设备端
- `build`: 构建系统

### 2. 语言规范

- **使用中文** 编写提交说明，确保表述清晰易懂
- **保留专业英语术语**，不强制翻译常用技术词汇，例如：
  - 网络: `recv`, `send`, `wifi`, `udp`, `tcp`, `server`, `client`, `socket`
  - USB: `device`, `host`, `endpoint`, `transfer`, `frame`, `packet`
  - 通用: `buffer`, `timeout`, `callback`, `init`, `deinit`
- **使用 ASCII 符号**，所有标点符号必须使用 ASCII 字符（如 `,` `.` `:` `-` 等）

### 3. 提交消息示例

**示例 1 - 新功能:**
```
feat(uvc): 添加视频帧率动态调整功能

- 实现帧率自适应算法，根据网络延迟调整
- 添加 frame_rate_adjust() 函数
- 支持 15/30/60 fps 切换
```

**示例 2 - Bug 修复:**
```
fix(udp_transfer): 修复 recv 超时导致的连接断开问题

在 wifi_usb_udp_recv() 中增加重试机制，避免偶发性超时导致连接中断。
```

**示例 3 - 重构:**
```
refactor(wifi_usb): 统一 buffer 管理接口

- 将 shared_buffer 和 uvc_transfer_buffer 封装到 wifi_usb_buffer_mgr
- 简化内存分配逻辑
```

**示例 4 - 性能优化:**
```
perf(sta_device): 优化 UDP send 批量发送性能

使用零拷贝机制，减少 memcpy 开销，吞吐量提升 20%。
```

## 任务

基于以上更改和提交规范，创建单个 git 提交。

**重要:** 提交消息中**不要**包含以下内容：
- Claude Code 生成标识（如 "Generated with Claude Code"）
- Co-Authored-By 签名
- 任何其他工具签名信息

提交消息应仅包含符合上述 Conventional Commits 规范的实际变更说明。