/*
    wainlux_k11_HIM.cpp
    Wainlux K11 HIM 自定义功能实现 (完全重构版)

    功能:
    1. 按钮多功能控制:
        - 短按 (< 0.5s): 归零+预览 / 暂停 / 恢复
        - 长按 3s: 启动离线雕刻 / 停止任务
        - 超长按 10s: 重置 WiFi 为 AP 模式

    2. LED 状态指示 (PWM 呼吸灯 + 多种闪烁):
        - 警告状态: 0.5s 亮 / 0.2s 灭
        - 安全保护: 0.5s 亮 / 0.5s 灭
        - 雕刻状态: PWM 呼吸灯 (2秒周期)
        - WiFi 连接中: 0.2s 亮 / 0.2s 灭
        - 待机模式: 常亮

    3. 安全门和倾斜传感器:
        - 自动暂停和恢复运行
        - QMI8658A 六轴 IMU 倾斜检测

    4. 风扇自动控制:
        - 开机前 5 分钟: 50% 占空比
        - 5 分钟后: 激光开启时 50%, 关闭时 0%

    5. 离线雕刻:
        - 从 SD 卡执行 /001.nc 文件
        - 预览边界 (移动到四个角点)

    注意:
    - 完全使用自定义代码, 不依赖 Grbl 预定义接口
    - 所有 RTOS 任务固定到核心 1
*/

#include "../src/Grbl.h"
#include <QMI8658.h>
#include <WiFi.h>
#include <Wire.h>

// ============================================================================
// 日志宏定义 (编译时优化)
// ============================================================================

// 日志等级控制 (0=关闭, 1=Error, 2=Warning, 3=Info, 4=Debug)
#define LOG_LEVEL 4

// ANSI 颜色代码
#if 0
#define COLOR_RED     "\033[0;31m"
#define COLOR_YELLOW  "\033[0;33m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_CYAN    "\033[0;36m"
#define COLOR_RESET   "\033[0m"
#else
#define COLOR_RED     ""
#define COLOR_YELLOW  ""
#define COLOR_GREEN   ""
#define COLOR_CYAN    ""
#define COLOR_RESET   ""
#endif

// 日志标签定义 (4字符对齐)
#define TAG_NONE    ""
#define TAG_INIT    "[INIT]"
#define TAG_LED     "[LED ]"
#define TAG_FAN     "[FAN ]"
#define TAG_BTN     "[BTN ]"
#define TAG_SAFE    "[SAFE]"
#define TAG_TILT    "[TILT]"
#define TAG_SD      "[SD  ]"
#define TAG_OFFL    "[OFFL]"
#define TAG_PREV    "[PREV]"
#define TAG_STAT    "[STAT]"
#define TAG_DOOR    "[DOOR]"

// 回车换行
#define CR_LF      "\r\n"

// 获取毫秒时间戳 (从启动开始)
#define LOG_TIMESTAMP() (millis())

// 日志宏定义 (带颜色, 时间戳, 标签)
#if LOG_LEVEL >= 1
#define LOG_ERROR(tag, fmt, ...) grbl_sendf(CLIENT_SERIAL, COLOR_RED "[E][%6lu]%s " fmt COLOR_RESET CR_LF, LOG_TIMESTAMP() , tag, ##__VA_ARGS__)
#else
#define LOG_ERROR(tag, fmt, ...)
#endif

#if LOG_LEVEL >= 2
#define LOG_WARN(tag, fmt, ...) grbl_sendf(CLIENT_SERIAL, COLOR_YELLOW "[W][%6lu]%s " fmt COLOR_RESET CR_LF, LOG_TIMESTAMP(), tag, ##__VA_ARGS__)
#else
#define LOG_WARN(tag, fmt, ...)
#endif

#if LOG_LEVEL >= 3
#define LOG_INFO(tag, fmt, ...) grbl_sendf(CLIENT_SERIAL, COLOR_GREEN "[I][%6lu]%s " fmt COLOR_RESET CR_LF, LOG_TIMESTAMP(), tag, ##__VA_ARGS__)
#else
#define LOG_INFO(tag, fmt, ...)
#endif

#if LOG_LEVEL >= 4
#define LOG_DEBUG(tag, fmt, ...) grbl_sendf(CLIENT_SERIAL, COLOR_CYAN "[D][%6lu]%s " fmt COLOR_RESET CR_LF, LOG_TIMESTAMP(), tag, ##__VA_ARGS__)
#else
#define LOG_DEBUG(tag, fmt, ...)
#endif

// ============================================================================
// 枚举定义
// ============================================================================

// 按钮状态机
enum class ButtonState {
    IDLE,                    // 未按下
    PRESSED,                 // 已按下, 等待判断
    SHORT_PRESS_DETECTED,    // 检测到短按 (< 0.5s)
    LONG_PRESS_3S_DETECTED,  // 检测到 3 秒长按
    LONG_PRESS_10S_DETECTED  // 检测到 10 秒超长按
};

// LED 状态
enum class LEDState {
    ALARM,              // 警告状态 (Alarm)
    SAFETY_DOOR,        // 安全门/倾斜保护
    CYCLE,              // 雕刻状态 (呼吸灯)
    WIFI_STA_CONNECTING,// WiFi 连接中
    IDLE,               // 待机模式 (常亮)
};

