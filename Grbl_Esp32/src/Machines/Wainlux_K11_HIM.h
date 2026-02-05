#pragma once
// clang-format off

/*
    Wainlux_K11_HIM.h
    Grbl_ESP32 项目的一部分

    Genmitsu Kiosk C V07 激光雕刻机的引脚配置
    基于原厂提供的 Wainlux_K11.h 配置文件

    机器规格:
    - 2 轴激光雕刻机 (仅使用 X 和 Y 轴, Z 轴已配置但未使用)
    - 工作区域: 100mm x 100mm
    - 激光主轴, 13 位 PWM 控制, 频率 5kHz
    - WiFi 连接, 带 web 界面
    - 安全功能: 盖板开关, 限位开关
    - 辅助功能: 风扇, 照明 LED

    配置来源: 原厂 Wainlux_K11.h
    修改日期: 2026-01-15
    原厂固件: Grbl_ESP32 Ver 1.3a Date 20211103
    ESP32 SDK: v3.2.3-14-gd3e562907

    Grbl_ESP32 是自由软件: 您可以根据自由软件基金会发布的
    GNU 通用公共许可证第 3 版或 (根据您的选择) 任何更高版本
    的条款重新分发和/或修改它.

    发布 Grbl 是希望它有用, 但不提供任何保证;
    甚至不提供适销性或特定用途适用性的默示保证.
    详情请参阅 GNU 通用公共许可证.

    您应该已经收到 Grbl_ESP32 附带的 GNU 通用公共许可证副本.
    如果没有, 请访问 <http://www.gnu.org/licenses/>.
*/

// ============================================================================
// ESP32 引脚使用注意事项
// ============================================================================
// 参考: https://github.com/bdring/Grbl_Esp32/wiki/Setting-Up-the-I-O-Pins
//
// 📌 仅输入引脚 (无内部上拉/下拉电阻)
//    - 只能用于输入, 不能配置为输出
//    - 没有内部上拉/下拉电阻, 如果悬空会产生随机电平
//    - 必须有外部上拉或下拉电阻, 否则不要使用!
//    - 适合用作限位开关, 传感器输入等有明确信号源的场景
//
//    GPIO_NUM_34    ✅ 本配置未使用
//    GPIO_NUM_35    ✅ 本配置用作 Y 轴限位开关 (外部常闭开关提供电平)
//    GPIO_NUM_36    ✅ 本配置用作 X 轴限位开关 (外部常闭开关提供电平)
//    GPIO_NUM_37    ⚠️  通常不存在 (ESP32 芯片未引出)
//    GPIO_NUM_38    ⚠️  通常不存在 (ESP32 芯片未引出)
//    GPIO_NUM_39    ✅ 本配置用作 SD 卡 MISO 引脚 (输入引脚, 安全)
//
// ⚠️  启动相关引脚 (使用时需特别注意)
//    - 这些引脚在上电启动时有特殊功能
//    - 启动时的电平状态会影响 ESP32 的启动模式
//    - 可以在启动后使用, 但要避免外部电路影响启动时的电平
//
//    GPIO_NUM_0     启动模式选择 (Boot)
//                   - 上电时下拉: 进入 UART 下载模式
//                   - 上电时上拉: 正常从 Flash 启动
//                   - 避免使用, 特别是不要有外部下拉电阻
//
//    GPIO_NUM_2     Flash 启动选择 (需要悬空或下拉)
//                   - 上电时必须悬空或下拉才能从 Flash 启动
//                   - 部分开发板的 LED 连接此引脚 (串联电阻, 不影响启动)
//                   - 可以谨慎使用, 但避免外部上拉
//
//    GPIO_NUM_5     VSPI CS / 启动时序控制
//                   - 上电时必须为高电平
//                   - ✅ 本配置用作 Z 轴 STEP_PIN, 启动后可安全使用
//
//    GPIO_NUM_12    Flash 电压选择 (MTDI)
//                   - 上电时电平决定 Flash 工作电压
//                   - 默认内部下拉 (选择 3.3V Flash)
//                   - ✅ 本配置用作 SD 卡 MISO (输入引脚, 安全)
//                   - ⚠️  不要外部上拉, 否则可能导致 Flash 工作异常
//
//    GPIO_NUM_15    调试输出 (MTDO) / HSPI CS
//                   - 启动时输出调试日志
//                   - 上电时必须为高电平
//                   - ✅ 本配置用作 SD 卡 CS, 启动后可安全使用
//
// 🔒 串口调试引脚 (强烈不推荐使用)
//    - 用于 UART0 串口通信, 是主要的调试和通信接口
//    - 占用这些引脚会导致无法串口调试和烧录
//
//    GPIO_NUM_1     UART0 TX (串口发送) - 禁止使用
//    GPIO_NUM_3     UART0 RX (串口接收) - 禁止使用
//
// 🔒 Flash 专用引脚 (禁止使用)
//    - 连接到 ESP32 内部 SPI Flash 存储器
//    - 占用这些引脚会导致系统无法启动
//
//    GPIO_NUM_6     Flash SCK  - 禁止使用
//    GPIO_NUM_7     Flash SD0  - 禁止使用
//    GPIO_NUM_8     Flash SD1  - 禁止使用
//    GPIO_NUM_9     Flash SD2  - 禁止使用
//    GPIO_NUM_10    Flash SD3  - 禁止使用
//    GPIO_NUM_11    Flash CMD  - 禁止使用
//
// ❌ 不存在的引脚编号
//    - ESP32 芯片没有这些 GPIO 编号
//    - 代码中使用会导致编译错误或运行时异常
//
//    GPIO_NUM_20    ESP32 上不存在
//    GPIO_NUM_24    ESP32 上不存在
//    GPIO_NUM_28    ESP32 上不存在
//    GPIO_NUM_29    ESP32 上不存在
//    GPIO_NUM_30    ESP32 上不存在
//    GPIO_NUM_31    ESP32 上不存在
//    GPIO_NUM_37    ESP32 上不存在 (未引出)
//    GPIO_NUM_38    ESP32 上不存在 (未引出)
//
// ✅ 推荐使用的通用 GPIO 引脚
//    - 这些引脚没有启动限制, 可以安全用于各种功能
//    - 启动后可自由配置为输入或输出
//
//    GPIO_NUM_4, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
//
// 📚 参考资料
//    - ESP32 技术参考手册: https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
//    - ESP32 硬件设计指南: https://www.espressif.com/sites/default/files/documentation/esp32_hardware_design_guidelines_en.pdf
//    - Grbl_ESP32 Wiki: https://github.com/bdring/Grbl_Esp32/wiki/Setting-Up-the-I-O-Pins
// ============================================================================

