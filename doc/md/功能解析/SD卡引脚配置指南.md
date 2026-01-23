# SD 卡引脚配置指南

## 概述

本文档说明 Grbl_Esp32 开源库中 SD 卡 SPI 引脚的定义方式和自定义方法。

---

## 默认引脚定义

### 1. SPI 引脚宏定义

在 [MachineCommon.h:6-15](../../../Grbl_Esp32/src/MachineCommon.h) 中定义了默认的 SPI 引脚：

```cpp
#ifndef GRBL_SPI_FREQ
// You can override these by defining them in a board file.
// To override, you must set all of them
//-1 means use the default board pin
#    define GRBL_SPI_SS -1
#    define GRBL_SPI_MOSI -1
#    define GRBL_SPI_MISO -1
#    define GRBL_SPI_SCK -1
#    define GRBL_SPI_FREQ 4000000
#endif
```

**关键点**：
- `-1` 表示使用 ESP32 默认的 VSPI 引脚
- 必须同时定义所有 5 个宏才能覆盖默认值
- 默认 SPI 频率为 4MHz

### 2. ESP32 VSPI 默认引脚映射

根据 [SDCard.h:4-16](../../../Grbl_Esp32/src/SDCard.h) 的注释：

| SD 卡引脚 | ESP32 引脚 | 功能 |
|----------|-----------|------|
| D3 (CS)  | GPIO5     | SS (片选) |
| CMD      | GPIO23    | MOSI (主出从入) |
| CLK      | GPIO18    | SCK (时钟) |
| D0       | GPIO19    | MISO (主入从出) |
| VDD      | 3.3V      | 电源 |
| VSS      | GND       | 地 |

---

## SD 卡初始化流程

### 初始化代码

在 [SDCard.cpp:154](../../../Grbl_Esp32/src/SDCard.cpp) 中：

```cpp
if (SD.begin((GRBL_SPI_SS == -1) ? SS : GRBL_SPI_SS, SPI, GRBL_SPI_FREQ, "/sd", 2)) {
    if (SD.cardSize() > 0) {
        sd_state = SDState::Idle;
    }
}
```

### 参数说明

| 参数 | 说明 |
|-----|------|
| `(GRBL_SPI_SS == -1) ? SS : GRBL_SPI_SS` | CS 引脚：-1 时使用 Arduino 默认 SS (GPIO5) |
| `SPI` | 使用默认 SPI 对象 (VSPI) |
| `GRBL_SPI_FREQ` | SPI 时钟频率（默认 4MHz） |
| `"/sd"` | 文件系统挂载路径 |
| `2` | 最大同时打开文件数 |

---

## SD 卡检测引脚

### 可选的卡检测功能

在 [Defaults.h:476-477](../../../Grbl_Esp32/src/Defaults.h) 中：

```cpp
#ifndef SDCARD_DET_PIN
#    define SDCARD_DET_PIN UNDEFINED_PIN
#endif
```

在 [SDCard.h:24](../../../Grbl_Esp32/src/SDCard.h) 中：

```cpp
const int SDCARD_DET_VAL = 0;  // for now, CD is close to ground
```

**说明**：
- `SDCARD_DET_PIN`：SD 卡检测引脚（Card Detect）
- `SDCARD_DET_VAL = 0`：检测逻辑为低电平有效（卡插入时接地）
- 默认未定义，表示不使用卡检测功能

---

## 自定义 SD 卡引脚

### 在机器配置文件中定义

在你的机器配置文件（如 `Machines/Wainlux_K11_HIM.h`）中添加：

```cpp
// ========================================
// SD 卡 SPI 引脚配置
// ========================================
#define GRBL_SPI_SS   GPIO_NUM_5   // CS 片选引脚
#define GRBL_SPI_MOSI GPIO_NUM_23  // MOSI 数据输出
#define GRBL_SPI_MISO GPIO_NUM_19  // MISO 数据输入
#define GRBL_SPI_SCK  GPIO_NUM_18  // SCK 时钟
#define GRBL_SPI_FREQ 4000000      // SPI 频率 4MHz

// 可选：SD 卡检测引脚
#define SDCARD_DET_PIN GPIO_NUM_XX  // 卡检测引脚（可选）
```

