# Settings 配置系统设计详解

## 概述

Grbl_ESP32 的配置系统基于一个优雅的**自注册链表模式**，通过面向对象设计实现了类型安全的配置管理和灵活的运行时访问。

## 类继承关系

```
Word (基类)
  └── Setting (抽象基类)
        ├── IntSetting (整数配置)
        ├── FloatSetting (浮点数配置)
        ├── StringSetting (字符串配置)
        ├── EnumSetting (枚举配置)
        ├── FlagSetting (布尔配置)
        ├── AxisMaskSetting (轴掩码配置)
        └── IPaddrSetting (IP地址配置)
```

## 核心机制：自动注册模式

### 1. 自动注册到全局链表

每个 `Setting` 对象在构造时会自动注册到全局链表 `Setting::List`：

```cpp
// Settings.cpp:34-39
Setting::Setting(...) : Word(...), _checker(checker) {
    link = List;      // 保存当前链表头到 link
    List = this;      // 将自己设为新的链表头
    // ... NVS key 生成逻辑
}
```

**关键特性：**
- 头插法：新创建的对象成为链表头
- 自动管理：无需手动维护设置列表
- 统一遍历：通过 `Setting::List` 可访问所有设置

### 2. 链表遍历

```cpp
// 遍历所有设置项
for (Setting* s = Setting::List; s; s = s->next()) {
    const char* name = s->getName();
    const char* value = s->getStringValue();
    // 处理每个设置...
}
```

## IntSetting 详解

### 构造函数

```cpp
IntSetting::IntSetting(
    const char*   description,      // 设置项描述（显示给用户）
    type_t        type,              // 类型：GRBL/EXTENDED/WEBSET
    permissions_t permissions,       // 权限：WG/WU/WA
    const char*   grblName,          // GRBL 兼容名称（如 "$0"）
    const char*   name,              // 完整名称（如 "Stepper/Pulse"）
    int32_t       defVal,            // 默认值
    int32_t       minVal,            // 最小值
    int32_t       maxVal,            // 最大值
    bool (*checker)(char*) = NULL,   // 可选的验证函数
    bool currentIsNvm = false        // 当前值是否从 NVM 读取
);
```

### 三个值的状态管理

```cpp
class IntSetting : public Setting {
private:
    int32_t _defaultValue;   // 默认值（编译时固定）
    int32_t _currentValue;   // 当前运行时值
    int32_t _storedValue;    // NVS 存储的值
    int32_t _minValue;       // 最小值
    int32_t _maxValue;       // 最大值
    bool    _currentIsNvm;   // 当前值是否直接从 NVM 读取
};
```

**值的生命周期：**

1. **启动时**：`load()` 从 NVS 读取 → `_storedValue` → `_currentValue`
2. **运行时**：用户修改 → `setStringValue()` → 更新 `_currentValue` 和 `_storedValue`
3. **重启后**：从 NVS 恢复 `_storedValue`

### 实际使用示例

#### 定义全局指针

```cpp
// SettingsDefinitions.h
extern IntSetting* pulse_microseconds;
extern IntSetting* stepper_idle_lock_time;
```

#### 创建设置对象

```cpp
// SettingsDefinitions.cpp
void make_settings() {
    // 创建设置对象（自动注册到全局链表）
    pulse_microseconds = new IntSetting(
        GRBL,                              // 类型：GRBL 标准设置
        WG,                                // 权限：Guest 可读写
        "0",                               // GRBL 名称："$0"
        "Stepper/Pulse",                   // 完整名称（层级结构）
        DEFAULT_STEP_PULSE_MICROSECONDS,   // 默认值
        3,                                 // 最小值
        1000                               // 最大值
    );

    stepper_idle_lock_time = new IntSetting(
        GRBL, WG, "1", "Stepper/IdleTime",
        DEFAULT_STEPPER_IDLE_LOCK_TIME, 0, 255
    );
}
```

#### 在代码中使用

