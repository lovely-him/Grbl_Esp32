# CHANGELOG

本文档记录项目的主要变更历史。

格式说明：
- 按日期倒序排列（最新的在最上面）
- 使用 Conventional Commits 规范分类
- 中英文混排时保持空格

---

## 2026-03-21

### 新功能 (Features)
- feat(him): 新增 WIFI_AP_CONNECTING LED 状态, 区分 STA 连接和 AP 配网模式
  - STA 连接中: 慢速等间隔闪烁 (500ms 周期)
  - AP 配网中: 快速 3 次闪烁 (100ms 间隔)

### 优化改进 (Optimizations)
- opt(him): 超长按 10s 改为直接恢复出厂设置 (settings_restore All)
- opt(him): 安全门 LED 模式改为等间隔快速闪烁 (200ms 周期)
- opt(wifi): WiFi STA 重试次数从 0 改为 3, 提升连接稳定性
- opt(him): 注释掉默认 STA WiFi 连接配置, 默认使用 AP 模式

### 重构 (Refactoring)
- refactor(him): 重命名 LED_PATTERN_WIFI_CONNECTING 为 LED_PATTERN_STA_CONNECTING
- refactor(him): 删除注释中的 emoji 字符, 提高跨平台兼容性
- refactor(build): 重构 merge_firmware.bat, 自动检测 framework 路径替代硬编码

---

## 2026-02-11

### 新功能 (Features)
- feat(mdns): 新增支持自定义超时的 mDNS 查询函数
  - `query_mdns_host()` 新增 `timeout_ms` 参数, 默认 20 秒
  - 新增 `query_mdns_services_with_timeout()` 函数, 使用 ESP-IDF 底层 API
  - 支持最多 20 个查询结果, 自动识别本机服务
  - 优化日志输出, 显示超时时间和查询结果详情
- feat(button): 新增按钮批量查询主机名功能
  - 新增 `$Button/Idle/Action=3` 选项
  - 支持查询基础主机名和 `-2` 到 `-5` 后缀变体
  - 集成 `query_mdns_services_with_timeout()` 查询 HTTP 服务

### Bug 修复 (Fixes)
- fix(test): 修复 Python mDNS 测试脚本的解析问题
  - 使用 `zeroconf` 库替代 `socket.getaddrinfo()` (不支持 mDNS)
  - 支持多种服务类型浏览 (_http, _workstation, _device-info)
  - 修复主机名匹配逻辑, 正确处理 FQDN 格式
  - 避免重复添加服务, 优化查询结果展示

### 优化改进 (Optimizations)
- opt(skills): 优化 fix-punctuation 工具的换行符处理
  - 使用 `newline=''` 参数保留原始换行符格式
  - 避免 Windows 环境下的 CRLF/LF 转换问题
  - 新增 `disable-model-invocation: true` 配置, 提高执行效率

### 文档更新 (Documentation)
- docs: 新增 Windows 无法发现 ESP32 mDNS 服务问题总结
  - 分析 IGMP 版本不匹配问题 (Windows IGMPv3 vs ESP32 IGMPv2)
  - 说明 Windows 不原生支持 mDNS 解析的限制
  - 提供 3 种解决方案: 修改 IGMP 版本/安装 Bonjour/使用 zeroconf 库
  - 新增 Wireshark 抓包数据 (IGMPv2.pcapng, IGMPv2_v3.pcapng)
  - 包含抓包数据的 C 数组和 CSV 格式导出
- docs: 移除旧的 mDNS 已知问题汇总文档
  - 内容已整合到新的 Windows 问题总结文档中

### 其他变更 (Others)
- chore: 新增 Claude Code 设置文件到版本控制
  - 配置 Bash 命令权限白名单 (cat, ls, git)
  - 禁止危险的 git 操作 (reset --hard, push --force 等)
  - 设置环境变量 (最大输出 token, 禁用非必要流量)
- chore: 更新 .gitignore, 允许提交 .claude/settings.json

---

## 2026-02-10

### 重构 (Refactoring)
- refactor(safety): 重构安全门状态检测机制, 使用弱函数覆盖
  - 新增 `custom_safety_door_check()` 弱函数, 允许机器特定代码覆盖
  - 在 `system_check_safety_door_ajar()` 中调用自定义检测函数
  - 移除直接操作 `sys.suspend.bit.safetyDoorAjar` 的代码
  - 使用 `safety_door_ajar` 静态标志位统一管理安全状态
  - 提高代码解耦性和可扩展性

### 新功能 (Features)
- feat(mdns): 添加 mDNS 查询和重初始化辅助函数
  - `query_mdns_host()`: 查询指定主机名的 IP 地址
  - `query_mdns_services()`: 查询网络中提供指定服务的所有设备
  - `reinit_mdns()`: 重新初始化 mDNS 服务, 强制重新广播
  - 支持 ENABLE_MDNS 条件编译, 提供空实现