// ============================================================================
// 机器名称
// ============================================================================
// 原厂定义了两个名称, 实际使用 MACHINE_NAME
// MACHINE_NAME_C 可能用于区分不同版本
#define MACHINE_NAME            "Genmitsu_Kiosk_V07_HIM"
#define MACHINE_NAME_C          "Genmitsu_Kiosk_C_V07_HIM"

#define CONNECT_TO_SSID         "MT3000"
#define SSID_PASSWORD           "88888888"

#define CONNECT_TO_AP_SSID      MACHINE_NAME
#define AP_SSID_PASSWORD        "12345678"

// ============================================================================
// 自定义代码文件 (可选)
// ============================================================================
// 原厂固件可能使用了自定义代码文件来实现特殊功能
// 如果需要自定义初始化逻辑, 可以取消注释并创建对应文件
#define CUSTOM_CODE_FILENAME "../Custom/wainlux_k11_HIM.cpp"

// ============================================================================
// 轴配置
// ============================================================================
// 本机器配置了 3 个轴, 但实际只使用 X 和 Y 轴
// Z 轴为兼容性而配置, 但没有限位开关
#define N_AXIS 3

// ============================================================================
// 步进电机引脚
// ============================================================================
// 所有三个轴共享同一个禁用引脚 (GPIO 27)
// 电机驱动类型: 标准 Step/Dir 接口

// X 轴 - 水平移动
// 启动日志: [MSG:X  Axis Standard Stepper Step:GPIO(33) Dir:GPIO(32) Disable:GPIO(27) Limits(-2.000,98.000)]
#define X_STEP_PIN              GPIO_NUM_33
#define X_DIRECTION_PIN         GPIO_NUM_32
#define X_DISABLE_PIN           GPIO_NUM_27

