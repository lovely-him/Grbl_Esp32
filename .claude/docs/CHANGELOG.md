# CHANGELOG

本文档记录项目的主要变更历史。

格式说明：
- 按日期倒序排列（最新的在最上面）
- 使用 Conventional Commits 规范分类
- 中英文混排时保持空格

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