- feat(button): 添加按钮 Idle 动作配置
  - 新增 `$Button/Idle/Action` 设置项 (0-99)
  - 支持 3 种动作: 0=预览边界, 1=归零, 2=mDNS 测试
  - 允许用户自定义 Idle 状态下的按钮行为

### 优化改进 (Optimizations)
- opt(i2c): 优化 I2C 初始化顺序和互斥锁管理
  - 将 I2C 互斥锁创建和总线初始化移到 `machine_init()`
  - 在 `init_tilt_sensor()` 中检查互斥锁是否已初始化
  - 添加互斥锁保护, 防止初始化时的竞态条件
  - 优化传感器重初始化逻辑, 使用 `need_reinit` 标志位
- opt(realtime): 统一使用 `execute_realtime_command()` 替代直接操作标志位
  - 替换 `sys_rt_exec_state.bit.*` 直接赋值为标准 API 调用
  - 提高代码规范性和可维护性
- opt(log): 优化日志级别, 减少冗余输出
  - 将按钮事件日志从 INFO 改为 DEBUG
  - 将状态切换日志从 INFO 改为 DEBUG
  - 保留关键操作的 INFO 日志

### 文档更新 (Documentation)
- docs: 新增 mDNS 测试工具集
  - `test_mdns_provider.py`: mDNS 服务提供者测试工具
  - `test_mdns_browser.py`: mDNS 服务浏览器测试工具
  - `diagnose_mdns.py`: mDNS 诊断工具
  - `test_mdns_check.bat`: 一键测试脚本
- docs: 新增 ESP32 mDNS 技术文档
  - `ESP32_mDNS_已知问题汇总.md`: 社区已知问题和解决方案
  - `ESP32_mDNS_数据包处理流程分析.md`: 源码级数据包处理流程
  - `mDNS网关与服务提供设备交互过程.md`: 网络交互详解
- docs: 移除旧的 mDNS 功能详解文档
  - 内容已整合到新的技术文档中

### 其他变更 (Others)
- chore(skills): 优化 fix-punctuation skill
  - 添加 Shell/批处理脚本支持 (.sh, .bat, .cmd, .ps1)
  - 优化 Python 环境检查逻辑, 优先使用 Python skill
  - 改进错误处理和用户提示
- style: 修复 Shell 脚本中的中文标点符号
  - 修复 `activate-pio-env.sh` 和 `activate-python-env.sh`
  - 统一使用英文逗号替代中文逗号

---

## 2026-02-09

### Bug 修复 (Fixes)
- fix(safety): 简化安全门状态恢复逻辑
  - 移除复杂的状态记忆和恢复机制
  - 使用 `sys.suspend.bit.safetyDoorAjar` 标志位统一管理
  - 短按按钮时检查门状态决定是否恢复
  - 提高代码可维护性和可靠性
- fix(webserver): 修复 DNS 服务器竞态条件
  - 等待 WiFi AP IP 初始化完成后再启动 DNS 服务器
  - 添加 `dnsServerStarted` 标志位跟踪启动状态
  - 在 `Web_Server::end()` 中正确停止 DNS 服务器
  - 防止 Captive Portal 启动失败
- fix(init): 修复电机对象初始化顺序问题
  - 将 `init_motors()` 移到 `stepper_init()` 之前
  - 避免定时器中断访问未初始化的电机对象
  - 防止归零时步进定时器中断导致的崩溃

### 优化改进 (Optimizations)
- opt(config): 优化 WiFi AP 配置
  - 将 AP SSID 从 `MACHINE_NAME` 改为 `Genmitsu_Kiosk`
  - 添加 STA/AP 模式配置注释说明
  - 提高配置可读性
- opt(log): 增强 SSDP 内存分配异常日志级别
  - 将 `log_n` 改为 `log_e` (错误级别)
  - 提高异常可见性，便于故障排查

### 文档更新 (Documentation)
- docs: 新增 mDNS 功能详解文档 (80KB)
  - mDNS 协议原理和工作机制
  - ESP32 mDNS 实现详解
  - 服务发现和主机名解析
  - 故障排查和最佳实践
- docs: 新增 3 个 ESP32 崩溃分析案例
  - WiFiUDP 断言失败崩溃分析
  - 归零时步进定时器中断访问未初始化电机对象崩溃分析
  - 看门狗超时崩溃分析
  - 包含完整的崩溃日志和根因分析
- docs: 新增 WiFi 服务启动优化文档
  - DNS 服务器竞态条件修复详解
  - WiFi AP 初始化时序分析
  - 解决方案和测试验证

