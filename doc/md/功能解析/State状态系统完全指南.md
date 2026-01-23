# State 状态系统完全指南

## 概述

`sys.state` 是 Grbl_Esp32 系统的核心状态变量，定义在 `System.h` 中，用于标识系统当前的运行状态。所有的运动控制、命令处理、用户交互都需要根据当前状态做出相应的响应。

## 状态枚举定义

**源文件位置**: [Grbl_Esp32/src/System.h:29-39](../../Grbl_Esp32/src/System.h#L29-L39)

```cpp
enum class State : uint8_t {
    Idle = 0,    // Must be zero.
    Alarm,       // In alarm state. Locks out all g-code processes. Allows settings access.
    CheckMode,   // G-code check mode. Locks out planner and motion only.
    Homing,      // Performing homing cycle
    Cycle,       // Cycle is running or motions are being executed.
    Hold,        // Active feed hold
    Jog,         // Jogging mode.
    SafetyDoor,  // Safety door is ajar. Feed holds and de-energizes system.
    Sleep,       // Sleep state.
};
```

---

## 状态详细说明

### 1. State::Idle (0)

**标志名**: 空闲状态
**数值**: 0（必须为零，系统初始化默认值）

#### 特性
- 系统空闲，无运动执行
- 可以接收和执行新的 G-code 命令
- 可以进行设置修改（$x=val）
- 可以启动归零（$H）
- 可以进入点动模式（$J）

#### 典型场景
- 系统启动后（如果没有报警）
- 任务执行完成后
- 从其他状态恢复后

#### 进入方式
- 系统初始化完成
- G-code 程序执行完成（M2/M30）
- 从 Hold 状态恢复并完成剩余运动
- 归零完成
- 点动停止

#### 允许的操作
- ✅ 执行 G-code 命令
- ✅ 启动归零（$H）
- ✅ 进入点动模式（$J）
- ✅ 修改系统设置（$x=val）
- ✅ 查看状态（?）
- ✅ 主轴控制（M3/M4/M5）

---

### 2. State::Alarm (1)

**标志名**: 报警状态
**数值**: 1

#### 特性
- 系统锁定，禁止所有 G-code 运动命令
- 主轴强制关闭
- 步进电机可能断电（取决于配置）
- 仅允许访问系统设置和查询命令
- 必须通过归零（$H）或解除锁定（$X）才能退出

#### 典型场景
- 系统上电后默认进入（硬限位启用时）
- 限位开关触发
- 硬件故障检测
- 归零失败
- 软限位超出
- 探针检测失败

#### 进入方式
- 系统启动（如果启用了硬限位）
- 限位开关在非归零时触发
- 归零过程失败
- 软限位越界
- 探针命令失败
- 手动触发报警（通过某些错误条件）

#### 允许的操作
- ✅ 查看状态（?）
- ✅ 修改系统设置（$x=val）
- ✅ 启动归零（$H）解除报警
- ✅ 解除锁定（$X）强制解除报警
- ❌ 执行运动 G-code
- ❌ 点动控制
- ❌ 主轴控制

#### 解除方法
```gcode
$H        # 归零解除报警（推荐，安全）
$X        # 强制解除报警（谨慎使用）
```

---

### 3. State::CheckMode (2)

**标志名**: G-code 检查模式
**数值**: 2

#### 特性
- 解析 G-code 但不实际执行运动
- 仅检查语法和逻辑错误
- 不控制步进电机和主轴
- 快速验证 G-code 文件有效性

#### 典型场景
- 测试 G-code 文件
- 调试 G-code 程序
- 验证语法正确性

#### 进入方式
```gcode
$C        # 进入检查模式
```

#### 退出方式
- 发送复位命令（Ctrl-X）
- 完成 G-code 文件检查

#### 允许的操作
- ✅ 解析 G-code（仅检查，不执行）
- ✅ 修改系统设置
- ✅ 查看状态
- ❌ 实际运动控制
- ❌ 主轴控制
- ❌ 点动控制

---

### 4. State::Homing (3)

**标志名**: 归零中
**数值**: 3

#### 特性
- 正在执行归零循环
- 按配置的顺序移动各轴到限位开关
- 建立机器坐标系原点
- 归零期间拒绝其他命令

#### 典型场景
- 开机后首次归零
- 从报警状态恢复
- 手动触发归零

#### 进入方式
```gcode
$H        # 归零命令
```

#### 归零流程
1. 快速接近限位开关
2. 回退一小段距离
3. 慢速精确接近限位开关
4. 回退到归零偏移位置
5. 设置机器坐标系原点

#### 允许的操作
- ✅ 查看状态（?）
- ❌ 执行其他 G-code
- ❌ 点动控制
- ❌ 修改设置（部分设置）

#### 退出方式
- 归零成功 → 进入 `Idle`
- 归零失败 → 进入 `Alarm`
- 手动中止 → 进入 `Alarm`

---

### 5. State::Cycle (4)

**标志名**: 运行中
**数值**: 4

#### 特性
- G-code 程序正在执行
- 运动规划器和步进控制器活跃
- 主轴可能正在运行
- 可以接收进给保持（暂停）命令

#### 典型场景
- 正常加工过程
- 激光雕刻过程
- 执行 G-code 文件

#### 进入方式
- 从 `Idle` 状态开始执行 G-code
- 从 `Hold` 状态恢复（发送循环启动命令）

#### 允许的操作
- ✅ 查看状态（?）
- ✅ 进给保持（Feed Hold，!）
- ✅ 实时速率调整（Feed Override）
- ✅ 实时主轴速率调整（Spindle Override）
- ✅ 新 G-code 命令排队（进入缓冲区）
- ❌ 点动控制
- ❌ 修改大部分设置

#### 实时命令
```
!         # 进给保持（暂停）
~         # 循环启动（恢复，仅在 Hold 状态有效）
0x18      # Ctrl-X 软复位（紧急停止）
```

---

### 6. State::Hold (5)

**标志名**: 暂停保持
**数值**: 5

#### 特性
- 运动已停止，但保持当前位置
- 主轴状态保持（取决于配置）
- 可以恢复运动或完全停止
- 适合检查工件或临时暂停

#### 典型场景
- 用户按下暂停按钮
- 检查雕刻进度
- 临时停止以调整工件

#### 进入方式
```
!         # 发送进给保持命令（实时命令）
```

#### 退出方式
```
~         # 循环启动（恢复运动）→ 返回 Cycle
0x18      # 软复位（停止任务）→ 进入 Idle
```

#### 允许的操作
- ✅ 查看状态（?）
- ✅ 循环启动（~）恢复运动
- ✅ 软复位（Ctrl-X）停止任务
- ✅ 新 G-code 命令排队
- ❌ 点动控制
- ❌ 修改大部分设置

---

### 7. State::Jog (6)

**标志名**: 点动模式
**数值**: 6

#### 特性
- 手动控制轴运动
- 用于对刀、工件定位
- 可以随时取消
- 不影响工作坐标系

#### 典型场景
- 手动对刀
- 工件定位
- 测试轴运动

#### 进入方式
```gcode
$J=X10 F100       # 点动 X 轴 +10mm，速度 100mm/min
```

#### 点动命令格式
```gcode
$J=G21 G91 X10 F500       # 点动命令（增量模式）
$J=G90 X50 Y50 F1000      # 点动到绝对位置
```

#### 允许的操作
- ✅ 点动命令（$J）
- ✅ 点动取消（0x85）
- ✅ 查看状态（?）
- ❌ 常规 G-code 命令
- ❌ 修改设置

#### 退出方式
- 点动完成 → 返回 `Idle`
- 点动取消（Jog Cancel，0x85）→ 返回 `Idle`
- 软复位（Ctrl-X）→ 返回 `Idle`

---

### 8. State::SafetyDoor (7)

**标志名**: 安全门打开
**数值**: 7

#### 特性
- 安全门传感器触发
- 自动进给保持并停止运动
- 主轴和冷却液关闭
- 可选：步进电机断电
- 必须关闭门才能恢复

#### 典型场景
- 用户打开机器防护门
- 安全门传感器触发
- 紧急安全干预

#### 进入方式
- 安全门输入引脚触发（硬件信号）

#### 退出方式
1. 关闭安全门
2. 发送恢复命令（~）→ 返回原状态（通常是 `Cycle` 或 `Idle`）

#### 允许的操作
- ✅ 查看状态（?）
- ✅ 等待门关闭
- ❌ 执行 G-code
- ❌ 点动控制
- ❌ 主轴控制

#### 配置项
```cpp
#define ENABLE_SAFETY_DOOR_INPUT_PIN  # 启用安全门功能
#define SAFETY_DOOR_PIN GPIO_NUM_XX   # 安全门输入引脚
```

---

### 9. State::Sleep (8)

**标志名**: 休眠状态
**数值**: 8

#### 特性
- 系统进入低功耗模式
- 主轴和冷却液关闭
- 步进电机断电
- WiFi 可能断开（取决于配置）
- 等待唤醒命令

#### 典型场景
- 长时间无操作后自动休眠
- 手动触发休眠节能

#### 进入方式
```gcode
$SLP      # 休眠命令
```

#### 退出方式
- 发送任意字符唤醒
- 硬件中断唤醒（按钮、限位等）

#### 允许的操作
- ✅ 唤醒命令（任意字符）
- ✅ 查看状态（?，会唤醒系统）
- ❌ 执行 G-code（需先唤醒）
- ❌ 点动控制（需先唤醒）

---

## 状态转换关系

### 状态转换图

```
                        系统启动
                           ↓
                 (硬限位启用)  (硬限位禁用)
                      ↓              ↓
                  [Alarm] ←─────→ [Idle]
                      ↓              ↓
                    $H 归零      G-code 执行
                      ↓              ↓
                  [Homing]       [Cycle] ←──────┐
                      ↓              ↓          │
                   成功/失败      ! 暂停     ~ 恢复
                      ↓              ↓          │
                  [Idle] ←──────  [Hold] ───────┘
                      ↓              ↓
                   $J 点动       Ctrl-X 复位
                      ↓              ↓
                   [Jog]          [Idle]
                      ↓
                  完成/取消
                      ↓
                   [Idle]


特殊转换:

任意状态 ───安全门打开──→ [SafetyDoor] ───门关闭+恢复──→ 原状态

[Idle] ───$SLP──→ [Sleep] ───任意字符──→ [Idle]

[Idle] ───$C──→ [CheckMode] ───Ctrl-X──→ [Idle]
```

### 状态转换矩阵

| 当前状态 | 可转换到的状态 | 触发条件 |
|----------|----------------|----------|
| `Idle` | `Alarm` | 限位触发、软限位越界 |
| `Idle` | `Homing` | $H 归零命令 |
| `Idle` | `Cycle` | 执行 G-code |
| `Idle` | `Jog` | $J 点动命令 |
| `Idle` | `CheckMode` | $C 检查模式 |
| `Idle` | `Sleep` | $SLP 休眠命令 |
| `Alarm` | `Idle` | $X 解除锁定 |
| `Alarm` | `Homing` | $H 归零命令 |
| `Homing` | `Idle` | 归零成功 |
| `Homing` | `Alarm` | 归零失败、中止 |
| `Cycle` | `Hold` | ! 进给保持 |
| `Cycle` | `Idle` | 程序完成（M2/M30） |
| `Cycle` | `Alarm` | 限位触发、错误 |
| `Hold` | `Cycle` | ~ 循环启动 |
| `Hold` | `Idle` | Ctrl-X 软复位 |
| `Jog` | `Idle` | 点动完成、取消 |
| `CheckMode` | `Idle` | Ctrl-X 退出 |
| `Sleep` | `Idle` | 任意字符唤醒 |
| `SafetyDoor` | 原状态 | 门关闭 + ~ 恢复 |
| 任意状态 | `SafetyDoor` | 安全门打开 |
| 任意状态 | `Idle` | Ctrl-X 软复位 |

---

## 各状态功能对比表

### 命令执行权限

| 状态 | G-code 运动 | 点动控制 | 设置修改 | 归零命令 | 查询状态 | 主轴控制 |
|------|-------------|----------|----------|----------|----------|----------|
| `Idle` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| `Alarm` | ❌ | ❌ | ✅ | ✅ | ✅ | ❌ |
| `CheckMode` | ✅ (仅检查) | ❌ | ✅ | ❌ | ✅ | ❌ |
| `Homing` | ❌ | ❌ | ❌ | 运行中 | ✅ | ❌ |
| `Cycle` | ✅ (排队) | ❌ | ❌ | ❌ | ✅ | 运行中 |
| `Hold` | ✅ (排队) | ❌ | ❌ | ❌ | ✅ | 保持 |
| `Jog` | ❌ | ✅ | ❌ | ❌ | ✅ | ✅ |
| `SafetyDoor` | ❌ | ❌ | ❌ | ❌ | ✅ | ❌ |
| `Sleep` | ❌ | ❌ | ✅ | ❌ | ✅ (唤醒) | ❌ |

### 硬件控制状态

| 状态 | 步进电机 | 主轴 | 冷却液 | 用户输出 |
|------|----------|------|--------|----------|
| `Idle` | 保持使能 | 可控 | 可控 | 可控 |
| `Alarm` | 可能断电 | 强制关闭 | 强制关闭 | 可控 |
| `CheckMode` | 不控制 | 不控制 | 不控制 | 可控 |
| `Homing` | 运动中 | 关闭 | 关闭 | 关闭 |
| `Cycle` | 运动中 | 根据 G-code | 根据 G-code | 根据 G-code |
| `Hold` | 保持位置 | 保持或关闭 | 保持或关闭 | 保持 |
| `Jog` | 运动中 | 保持原状 | 保持原状 | 保持原状 |
| `SafetyDoor` | 断电/保持 | 强制关闭 | 强制关闭 | 关闭 |
| `Sleep` | 断电 | 关闭 | 关闭 | 关闭 |

---

## 实时命令对照表

这些命令可以在任意状态下发送（不会被缓冲）：

| 命令 | ASCII | 十六进制 | 功能 | 影响的状态 |
|------|-------|----------|------|------------|
| `?` | 0x3F | 0x3F | 查询状态 | 所有状态 |
| `!` | 0x21 | 0x21 | 进给保持（暂停） | `Cycle` → `Hold` |
| `~` | 0x7E | 0x7E | 循环启动（恢复） | `Hold` → `Cycle` |
| Ctrl-X | 0x18 | 0x18 | 软复位 | 所有状态 → `Idle` |
| Jog Cancel | 0x85 | 0x85 | 取消点动 | `Jog` → `Idle` |
| Safety Door | - | - | 安全门触发 | 任意 → `SafetyDoor` |

---

## 状态查询和显示

### 查询命令
```gcode
?         # 实时状态查询
```

### 状态报告格式
```
<Idle|MPos:0.000,0.000,0.000|FS:0,0>
<Alarm|MPos:0.000,0.000,0.000>
<Cycle|MPos:10.500,5.200,0.000|FS:500,8000>
<Hold:0|MPos:10.500,5.200,0.000|FS:0,8000>
<Jog|MPos:15.000,10.000,0.000|FS:1000,0>
```

状态报告格式说明：
- `<状态|数据字段>`
- `MPos` - 机器坐标
- `FS` - Feed rate (进给速率) 和 Spindle speed (主轴速度)

---

## 在按钮控制中的应用

基于 `wainlux_k11_HIM.cpp` 中的按钮控制实现：

### 短按处理建议

| 当前状态 | 短按行为 | 实现建议 |
|----------|----------|----------|
| `Idle` | ✅ 预览边界 | 已实现 |
| `Cycle` | ✅ 暂停任务 | 已实现：`sys_rt_exec_state.bit.feedHold = true` |
| `Hold` | ✅ 恢复任务 | 已实现：`sys_rt_exec_state.bit.cycleStart = true` |
| `Alarm` | ✅ 发送 $H 归零 | 已实现：`WebUI::inputBuffer.push("$H\r")` |
| `Homing` | 🔸 忽略 | 建议：记录日志，不做任何操作 |
| `Jog` | 🔸 停止点动 | 建议：发送点动取消命令 `0x85` |
| `CheckMode` | 🔸 退出检查模式 | 建议：发送 `Ctrl-X` |
| `SafetyDoor` | 🔸 忽略 | 建议：记录日志，等待门关闭 |
| `Sleep` | 🔸 唤醒系统 | 建议：发送任意字符唤醒 |

### 长按 3 秒处理建议

| 当前状态 | 长按行为 | 实现建议 |
|----------|----------|----------|
| `Idle` | ✅ 启动离线雕刻 | 已实现 |
| `Cycle` | ✅ 强制停止 | 已实现：`sys_rt_exec_state.bit.reset = true` |
| `Hold` | ✅ 强制停止 | 已实现：`sys_rt_exec_state.bit.reset = true` |
| `Alarm` | 🔸 解除报警 | 建议：`WebUI::inputBuffer.push("$X\r")` |
| `Homing` | 🔸 中止归零 | 建议：`sys_rt_exec_state.bit.reset = true` |
| `Jog` | 🔸 停止点动 | 建议：`sys_rt_exec_state.bit.reset = true` |
| `CheckMode` | 🔸 退出检查模式 | 建议：`sys_rt_exec_state.bit.reset = true` |
| `SafetyDoor` | 🔸 强制复位 | 建议：`sys_rt_exec_state.bit.reset = true`（谨慎） |
| `Sleep` | 🔸 唤醒 | 建议：发送任意字符唤醒 |

---

## 实际使用频率参考

基于典型激光雕刻机使用场景：

### 高频状态（每次使用都会经历）
- **`Idle`** - 最常见，等待命令状态
- **`Cycle`** - 工作时的主要状态
- **`Hold`** - 检查工件、调整参数

### 中频状态（定期会遇到）
- **`Alarm`** - 开机后、限位触发后
- **`Homing`** - 开机首次操作、失步后重新归零

### 低频状态（特殊场景）
- **`Jog`** - 仅在手动控制时使用
- **`CheckMode`** - 仅在调试 G-code 时使用
- **`SafetyDoor`** - 仅在配置了安全门硬件时
- **`Sleep`** - 仅在启用了休眠功能且长时间不操作时

### 推荐优先级
1. **必须实现**: `Idle`, `Cycle`, `Hold`, `Alarm`
2. **建议实现**: `Homing`
3. **可选实现**: `Jog`, `Sleep`
4. **根据硬件**: `SafetyDoor`
5. **调试功能**: `CheckMode`

---

## 常见问题和注意事项

### 1. 为什么 Idle 必须是 0？
```cpp
Idle = 0,    // Must be zero.
```
- 系统初始化时变量默认为 0
- 确保启动后处于正确状态
- 简化状态检查（`if (!sys.state)` 等价于 `if (sys.state == State::Idle)`）

### 2. 如何处理状态转换？
状态转换应该通过专门的函数进行，不要直接赋值：
```cpp
// ❌ 错误：直接修改状态
sys.state = State::Alarm;

// ✅ 正确：通过系统函数修改
mc_alarm(AlarmCode);  // 设置报警状态
mc_reset();           // 复位到 Idle 状态
```

### 3. 状态查询的性能考虑
`sys.state` 查询是极低开销的操作（单字节读取），可以在任何地方频繁调用：
```cpp
if (sys.state == State::Idle) {
    // 执行空闲状态的操作
}
```

### 4. 线程安全性
`sys.state` 的修改主要在主任务中进行，但读取可能在多个任务中发生。ESP32 上单字节读取是原子操作，因此不需要额外的锁机制。

### 5. 状态持久化
`sys.state` 是运行时变量，不会保存到 NVS（非易失性存储）。每次启动后状态都会重置。

---

## 代码示例

### 示例 1：状态检查和处理
```cpp
void handle_user_button() {
    switch (sys.state) {
        case State::Idle:
            // 空闲时可以启动新任务
            start_new_job();
            break;

        case State::Cycle:
            // 运行时暂停
            sys_rt_exec_state.bit.feedHold = true;
            break;

        case State::Hold:
            // 暂停时恢复
            sys_rt_exec_state.bit.cycleStart = true;
            break;

        case State::Alarm:
            // 报警时尝试归零
            WebUI::inputBuffer.push("$H\r");
            break;

        default:
            LOG_WARN("Button pressed in unsupported state: %d", (int)sys.state);
            break;
    }
}
```

### 示例 2：等待特定状态
```cpp
bool wait_for_idle(uint32_t timeout_ms) {
    uint32_t start = millis();
    while (sys.state != State::Idle) {
        if (millis() - start > timeout_ms) {
            return false;  // 超时
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return true;  // 成功
}
```

### 示例 3：状态转换日志
```cpp
State last_state = State::Idle;

void monitor_state_changes() {
    if (sys.state != last_state) {
        LOG_INFO("State changed: %s -> %s",
                 state_to_string(last_state),
                 state_to_string(sys.state));
        last_state = sys.state;
    }
}

const char* state_to_string(State state) {
    switch (state) {
        case State::Idle:       return "Idle";
        case State::Alarm:      return "Alarm";
        case State::CheckMode:  return "CheckMode";
        case State::Homing:     return "Homing";
        case State::Cycle:      return "Cycle";
        case State::Hold:       return "Hold";
        case State::Jog:        return "Jog";
        case State::SafetyDoor: return "SafetyDoor";
        case State::Sleep:      return "Sleep";
        default:                return "Unknown";
    }
}
```

---

## 相关文件和函数

### 核心文件
- [System.h](../../Grbl_Esp32/src/System.h) - 状态枚举定义
- [System.cpp](../../Grbl_Esp32/src/System.cpp) - 系统控制函数
- [Protocol.cpp](../../Grbl_Esp32/src/Protocol.cpp) - 协议处理和状态转换
- [Report.cpp](../../Grbl_Esp32/src/Report.cpp) - 状态报告

### 关键函数
- `mc_alarm()` - 设置报警状态
- `mc_reset()` - 复位系统到 Idle
- `protocol_execute_realtime()` - 处理实时命令和状态转换
- `report_realtime_status()` - 报告当前状态

---

## 参考资源

- [Grbl v1.1 状态文档](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Interface)
- [实时标志位系统设计详解](../框架设计/实时标志位系统设计详解.md)
- [System.h 源码](../../Grbl_Esp32/src/System.h)

---

**最后更新**: 2026-01-21
**适用版本**: Grbl_Esp32 (所有版本)