```cpp
// 获取当前值
int32_t pulse = pulse_microseconds->get();

// 设置新值（通过字符串）
Error err = pulse_microseconds->setStringValue("5");
if (err == Error::Ok) {
    // 设置成功
}
```

## 为什么使用 `new`？

### 原因 1：动态注册到链表
- 构造函数会修改全局链表 `Setting::List`
- 必须在运行时动态创建，不能是栈对象

### 原因 2：生命周期管理
- 这些设置对象需要在整个程序运行期间存在
- 使用 `new` 分配在堆上，直到程序结束

### 原因 3：多态访问
- 通过 `Setting*` 基类指针遍历链表
- 可以统一处理 `IntSetting`、`FloatSetting`、`StringSetting` 等

## 完整的数据流

```
编译时：
  DEFAULT_STEP_PULSE_MICROSECONDS (Config.h)
         ↓
  new IntSetting(..., DEFAULT_VALUE, ...)
         ↓
  _defaultValue = DEFAULT_VALUE
  _currentValue = DEFAULT_VALUE

启动时：
  settings_init()
         ↓
  pulse_microseconds->load()
         ↓
  从 NVS 读取 → _storedValue
         ↓
  _currentValue = _storedValue (如果存在)

运行时：
  用户输入 "$0=5"
         ↓
  pulse_microseconds->setStringValue("5")
         ↓
  验证范围 (3 ≤ 5 ≤ 1000) ✓
         ↓
  _currentValue = 5
  nvs_set_i32(..., 5)
  _storedValue = 5

使用时：
  int pulse = pulse_microseconds->get()
         ↓
  返回 _currentValue (5)
```

## 系统初始化流程

### 1. 创建所有设置对象

```cpp
// Grbl.cpp
void grbl_init() {
    make_settings();  // 创建所有设置对象
    // ...
}
```

### 2. 从 NVS 加载设置

```cpp
void settings_init() {
    Setting::init();  // 打开 NVS 句柄

    // 遍历所有设置，从 NVS 加载
    for (Setting* s = Setting::List; s; s = s->next()) {
        s->load();
    }
}
```

### 3. 运行时访问

```cpp
// 通过全局指针直接访问
int32_t pulse = pulse_microseconds->get();

// 或通过链表查找（较慢，用于用户命令）
Setting* find_setting(const char* name) {
    for (Setting* s = Setting::List; s; s = s->next()) {
        if (strcmp(s->getName(), name) == 0) {
            return s;
        }
    }
    return nullptr;
}
```

## 用户访问方式详解

### 三种访问方式对比

根据 `grblName` 参数的不同，设置项有三种访问方式：

| grblName 值 | 访问方式 | 示例 | 说明 |
|------------|---------|------|------|
| `"$数字"` (如 `"0"`, `"100"`) | `$数字=值` | `$100=80` | GRBL 标准命令，兼容原始 Grbl |
| `"ESP数字"` (如 `"ESP100"`) | `[ESP数字]值` | `[ESP100]MySSID` | ESP3D WebUI 命令格式 |
| `NULL` | `完整名称=值` | `Sta/RetryCount=5` | 仅通过完整名称访问 |

### 实际示例解析