### 其他变更 (Others)
- chore: 更新 PlatformIO 和 Python 环境激活脚本
- chore: 更新 .gitignore 忽略规则
- chore: 调整 platformio.ini 配置

---

## 2026-02-05

### 优化改进 (Optimizations)
- opt(wifi): 优化 10 秒长按 WiFi 重置功能
  - 直接操作 NVS 设置 WiFi 模式为 AP
  - 使用 ESP.restart() 替代命令系统重启
  - 提高重置可靠性, 避免依赖指令系统

### 重构 (Refactoring)
- refactor(file_transfer): 重构文件传输服务器数据包处理逻辑
  - 改用流式处理替代固定 buffer 累积
  - 优化状态机, 支持跨 MTU 包的数据包解析
  - 添加详细的连接/断开日志
  - 提高传输稳定性和内存使用效率
- refactor(safety): 修复自动归零重复触发问题
  - 添加静态标志位防止重复执行
  - 优化安全监控任务逻辑

### 文档更新 (Documentation)
- docs: 新增 WiFi 网络服务测试工具集
  - 7 个 Python 测试脚本 (HTTP, WebSocket, Telnet, File Transfer, mDNS, SSDP)
  - 一键测试所有服务的集成脚本
  - 详细的使用说明和示例输出
- docs: 新增网络服务完全指南
  - 7 大网络服务详解 (HTTP, WebSocket, Telnet, File Transfer, mDNS, SSDP, OTA)
  - 服务架构、端口配置、使用场景
  - 故障排查和最佳实践
- docs: 新增 SD 卡状态管理与 error61 问题排查文档
  - SD 卡状态管理机制详解
  - error61 问题根因分析
  - 解决方案和预防措施

### 其他变更 (Others)
- style: 修复全项目中文标点符号
  - 修复 231 个文件中的中文标点符号
  - 统一使用英文标点符号 (逗号, 句号, 冒号等)
  - 提高代码可读性和规范性

---

## 2026-02-03

### Bug 修复 (Fixes)
- fix(ssdp): 修复定时器竞态条件和内存分配异常
  - 添加 parsePacket() 和 buffer 分配的异常捕获
  - 修复定时器回调中的竞态条件，添加 ets_timer_done()
  - 防止 WiFi 不稳定时的崩溃问题

### 新功能 (Features)
- feat(skills): 新增 fix-punctuation 开发工具
  - 自动修复代码文件中的中文标点符号
  - 支持 C/C++, Python, JavaScript 等多种语言
  - 批量处理和通配符匹配

### 重构 (Refactoring)
- refactor(log): 统一日志系统格式
  - FileTransferServer: 简化日志宏定义，使用标准格式
  - wainlux_k11_HIM: 统一日志格式，添加函数名和行号
  - 将动态分配的 RX buffer 改为静态分配，提高稳定性

### 文档更新 (Documentation)
- docs: 重组崩溃分析文档结构
  - 新增 ESP32 崩溃分析方法论文档
  - 整理 4 个崩溃分析案例（ArduinoOTA, 文件传输, SSDP, SD 卡）
  - 移除旧的日志分析文档，统一到崩溃分析目录
- docs: 新增 WiFi 运行时断联自动重连机制实现文档

### 其他变更 (Others)
- style: 修复全项目中文标点符号
  - 修复 225 个文件中的中文标点符号
  - 统一使用英文标点符号（逗号、句号、冒号等）
  - 提高代码可读性和规范性

---

## 2026-01-31

### 新功能 (Features)
- feat(webui): 实现 Web 文件管理 TCP Socket 传输功能
  - 新增 FileTransferServer 模块，支持文件上传/下载/删除/列表
  - 实现基于 TCP Socket 的二进制传输协议
  - 支持 CRC32 校验和断点续传
  - 添加完整的错误处理和超时机制
- feat(skills): 新增 pio-runner 和 python-runner 技能
  - pio-runner: 在 Git Bash 环境运行 PlatformIO 命令
  - python-runner: 在 Git Bash 环境运行 Python 脚本
  - 自动处理环境配置和路径问题

### 优化改进 (Optimizations)
- opt(wifi): 优化 WiFi 配置和 WebSettings 模块
  - 改进 WiFi 连接状态管理
  - 优化 Web 设置界面响应速度

### 重构 (Refactoring)
- refactor(skills): 移除 verify-esp32 技能
  - 用 pio-runner 替代原有的 ESP-IDF 构建验证功能
  - 简化技能系统架构

### 文档更新 (Documentation)
- docs: 新增 TCP Socket 文件传输协议文档
  - 协议设计说明和参考代码
  - Python/Dart 客户端实现示例
  - 自动化测试脚本和配置文件
- docs: 新增 Web 文件管理 API 详解文档
  - FileTransferServer 架构设计
  - TCP Socket 协议规范
  - 安全性和性能优化建议