### 重要注意事项

1. **必须同时定义所有 5 个宏**
   - 如果只定义部分宏，系统会使用默认值 `-1`
   - 必须定义：`GRBL_SPI_SS`, `GRBL_SPI_MOSI`, `GRBL_SPI_MISO`, `GRBL_SPI_SCK`, `GRBL_SPI_FREQ`

2. **引脚选择建议**
   - 避开启动时的特殊引脚（GPIO 0, 2, 5, 12, 15）
   - 推荐使用：GPIO 4, 13, 14, 16-19, 21-23, 25-27, 32-33
   - 确保引脚不与其他功能冲突（步进电机、限位开关等）

3. **SPI 频率设置**
   - 默认 4MHz 适用于大多数 SD 卡
   - 可以尝试提高到 8MHz 或 16MHz 以���升速度
   - 过高频率可能导致读写不稳定

---

## 使用示例

### 示例 1：使用默认 VSPI 引脚

```cpp
// 不定义任何 GRBL_SPI_* 宏，系统自动使用默认引脚
// CS=GPIO5, MOSI=GPIO23, MISO=GPIO19, SCK=GPIO18
```

### 示例 2：自定义引脚

```cpp
// 使用自定义引脚
#define GRBL_SPI_SS   GPIO_NUM_15
#define GRBL_SPI_MOSI GPIO_NUM_13
#define GRBL_SPI_MISO GPIO_NUM_12
#define GRBL_SPI_SCK  GPIO_NUM_14
#define GRBL_SPI_FREQ 8000000  // 8MHz
```

### 示例 3：带卡检测功能

```cpp
// 自定义引脚 + 卡检测
#define GRBL_SPI_SS   GPIO_NUM_5
#define GRBL_SPI_MOSI GPIO_NUM_23
#define GRBL_SPI_MISO GPIO_NUM_19
#define GRBL_SPI_SCK  GPIO_NUM_18
#define GRBL_SPI_FREQ 4000000

#define SDCARD_DET_PIN GPIO_NUM_34  // 使用 GPIO34 作为卡检测引脚
```

---

## 常见问题

### Q1: 为什么 SD 卡初始化失败？

**可能原因**：
1. 引脚定义不完整（只定义了部分宏）
2. 引脚与其他功能冲突
3. SPI 频率过高
4. SD 卡接触不良或损坏
5. 电源不稳定（SD 卡需要稳定的 3.3V）

**解决方法**：
- 确保同时定义所有 5 个 `GRBL_SPI_*` 宏
- 检查引脚是否与其他功能冲突
- 降低 `GRBL_SPI_FREQ` 到 2MHz 或 1MHz 测试
- 使用万用表检查引脚连接和电压

### Q2: 可以使用 HSPI 而不是 VSPI 吗？

**回答**：
- 当前代码使用默认的 `SPI` 对象（VSPI）
- 如需使用 HSPI，需要修改 [SDCard.cpp:154](../../../Grbl_Esp32/src/SDCard.cpp) 中的 `SPI` 参数
- HSPI 默认引脚：CS=GPIO15, MOSI=GPIO13, MISO=GPIO12, SCK=GPIO14

### Q3: SD 卡检测引脚是必需的吗？

**回答**：
- 不是必需的，默认未启用
- 如果不定义 `SDCARD_DET_PIN`，系统会尝试直接访问 SD 卡
- 建议在生产环境中使用卡检测功能，避免无卡时的错误

---

## 实际案例：Wainlux K11 HIM 引脚冲突问题

### 问题描述

在 Wainlux K11 HIM 机器配置中，发现 LED 在运行过程中一直不亮，只有在刚上电时亮一会。

### 问题根源

**引脚冲突**：
- `STATUS_LED_PIN = GPIO_NUM_23` (LED 引脚)
- ESP32 VSPI 默认 `MOSI = GPIO_NUM_23` (SD 卡数据输出)

