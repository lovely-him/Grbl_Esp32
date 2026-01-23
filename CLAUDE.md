# Grbl_Esp32 项目开发指南

## 项目概述

Grbl_Esp32 是一个运行在 ESP32 微控制器上的开源 CNC 控制器固件，源自经典的 Grbl 项目。

### 当前状态
- **维护模式**: 现有功能保持维护，新功能开发已转向 [FluidNC](https://github.com/bdring/FluidNC)
- **硬件兼容**: 所有 Grbl_ESP32 硬件与 FluidNC 兼容

### 核心特性
- 支持最多 **6 轴协调控制** (XYZABC)，每轴可配置 1-2 个电机
- 步进速率高达 **120,000 步/秒**
- 支持多种电机驱动：标准步进、Trinamic (SPI)、单极步进、RC 伺服
- 支持多种主轴类型：PWM、RS485 Modbus、DAC (0-10V)、继电器、激光
- 丰富的连接方式：USB/Serial、Bluetooth、WiFi (AP/STA 模式)
- 内置 Web 服务器和完整的 CNC 控制界面
- SD 卡支持、OTA 固件升级、推送通知

### 技术栈
- **平台**: ESP32 (espressif32@3.0.0)
- **框架**: Arduino
- **构建工具**: PlatformIO
- **主要依赖**: TMCStepper (>=0.7.0), SSD1306 OLED driver
- **代码规范**: 使用 `.clang-format` 强制执行

---

## 架构设计

### 分层架构

```
应用层 (Application Layer)
├── Web UI (WebSettings.cpp)
├── SD Card Support
└── User Outputs & Notifications

通信层 (Communication Layer)
├── Serial/UART (Serial.cpp)
├── Web Server (WebServer.cpp)
├── Telnet/Socket Bridges
└── WiFi/Bluetooth Support

系统控制层 (System Control Layer)
├── Protocol Handler (Protocol.cpp)
├── Settings Management (Settings.cpp)
├── System Management (System.cpp)
└── Error Handling (Error.cpp)

运动控制层 (Motion Control Layer)
├── G-code Parser (GCode.cpp)
├── Motion Planner (Planner.cpp)
├── Stepper Control (Stepper.cpp)
└── Limit/Homing Control (Limits.cpp)

硬件抽象层 (Hardware Abstraction Layer)
├── Machine Definitions (Machines/*.h)
├── Motor Drivers (Motors/*.h)
└── Spindle Drivers (Spindles/*.h)
```

### 关键设计模式

#### 1. 多态扩展 (Polymorphic Extension)
- **Motor 基类**: 支持 StandardStepper, TrinamicDriver, UnipolarMotor, RcServo 等
- **Spindle 基类**: 支持 PWM, Laser, VFD, DAC, Relay 等多种主轴类型
- 通过继承和虚函数实现硬件驱动的灵活扩展

#### 2. 配置驱动 (Configuration-Driven)
- 通过 `Machine.h` 选择机器配置文件
- 机器配置文件定义 GPIO 引脚映射、电机类型、主轴类型等
- 编译时配置 vs 运行时设置的清晰分离

#### 3. 弱函数覆盖 (Weak Function Override)
- `machine_init()` - 机器特定初始化
- `display_init()` - 显示器初始化
- `user_m30()` - 自定义 M30 (程序结束) 行为
- `user_tool_change()` - 自定义换刀序列
- 允许在不修改核心代码的情况下定制行为

---

## 开发约束和设计原则

### ⚠️ 不要修改的核心部分

以下模块包含经过严格测试的核心算法，**除非你完全理解其工作原理，否则不要修改**：

1. **G-code 解析器** ([GCode.cpp](Grbl_Esp32/src/GCode.cpp))
   - RS274/NGC 标准实现
   - 复杂的状态机和错误处理

2. **运动规划器** ([Planner.cpp](Grbl_Esp32/src/Planner.cpp))
   - 加速度规划和速度优化
   - 前瞻算法 (look-ahead)

3. **步进控制器** ([Stepper.cpp](Grbl_Esp32/src/Stepper.cpp))
   - 实时步进脉冲生成
   - Bresenham 算法实现

4. **实时中断处理**
   - RMT/I2S 步进输出
   - 定时器中断处理

### ✅ 推荐的扩展点

1. **机器配置层** (`Machines/*.h`)
   - 在 `Grbl_Esp32/src/Machines/` 创建新的机器配置文件
   - 定义 GPIO 引脚映射、电机/主轴类型、辅助功能
   - 在 [Machine.h](Grbl_Esp32/src/Machine.h) 中激活配置

2. **弱函数自定义**
   - `machine_init()` - 机器特定初始化
   - `user_m30()` - 自定义 M30 (程序结束) 行为
   - `user_tool_change()` - 自定义换刀序列
   - 在机器配置文件或自定义 `.cpp` 文件中实现

3. **硬件驱动扩展** (高级,谨慎使用)
   - 现有驱动已覆盖大多数需求 (StandardStepper, TrinamicDriver, PWM, Laser 等)
   - 如需添加新驱动,继承 `Motors::Motor` 或 `Spindle` 基类
   - 参考 [Motor.h](Grbl_Esp32/src/Motors/Motor.h) 和 [Spindle.h](Grbl_Esp32/src/Spindles/Spindle.h)

### 代码规范

遵循 [CodingStyle.md](CodingStyle.md) 中的规范：

- **类名**: `CamelCase` (例如: `StandardStepper`, `PWMSpindle`)
- **成员函数**: `snake_case` (例如: `set_disable()`, `get_state()`)
- **成员变量**: `_snake_case` 带前导下划线 (例如: `_axis_index`, `_current_rpm`)
- **头文件保护**: 使用 `#pragma once`
- **机器定义文件**: 必须使用 `// clang-format off` 禁用自动格式化
- **Include 顺序**: 先包含对应的 `.h` 文件，再包含其他文件
- **Include 语法**: 系统/库文件用 `<...>`，本地文件用 `"..."`

### 配置原则

#### 编译时配置 vs 运行时设置
- **编译时配置** ([Config.h](Grbl_Esp32/src/Config.h)): 硬件相关、性能相关
  - GPIO 引脚定义
  - 电机/主轴类型选择
  - 启用/禁用功能模块 (WiFi, Bluetooth)

- **运行时设置** ([Settings.h](Grbl_Esp32/src/Settings.h)): 机器参数、用户偏好
  - 步进参数 (steps/mm, max rate, acceleration)
  - 限位开关配置
  - WiFi SSID/密码

#### GPIO 引脚选择注意事项
避开 ESP32 启动时的特殊引脚：
- **GPIO 0**: 启动模式选择 (下拉进入下载模式)
- **GPIO 2**: 必须悬空或下拉才能从 Flash 启动
- **GPIO 5**: VSPI CS0，启动时必须为高电平
- **GPIO 12**: MTDI，启动时电平影响 Flash 电压
- **GPIO 15**: MTDO，启动时必须为高电平

推荐使用的 GPIO: 4, 13, 14, 16-19, 21-23, 25-27, 32-33

### 常见陷阱

#### 1. 实时性要求
- ❌ **不要**在步进中断中执行耗时操作 (>10μs)
- ❌ **不要**在中断处理函数中使用 `Serial.print()` 或 `delay()`
- ✅ **应该**使用 RTOS 任务和队列进行异步处理

#### 2. 内存管理
- ESP32 虽然比 AVR 强大，但仍需注意内存使用
- 避免在栈上分配大数组
- 使用 `PROGMEM` 存储常量字符串

#### 3. WiFi 和 Bluetooth
- ❌ **不能**同时全功率使用 WiFi 和 Bluetooth (共享射频)
- ✅ **应该**根据需要选择其中一个，或降低功率

#### 4. 配置分离
- ❌ **不要**在核心代码中硬编码 GPIO 引脚
- ✅ **应该**在机器配置文件中定义所有硬件相关配置

### 自定义代码优化原则

**本项目处于产品原型迭代阶段，现有代码是阶段性的临时实现。**

对于自定义代码（非官方开源库部分），应遵循以下优化原则：

#### ✅ 应该做的事情

1. **大胆重构**
   - 发现更好的实现方式时，直接重构现有代码
   - 不要因为"已经能用"就保留低效实现
   - 优先考虑代码简洁性和可维护性

2. **删除冗余代码**
   - 不再使用的函数、变量、注释应立即删除
   - 不要保留"可能以后用到"的死代码
   - 不要用注释标记"已弃用"的代码，直接删除

3. **优先简洁性**
   - 能用一个函数实现的，不要拆成多个
   - 能合并的任务，不要创建多个独立任务
   - 避免过度抽象和过度设计

#### ❌ 不应该做的事情

1. **不要保持向后兼容**
   - 原型阶段不需要考虑向后兼容性
   - 发现设计问题时，直接修改而非添加兼容层
   - 不要保留旧接口"以防万一"

2. **不要过度抽象**
   - 不要为"可能的未来需求"设计扩展点
   - 不要创建只有一个实现的抽象类
   - 实际需要时再抽象，不要提前设计

3. **不要保留废弃代码**
   - 不要用注释标记"不再使用"的函数
   - 不要保留"可能以后用到"的变量
   - Git 历史记录已经保存了所有代码，删除不会丢失

4. **不要两种风格并存**
   - 引入新的编码风格时，应全面替换旧风格
   - 不要让新旧两种实现方式共存

#### 实践示例

**场景：发现风扇控制可以合并到 LED 任务中**

❌ **错误做法**：
```cpp
// 保留旧函数但标记为废弃
void fan_control_task(void* parameter) {
    // 注意：此函数不再使用，已合并到 led_control_task
    // 保留以防需要恢复独立控制
    while(true) { ... }
}
```

✅ **正确做法**：
```cpp
// 直接删除 fan_control_task() 函数
// 删除前向声明
// 删除任务创建代码
// 更新任务创建日志："Button, LED+Fan, Safety"
```

#### 核心理念

> **"原型阶段的代码应该像粘土一样可塑，而不是像混凝土一样固化。"**

- 快速迭代比稳定性更重要
- 简洁性比完整性更重要
- 实用性比可扩展性更重要

当项目进入生产阶段时，再考虑向后兼容、稳定性和扩展性。在原型阶段，保持代码的灵活性和可重构性是第一优先级。

---

## 二次开发指南

### 添加新机器配置

1. **创建机器配置文件**
   - 在 `Grbl_Esp32/src/Machines/` 创建 `My_Machine.h`
   - 参考 [3axis_v4.h](Grbl_Esp32/src/Machines/3axis_v4.h)

2. **定义基本信息**
   ```cpp
   #pragma once
   // clang-format off  // 机器定义文件必须禁用格式化

   #define MACHINE_NAME "My_CNC_Machine"
   #define N_AXIS 3  // 轴数量 (3-6)
   ```

3. **定义轴引脚**
   ```cpp
   #define X_STEP_PIN      GPIO_NUM_26
   #define X_DIRECTION_PIN GPIO_NUM_16
   #define X_LIMIT_PIN     GPIO_NUM_36
   // Y, Z 轴类似...
   ```

4. **定义主轴配置**
   ```cpp
   #define SPINDLE_TYPE        SpindleType::PWM
   #define SPINDLE_OUTPUT_PIN  GPIO_NUM_2
   #define SPINDLE_ENABLE_PIN  GPIO_NUM_22
   ```

5. **定义辅助功能**
   ```cpp
   #define COOLANT_MIST_PIN  GPIO_NUM_21
   #define COOLANT_FLOOD_PIN GPIO_NUM_25
   #define PROBE_PIN         GPIO_NUM_32
   ```

6. **激活配置**
   - 编辑 [Machine.h](Grbl_Esp32/src/Machine.h)，设置 `MACHINE_FILENAME`:
   ```cpp
   #define MACHINE_FILENAME "Machines/My_Machine.h"
   ```

### 自定义初始化和回调

在机器配置文件中定义弱函数：

```cpp
// 机器初始化 (在 grbl_init() 中调用)
void machine_init() {
    // 初始化外部硬件
    // 配置 I/O 扩展器
    // 设置自定义 GPIO
}

// 所有带有 __attribute__((weak)) 修饰符的函数，可以被其他文件覆盖
void __attribute__((weak)) machine_init() {}
```

---

## 关键文件索引

### 核心系统
- [Grbl.h](Grbl_Esp32/src/Grbl.h) / [Grbl.cpp](Grbl_Esp32/src/Grbl.cpp) - 主系统头文件和初始化
- [Config.h](Grbl_Esp32/src/Config.h) - 编译时配置
- [Machine.h](Grbl_Esp32/src/Machine.h) - 机器配置选择器
- [Defaults.h](Grbl_Esp32/src/Defaults.h) - 默认设置

### 运动控制
- [GCode.cpp](Grbl_Esp32/src/GCode.cpp) - G-code 解析器 (92KB)
- [Planner.cpp](Grbl_Esp32/src/Planner.cpp) - 运动规划器 (25KB)
- [Stepper.cpp](Grbl_Esp32/src/Stepper.cpp) - 步进控制器 (53KB)
- [MotionControl.cpp](Grbl_Esp32/src/MotionControl.cpp) - 运动执行控制
- [Limits.cpp](Grbl_Esp32/src/Limits.cpp) - 限位开关和归零 (19KB)

### 硬件驱动
- [Motor.h](Grbl_Esp32/src/Motors/Motor.h) - 电机基类
- [StandardStepper.h](Grbl_Esp32/src/Motors/StandardStepper.h) - 标准步进电机
- [TrinamicDriver.h](Grbl_Esp32/src/Motors/TrinamicDriver.h) - Trinamic 驱动
- [Spindle.h](Grbl_Esp32/src/Spindles/Spindle.h) - 主轴基类
- [PWMSpindle.h](Grbl_Esp32/src/Spindles/PWMSpindle.h) - PWM 主轴
- [Laser.h](Grbl_Esp32/src/Spindles/Laser.h) - 激光主轴

### 通信
- [Protocol.cpp](Grbl_Esp32/src/Protocol.cpp) - 协议处理 (41KB)
- [Serial.cpp](Grbl_Esp32/src/Serial.cpp) - 串口通信 (14KB)
- [WebServer.cpp](Grbl_Esp32/src/WebUI/WebServer.cpp) - Web 服务器 (76KB)
- [WebSettings.cpp](Grbl_Esp32/src/WebUI/WebSettings.cpp) - Web 设置界面 (47KB)

### 设置和系统
- [Settings.cpp](Grbl_Esp32/src/Settings.cpp) - 设置管理 (24KB)
- [System.cpp](Grbl_Esp32/src/System.cpp) - 系统功能 (14KB)
- [Error.cpp](Grbl_Esp32/src/Error.cpp) - 错误处理
- [Report.cpp](Grbl_Esp32/src/Report.cpp) - 状态报告

### 机器配置示例
- [3axis_v4.h](Grbl_Esp32/src/Machines/3axis_v4.h) - 3 轴激光配置
- [6_pack_stepstick.h](Grbl_Esp32/src/Machines/6_pack_stepstick.h) - 6 轴配置
- [test_drive.h](Grbl_Esp32/src/Machines/test_drive.h) - 测试配置（无硬件）

### 文档
- [README.md](README.md) - 项目说明
- [CodingStyle.md](CodingStyle.md) - 代码规范
- [doc/Commands.txt](doc/Commands.txt) - G-code 命令参考

---

## 参考资源

- **官方仓库**: https://github.com/bdring/Grbl_Esp32
- **下一代项目**: [FluidNC](https://github.com/bdring/FluidNC)
- **原始 Grbl**: [gnea/grbl](https://github.com/gnea/grbl)
- **Wiki**: https://github.com/bdring/Grbl_Esp32/wiki
- **Discord**: 开发者社区（请求邀请）

---

**最后更新**: 2026-01-20