```cpp
// 示例 1: 有 ESP 命令编号（可通过 [ESP100] 访问）
wifi_sta_ssid = new StringSetting(
    "Station SSID",
    WEBSET,
    WA,
    "ESP100",              // ← grblName = "ESP100"
    "Sta/SSID",            // ← fullName = "Sta/SSID"
    DEFAULT_STA_SSID,
    MIN_SSID_LENGTH,
    MAX_SSID_LENGTH,
    (bool (*)(char*))WiFiConfig::isSSIDValid
);

// 用户可以使用以下方式访问：
// 方式 1: [ESP100]MyWiFi          （推荐，ESP3D 标准格式）
// 方式 2: Sta/SSID=MyWiFi          （通过完整名称）

// 示例 2: 无 ESP 命令编号（只能通过完整名称访问）
wifi_sta_retry_count = new IntSetting(
    "Station Retry Count",
    WEBSET,
    WA,
    NULL,                  // ← grblName = NULL（无命令编号）
    "Sta/RetryCount",      // ← fullName = "Sta/RetryCount"
    DEFAULT_STA_RETRY_COUNT,
    MIN_STA_RETRY_COUNT,
    MAX_STA_RETRY_COUNT,
    NULL
);

// 用户只能使用以下方式访问：
// 方式 1: Sta/RetryCount=5         （唯一方式）

// 示例 3: GRBL 标准设置（可通过 $ 命令访问）
FloatSetting* x_steps_per_mm = new FloatSetting(
    GRBL, WG,
    "100",                 // ← grblName = "100"（GRBL 编号）
    "X/StepsPerMm",        // ← fullName = "X/StepsPerMm"
    DEFAULT_X_STEPS_PER_MM, 0.0, 10000.0
);

// 用户可以使用以下方式访问：
// 方式 1: $100=80                  （推荐，GRBL 标准格式）
// 方式 2: X/StepsPerMm=80          （通过完整名称）
```

### 访问方式的选择规则

#### GRBL 类型设置 (type = GRBL)
- **必须**提供 `grblName`（如 `"0"`, `"100"`）
- 用户使用 `$数字=值` 格式
- 保持与原始 Grbl 的兼容性

#### WEBSET 类型设置 (type = WEBSET)
- **可选**提供 `grblName`（如 `"ESP100"`）
- 如果提供：用户使用 `[ESP数字]值` 格式
- 如果不提供：用户只能使用完整名称

#### EXTENDED 类型设置 (type = EXTENDED)
- 通常 `grblName = NULL`
- 用户使用完整名称访问

### ESP 命令格式详解

```cpp
// ESP 命令的通用格式
[ESP数字]参数 pwd=密码

// 实际示例
[ESP100]MyWiFi pwd=admin          // 设置 WiFi SSID
[ESP101]MyPassword pwd=admin      // 设置 WiFi 密码
[ESP121]8080 pwd=admin            // 设置 HTTP 端口
[ESP400]                          // 列出所有 WEBSET 设置
[ESP401]P=Sta/SSID V=NewSSID      // 通用设置命令
```

### 通用设置命令 [ESP401]

对于**所有**设置（包括没有 ESP 编号的），都可以使用 `[ESP401]` 通用命令：

```cpp
// 格式
[ESP401]P=完整名称 V=值 pwd=密码

// 示例
[ESP401]P=Sta/SSID V=MyWiFi pwd=admin
[ESP401]P=Sta/RetryCount V=5 pwd=admin
[ESP401]P=X/StepsPerMm V=80 pwd=admin
```