// ============================================================================
// 全局变量
// ============================================================================

// 系统状态名称
static const char* sys_state_names[] = {
    "Idle",         // Must be zero.
    "Alarm",        // In alarm state. Locks out all g-code processes. Allows settings access.
    "CheckMode",    // G-code check mode. Locks out planner and motion only.
    "Homing",       // Performing homing cycle
    "Cycle",        // Cycle is running or motions are being executed.
    "Hold",         // Active feed hold
    "Jog",          // Jogging mode.
    "SafetyDoor",   // Safety door is ajar. Feed holds and de-energizes system.
    "Sleep",        // Sleep state.
};

// PWM 输出对象
static UserOutput::AnalogOutput* led_output = nullptr;
static UserOutput::AnalogOutput* fan_output = nullptr;

// QMI8658A 倾斜传感器对象
static QMI8658 imu;

// 循环任务时间
#define LOOP_TASK_PERIOD_MS (20)

// PWM 配置
#define PWM_FREQ_SHARED     (25000)     // 25kHz 共享频率 (超声波范围, 完全静音)
#define LED_ACTIVE_HIGH     (1)         // LED 极性配置 0=共阳极(低电平点亮), 1=共阴极(高电平点亮)
#define LED_PER_ON          (50)
#define LED_PER_OFF         (0)
#define FAN_PER_ON          (50)
#define FAN_PER_OFF         (0)

// ============================================================================
// QMI8658A 倾斜传感器初始化和读取
// ============================================================================

// 配置开关: 0 = 使用库实现, 1 = 使用直接寄存器操作
#define USE_QMI8658_REGISTER_DIRECT 0

#if USE_QMI8658_REGISTER_DIRECT
// ============================================================================
// 方案 A: 直接寄存器操作 (验证修复后的实现)
// ============================================================================

// QMI8658A I2C 地址
#define QMI8658A_ADDR       0x6A

// 寄存器地址定义
#define QMI8658_WHO_AM_I    0x00
#define QMI8658_CTRL1       0x02
#define QMI8658_CTRL2       0x03  // 加速度计配置
#define QMI8658_CTRL3       0x04  // 陀螺仪配置
#define QMI8658_CTRL7       0x08  // 传感器启用开关 ⚠️ 关键寄存器
#define QMI8658_AX_L        0x35
#define QMI8658_AX_H        0x36
#define QMI8658_AY_L        0x37
#define QMI8658_AY_H        0x38
#define QMI8658_AZ_L        0x39
#define QMI8658_AZ_H        0x3A

// 写入寄存器
bool writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(QMI8658A_ADDR);
    Wire.write(reg);
    Wire.write(value);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

// 读取单字节寄存器
bool readRegister_u8(uint8_t reg, uint8_t &value) {
    Wire.beginTransmission(QMI8658A_ADDR);
    Wire.write(reg);
    uint8_t error = Wire.endTransmission(false);  // Repeated START
    if (error != 0) return false;

    uint8_t received = Wire.requestFrom(QMI8658A_ADDR, 1);
    if (received != 1) return false;

    value = Wire.read();
    return true;
}

// 读取 16 位寄存器 (小端序: 先读 LSB, 再读 MSB)
bool readRegister_i16(uint8_t reg_lsb, int16_t &value) {
    Wire.beginTransmission(QMI8658A_ADDR);
    Wire.write(reg_lsb);
    uint8_t error = Wire.endTransmission(false);
    if (error != 0) return false;

    uint8_t received = Wire.requestFrom(QMI8658A_ADDR, 2);
    if (received != 2) return false;

    uint8_t lsb = Wire.read();  // 先读低字节
    uint8_t msb = Wire.read();  // 再读高字节
    value = (int16_t)((uint16_t)msb << 8 | lsb);
    return true;
}