// Y 轴 - 垂直移动
// 启动日志: [MSG:Y  Axis Standard Stepper Step:GPIO(26) Dir:GPIO(25) Disable:GPIO(27) Limits(-2.000,98.000)]
#define Y_STEP_PIN              GPIO_NUM_26
#define Y_DIRECTION_PIN         GPIO_NUM_25
#define Y_DISABLE_PIN           GPIO_NUM_27

// Z 轴 - 已配置但实际未使用 (可能用于焦距调整)
// 原厂日志: [MSG:Z  Axis Standard Stepper Step:GPIO(5) Dir:GPIO(18) Disable:GPIO(27) Limits(0.000,1000.000)]
#define Z_STEP_PIN              GPIO_NUM_5
#define Z_DIRECTION_PIN         GPIO_NUM_18
#define Z_DISABLE_PIN           GPIO_NUM_27

// ============================================================================
// 限位开关
// ============================================================================
// 仅 X 和 Y 轴有限位开关
// Z 轴没有限位开关 (本设备是 2 轴激光雕刻机)
// 启动日志: [MSG:X  Axis limit switch on pin GPIO(36)]
//           [MSG:Y  Axis limit switch on pin GPIO(35)]
//
// 硬件特性: 常闭开关 (NC - Normally Closed)
// - 正常状态: 低电平 (0V)
// - 触发状态: 高电平 (3.3V)
// - 因此 $5=0 (禁用反转逻辑)

#define X_LIMIT_PIN             GPIO_NUM_36  // 仅输入引脚
#define Y_LIMIT_PIN             GPIO_NUM_35  // 仅输入引脚
// Z_LIMIT_PIN 故意未定义 - 没有 Z 轴限位开关

// ============================================================================
// 辅助控制引脚
// ============================================================================

#define USER_BUTTON_PIN         GPIO_NUM_19     // 用户按钮 (短按/长按/超长按)
#define COOLING_FAN_PIN         GPIO_NUM_16     // 冷却风扇 (PWM 控制)
#define SAFETY_DOOR_PIN         GPIO_NUM_17     // 安全门开关 (常闭)
#define STATUS_LED_PIN          GPIO_NUM_23     // 状态指示 LED (PWM 呼吸灯)

// ============================================================================
// User Output 配置 (风扇控制)
// ============================================================================
// 没有内容使用 Grbl_ESP32 标准 User Output 系统
// LED, 按钮, 安全门风扇, 全部使用自定义代码实现

// ============================================================================
// 自定义功能引脚 (不使用 Grbl 预定义接口)
// ============================================================================
// 以下引脚全部在自定义代码 (wainlux_k11_HIM.cpp) 中直接控制:
// - USER_BUTTON_PIN (GPIO_NUM_19): 用户按钮, 支持短按/3秒长按/10秒超长按
// - STATUS_LED_PIN (GPIO_NUM_23): 状态指示 LED, PWM 呼吸灯和多种闪烁模式
// - COOLING_FAN_PIN (GPIO_NUM_16): 冷却风扇, PWM 速度控制
// - SAFETY_DOOR_PIN (GPIO_NUM_17): 安全门开关, 自动暂停和恢复
// - TILT_SENSOR_SCL_PIN/TILT_SENSOR_SDA_PIN (GPIO_NUM_4/22): QMI8658A 倾斜传感器 I2C

// 注意: 不使用以下 Grbl 预定义接口:
// - USER_DIGITAL_PIN_0 (LED) - 已移除
// - MACRO_BUTTON_0_PIN (按钮) - 已移除
// - CONTROL_SAFETY_DOOR_PIN (安全门) - 已移除

// ============================================================================
// 激光主轴配置
// ============================================================================
// 启动日志: [MSG:Laser spindle on Pin:GPIO(21), Enbl:None, Freq:5000Hz, Res:13bits Laser mode:On]
//
// 激光规格:
// - PWM 输出在 GPIO 21
// - 没有单独的使能引脚
// - 5000 Hz PWM 频率
// - 13 位分辨率 (0-8191 级)
// - 激光模式已启用 (基于速度的动态功率调整)
//
// 重要: Grbl_ESP32 激光主轴使用 LASER_OUTPUT_PIN, 而不是 SPINDLE_OUTPUT_PIN
// 详见 Grbl_Esp32/src/Spindles/Laser.cpp:48-52 的实现细节

