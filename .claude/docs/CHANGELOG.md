# CHANGELOG

本文档记录项目的主要变更历史。

格式说明：
- 按日期倒序排列（最新的在最上面）
- 使用 Conventional Commits 规范分类
- 中英文混排时保持空格

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