// 初始化传感器 (修复后的正确实现)
bool init_tilt_sensor() {
    // 确保 Wire.begin() 只调用一次
    static bool wire_initialized = false;
    if (!wire_initialized) {
        Wire.begin(TILT_SENSOR_SDA_PIN, TILT_SENSOR_SCL_PIN);
        wire_initialized = true;
        LOG_INFO(TAG_TILT, "I2C initialized (SDA=%d, SCL=%d)", TILT_SENSOR_SDA_PIN, TILT_SENSOR_SCL_PIN);
    }

    // 1. 验证通信
    uint8_t whoami = 0;
    if (!readRegister_u8(QMI8658_WHO_AM_I, whoami)) {
        LOG_ERROR(TAG_TILT, "Failed to read WHO_AM_I");
        return false;
    }

    if (whoami != 0x05) {
        LOG_ERROR(TAG_TILT, "WHO_AM_I mismatch: 0x%02X (expected 0x05)", whoami);
        return false;
    }

    LOG_INFO(TAG_TILT, "WHO_AM_I = 0x%02X ✓", whoami);

    // 2. 初始化 CTRL1 (不是复位! )
    if (!writeRegister(QMI8658_CTRL1, 0x60)) {
        LOG_ERROR(TAG_TILT, "Failed to write CTRL1");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    // 3. 配置加速度计: ±8g (0x02), 1000Hz (0x03)
    // CTRL2 = (Range << 4) | ODR = (0x02 << 4) | 0x03 = 0x23
    if (!writeRegister(QMI8658_CTRL2, 0x23)) {
        LOG_ERROR(TAG_TILT, "Failed to write CTRL2");
        return false;
    }

    // 4. 关键修复: 启用加速度计 (写 CTRL7, 不是 CTRL1! )
    if (!writeRegister(QMI8658_CTRL7, 0x01)) {  // 0x01 = 启用加速度计
        LOG_ERROR(TAG_TILT, "Failed to enable accelerometer (CTRL7)");
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(50));  // 等待传感器稳定

    // 5. 验证配置
    uint8_t ctrl2 = 0, ctrl7 = 0;
    readRegister_u8(QMI8658_CTRL2, ctrl2);
    readRegister_u8(QMI8658_CTRL7, ctrl7);
    LOG_INFO(TAG_TILT, "CTRL2=0x%02X, CTRL7=0x%02X", ctrl2, ctrl7);

    // 6. 读取一次数据验证
    int16_t ax = 0, ay = 0, az = 0;
    readRegister_i16(QMI8658_AX_L, ax);
    readRegister_i16(QMI8658_AY_L, ay);
    readRegister_i16(QMI8658_AZ_L, az);

    LOG_INFO(TAG_TILT, "Initial raw data: AX=%d, AY=%d, AZ=%d", ax, ay, az);

    return true;
}

// 读取传感器, 返回是否倾斜
bool check_tilt_sensor(uint8_t debug_log) {
    int16_t raw_ax = 0, raw_ay = 0, raw_az = 0;

    if (!readRegister_i16(QMI8658_AX_L, raw_ax) ||
        !readRegister_i16(QMI8658_AY_L, raw_ay) ||
        !readRegister_i16(QMI8658_AZ_L, raw_az)) {
        if (debug_log) {
            LOG_ERROR(TAG_TILT, "Failed to read accel data");
        }
        return false;
    }

    // 转换为 mg (±8g 范围, LSB/g = 4096)
    float accelX = (raw_ax * 1000.0f) / 4096.0f;  // mg
    float accelY = (raw_ay * 1000.0f) / 4096.0f;
    float accelZ = (raw_az * 1000.0f) / 4096.0f;

    // 计算倾斜角度 (基于重力加速度分量)
    // 正常水平: Z轴≈1000mg, X/Y轴≈0mg
    float tilt_angle = atan2(sqrt(accelX * accelX + accelY * accelY), abs(accelZ)) * 180.0 / PI;

    bool is_tilted = (tilt_angle > 15.0);  // 15度阈值

    if (debug_log || is_tilted) {
        LOG_INFO(TAG_TILT, "%.1f° (X=%.1f Y=%.1f Z=%.1f mg) [Raw: %d %d %d]",
            tilt_angle, accelX, accelY, accelZ, raw_ax, raw_ay, raw_az);
    }

    return is_tilted;
}

#else
// ============================================================================
// 方案 B: 使用库实现 (当前稳定版本)
// ============================================================================

// 初始化传感器
bool init_tilt_sensor() {
    if (!imu.begin(TILT_SENSOR_SDA_PIN, TILT_SENSOR_SCL_PIN)) {
        LOG_ERROR(TAG_TILT, "QMI8658 init failed");
        return false;
    }

    imu.setAccelRange(QMI8658_ACCEL_RANGE_8G);
    imu.setAccelODR(QMI8658_ACCEL_ODR_1000HZ);
    imu.enableSensors(QMI8658_ENABLE_ACCEL);

    LOG_INFO(TAG_TILT, "QMI8658 initialized (SDA=%d, SCL=%d)", TILT_SENSOR_SDA_PIN, TILT_SENSOR_SCL_PIN);
    return true;
}

// 读取传感器, 返回是否倾斜
bool check_tilt_sensor(uint8_t debug_log) {
    static uint8_t consecutive_failures = 0;
    const uint8_t MAX_CONSECUTIVE_FAILURES = 5;
    const uint8_t SAMPLE_COUNT = 16;
    const float TILT_THRESHOLD_DEG = 30.0f;

    float sum_X = 0.0f, sum_Y = 0.0f, sum_Z = 0.0f;
    float min_X = 0.0f, max_X = 0.0f;
    float min_Y = 0.0f, max_Y = 0.0f;
    float min_Z = 0.0f, max_Z = 0.0f;
    uint8_t valid_samples = 0;

    for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
        QMI8658_Data data;
        if (imu.readSensorData(data)) {
            sum_X += data.accelX;
            sum_Y += data.accelY;
            sum_Z += data.accelZ;

            if (valid_samples == 0) {
                min_X = max_X = data.accelX;
                min_Y = max_Y = data.accelY;
                min_Z = max_Z = data.accelZ;
            } else {
                if (data.accelX < min_X) min_X = data.accelX;
                if (data.accelX > max_X) max_X = data.accelX;
                if (data.accelY < min_Y) min_Y = data.accelY;
                if (data.accelY > max_Y) max_Y = data.accelY;
                if (data.accelZ < min_Z) min_Z = data.accelZ;
                if (data.accelZ > max_Z) max_Z = data.accelZ;
            }
            valid_samples++;
        }
    }

    if (valid_samples < 4) {
        LOG_ERROR(TAG_TILT, "sensor valid samples : %d", valid_samples);
        consecutive_failures++;
        if (consecutive_failures >= MAX_CONSECUTIVE_FAILURES) {
            LOG_INFO(TAG_TILT, "Consecutive failures: %d, attempting re-initialization", consecutive_failures);
            if (init_tilt_sensor()) {
                LOG_INFO(TAG_TILT, "Re-initialization succeeded");
            } else {
                LOG_ERROR(TAG_TILT, "Re-initialization failed, will retry on next failure");
            }
        }
        return false;
    }

    consecutive_failures = 0;

    float avg_X = (sum_X - min_X - max_X) / (valid_samples - 2);
    float avg_Y = (sum_Y - min_Y - max_Y) / (valid_samples - 2);
    float avg_Z = (sum_Z - min_Z - max_Z) / (valid_samples - 2);

    // 归一化重力向量
    float magnitude = sqrt(avg_X * avg_X + avg_Y * avg_Y + avg_Z * avg_Z);
    if (magnitude < 500.0f) {
        LOG_ERROR(TAG_TILT, "Low magnitude: %.1f mg", magnitude);
        return false;
    }

    float norm_X = avg_X / magnitude * 1000.0f;
    float norm_Y = avg_Y / magnitude * 1000.0f;
    float norm_Z = avg_Z / magnitude * 1000.0f;

    // 计算倾斜角度
    float tilt_angle = atan2(sqrt(norm_X * norm_X + norm_Y * norm_Y), abs(norm_Z)) * 180.0 / PI;
    bool is_tilted = (tilt_angle > TILT_THRESHOLD_DEG);

    if (debug_log || is_tilted) {
        LOG_INFO(TAG_TILT, "%.1f° (Norm: X=%.0f Y=%.0f Z=%.0f mg, Samples=%d/%d)",
            tilt_angle, norm_X, norm_Y, norm_Z, valid_samples, SAMPLE_COUNT);
    }

    return is_tilted;
}