- docs: 新增 ESP32 崩溃分析案例文档
  - ArduinoOTA 内存不足问题分析
  - 堆栈回溯和根因定位
  - 解决方案和预防措施
- docs: 新增 C++ 静态常量成员详解文档
  - 声明与定义规则
  - 内联初始化和 constexpr 用法
- docs: 更新 Settings 配置系统设计详解

### 其他变更 (Others)
- chore: 更新 .gitignore 忽略规则
- chore: 添加 Claude Code Git Bash 环境配置脚本

---

## 2026-01-28

### 新功能 (Features)
- feat(wifi): 实现 WiFi STA 模式随机 MAC 地址生成
  - 基于 IEEE 802 标准生成本地管理 MAC 地址
  - 设置本地管理位 (bit 1) 和清除多播位 (bit 0)
  - 随机化后三个字节，保留厂商标识
  - 增强隐私保护，防止设备跟踪
- feat(led): 添加 WiFi 连接状态 LED 指示
  - 新增 `WIFI_STA_CONNECTING` LED 状态
  - WiFi 连接中显示等间隔闪烁 (500ms on/off)

### Bug 修复 (Fixes)
- fix(wifi): 修复 WiFi 主机名设置时机错误问题
  - 在 `SYSTEM_EVENT_STA_START` 事件中设置主机名
  - 确保 LwIP 网络接口已初始化后再设置
  - 修复路由器显示默认 `espressif` 名称的问题

### 优化改进 (Optimizations)
- opt(led): 重构 LED 控制逻辑，实现多段式闪烁模式
  - 替换简单的 `set_led_blink()` 为状态机驱动的 `set_led_pattern()`
  - 支持自定义时长和亮度的多段式闪烁序列
  - 确保每个时间段完整执行，避免时序错乱
- opt(wifi): 增加 WiFi STA 重试次数从 3 次到 6 次
  - 提高连接成功率，适应网络波动
- opt(machine): 统一循环任务周期为 20ms
  - 按键监控、LED 控制、安全监控任务统一使用 `LOOP_TASK_PERIOD_MS`
  - 提高代码可维护性和一致性

### 重构 (Refactoring)
- refactor(led): 简化 LED 状态判断和日志输出
  - 添加系统状态名称数组 `sys_state_names[]`
  - 替换冗长的三元表达式为数组索引
  - 优化 LED 状态切换逻辑，WiFi 连接状态优先级最高
- refactor(machine): 优化日志输出格式
  - 添加编译时间和 ESP32 SDK 版本信息
  - 统一日志格式，提高可读性

### 文档更新 (Documentation)
- docs: 添加 ESP32 WiFi MAC 地址分析与随机化实现文档
  - 官方文档确认和论坛讨论分析
  - 随机 MAC 地址实现原理和代码详解
  - 潜在问题、测试建议和后续优化方向
- docs: 添加 ESP32 WiFi 主机名设置分析与实现文档
  - mDNS vs DHCP 主机名概念区分
  - 问题根源分析和正确时机说明
  - WiFi 事件序列图和常见误区
  - DHCP Option 12 技术细节

---

## 2026-01-23

### 新功能 (Features)
- feat(machine): 添加 Wainlux K11 HIM 硬件配置文件
  - 基于 Genmitsu Kiosk C V07 激光雕刻机
  - 2 轴激光雕刻机 (X/Y 轴)，工作区域 100mm x 100mm
  - 激光主轴 13 位 PWM 控制，频率 5kHz
  - 支持安全门开关、倾斜传感器、用户按钮、状态 LED、冷却风扇
  - 自定义 SD 卡 SPI 引脚配置，避免与 LED 引脚冲突
  - 详细的 GPIO 引脚使用说明和注意事项

### 文档更新 (Documentation)
- docs: 添加项目开发指南 CLAUDE.md
  - 项目概述、架构设计、开发约束
  - 二次开发指南、关键文件索引
  - 代码规范和配置原则
- docs: 添加 Claude 开发辅助工具
  - commit/opt/release 命令模板
  - verify-esp32 技能（代码检查和构建验证）
  - Doxygen 文档生成指南
- docs: 添加完整的中文技术文档
  - 学习指南系列（快速入门、工程结构、架构设计等）
  - 功能解析系列（WiFi、GPIO、Settings、State 等）
  - 配置分析系列（机器配置、查看命令等）
  - 日志分析和逆向工程文档
  - 框架设计文档（状态机、实时标志位系统）

### 其他变更 (Others)
- chore: 更新 platformio.ini 配置
  - 添加 QMI8658 倾斜传感器库依赖
  - 添加编译警告抑制选项
- chore: 添加固件合并脚本 merge_firmware.bat
- chore: 更新 .gitignore 忽略规则

---
