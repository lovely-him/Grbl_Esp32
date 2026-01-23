# Grbl_Esp32 Sleep 模式完全指南

## 概述

Sleep（睡眠）模式是 Grbl_Esp32 的一种特殊系统状态，用于将机器置于低功耗等待状态。这是一种**保护性状态**，设计上要求必须通过完整的系统重新初始化才能恢复正常运行。

---

## 系统状态定义

Sleep 模式在 [System.h:38](../../Grbl_Esp32/src/System.h#L38) 中定义：

```cpp
enum class State : uint8_t {
    Idle = 0,    // 空闲状态 - 可以接收命令
    Alarm,       // 报警状态 - 锁定 G-code 处理
    CheckMode,   // G-code 检查模式
    Homing,      // 归零中
    Cycle,       // 运行中
    Hold,        // 暂停（可恢复）
    Jog,         // 点动模式
    SafetyDoor,  // 安全门打开（可恢复）
    Sleep,       // 睡眠状态（只能复位退出）
};
```

---

## Sleep 模式的行为特征

### 进入 Sleep 模式时的操作

当系统进入 Sleep 模式时（[Protocol.cpp:636-645](../../Grbl_Esp32/src/Protocol.cpp#L636-L645)），会执行以下操作：

```cpp
if (sys.state == State::Sleep) {
    report_feedback_message(Message::SleepMode);

    // 1. 关闭主轴
    spindle->set_state(SpindleState::Disable, 0);

    // 2. 关闭冷却液
    coolant_off();

    // 3. 禁用步进电机（释放保持力矩）
    st_go_idle();

    // 4. 进入等待循环，只响应复位信号
    while (!(sys.abort)) {
        protocol_exec_rt_system();  // 只处理实时系统命令
    }

    return;  // 收到复位信号后返回重新初始化
}
```

### 关键特性

| 特性 | 说明 |
|------|------|
| **主轴状态** | 强制关闭 |
| **冷却液状态** | 强制关闭 |
| **步进电机** | 禁用（无保持力矩） |
| **命令响应** | 只响应复位命令 |
| **G-code 处理** | 完全停止 |
| **实时命令** | 只处理系统级实时命令 |

---

## 如何触发 Sleep 模式

### 触发机制

在 [Protocol.cpp:365-372](../../Grbl_Esp32/src/Protocol.cpp#L365-L372) 中，当实时执行标志被设置时触发：

```cpp
if (rt_exec_state.bit.sleep) {
    if (sys.state == State::Alarm) {
        // 如果当前在 Alarm 状态，标记挂起完成
        sys.suspend.bit.retractComplete = true;
        sys.suspend.bit.holdComplete    = true;
    }
    sys.state = State::Sleep;
    sys_rt_exec_state.bit.sleep = false;
}
```

### 触发方式

1. **实时命令**：发送 Sleep 实时命令（具体字符需查看实时命令映射表）
2. **程序控制**：通过设置 `sys_rt_exec_state.bit.sleep = true` 标志
3. **特殊条件**：某些错误或保护条件可能自动触发

---

## 如何退出 Sleep 模式

### ⚠️ 唯一退出方式：系统复位

**Sleep 模式是单向状态**，无法通过 Cycle Start 或其他命令直接恢复到 Idle 状态。

### 复位方法

#### 1. 硬件复位
- 按下 ESP32 开发板上的 **RESET 按钮**
- 断电重启

#### 2. 软件复位
- 发送 **Ctrl-X** (ASCII 0x18) 实时命令
- 通过串口工具发送复位字符

#### 3. 串口命令
```bash
# 通过串口发送复位命令
echo -e "\x18" > /dev/ttyUSB0  # Linux
```

### 复位后的行为

系统复位后会：
1. 重新初始化所有硬件
2. 重新加载设置
3. 进入 **Idle** 或 **Alarm** 状态（取决于限位开关状态）
4. 清除所有运动缓冲区
5. 重置工作坐标系（如果未保存）

---

## 与其他状态的对比

### 可恢复状态 vs 不可恢复状态

| 状态 | 恢复方式 | 是否保留运动缓冲区 | 是否需要重新初始化 |
|------|----------|-------------------|-------------------|
| **Hold** | Cycle Start (~) | ✅ 是 | ❌ 否 |
| **SafetyDoor** | 关闭门 + Cycle Start | ✅ 是 | ❌ 否 |
| **Sleep** | **只能复位** | ❌ 否 | ✅ 是 |
| **Alarm** | $X 解锁命令 | ❌ 否 | ❌ 否 |

### 代码对比

#### Hold 状态恢复（可直接恢复）
```cpp
// Hold 状态可以通过 Cycle Start 恢复
if (sys.state == State::Hold && sys.suspend.bit.holdComplete) {
    sys.suspend.value = 0;  // 清除挂起标志
    sys.state = State::Idle;  // 恢复到 Idle
}
```

#### Sleep 状态恢复（只能复位）
```cpp
// Sleep 状态进入死循环，只等待 sys.abort
while (!(sys.abort)) {
    protocol_exec_rt_system();  // 只处理实时系统命令
}
return;  // 只有复位才能退出循环
```

---

## 使用场景

### 适用场景

1. **长时间停机**
   - 机器需要长时间不使用时
   - 节省电力和减少电机发热

2. **紧急停止后的安全状态**
   - 某些错误条件下自动进入
   - 防止意外启动

3. **维护模式**
   - 需要手动操作机器时
   - 确保所有动力系统关闭

### 不适用场景

❌ **不要在以下情况使用 Sleep 模式**：

1. **短暂暂停**：使用 Feed Hold (!) 代替
2. **换刀操作**：使用 M0/M1 程序暂停
3. **调试过程**：使用 Hold 状态更方便恢复
4. **需要保持位置**：Sleep 会禁用步进电机，可能导致位置丢失

---

## 常见问题

### Q1: Sleep 模式下步进电机会保持力矩吗？

**答**：不会。Sleep 模式调用 `st_go_idle()` 禁用步进电机，电机会失去保持力矩。如果机器有重力负载（如 Z 轴），可能会掉落。

### Q2: 能否通过发送 G-code 命令退出 Sleep？

**答**：不能。Sleep 模式下系统不处理 G-code 命令，只响应复位信号。

### Q3: Sleep 模式和 Alarm 状态有什么区别？

**答**：
- **Alarm**：可以通过 `$X` 解锁命令恢复，不需要复位
- **Sleep**：只能通过复位退出，会重新初始化整个系统

### Q4: 复位后工作坐标系会丢失吗？

**答**：取决于设置：
- 如果启用了 `$N` 启动脚本保存坐标系，会自动恢复
- 否则需要重新设置工作坐标系（G54-G59）

### Q5: 如何在代码中检测 Sleep 状态？

**答**：
```cpp
if (sys.state == State::Sleep) {
    // 系统处于 Sleep 模式
}
```

---

## 代码实现细节

### 进入 Sleep 的完整流程

```cpp
// 1. 设置 Sleep 标志（通过实时命令或程序控制）
sys_rt_exec_state.bit.sleep = true;

// 2. 在主循环中检测标志
if (rt_exec_state.bit.sleep) {
    if (sys.state == State::Alarm) {
        sys.suspend.bit.retractComplete = true;
        sys.suspend.bit.holdComplete    = true;
    }
    sys.state = State::Sleep;  // 切换到 Sleep 状态
    sys_rt_exec_state.bit.sleep = false;
}

// 3. 在状态处理中执行 Sleep 逻辑
if (sys.state == State::Sleep) {
    report_feedback_message(Message::SleepMode);
    spindle->set_state(SpindleState::Disable, 0);
    coolant_off();
    st_go_idle();

    // 4. 进入等待循环
    while (!(sys.abort)) {
        protocol_exec_rt_system();
    }

    // 5. 收到复位信号后返回
    return;  // 返回到主循环重新初始化
}
```

### 相关文件索引

| 文件 | 行号 | 说明 |
|------|------|------|
| [System.h](../../Grbl_Esp32/src/System.h) | 38 | State 枚举定义 |
| [Protocol.cpp](../../Grbl_Esp32/src/Protocol.cpp) | 365-372 | Sleep 标志处理 |
| [Protocol.cpp](../../Grbl_Esp32/src/Protocol.cpp) | 636-645 | Sleep 模式执行逻辑 |
| [Stepper.cpp](../../Grbl_Esp32/src/Stepper.cpp) | 442-447 | Sleep 状态下的步进控制 |
| [Report.cpp](../../Grbl_Esp32/src/Report.cpp) | 882 | Sleep 状态报告 |

---

## 最佳实践

### ✅ 推荐做法

1. **使用前确认**
   - 确保机器处于安全位置
   - 确认没有重力负载会掉落
   - 保存当前工作坐标系（如需要）

2. **退出后检查**
   - 复位后检查机器状态
   - 重新归零（如果需要绝对位置）
   - 验证工作坐标系是否正确

3. **替代方案**
   - 短暂暂停：使用 Feed Hold (!)
   - 程序暂停：使用 M0/M1
   - 紧急停止：使用 Reset (Ctrl-X)

### ❌ 避免做法

1. **不要在运动中进入 Sleep**
   - 应先暂停运动（Feed Hold）
   - 等待运动完全停止后再进入

2. **不要依赖 Sleep 保持位置**
   - Sleep 会禁用步进电机
   - 使用 Hold 状态保持位置

3. **不要频繁使用 Sleep**
   - 每次退出都需要完整复位
   - 频繁复位会影响效率

---

## 总结

- **Sleep 模式**：系统进入低功耗等待状态，所有运动停止，步进电机禁用
- **单向状态**：只能通过复位退出，无法直接恢复到 Idle
- **复位方法**：硬件 RESET 按钮、Ctrl-X 软件复位、串口复位命令
- **设计目的**：保护性状态，确保系统通过完整重新初始化恢复运行
- **使用建议**：仅在长时间停机或维护时使用，短暂暂停应使用 Hold 状态

---

**文档版本**: v1.0
**最后更新**: 2026-01-20
**相关文档**:
- [实时命令与状态机](../learning/09_实时命令与状态机.md)
- [安全门功能分析指南](安全门功能分析指南.md)