#endif  // USE_QMI8658_REGISTER_DIRECT

// ============================================================================
// 离线雕刻和预览边界
// ============================================================================

// 启动离线雕刻 (从 SD 卡执行 001.nc 文件)
void start_offline_engraving() {
    SDState sd_state = get_sd_state(true);  // 强制实际挂载
    if (sd_state != SDState::Idle) {
        LOG_ERROR(TAG_SD, "SD card not ready (state=%d)", (int)sd_state);
        return;
    }

    if (!SD.exists("/001.nc")) {
        LOG_ERROR(TAG_SD, "File 001.nc not found! Listing SD root directory:");
        File root = SD.open("/");
        if (root) {
            File file = root.openNextFile();
            int file_count = 0;
            while (file) {
                if (!file.isDirectory()) {
                    LOG_INFO(TAG_SD, "  - %s (%d bytes)", file.name(), file.size());
                    file_count++;
                }
                file = root.openNextFile();
            }
            if (file_count == 0) {
                LOG_INFO(TAG_SD, "  (no files found)");
            }
        } else {
            LOG_ERROR(TAG_SD, "Failed to open root directory");
        }
        return;
    }

    LOG_INFO(TAG_OFFL, "Wait Homing complete, starting: 001.nc");
    WebUI::inputBuffer.push("$H\r");
    WebUI::inputBuffer.push("$SD/Run=/001.nc\r");
}

// 归位后, 预览边界 (激光最小功率指示)
void preview_boundary() {
    float x_max = DEFAULT_X_MAX_TRAVEL;
    float y_max = DEFAULT_Y_MAX_TRAVEL;
    char gcode[100];

    LOG_INFO(TAG_PREV, "Wait Homing complete, Boundary: %.1f x %.1f mm (laser preview)", x_max, y_max);

    WebUI::inputBuffer.push("$H\r");
    WebUI::inputBuffer.push("G90G0X0Y0\r");
    WebUI::inputBuffer.push("M4S1\r");  // 打开激光, 最小功率 (S1 = 0.1%)

    sprintf(gcode, "G1X%.1fY0F3000\r", x_max);
    WebUI::inputBuffer.push(gcode);
    sprintf(gcode, "G1X%.1fY%.1f\r", x_max, y_max);
    WebUI::inputBuffer.push(gcode);
    sprintf(gcode, "G1X0Y%.1f\r", y_max);
    WebUI::inputBuffer.push(gcode);
    WebUI::inputBuffer.push("G1X0Y0\r");

    WebUI::inputBuffer.push("M5\r");  // 关闭激光
}

// ============================================================================
// 按钮处理函数
// ============================================================================

