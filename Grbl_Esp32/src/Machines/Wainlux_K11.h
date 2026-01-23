#pragma once
// clang-format off

/*
    3axis_v4.h
    Part of Grbl_ESP32

    Pin assignments for the ESP32 Development Controller, v4.1 and later.
    https://github.com/bdring/Grbl_ESP32_Development_Controller
    https://www.tindie.com/products/33366583/grbl_esp32-cnc-development-board-v35/

    2018    - Bart Dring
    2020    - Mitch Bradley

    Grbl_ESP32 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Grbl is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Grbl_ESP32.  If not, see <http://www.gnu.org/licenses/>.
*/
// #include "./src/qmi8658.h"
#define MACHINE_NAME            "Genmitsu_Kiosk_V07"
#define MACHINE_NAME_C          "Genmitsu_Kiosk_C_V07"
// #define CUSTOM_CODE_FILENAME "Custom/wainlux_k11.cpp"

#define X_STEP_PIN              GPIO_NUM_33
#define X_DIRECTION_PIN         GPIO_NUM_32
#define Y_STEP_PIN              GPIO_NUM_26
#define Y_DIRECTION_PIN         GPIO_NUM_25
#define Z_STEP_PIN              GPIO_NUM_5
#define Z_DIRECTION_PIN         GPIO_NUM_18
#define X_LIMIT_PIN             GPIO_NUM_36
#define Y_LIMIT_PIN             GPIO_NUM_35
#define X_DISABLE_PIN           GPIO_NUM_27
#define Y_DISABLE_PIN           GPIO_NUM_27
#define Z_DISABLE_PIN           GPIO_NUM_27

#define An_JIAN                 GPIO_NUM_19     // 按键
#define Feng_Shan               GPIO_NUM_16     // 风扇
#define MEN_KAI_GUAN            GPIO_NUM_17     // 门开关
#define LED_B_PIN               GPIO_NUM_23     // LED

#define SPINDLE_TYPE            SpindleType::LASER // only one spindle at a time
#define LASER_OUTPUT_PIN        GPIO_NUM_21



//原始K11
#define DEFAULT_STEP_PULSE_MICROSECONDS 3   // $0
#define DEFAULT_STEPPER_IDLE_LOCK_TIME 25   // 25ms $1
#define DEFAULT_STEPPING_INVERT_MASK 0      // uint8_t $2
#define DEFAULT_DIRECTION_INVERT_MASK 4     // uint8_t $3
#define DEFAULT_INVERT_ST_ENABLE 0          // boolean $4
#define DEFAULT_INVERT_LIMIT_PINS 0         //$5
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

#define MEN_KAI_GUAN_SET                    // 门开关设置选项有效，不需要可以注释掉，
#define DEFAULT_MEN_KAI_GUAN    1           // 门开关默认是否有效 ==0无效 ==1有效
#define DEFAULT_QING_JIAO       1
#define scl_pin GPIO_NUM_4                  // 倾角传感器IO口
#define sda_pin GPIO_NUM_22

#define ENABLE_SOFTWARE_DEBOUNCE            // 限位开关防抖

void machine_init();