#define SPINDLE_TYPE            SpindleType::LASER  // only one spindle at a time
#define LASER_OUTPUT_PIN        GPIO_NUM_21
// LASER_ENABLE_PIN 故意未定义 - 激光没有单独的使能引脚

// ============================================================================
// 默认设置值 (覆盖 Defaults.h 中的全局默认值)
// ============================================================================
// 这些值来自原厂配置文件, 已经过实际测试验证
// 注释中的 $xxx 表示对应的 Grbl 设置参数编号

//原始K11
#define DEFAULT_STEP_PULSE_MICROSECONDS 3   // $0
#define DEFAULT_STEPPER_IDLE_LOCK_TIME 25   // 25ms $1
#define DEFAULT_STEPPING_INVERT_MASK 0      // uint8_t $2
#define DEFAULT_DIRECTION_INVERT_MASK 4     // uint8_t $3
#define DEFAULT_INVERT_ST_ENABLE 0          // boolean $4
#define DEFAULT_INVERT_LIMIT_PINS 0         // $5
#define DEFAULT_INVERT_PROBE_PIN 0          // false $6
#define DEFAULT_STATUS_REPORT_MASK 1        // $10
#define DEFAULT_JUNCTION_DEVIATION 0.01     // $11
#define DEFAULT_ARC_TOLERANCE 0.002         // $12
#define DEFAULT_REPORT_INCHES 0             // $13
#define DEFAULT_SOFT_LIMIT_ENABLE 0         // false $20
#define DEFAULT_HARD_LIMIT_ENABLE 1         // false $21
#define DEFAULT_HOMING_ENABLE 1             // false $22
#define DEFAULT_HOMING_DIR_MASK 7           // $23
#define DEFAULT_HOMING_FEED_RATE 800.0      // mm/min $24
#define DEFAULT_HOMING_SEEK_RATE 2000.0     // mm/min $25
#define DEFAULT_HOMING_DEBOUNCE_DELAY 30    // msec (0-65k) $26
#define DEFAULT_HOMING_PULLOFF 2.0          // mm $27
#define DEFAULT_SPINDLE_RPM_MAX 1000.0      // $30
#define DEFAULT_SPINDLE_RPM_MIN 0.0         // $31
#define DEFAULT_LASER_MODE 1                // $32
#define DEFAULT_X_STEPS_PER_MM 100          // $100
#define DEFAULT_Y_STEPS_PER_MM 100          // $101
#define DEFAULT_Z_STEPS_PER_MM 100          // $102

#define DEFAULT_X_MAX_RATE 12000.0          // mm/min $110
#define DEFAULT_Y_MAX_RATE 12000.0          // mm/min $111
#define DEFAULT_Z_MAX_RATE 1000.0           // mm/min $112

#define DEFAULT_X_ACCELERATION 800.0        // mm/sec^2. 500 mm/sec^2 = 1800000 mm/min^2 $120
#define DEFAULT_Y_ACCELERATION 240.0        // mm/sec^2 $121
#define DEFAULT_Z_ACCELERATION 200.0        // mm/sec^2 $122

#define DEFAULT_X_MAX_TRAVEL 100.0          // mm NOTE: Must be a positive value. $130
#define DEFAULT_Y_MAX_TRAVEL 100.0          // mm NOTE: Must be a positive value. $131
#define DEFAULT_Z_MAX_TRAVEL 1000.0         // mm NOTE: Must be a positive value. $132


#define DEFAULT_HOMING_CYCLE_0 (bit(X_AXIS) | bit(Y_AXIS))
#define DEFAULT_HOMING_CYCLE_1 0

#define DEFAULT_X_HOMING_MPOS -2
#define DEFAULT_Y_HOMING_MPOS -2

#define AP_SSID           "Genmitsu_Kiosk"
#define AP_PASSWORD       "12345678"