// | 位 | 标志名         | 作用                          | 触发方式                     |
// |----|----------------|-------------------------------|------------------------------|
// | 0  | statusReport   | 请求发送状态报告 (? 命令)     | 串口/Web 接收 ?              |
// | 1  | cycleStart     | 启动运动循环 (~ 命令)         | 串口/Web 接收 ~ 或按钮       |
// | 2  | cycleStop      | 停止循环 (已弃用)             | 不再使用                     |
// | 3  | feedHold       | 进给保持/暂停 (! 命令)        | 串口/Web 接收 ! 或按钮       |
// | 4  | reset          | 系统复位 (Ctrl+X)             | 串口接收 0x18 或复位按钮     |
// | 5  | safetyDoor     | 安全门触发                    | 安全门开关中断               |
// | 6  | motionCancel   | 取消当前运动                  | 探针失败或 Jog 取消          |
// | 7  | sleep          | 进入睡眠模式 ($SLP 命令)      | 串口/Web 接收 $SLP           |

// 按钮短按处理函数
void handle_short_press() {
    LOG_INFO(TAG_BTN, "Short press");

    switch (sys.state) {
        case State::Idle:
            preview_boundary();
            break;

        case State::Cycle:
            LOG_INFO(TAG_BTN, "Pause");
            sys_rt_exec_state.bit.feedHold = true;
            break;

        case State::Hold:
            LOG_INFO(TAG_BTN, "Resume");
            sys_rt_exec_state.bit.cycleStart = true;
            break;

        case State::Alarm:
            LOG_INFO(TAG_BTN, "In ALARM, send $H");
            WebUI::inputBuffer.push("$H\r");
            break;

        case State::Homing:
            break;

        case State::Jog:
            LOG_INFO(TAG_BTN, "Stop jog");
            sys_rt_exec_state.bit.feedHold = true;
            break;

        case State::CheckMode:
            LOG_INFO(TAG_BTN, "Exit check mode");
            WebUI::inputBuffer.push("?\r");
            break;

        case State::SafetyDoor:
            LOG_INFO(TAG_BTN, "Safety door is open, please close it");
            WebUI::inputBuffer.push("?\r");
            break;

        case State::Sleep:
            LOG_INFO(TAG_BTN, "Short press during Sleep");
            WebUI::inputBuffer.push("?\r");
            break;

        default:
            LOG_INFO(TAG_BTN, "Unknown state %d", sys.state);
            break;
    }
}

// 按钮长按处理函数
void handle_long_press_3s() {
    LOG_INFO(TAG_BTN, "Long press 3s");

    switch (sys.state) {
        case State::Idle:
            start_offline_engraving();
            break;

        case State::Cycle:
        case State::Hold:
            LOG_INFO(TAG_BTN, "Stop");
            sys_rt_exec_state.bit.reset = true;
            break;

        case State::Alarm:
        case State::Homing:
        case State::Jog:
        case State::CheckMode:
        case State::SafetyDoor:
        case State::Sleep:
        default:
            break;
    }
}

// 按钮超长按处理函数
void handle_long_press_10s() {
    LOG_INFO(TAG_BTN, "Long press 10s - Reset WiFi to AP");
    WebUI::inputBuffer.push("[ESP410]AP\r");
    vTaskDelay(pdMS_TO_TICKS(1000));
    WebUI::inputBuffer.push("[ESP444]RESTART\r");
}

// ============================================================================
// 按钮监控任务
// ============================================================================

void button_monitor_task(void* parameter) {
    const uint32_t SHORT_PRESS_MAX = 500;
    const uint32_t LONG_PRESS_3S_THRESHOLD = 3000;
    const uint32_t LONG_PRESS_10S_THRESHOLD = 10000;

    static ButtonState button_state = ButtonState::IDLE;
    static uint32_t button_press_time = 0;
    static bool button_action_executed = false;
    static bool wifi_ready = false;

    while (true) {
        if (!wifi_ready) {
            wifi_mode_t mode = WiFi.getMode();
            if (mode == WIFI_AP || ((mode == WIFI_STA || mode == WIFI_AP_STA) && WiFi.status() == WL_CONNECTED)) {
                wifi_ready = true;

                if (sys.state == State::Alarm) {
                    bool door_open = (DEFAULT_SAFETY_DOOR_ENABLED == 1) && (digitalRead(SAFETY_DOOR_PIN) == LOW);
                    bool is_tilted = (DEFAULT_TILT_SENSOR_ENABLED == 1) && check_tilt_sensor(false);

                    if (door_open) {
                        LOG_WARN(TAG_BTN, "Skip auto-homing: safety door is open");
                    } else if (is_tilted) {
                        LOG_WARN(TAG_BTN, "Skip auto-homing: tilt sensor triggered");
                    } else {
                        LOG_INFO(TAG_BTN, "Auto-homing triggered (state=Alarm)");
                        WebUI::inputBuffer.push("$H\r");
                    }
                } else {
                    LOG_INFO(TAG_BTN, "Skip auto-homing: state=%d (not Alarm)", (int)sys.state);
                }
            }
        }

        bool button_pressed = (digitalRead(USER_BUTTON_PIN) == LOW);
        uint32_t now = millis();
        uint32_t duration = now - button_press_time;

        switch (button_state) {
            case ButtonState::IDLE:
                if (button_pressed) {
                    button_state = ButtonState::PRESSED;
                    button_press_time = now;
                    button_action_executed = false;
                }
                break;

            case ButtonState::PRESSED:
                if (!button_pressed) {
                    if (duration < SHORT_PRESS_MAX) {
                        button_state = ButtonState::SHORT_PRESS_DETECTED;
                    } else if (duration >= LONG_PRESS_3S_THRESHOLD && duration < LONG_PRESS_10S_THRESHOLD) {
                        button_state = ButtonState::LONG_PRESS_3S_DETECTED;
                    } else {
                        button_state = ButtonState::IDLE;
                    }
                } else {
                    if (duration >= LONG_PRESS_10S_THRESHOLD && !button_action_executed) {
                        button_state = ButtonState::LONG_PRESS_10S_DETECTED;
                    }
                }
                break;

            case ButtonState::SHORT_PRESS_DETECTED:
                if (wifi_ready) {
                    handle_short_press();
                }
                button_state = ButtonState::IDLE;
                break;

            case ButtonState::LONG_PRESS_3S_DETECTED:
                if (wifi_ready) {
                    handle_long_press_3s();
                }
                button_state = ButtonState::IDLE;
                break;

            case ButtonState::LONG_PRESS_10S_DETECTED:
                handle_long_press_10s();
                button_action_executed = true;
                if (!button_pressed) {
                    button_state = ButtonState::IDLE;
                }
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(LOOP_TASK_PERIOD_MS));
    }
}