**冲突发生时机**：
1. 上电时：LED 初始化并点亮 ✅
2. 调用 `get_sd_state(true)` 初始化 SD 卡时：
   - 执行 `SD.begin()` 初始化 SPI 总线
   - GPIO23 被重新配置为 SPI MOSI 输出
   - LED 的 PWM 控制失效 ❌

### 解决方案

在 [Wainlux_K11_HIM.h](../../../Grbl_Esp32/src/Machines/Wainlux_K11_HIM.h) 中添加自定义 SD 卡 SPI 引脚：

```cpp
// ============================================================================
// SD 卡 SPI 引脚配置
// ============================================================================
// 自定义 SD 卡 SPI 引脚，避免与 STATUS_LED_PIN (GPIO_NUM_23) 冲突
// 默认 VSPI MOSI 引脚是 GPIO23，与 LED 引脚冲突，导致 SD 卡初始化后 LED 失效
//
// 引脚分配:
// - GRBL_SPI_SS (CS):     GPIO_NUM_15 - SD 卡片选
// - GRBL_SPI_MOSI (CMD):  GPIO_NUM_13 - SD 卡数据输出（避开 GPIO23）
// - GRBL_SPI_MISO (D0):   GPIO_NUM_12 - SD 卡数据输入
// - GRBL_SPI_SCK (CLK):   GPIO_NUM_14 - SD 卡时钟
// - GRBL_SPI_FREQ:        4000000 (4MHz) - SPI 时钟频率
//
// 注意: 必须同时定义所有 5 个宏，否则会使用默认值 -1（VSPI 默认引脚）
#define GRBL_SPI_SS   GPIO_NUM_15   // SD 卡片选引脚
#define GRBL_SPI_MOSI GPIO_NUM_13   // SD 卡 MOSI（避开 GPIO23 LED 引脚）
#define GRBL_SPI_MISO GPIO_NUM_12   // SD 卡 MISO
#define GRBL_SPI_SCK  GPIO_NUM_14   // SD 卡时钟
#define GRBL_SPI_FREQ 4000000       // SPI 频率 4MHz
```

### 硬件接线要求

使用自定义 SPI 引脚后，需要修改 SD 卡的硬件接线：

| SD 卡引脚 | 原接线 (VSPI) | 新接线 (自定义) |
|----------|--------------|----------------|
| CS (D3)  | GPIO5        | **GPIO15** |
| MOSI (CMD) | GPIO23 ❌   | **GPIO13** ✅ |
| MISO (D0) | GPIO19       | **GPIO12** |
| SCK (CLK) | GPIO18       | **GPIO14** |
| VDD      | 3.3V         | 3.3V |
| VSS      | GND          | GND |

### 验证方法

1. **编译并烧录固件**
2. **检查启动日志**，确认 SD 卡初始化成功
3. **测试 LED 功能**，确认在 SD 卡初始化后 LED 仍然正常工作
4. **测试 SD 卡功能**，确认可以正常读写文件

### 其他解决方案

如果不想修改硬件接线，可以考虑：

**方案 A：更换 LED 引脚**
```cpp
// 将 LED 引脚改为其他可用引脚
#define STATUS_LED_PIN GPIO_NUM_25  // 或 GPIO_NUM_26, GPIO_NUM_27
```

**方案 B：禁用 SD 卡功能**
```cpp
// 在 Config.h 或 platformio.ini 中注释掉
// #define ENABLE_SD_CARD
```

---

## 相关文件

- [MachineCommon.h](../../../Grbl_Esp32/src/MachineCommon.h) - SPI 引脚默认定义
- [SDCard.h](../../../Grbl_Esp32/src/SDCard.h) - SD 卡接口定义
- [SDCard.cpp](../../../Grbl_Esp32/src/SDCard.cpp) - SD 卡初始化和操作
- [Defaults.h](../../../Grbl_Esp32/src/Defaults.h) - 默认配置
- [Wainlux_K11_HIM.h](../../../Grbl_Esp32/src/Machines/Wainlux_K11_HIM.h) - Wainlux K11 HIM 机器配置

---

**最后更新**: 2026-01-20