// ============================================================================
// SD 卡 SPI 引脚配置
// ============================================================================
// 自定义 SD 卡 SPI 引脚, 避免与 STATUS_LED_PIN (GPIO_NUM_23) 冲突
// 默认 VSPI MOSI 引脚是 GPIO23, 与 LED 引脚冲突, 导致 SD 卡初始化后 LED 失效
//
// 引脚分配:
// - GRBL_SPI_SS (CS):     GPIO_NUM_15 - SD 卡片选
// - GRBL_SPI_MOSI (CMD):  GPIO_NUM_13 - SD 卡数据输出
// - GRBL_SPI_MISO (D0):   GPIO_NUM_12 - SD 卡数据输入
// - GRBL_SPI_SCK (CLK):   GPIO_NUM_14 - SD 卡时钟
// - GRBL_SPI_FREQ:        4000000 (4MHz) - SPI 时钟频率
//
// 注意: 必须同时定义所有 5 个宏, 否则会使用默认值 -1 (VSPI 默认引脚)
#define GRBL_SPI_SS     GPIO_NUM_15     // SD 卡片选引脚
#define GRBL_SPI_MOSI   GPIO_NUM_13     // SD 卡 MOSI
#define GRBL_SPI_MISO   GPIO_NUM_12     // SD 卡 MISO
#define GRBL_SPI_SCK    GPIO_NUM_14     // SD 卡时钟
#define GRBL_SPI_FREQ   4000000         // SPI 频率 4MHz

#define SDCARD_DET_PIN  GPIO_NUM_39     // SD 卡片检测引脚

// SD 卡命令暂存功能
// 当 SD 卡忙时 (如文件上传), 暂存接收到的命令, 等 SD 卡空闲后再执行
// 注释此行可禁用命令暂存功能 (SD 卡忙时直接返回错误)
// #define ENABLE_SD_PENDING_COMMAND

// ============================================================================
// 调试功能配置
// ============================================================================
// 协议层调试日志
// 启用后会打印从缓冲区读取的字符和接收到的完整命令行
// 注释此行可禁用调试日志 (减少串口输出, 提高性能)
// #define ENABLE_PROTOCOL_DEBUG_LOG

// Telnet 客户端连接日志
// 启用后会打印 Telnet 客户端的连接和断开信息
// 注释此行可禁用日志 (减少串口输出)
#define ENABLE_TELNET_CLIENT_LOG

// WebSocket 客户端连接日志
// 启用后会打印 WebSocket 客户端的连接和断开信息
// 注释此行可禁用日志 (减少串口输出)
#define ENABLE_WEBSOCKET_CLIENT_LOG

// ============================================================================
// 安全功能配置
// ============================================================================
#define DEFAULT_SAFETY_DOOR_ENABLED    1    // 安全门开关默认启用 (0=禁用, 1=启用)
#define DEFAULT_TILT_SENSOR_ENABLED    1    // 倾斜传感器默认启用 (0=禁用, 1=启用)
#define TILT_SENSOR_SCL_PIN            GPIO_NUM_4   // 倾斜传感器 I2C SCL 引脚
#define TILT_SENSOR_SDA_PIN            GPIO_NUM_22  // 倾斜传感器 I2C SDA 引脚

// ============================================================================
// 启动行为配置
// ============================================================================
// 禁用 HOMING_INIT_LOCK: 系统启动后不进入 Alarm 状态, 直接执行启动脚本
// 启动脚本将在 machine_init() 中设置为 "$H" (自动归位)
// Grbl_Esp32\src\Config.h: #define HOMING_INIT_LOCK  // Comment to disable

// 自动判断是否需要控制开关防抖
// 只有在定义了 Grbl 预定义控制引脚时才启用防抖任务
#if defined(CONTROL_SAFETY_DOOR_PIN) || defined(CONTROL_RESET_PIN) || \
    defined(CONTROL_FEED_HOLD_PIN) || defined(CONTROL_CYCLE_START_PIN) || \
    defined(MACRO_BUTTON_0_PIN) || defined(MACRO_BUTTON_1_PIN) || \
    defined(MACRO_BUTTON_2_PIN) || defined(MACRO_BUTTON_3_PIN)
    #warning "Note whether the `user_defined_macro` button callback function has been rewritten. This warning line can be commented out."
    #define ENABLE_CONTROL_SW_DEBOUNCE
#endif

#define ENABLE_SOFTWARE_DEBOUNCE            // 限位开关防抖

// clang-format on