// ============================================================================
// LED 和风扇 PWM 辅助函数
// ============================================================================

// LED 亮度设置 (0-100%, 自动处理极性反转)
void set_led_brightness(uint8_t percent) {
    if (!led_output) return;

    if (percent > 100) percent = 100;

    uint32_t max_duty = led_output->denominator() - 1;
    uint32_t duty = ((uint32_t)percent * max_duty) / 100;

    if (!LED_ACTIVE_HIGH) {
        duty = max_duty - duty;
    }

    led_output->set_level(duty);
}

// 风扇速度设置 (0-100%)
void set_fan_speed(uint8_t percent) {
    if (!fan_output) return;

    if (percent > 100) percent = 100;

    uint32_t max_duty = fan_output->denominator() - 1;
    uint32_t duty = ((uint32_t)percent * max_duty) / 100;

    fan_output->set_level(duty);
}

// ============================================================================
// LED 和风扇控制模块
// ============================================================================

// 三角波呼吸灯 (Gamma 校正, 符合人眼感知)
void set_led_breath() {
    if (!led_output) {
        return;
    }

    uint32_t cycle_time = 2000;
    uint32_t phase = millis() % cycle_time;

    // 计算线性亮度值 (0.0 - 1.0)
    float linear_brightness;
    if (phase < cycle_time / 2) {
        linear_brightness = (float)phase / (cycle_time / 2);
    } else {
        linear_brightness = 1.0f - (float)(phase - cycle_time / 2) / (cycle_time / 2);
    }

    // Gamma 校正 (Gamma = 2.5, 适合 LED 呼吸灯)
    // 公式: perceived_brightness = linear_brightness ^ gamma
    float gamma_corrected = pow(linear_brightness, 2.5f);
    uint8_t brightness_percent = (uint8_t)(gamma_corrected * 100.0f);

    set_led_brightness(brightness_percent);
}

// 多段式闪烁模式 (状态机实现, 确保每个时间段都被完整执行)
// pattern 格式: {时长1, 亮度1, 时长2, 亮度2, ...}
// 例如: {50, 0, 1950, 50} 表示灭50ms, 亮1950ms
void set_led_pattern(const uint32_t* pattern, uint8_t pattern_length) {
    if (!led_output || pattern_length < 2 || pattern_length % 2 != 0) {
        return;
    }

    static const uint32_t* current_pattern = nullptr;
    static uint8_t current_pattern_length = 0;
    static uint8_t current_segment_index = 0;
    static uint32_t segment_start_time = 0;

    // 检测模式切换
    if (pattern != current_pattern || pattern_length != current_pattern_length) {
        current_pattern = pattern;
        current_pattern_length = pattern_length;
        current_segment_index = 0;
        segment_start_time = millis();
    }

    // 检查是否需要切换到下一个时间段
    uint32_t now = millis();
    uint32_t elapsed = now - segment_start_time;
    uint32_t segment_duration = pattern[current_segment_index];

    if (elapsed >= segment_duration) {
        // 切换到下一个时间段
        current_segment_index += 2;
        if (current_segment_index >= pattern_length) {
            current_segment_index = 0;  // 循环到第一个时间段
        }
        segment_start_time = now;
    }

    // 设置当前时间段的亮度
    uint8_t brightness = pattern[current_segment_index + 1];
    set_led_brightness(brightness);
}

