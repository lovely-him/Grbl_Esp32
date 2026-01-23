# Grbl_ESP32 基本控制开关功能分析

## 概述

Grbl_ESP32允许最多4个基本控制开关，这些开关包括：安全门/周期启动/复位/进给保持，以及额外的4个宏按钮。这些开关通过硬件引脚连接到外部按钮或传感器，为用户提供实时控制功能。

## 控制开关类型及功能

### 1. 安全门 (Safety Door)

#### 配置定义
```cpp
#define CONTROL_SAFETY_DOOR_PIN GPIO_NUM_X
```

#### 功能特点
- **触发条件**：检测到安全门打开时触发
- **执行操作**：
  - 立即执行进给保持（Feed Hold）
  - 停止主轴和冷却液输出
  - 进入SafetyDoor状态
  - 系统完全停止，切断动力输出
- **恢复操作**：安全门关闭后，需要手动执行`~`（循环启动）命令恢复

#### 状态显示
- 状态报告中显示为`Pn:D`
- 安全门打开时显示`<Door:1|...>`
- 安全门关闭时显示`<Door:0|...>`

#### 实时命令
- 对应实时命令：`0x84` (十六进制)
- 在状态报告中显示为"D"

### 2. 复位 (Reset)

#### 配置定义
```cpp
#define CONTROL_RESET_PIN GPIO_NUM_X
```

#### 功能特点
- **触发条件**：按下复位按钮时触发
- **执行操作**：
  - 调用`mc_reset()`运动控制复位程序
  - 停止所有运动
  - 清空计划缓冲区
  - 将系统状态重置为初始状态
  - 如果在Alarm状态，会尝试解锁并回到Idle状态

#### 状态显示
- 状态报告中显示为`Pn:R`

#### 实时命令
- 对应实时命令：`0x18` (Ctrl-X)
- 在状态报告中显示为"R"

### 3. 进给保持 (Feed Hold)

#### 配置定义
```cpp
#define CONTROL_FEED_HOLD_PIN GPIO_NUM_X
```

#### 功能特点
- **触发条件**：按下进给保持按钮时触发
- **执行操作**：
  - 立即停止进给运动
  - 保持当前位置
  - 将系统状态切换到Hold状态
  - 可通过`~`（循环启动）命令恢复

#### 状态显示
- 状态报告中显示为`Pn:H`

#### 实时命令
- 对应实时命令：`!` (感叹号)
- 在状态报告中显示为"H"

### 4. 周期启动 (Cycle Start)

#### 配置定义
```cpp
#define CONTROL_CYCLE_START_PIN GPIO_NUM_X
```

#### 功能特点
- **触发条件**：按下周期启动按钮时触发
- **执行操作**：
  - 启动步进中断以开始执行队列中的运动块
  - 仅在Idle状态或保持完成且准备恢复时才有效
  - 从Hold状态恢复时使用
  - 从SafetyDoor状态恢复时使用

#### 状态显示
- 状态报告中显示为`Pn:S`

#### 实时命令
- 对应实时命令：`~` (波浪号)
- 在状态报告中显示为"S"

### 5. 宏按钮 (Macro Buttons)

#### 配置定义
```cpp
#define MACRO_BUTTON_0_PIN GPIO_NUM_X
#define MACRO_BUTTON_1_PIN GPIO_NUM_X
#define MACRO_BUTTON_2_PIN GPIO_NUM_X
#define MACRO_BUTTON_3_PIN GPIO_NUM_X
```

#### 功能特点
- **触发条件**：按下宏按钮时触发
- **执行操作**：
  - 执行用户自定义的宏函数
  - 通过`user_defined_macro(index)`函数实现
  - 用户可以根据需要实现特定功能

#### 状态显示
- 状态报告中显示为`Pn:0`, `Pn:1`, `Pn:2`, `Pn:3`

## 硬件实现细节

### 引脚配置
控制开关通常配置为内部上拉输入：
```cpp
pinMode(CONTROL_SAFETY_DOOR_PIN, INPUT_PULLUP);
attachInterrupt(digitalPinToInterrupt(CONTROL_SAFETY_DOOR_PIN), isr_control_inputs, CHANGE);
```

### 信号反转
通过`INVERT_CONTROL_PIN_MASK`宏可以反转引脚逻辑：
```cpp
// The mask order is ...
// Macro3 | Macro2 | Macro 1| Macr0 |Cycle Start | Feed Hold | Reset | Safety Door
#define INVERT_CONTROL_PIN_MASK B00001111
```

### 消抖处理
可以选择启用硬件消抖：
```cpp
#define ENABLE_CONTROL_SW_DEBOUNCE
#define CONTROL_SW_DEBOUNCE_PERIOD 32  // 32毫秒
```

## 状态报告格式

当控制开关激活时，状态报告会显示"Pn:"字段，例如：
- `Pn:D` - 安全门开关激活
- `Pn:R` - 复位开关激活
- `Pn:H` - 进给保持开关激活
- `Pn:S` - 周期启动开关激活
- `Pn:0-3` - 宏按钮激活

## 优先级和冲突处理

控制开关具有不同的优先级：
1. **复位** - 最高优先级，可以中断任何操作
2. **安全门** - 次高优先级，会切断动力输出
3. **进给保持** - 中等优先级，暂停运动
4. **周期启动** - 恢复操作，只有在适当状态下有效

当多个开关同时激活时，系统会按优先级顺序处理。

## 应用场景

- **安全门**：用于带有防护罩的机器，当打开防护罩时自动停止
- **复位**：紧急情况下快速停止所有操作
- **进给保持**：正常暂停操作，检查工件或刀具
- **周期启动**：手动启动已暂停的加工程序
- **宏按钮**：用户自定义功能，如快速移动到特定位置

## 总结

Grbl_ESP32的控制开关系统提供了丰富的硬件接口，允许用户通过物理按钮对机器进行实时控制。每个开关都有明确的功能和状态，确保操作安全可靠。这些开关与软件实时命令相对应，提供了双重控制手段，增强了系统的易用性和安全性。