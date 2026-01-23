---
name: verify-esp32
description: Perform code style check and build verification for ESP32 projects. Use when code is modified, feature development completed, or user requests verify/build/compile/check/review for ESP32 projects.
allowed-tools: Read, Bash, Grep, Edit
---

# Verify Skill - ESP32 专用

自动执行 **ESP32 项目**的**代码规范检查 + 编译验证**的完整流程，确保代码质量和可编译性。

**注意**：此技能专用于 ESP32 项目（基于 ESP-IDF 5.5.0），不适用于 AIC8800M40B 项目。

## 验证流程

### 阶段一：代码规范检查

**检查范围**：
- 仅检查本次修改的 `.c` 和 `.h` 文件（通过 `git diff --name-only` 获取）
- 检查以下目录：
  - `main/` 目录下的文件
  - `components/` 目录下的文件
  - `library_components/` 目录下的文件
- 排除外部依赖：
  - `managed_components/` 目录（外部依赖组件）
  - 其他第三方库目录

**检查项**（详见 [../../docs/doxygen-guide.md](../../docs/doxygen-guide.md)）：
1. **文件头部注释**：
   - `.h` 文件必需：`@file`, `@brief`, `@author`, `@date`, `@version`, `@copyright`
   - `.c` 文件必需：`@file`, `@brief`
2. **函数注释**（仅检查 `.h` 文件）：
   - `@brief`, `@param[in/out]`, `@return`
3. **关键字规范**：
   - `.h` 文件中的全局函数/变量必须用 `extern`
   - `.c` 文件中的内部函数/变量必须用 `static`
4. **格式规范**：
   - 4 个空格缩进，禁止 Tab
   - 操作符两侧保留空格
   - 左花括号不另起一行，右花括号单独一行
   - Switch 语句每个 `case` 都有 `{}` 作用域和 `break`
5. **注释与日志**：
   - 必须使用中文进行说明，确保表述清晰易懂
   - Doxygen 文档注释使用中文描述功能和参数
6. **专业术语**：
   - 保留通用的专业英语缩写或单词，不强制翻译
   - 例如：`recv`, `send`, `uart`, `buffer`, `frame`, `timeout` 等
7. **中英混排**：
   - 英语单词/缩写与中文文字之间必须保留一个空格
   - 例如："串口 buffer 已满"
8. **字符集**：
   - 所有代码符号仅限使用 ASCII 字符集内的符号
   - 禁止使用全角符号或特殊字符

**修复策略**：
- ✅ **直接修复**：缩进、空格、全角符号（无歧义，直接应用）
- 🔄 **生成方案 → 用户确认**：缺失注释、缺失关键字（生成修复建议，展示后等待确认再应用）

### 阶段二：编译与烧录验证

**构建命令**：
```bash
# 1. 加载 ESP-IDF Bash 包装环境
source .claude/skills/verify-esp32/activate-esp-idf.sh

# 2. 执行编译+烧录 (通过 PowerShell 跨环境调用)
esp_idf_build_flash

# 或仅编译，不烧录
esp_idf_build

# 或仅烧录，不编译（假设已编译）
esp_idf_flash
```

**成功标志**：
- ✅ Build: 输出包含 "Build Completed Successfully"
- ✅ Flash: 输出包含 "Hash of data verified" + "Done"

**常见错误**：见 [build-errors.md](build-errors.md)

### 错误循环处理

如果编译失败：
1. 分析编译错误信息
2. 修复代码问题
3. **返回阶段一**：重新执行代码规范检查
4. 通过后重新编译
5. 重复直到成功

## 何时触发

✅ **自动触发**：
- 修改了 `main/`, `components/`, `library_components/` 下的 `.c/.h` 文件
- 功能开发完成后
- bug 修复后
- 用户明确请求验证/编译/检查

❌ **不触发**：
- 仅修改文档文件（`.md`）
- 仅修改配置文件（`sdkconfig`, `Kconfig`）
- 仅查看/阅读代码，未做修改
- 用户明确表示"不需要验证"

## 输出格式

### 完整成功
```
✅ 代码验证通过！

【代码规范检查】
- 检查文件: wifi_init_config.c, wifi_init_config.h
- 检查项: 8/8 通过
- 自动修复: 0

【编译验证】
- 编译时间: 35 秒
- 警告: 2（已检查，无影响）
- 错误: 0

✨ 代码质量良好，可以提交！
```

### 规范检查失败 → 修复流程
```
⚠️ 代码规范检查发现问题

【不符合项】
1. wifi_init_config.c:42 - 操作符缺少空格
2. wifi_init_config.h:15 - 缺少函数 Doxygen 注释

【直接修复】
✅ 已修复：操作符空格问题

【生成修复方案】
📝 wifi_init_config.h:15 - 生成函数注释建议：

/**
 * @brief 初始化 WiFi 配置
 *
 * @param[in] config WiFi 配置参数指针
 * @return ESP_OK 表示成功，ESP_FAIL 表示失败
 */

是否应用此修复？[等待用户确认]
```

### 编译失败 → 修复 → 重新验证
```
❌ 编译失败

【错误信息】
wifi_init_config.c:78: error: 'CONFIG_WIFI_SSID' undeclared
wifi_init_config.c:79: error: implicit declaration of function 'esp_wifi_init'

【分析】
1. CONFIG_WIFI_SSID 未定义 - 可能需要在 Kconfig 中配置
2. 缺少头文件 - 需要 #include "esp_wifi.h"

【修复并返回规范检查...】
```

## 参考文档

- **代码规范详细标准**：[../../docs/doxygen-guide.md](../../docs/doxygen-guide.md)
- **常见构建错误**：[build-errors.md](build-errors.md)（按需记录真实错误）

## 技术说明

**ESP-IDF 特定注意事项** 和 **Claude Code 环境适配说明** 详见: [esp-idf-technical-notes.md](esp-idf-technical-notes.md)

---

**创建时间**: 2026-01-09
**维护者**: 建豪 (JianHao)
**版本**: 2.0 (添加 PowerShell 跨环境调用支持)