// LED 自动控制逻辑
void update_led_auto_control() {
    // LED 闪烁模式定义 (函数内部 static const, 遵循最小作用域)
    // ALARM: 闪烁1次 (灭50ms, 亮1950ms, 总周期2000ms)
    static const uint32_t LED_PATTERN_ALARM[] = {100, LED_PER_OFF, 1900, LED_PER_ON};
    static const uint8_t LED_PATTERN_ALARM_LEN = sizeof(LED_PATTERN_ALARM) / sizeof(uint32_t);

    // SAFETY_DOOR: 闪烁2次 (灭50ms, 亮50ms, 灭50ms, 亮1850ms, 总周期2000ms)
    static const uint32_t LED_PATTERN_SAFETY_DOOR[] = {100, LED_PER_OFF, 100, LED_PER_ON, 100, LED_PER_OFF, 1700, LED_PER_ON};
    static const uint8_t LED_PATTERN_SAFETY_DOOR_LEN = sizeof(LED_PATTERN_SAFETY_DOOR) / sizeof(uint32_t);

    // WIFI_STA_CONNECTING: 等间隔闪烁 (灭1000ms, 亮1000ms, 总周期2000ms)
    static const uint32_t LED_PATTERN_WIFI_CONNECTING[] = {500, LED_PER_OFF, 500, LED_PER_ON};
    static const uint8_t LED_PATTERN_WIFI_CONNECTING_LEN = sizeof(LED_PATTERN_WIFI_CONNECTING) / sizeof(uint32_t);

    static State current_sys_state = State::Idle;
    LEDState new_state = LEDState::ALARM;

    switch (sys.state) {
        case State::Alarm:
            new_state = LEDState::ALARM;
            break;

        case State::SafetyDoor:
            new_state = LEDState::SAFETY_DOOR;
            break;

        case State::Cycle:
            new_state = LEDState::CYCLE;
            break;

        case State::Idle:
        case State::Hold:
        case State::Homing:
        case State::Jog:
        case State::CheckMode:
        case State::Sleep:
            new_state = LEDState::IDLE;
            break;
    }

    // 后置判断: WiFi 连接状态优先级最高 (覆盖所有状态)
    wifi_mode_t wifi_mode = WiFi.getMode();
    if (wifi_mode == WIFI_STA || wifi_mode == WIFI_AP_STA) {
        if (WiFi.status() != WL_CONNECTED) {
            new_state = LEDState::WIFI_STA_CONNECTING;
        }
    }

    if (current_sys_state != sys.state) {
        LOG_INFO(TAG_LED, "State: %s -> %s", sys_state_names[(int)current_sys_state], sys_state_names[(int)sys.state]);
        current_sys_state = sys.state;
    }

    switch (new_state) {
        case LEDState::ALARM:
            set_led_pattern(LED_PATTERN_ALARM, LED_PATTERN_ALARM_LEN);
            break;
        case LEDState::SAFETY_DOOR:
            set_led_pattern(LED_PATTERN_SAFETY_DOOR, LED_PATTERN_SAFETY_DOOR_LEN);
            break;
        case LEDState::WIFI_STA_CONNECTING:
            set_led_pattern(LED_PATTERN_WIFI_CONNECTING, LED_PATTERN_WIFI_CONNECTING_LEN);
            break;
        case LEDState::CYCLE:
            set_led_breath();
            break;
        case LEDState::IDLE:
            set_led_brightness(LED_PER_ON);
            break;
    }
}

// 风扇自动控制逻辑
void update_fan_auto_control() {
    if (!fan_output) {
        return;
    }

    static uint32_t last_active_time = 0;
    static bool was_active = false;
    static bool last_fan_on = true;

    uint32_t now = millis();
    const uint32_t COOLDOWN_TIME_MS = 5 * 60 * 1000;

    bool is_active = false;
    if (spindle && spindle->get_state() != SpindleState::Disable) {
        is_active = true;
    }
    if (sys.state == State::Cycle || sys.state == State::Jog) {
        is_active = true;
    }
    if (was_active && !is_active) {
        last_active_time = now;
    }
    was_active = is_active;

    bool should_fan_on;
    if (is_active) {
        should_fan_on = true;
    } else {
        uint32_t idle_duration = (uint32_t)(now - last_active_time);
        should_fan_on = (idle_duration < COOLDOWN_TIME_MS);
    }

    if (should_fan_on != last_fan_on) {
        if (should_fan_on) {
            LOG_INFO(TAG_FAN, "Fan ON");
        } else {
            LOG_INFO(TAG_FAN, "Fan OFF (cooldown complete)");
        }
        last_fan_on = should_fan_on;
    }

    set_fan_speed(should_fan_on ? FAN_PER_ON : FAN_PER_OFF);
}

// LED 和风扇控制任务 (包含内存监控)
void led_fan_control_task(void* parameter) {
    // 内存监控配置
    static uint32_t last_mem_log_time = 0;
    const uint32_t MEM_LOG_INTERVAL_MS = 10000;  // 每 10 秒打印一次

    while (true) {
        update_led_auto_control();

        update_fan_auto_control();

        // 内存监控 (定期打印)
        uint32_t now = millis();
        if (now - last_mem_log_time >= MEM_LOG_INTERVAL_MS) {
            uint32_t free_heap = ESP.getFreeHeap();
            uint32_t min_free_heap = ESP.getMinFreeHeap();
            uint32_t max_alloc_heap = ESP.getMaxAllocHeap();

            LOG_DEBUG(TAG_STAT, "Memory: Free=%u, Min=%u, MaxAlloc=%u bytes",
                free_heap, min_free_heap, max_alloc_heap);

            last_mem_log_time = now;
        }

        vTaskDelay(pdMS_TO_TICKS(LOOP_TASK_PERIOD_MS));
    }
}

// ============================================================================
// 安全监控任务
// ============================================================================

