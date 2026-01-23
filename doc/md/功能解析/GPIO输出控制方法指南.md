# Grbl_ESP32 GPIO输出控制方法指南

## 概述

Grbl_ESP32提供了灵活的GPIO输出控制功能，允许用户通过多种方式控制数字输出引脚，包括状态LED、继电器、风扇等外设。本指南详细介绍了GPIO输出控制的各种方法和最佳实践。

---

## 1. 用户输出(User Output)功能

### 1.1 功能简介

用户输出功能是Grbl_ESP32提供的通用GPIO控制机制，支持数字输出和模拟输出（PWM）两种模式。

### 1.2 数字输出控制

数字输出控制通过以下G-code命令实现：

| 命令 | 功能 | 说明 |
|------|------|------|
| `M62 Pn` | 同步数字输出ON | `Pn`为输出通道号(0-3)，等待缓冲区清空后执行 |
| `M63 Pn` | 同步数字输出OFF | `Pn`为输出通道号(0-3)，等待缓冲区清空后执行 |
| `M64 Pn` | 立即数字输出ON | `Pn`为输出通道号(0-3)，立即执行 |
| `M65 Pn` | 立即数字输出OFF | `Pn`为输出通道号(0-3)，立即执行 |

### 1.3 模拟输出控制

模拟输出（PWM）控制通过以下G-code命令实现：

| 命令 | 功能 | 说明 |
|------|------|------|
| `M67 En Qxx.xx` | 同步模拟输出 | `En`为输出通道号(0-3)，`Q`为占空比百分比 |
| `M68 En Qxx.xx` | 立即模拟输出 | `En`为输出通道号(0-3)，`Q`为占空比百分比 |

---

## 2. 配置方法

### 2.1 配置数字输出引脚

在机器配置文件中定义数字输出引脚：

```cpp
// 数字输出引脚定义（最多4个）
#define USER_DIGITAL_PIN_0      GPIO_NUM_21  // 用户数字输出0
#define USER_DIGITAL_PIN_1      GPIO_NUM_25  // 用户数字输出1
#define USER_DIGITAL_PIN_2      GPIO_NUM_5   // 用户数字输出2
#define USER_DIGITAL_PIN_3      GPIO_NUM_18  // 用户数字输出3
```

### 2.2 配置模拟输出引脚

在机器配置文件中定义模拟输出引脚：

```cpp
// 模拟输出引脚定义（最多4个）
#define USER_ANALOG_PIN_0       GPIO_NUM_22    // 用户模拟输出0
#define USER_ANALOG_PIN_1       GPIO_NUM_26    // 用户模拟输出1
#define USER_ANALOG_PIN_2       GPIO_NUM_4     // 用户模拟输出2
#define USER_ANALOG_PIN_3       GPIO_NUM_15    // 用户模拟输出3

// 可选：定义PWM频率（默认5000Hz）
#define USER_ANALOG_PIN_0_FREQ  1000  // 1kHz频率
#define USER_ANALOG_PIN_1_FREQ  5000  // 5kHz频率
```

---

## 3. 硬件连接

### 3.1 直接LED连接

适用于小功率LED（<12mA）：

```
ESP32 GPIO ──┬── LED正极(通过限流电阻)
             │
            GND ── LED负极
```

**注意**：ESP32 GPIO最大输出电流约40mA，为保证安全建议不超过12mA。

### 3.2 MOSFET驱动连接

适用于高功率设备：

```
                    +电源正极
                      │
                      ├─── 设备正极
                      │
ESP32 GPIO ───┬───┤ MOSFET (如IRLZ44N)
              │    │   Gate
             10K   └─── 设备负极
              │         │
             GND ───────┴─── 电源负极
```

**元件选择**：
- MOSFET：选用逻辑电平MOSFET（Vgs(th) < 2V），如IRLZ44N
- 下拉电阻：10kΩ，防止GPIO悬空时误触发
- 续流二极管：1N4007（驱动感性负载时）

### 3.3 继电器模块连接

适用于大功率或高压设备：

```
ESP32 GPIO ──→ 继电器模块 IN
ESP32 GND ──→ 继电器模块 GND
ESP32 3.3V ──→ 继电器模块 VCC

继电器输出:
  COM ──→ 电源正极
  NO  ──→ 设备正极
  设备负极 ──→ 电源负极
```

---

## 4. 应用示例

### 4.1 状态LED控制

在机器配置文件中定义状态LED：