**实现原理：** 查看 [WebSettings.cpp:647-661](Grbl_Esp32/src/WebUI/WebSettings.cpp#L647-L661)

```cpp
static Error setWebSetting(char* parameter, AuthenticationLevel auth_level) {  // ESP401
    if (!split_params(parameter)) {
        return Error::InvalidValue;
    }
    char*       sval = get_param("V", true);   // 获取值
    const char* spos = get_param("P", false);  // 获取名称
    if (*spos == '\0') {
        webPrintln("Missing parameter");
        return Error::InvalidValue;
    }
    Error ret = do_command_or_setting(spos, sval, auth_level, espresponse);
    return ret;
}
```

## 设置类型详解

### IntSetting - 整数设置

```cpp
IntSetting* laser_full_power = new IntSetting(
    EXTENDED, WG, NULL, "Laser/FullPower",
    DEFAULT_LASER_FULL_POWER, 0, 10000, checkSpindleChange
);

// 代码中使用
int32_t power = laser_full_power->get();

// 用户访问（因为 grblName = NULL）
// 方式 1: Laser/FullPower=5000
// 方式 2: [ESP401]P=Laser/FullPower V=5000
```

### FloatSetting - 浮点数设置

```cpp
FloatSetting* steps_per_mm = new FloatSetting(
    GRBL, WG, "100", "X/StepsPerMm",
    DEFAULT_X_STEPS_PER_MM, 0.0, 10000.0
);

// 代码中使用
float steps = steps_per_mm->get();

// 用户访问（因为 grblName = "100"）
// 方式 1: $100=80              （推荐，GRBL 标准）
// 方式 2: X/StepsPerMm=80
// 方式 3: [ESP401]P=X/StepsPerMm V=80
```

### StringSetting - 字符串设置

```cpp
StringSetting* wifi_ssid = new StringSetting(
    WEBSET, WA, "ESP100", "WiFi/SSID",
    "", 1, 32, WiFiConfig::isSSIDValid
);

// 代码中使用
const char* ssid = wifi_ssid->get();

// 用户访问（因为 grblName = "ESP100"）
// 方式 1: [ESP100]MyWiFi pwd=admin    （推荐，ESP3D 标准）
// 方式 2: WiFi/SSID=MyWiFi
// 方式 3: [ESP401]P=WiFi/SSID V=MyWiFi pwd=admin
```

### EnumSetting - 枚举设置

```cpp
enum_opt_t messageLevels = {
    { "None",    static_cast<int8_t>(MsgLevel::None) },
    { "Error",   static_cast<int8_t>(MsgLevel::Error) },
    { "Warning", static_cast<int8_t>(MsgLevel::Warning) },
    { "Info",    static_cast<int8_t>(MsgLevel::Info) },
    { "Debug",   static_cast<int8_t>(MsgLevel::Debug) },
    { "Verbose", static_cast<int8_t>(MsgLevel::Verbose) }
};

EnumSetting* message_level = new EnumSetting(
    NULL, EXTENDED, WG, NULL, "Message/Level",
    static_cast<int8_t>(MsgLevel::Info), &messageLevels
);

// 使用
int8_t level = message_level->get();
```

### FlagSetting - 布尔设置

```cpp
FlagSetting* probe_invert = new FlagSetting(
    GRBL, WG, "6", "Probe/Invert",
    DEFAULT_INVERT_PROBE_PIN
);

// 使用
bool inverted = probe_invert->get();
```

### AxisMaskSetting - 轴掩码设置

```cpp
AxisMaskSetting* dir_invert_mask = new AxisMaskSetting(
    GRBL, WG, "3", "Stepper/DirInvert",
    DEFAULT_DIRECTION_INVERT_MASK, postMotorSetting
);

// 使用
int32_t mask = dir_invert_mask->get();
if (mask & bit(X_AXIS)) {
    // X 轴方向反转
}
```

### IPaddrSetting - IP 地址设置

```cpp
IPaddrSetting* ip_address = new IPaddrSetting(
    "IP Address", WEBSET, WA, NULL, "WiFi/IP",
    "192.168.1.100"
);

// 使用
uint32_t ip = ip_address->get();
IPAddress addr(ip);
```

## NVS 存储机制

### Key 名称生成

```cpp
// Settings.cpp:41-55
size_t len = strlen(fullName);
if (len <= 15) {
    _keyName = _fullName;  // 直接使用完整名称
} else {
    // 使用 Donald Knuth 哈希算法生成 15 字符的 key
    char* hashName = (char*)malloc(16);
    uint32_t hash = len;
    for (const char* s = fullName; *s; s++) {
        hash = ((hash << 5) ^ (hash >> 27)) ^ (*s);
    }
    sprintf(hashName, "%.7s%08x", fullName, hash);
    _keyName = hashName;
}
```

**原因：** ESP32 NVS 限制 key 名称最多 15 字符

### 存储和读取

```cpp
// 存储（IntSetting 为例）
void IntSetting::setStringValue(char* s) {
    int32_t convertedValue = strtol(s, &endptr, 10);
    // 验证范围...

    if (_storedValue != convertedValue) {
        if (convertedValue == _defaultValue) {
            nvs_erase_key(_handle, _keyName);  // 恢复默认值时删除
        } else {
            nvs_set_i32(_handle, _keyName, convertedValue);
            _storedValue = convertedValue;
        }
    }
    _currentValue = convertedValue;
}

// 读取
void IntSetting::load() {
    esp_err_t err = nvs_get_i32(_handle, _keyName, &_storedValue);
    if (err) {
        _storedValue = std::numeric_limits<int32_t>::min();
        _currentValue = _defaultValue;
    } else {
        _currentValue = _storedValue;
    }
}
```

## 权限系统

### 权限级别

```cpp
typedef enum : uint8_t {
    WG,  // Readable and writable as guest（访客可读写）
    WU,  // Readable and writable as user and admin（用户和管理员可读写）
    WA,  // Readable as user and admin, writable as admin（管理员可写）
} permissions_t;
```

### 权限检查

```cpp
// WebSettings.cpp
if (setting->getPermissions() == WA && auth_level < WebUI::AuthenticationLevel::LEVEL_ADMIN) {
    return Error::AuthenticationFailed;
}
```

## 设置类型分类

### type_t 枚举

```cpp
typedef enum : uint8_t {
    GRBL = 1,  // Classic GRBL settings like $100
    EXTENDED,  // Settings added by early versions of Grbl_Esp32
    WEBSET,    // Settings for ESP3D_WebUI, stored in NVS
    GRBLCMD,   // Non-persistent GRBL commands like $H
    WEBCMD,    // ESP3D_WebUI commands that are not directly settings
} type_t;
```

### 用途

- **GRBL**: 兼容原始 Grbl 的设置（如 `$0`, `$1`, `$100` 等）
- **EXTENDED**: Grbl_ESP32 扩展的设置（如 `Laser/FullPower`）
- **WEBSET**: Web UI 专用设置（如 WiFi 配置）
- **GRBLCMD**: 非持久化命令（如 `$H` 归零）
- **WEBCMD**: Web UI 命令（如 `[ESP410]` 扫描 WiFi）

## 验证函数（Checker）

### 自定义验证

```cpp
// 验证主轴变更是否允许
bool checkSpindleChange(char* value) {
    if (sys.state == State::Cycle) {
        return false;  // 运行中不允许修改
    }
    return true;
}

// 使用验证函数
IntSetting* laser_full_power = new IntSetting(
    EXTENDED, WG, NULL, "Laser/FullPower",
    DEFAULT_LASER_FULL_POWER, 0, 10000,
    checkSpindleChange  // 验证函数
);
```

### 内置验证

```cpp
// Settings.cpp:58-66
Error Setting::check(char* s) {
    if (sys.state != State::Idle && sys.state != State::Alarm) {
        return Error::IdleError;  // 必须在空闲或报警状态
    }
    if (!_checker) {
        return Error::Ok;
    }
    return _checker(s) ? Error::Ok : Error::InvalidValue;
}
```

## Web UI 集成

### JSON 序列化

```cpp
// IntSetting::addWebui()
void IntSetting::addWebui(WebUI::JSONencoder* j) {
    if (getDescription()) {
        j->begin_webui(getName(), getDescription(), "I",
                       getStringValue(), _minValue, _maxValue);
        j->end_object();
    }
}
```

### 输出示例

```json
{
  "name": "Stepper/Pulse",
  "description": "Step pulse duration",
  "type": "I",
  "value": "5",
  "min": 3,
  "max": 1000
}
```

## 设计优势

### 1. 类型安全
- 编译时类型检查：`int32_t pulse = pulse_microseconds->get()`
- 运行时范围验证：`_minValue` 和 `_maxValue`

### 2. 统一接口
- 字符串接口：用于用户交互（串口、Web UI）
- 类型化接口：用于代码访问（`get()` 方法）

### 3. 自动管理
- 无需手动维护设置列表
- 自动注册到全局链表
- 统一的遍历和序列化

### 4. 持久化存储
- 自动 NVS 存储
- 默认值优化（等于默认值时删除 NVS 条目）
- 长名称哈希处理

### 5. 扩展性
- 易于添加新的设置类型
- 支持自定义验证函数
- 支持权限控制

## 常见使用模式

### 模式 1：简单整数设置

```cpp
IntSetting* my_setting = new IntSetting(
    EXTENDED, WG, NULL, "My/Setting",
    100, 0, 1000  // 默认值 100，范围 0-1000
);

// 使用
int32_t value = my_setting->get();
```

### 模式 2：带验证的设置

```cpp
bool validateMotorCurrent(char* value) {
    // 自定义验证逻辑
    return true;
}

IntSetting* motor_current = new IntSetting(
    EXTENDED, WG, NULL, "Motor/Current",
    500, 0, 2000, validateMotorCurrent
);
```

### 模式 3：GRBL 兼容设置

```cpp
// 使用 GRBL 编号（如 $100）
FloatSetting* x_steps_per_mm = new FloatSetting(
    GRBL, WG, "100", "X/StepsPerMm",
    80.0, 0.0, 10000.0
);

// 用户可以使用 "$100=100" 或 "X/StepsPerMm=100"
```

### 模式 4：轴相关设置

```cpp
// 为每个轴创建设置
for (int axis = 0; axis < N_AXIS; axis++) {
    char name[32];
    sprintf(name, "%c/StepsPerMm", "XYZABC"[axis]);

    auto setting = new FloatSetting(
        GRBL, WG, grblName, name,
        defaultValue, 0.0, 10000.0
    );
    setting->setAxis(axis);  // 关联到轴
}
```

## 最佳实践

### 1. 命名规范
- 使用层级结构：`"Category/Subcategory/Name"`
- 示例：`"Stepper/Pulse"`, `"WiFi/SSID"`, `"Laser/FullPower"`

### 2. 默认值
- 在 [Config.h](Grbl_Esp32/src/Config.h) 或 [Defaults.h](Grbl_Esp32/src/Defaults.h) 中定义
- 使用宏定义：`#define DEFAULT_STEP_PULSE_MICROSECONDS 3`

### 3. 范围设置
- 整数：使用合理的最小/最大值
- 浮点数：考虑物理限制
- 字符串：设置长度限制

### 4. 验证函数
- 仅在需要复杂验证时使用
- 简单范围检查由类自动处理
- 验证函数应快速返回

### 5. 权限控制
- 安全相关设置使用 `WA`（仅管理员可写）
- 常规设置使用 `WG`（访客可写）
- 敏感信息使用 `WU` 或 `WA`

## 相关文件

### 核心文件
- [Settings.h](Grbl_Esp32/src/Settings.h) - 类定义
- [Settings.cpp](Grbl_Esp32/src/Settings.cpp) - 类实现
- [SettingsDefinitions.h](Grbl_Esp32/src/SettingsDefinitions.h) - 全局指针声明
- [SettingsDefinitions.cpp](Grbl_Esp32/src/SettingsDefinitions.cpp) - 设置对象创建

### 配置文件
- [Config.h](Grbl_Esp32/src/Config.h) - 编译时配置
- [Defaults.h](Grbl_Esp32/src/Defaults.h) - 默认值定义

### 使用示例
- [Protocol.cpp](Grbl_Esp32/src/Protocol.cpp) - 命令处理
- [WebSettings.cpp](Grbl_Esp32/src/WebUI/WebSettings.cpp) - Web UI 集成

## 总结

Grbl_ESP32 的 Settings 系统是一个**自注册单例模式**的优秀实现：

✅ **自动注册**：每个设置对象在创建时自动加入全局链表
✅ **类型安全**：编译时类型检查 + 运行时范围验证
✅ **统一接口**：字符串接口（用户）+ 类型化接口（代码）
✅ **持久化存储**：自动 NVS 存储和恢复
✅ **多态访问**：通过基类指针统一处理不同类型
✅ **扩展性强**：易于添加新的设置类型和验证逻辑

这种设计非常适合嵌入式系统的配置管理，既保证了类型安全，又提供了灵活的运行时访问能力。