void safety_monitor_task(void* parameter) {
    LOG_INFO(TAG_SAFE, "DEFAULT_SAFETY_DOOR_ENABLED: %d", DEFAULT_SAFETY_DOOR_ENABLED);
    LOG_INFO(TAG_SAFE, "DEFAULT_TILT_SENSOR_ENABLED: %d", DEFAULT_TILT_SENSOR_ENABLED);

    bool last_door_state = false;
    bool last_tilt_state = false;
    State state_before_safety_door = State::Idle;

    while (true) {
        bool door_open_now = false;
        if (DEFAULT_SAFETY_DOOR_ENABLED == 1) {
            door_open_now = (digitalRead(SAFETY_DOOR_PIN) == LOW);
        }
        if (door_open_now != last_door_state) {
            LOG_INFO(TAG_DOOR, "Door %s (pin: %d)",
                door_open_now ? "opened" : "closed",
                digitalRead(SAFETY_DOOR_PIN));
            last_door_state = door_open_now;
        }

        bool tilt_now = false;
        if (DEFAULT_TILT_SENSOR_ENABLED == 1) {
            tilt_now = check_tilt_sensor(false);
        }
        if (tilt_now != last_tilt_state) {
            LOG_INFO(TAG_TILT, "Tilt %s", tilt_now ? "detected" : "cleared");
            last_tilt_state = tilt_now;
        }

        if (door_open_now || tilt_now) {
            switch (sys.state) {
                case State::Idle:
                case State::CheckMode:
                case State::Homing:
                case State::Cycle:
                case State::Hold:
                case State::Jog:
                    LOG_INFO(TAG_SAFE, "Safety door opened (was in %s state)", sys_state_names[(int)sys.state]);
                    state_before_safety_door = sys.state;
                    sys_rt_exec_state.bit.safetyDoor = true;
                    break;

                case State::Alarm:
                case State::SafetyDoor:
                case State::Sleep:
                    break;
            }
        } else {
            switch (sys.state) {
                case State::SafetyDoor:
                    if (!sys.suspend.bit.safetyDoorAjar) {
                        LOG_INFO(TAG_SAFE, "Safety door closed (was in %s before)", sys_state_names[(int)state_before_safety_door]);
                        sys.suspend.bit.safetyDoorAjar = false;

                        if (state_before_safety_door == State::Hold) {
                            sys.suspend.value = 0;
                            sys.suspend.bit.holdComplete = true;
                            sys.state = State::Hold;
                            LOG_INFO(TAG_SAFE, "Returned to Hold state (was paused before door opened)");
                        } else if (state_before_safety_door == State::Cycle || state_before_safety_door == State::Jog) {
                            sys.suspend.value = 0;
                            sys.suspend.bit.holdComplete = true;
                            sys.state = State::Hold;
                            LOG_INFO(TAG_SAFE, "Entered Hold state (was moving before, press button to resume)");
                        } else {
                            sys.suspend.value = 0;
                            sys.state = state_before_safety_door;
                            LOG_INFO(TAG_SAFE, "Returned to %s state", sys_state_names[(int)state_before_safety_door]);
                        }
                        state_before_safety_door = State::Idle;
                    }
                    break;

                case State::Idle:
                case State::Alarm:
                case State::CheckMode:
                case State::Homing:
                case State::Cycle:
                case State::Hold:
                case State::Jog:
                case State::Sleep:
                    break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(LOOP_TASK_PERIOD_MS));
    }
}

// ============================================================================
// 机器初始化
// ============================================================================

void machine_init() {
    LOG_INFO(TAG_INIT, "Wainlux K11 HIM Custom Init (Full Rewrite)");
    LOG_INFO(TAG_INIT, "Compiled: %s at %s", __DATE__, __TIME__);
    LOG_INFO(TAG_INIT, "ESP32 SDK: %s", ESP.getSdkVersion());

    // 初始化 GPIO
    pinMode(USER_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SAFETY_DOOR_PIN, INPUT_PULLUP);
    LOG_INFO(TAG_INIT, "%s(%d) | %s(%d)", TAG_BTN, USER_BUTTON_PIN, TAG_DOOR, SAFETY_DOOR_PIN);

    // 初始化 PWM (使用 UserOutput::AnalogOutput)
    led_output = new UserOutput::AnalogOutput(0, STATUS_LED_PIN, PWM_FREQ_SHARED);
    set_led_brightness(LED_PER_ON);
    fan_output = new UserOutput::AnalogOutput(1, COOLING_FAN_PIN, PWM_FREQ_SHARED);
    set_fan_speed(FAN_PER_ON);

    // 初始化倾斜传感器
    (void)init_tilt_sensor();
    (void)check_tilt_sensor(true);

    // 创建任务 (固定到核心 1)
    xTaskCreatePinnedToCore(button_monitor_task, "Button", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(safety_monitor_task, "Safety", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(led_fan_control_task, "LED+Fan", 4096, NULL, 1, NULL, 1);

    LOG_INFO(TAG_INIT, "Custom tasks created: Button, Safety, LED+Fan");
}

// ============================================================================
// M30 程序结束回调
// ============================================================================

void user_m30() {
    LOG_INFO(TAG_NONE, "Program complete");
}