```cpp
#define USER_DIGITAL_PIN_0      GPIO_NUM_21  // 状态LED

// 可选：机器初始化时的操作
void machine_init() {
    grbl_msg_sendf(CLIENT_ALL, MsgLevel::Info, "Status LED on GPIO %d", USER_DIGITAL_PIN_0);

    // 初始状态：关闭LED
    if (myDigitalOutputs[0]) {
        myDigitalOutputs[0]->set_level(false);
    }
}
```

**G-code控制示例**：
```
M62 P0 ; 开启状态LED
G4 P1 ; 延时1秒
M63 P0 ; 关闭状态LED
```

### 4.2 风扇控制

使用数字输出控制散热风扇：

```cpp
#define USER_DIGITAL_PIN_0      GPIO_NUM_25  // 风扇控制
```

**G-code控制示例**：
```
M62 P0 ; 开启风扇
M63 P0 ; 关闭风扇
```

### 4.3 PWM调光控制

使用模拟输出控制LED亮度：

```cpp
#define USER_ANALOG_PIN_0       GPIO_NUM_22  // PWM调光输出
```

**G-code控制示例**：
```
M67 E0 Q100.0 ; 最大亮度
M67 E0 Q50.0  ; 50%亮度
M67 E0 Q10.0  ; 10%亮度
M67 E0 Q0.0   ; 关闭
```

---

## 5. 编程控制方法

### 5.1 在自定义代码中控制

```cpp
#include "Grbl.h"

// 控制数字输出
void control_digital_output(int channel, bool state) {
    if (myDigitalOutputs[channel]) {
        myDigitalOutputs[channel]->set_level(state);
    }
}

// 控制模拟输出
void control_analog_output(int channel, float duty_percent) {
    if (myAnalogOutputs[channel]) {
        uint32_t value = (uint32_t)(myAnalogOutputs[channel]->denominator() * duty_percent / 100.0f);
        myAnalogOutputs[channel]->set_level(value);
    }
}
```

### 5.2 状态感知控制

```cpp
// 根据系统状态控制LED
void update_status_led() {
    static uint32_t last_check = 0;

    if (millis() - last_check > 100) {
        last_check = millis();

        switch (sys.state) {
            case State::Idle:
                // 空闲状态：慢闪LED
                if ((millis() / 1000) % 10 < 2) {
                    myDigitalOutputs[0]->set_level(true);
                } else {
                    myDigitalOutputs[0]->set_level(false);
                }
                break;

            case State::Cycle:
                // 运行状态：快闪LED
                if ((millis() / 200) % 2) {
                    myDigitalOutputs[0]->set_level(true);
                } else {
                    myDigitalOutputs[0]->set_level(false);
                }
                break;

            case State::Alarm:
                // 报警状态：快速闪烁
                if ((millis() / 100) % 2) {
                    myDigitalOutputs[0]->set_level(true);
                } else {
                    myDigitalOutputs[0]->set_level(false);
                }
                break;

            default:
                // 其他状态：关闭LED
                myDigitalOutputs[0]->set_level(false);
                break;
        }
    }
}
```

---

## 6. 注意事项

### 6.1 GPIO选择注意事项

- **避免启动引脚**：GPIO 0, 2, 4, 12, 15等引脚在启动时有特殊用途，避免用作输出
- **检查冲突**：确保GPIO没有与其他功能冲突
- **电流限制**：单个GPIO输出电流不超过40mA（推荐<12mA）

### 6.2 安全措施

- **限流电阻**：直接驱动LED时必须串联限流电阻
- **保护电路**：驱动感性负载时添加续流二极管
- **隔离措施**：控制高压设备时使用继电器或固态继电器

### 6.3 调试技巧

- **串口输出**：查看启动信息确认引脚配置正确
- **WebUI监控**：通过WebUI实时监控输出状态
- **状态查询**：发送`?`命令查看系统状态

---

## 7. 故障排除

### 7.1 LED不亮

1. 检查配置文件中引脚定义是否正确
2. 确认GPIO未与其他功能冲突
3. 检查硬件连接是否正确
4. 测试GPIO是否损坏

### 7.2 控制无效

1. 确认G-code语法正确
2. 检查引脚编号是否在0-3范围内
3. 验证设备驱动能力是否足够
4. 检查是否达到硬件电流限制

### 7.3 输出不稳定

1. 检查电源供电是否稳定
2. 确认接地连接良好
3. 添加适当的滤波电容
4. 检查是否存在电气干扰