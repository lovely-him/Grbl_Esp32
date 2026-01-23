# ESP410 AP扫描指令深度分析

> **文档版本**: v1.0
> **创建日期**: 2026-01-16
> **适用版本**: Grbl_Esp32 (espressif32@3.0.0)

## 📋 文档概述

本文档深入分析 Grbl_ESP32 中 `[ESP410]` 指令的完整实现机制，包括：
- ESP32 WiFi AP 扫描的底层实现
- 命令接收和路由机制
- 异步处理和阻塞避免策略
- JSON 响应打包机制
- 关键设计模式和最佳实践

---

## 🎯 核心功能

**指令名称**: `[ESP410]` / `WiFi/ListAPs`
**功能**: 扫描并返回周围可用的 WiFi 接入点列表
**权限要求**: WEBUI_USER (WU)
**支持客户端**: 串口、Web UI、Telnet

**返回格式**:
```json
{
  "AP_LIST": [
    {
      "SSID": "MyWiFi",
      "SIGNAL": 85,
      "IS_PROTECTED": true
    },
    {
      "SSID": "OpenNetwork",
      "SIGNAL": 60,
      "IS_PROTECTED": false
    }
  ]
}
```

---

## 🔄 完整调用链路

### 调用流程图

```
用户输入 [ESP410]
    ↓
┌─────────────────────────────────────────────────────────┐
│ 入口层 (Entry Point)                                     │
├─────────────────────────────────────────────────────────┤
│ • 串口: Protocol.cpp:84 execute_line()                  │
│ • Web:  WebServer.cpp:468 HTTP请求处理                  │
└─────────────────────────────────────────────────────────┘
    ↓ system_execute_line()
┌─────────────────────────────────────────────────────────┐
│ 路由层 (Command Router)                                 │
├─────────────────────────────────────────────────────────┤
│ ProcessSettings.cpp:500 do_command_or_setting()         │
│ • 遍历 Command::List 链表                               │
│ • 匹配 "ESP410" 或 "WiFi/ListAPs"                       │
│ • 权限验证                                              │
└─────────────────────────────────────────────────────────┘
    ↓ WebCommand::action()
┌─────────────────────────────────────────────────────────┐
│ 执行层 (Command Handler)                                │
├─────────────────────────────────────────────────────────┤
│ WebSettings.cpp:605 listAPs()                           │
│ • 创建 JSONencoder                                      │
│ • 检查 WiFi.scanComplete() 状态                        │
│ • 打包 JSON 响应                                        │
└─────────────────────────────────────────────────────────┘
    ↓ espresponse->print()
┌─────────────────────────────────────────────────────────┐
│ 响应层 (Response Stream)                                │
├─────────────────────────────────────────────────────────┤
│ ESPResponse.cpp:75 ESPResponseStream::print()           │
│ • 根据客户端类型路由输出                                │
│ • 串口/Telnet: 直接输出                                 │
│ • Web: 缓冲后统一发送                                   │
└─────────────────────────────────────────────────────────┘
    ↓
用户收到 JSON 响应
```

---

## 1️⃣ ESP32 WiFi AP 扫描机制

### 1.1 核心 API

Grbl_ESP32 使用 Arduino WiFi 库的异步扫描 API：

```cpp
// 启动异步扫描
WiFi.scanNetworks(true);  // true = 异步模式，不阻塞

// 检查扫描状态
int n = WiFi.scanComplete();
// 返回值:
//   -2: 未触发扫描
//   -1: 扫描正在进行中
//   >=0: 扫描完成，返回发现的 AP 数量

// 获取扫描结果
String ssid = WiFi.SSID(i);              // AP 名称
int32_t rssi = WiFi.RSSI(i);             // 信号强度 (dBm)
wifi_auth_mode_t enc = WiFi.encryptionType(i);  // 加密类型

// 清理扫描结果
WiFi.scanDelete();
```

### 1.2 扫描触发时机

#### 时机 1: 系统启动时预扫描

**位置**: `Grbl_Esp32/src/WebUI/WifiServices.cpp:123`

```cpp
bool WiFiServices::begin() {
    // ... 初始化 Web/Telnet/OTA 服务

    // 启动后台异步扫描
    WiFi.scanNetworks(true);

    return no_error;
}
```

**目的**: 确保用户第一次调用 `[ESP410]` 时就有数据可用。

#### 时机 2: 用户请求时

**位置**: `Grbl_Esp32/src/WebUI/WebSettings.cpp:614`

```cpp
int n = WiFi.scanComplete();
switch (n) {
    case -2:  // 未触发扫描
        WiFi.scanNetworks(true);  // 启动新扫描
        break;
    // ...
}
```

#### 时机 3: 扫描完成后自动重启

**位置**: `Grbl_Esp32/src/WebUI/WebSettings.cpp:627-633`

```cpp
// 返回结果后
WiFi.scanDelete();
n = WiFi.scanComplete();
if (n == -2) {
    WiFi.scanNetworks(true);  // 重启扫描，保持数据新鲜
}
```

### 1.3 WiFi 模式管理

**关键问题**: ESP32 在 AP 模式下扫描时会临时切换到 `WIFI_AP_STA` 混合模式。

**解决方案**: `Grbl_Esp32/src/WebUI/WifiServices.cpp:148-158`

```cpp
void WiFiServices::handle() {
    COMMANDS::wait(0);  // 喂看门狗

    // 防止 AP+STA 混合模式持续存在
    if (WiFi.getMode() == WIFI_AP_STA) {
        // 扫描完成后立即禁用 STA
        if (WiFi.scanComplete() >= 0) {
            WiFi.enableSTA(false);
        }
    }
    // ...
}
```

**为什么重要**:
- 混合模式会增加功耗
- 可能影响 AP 模式的稳定性
- 必须在主循环中持续监控并及时切回

---

## 2️⃣ 命令接收和路由机制

### 2.1 统一命令入口

Grbl_ESP32 使用统一的命令处理框架，支持多种客户端。

#### 串口入口

**位置**: `Grbl_Esp32/src/Protocol.cpp:77-92`

```cpp
Error execute_line(char* line, uint8_t client, WebUI::AuthenticationLevel auth_level) {
    if (line[0] == 0) {
        return Error::Ok;  // 空行
    }

    // 系统命令: $ 或 [
    if (line[0] == '$' || line[0] == '[') {
        return system_execute_line(line, client, auth_level);
        //     ↑ ESP410 走这里
    }

    // G-code 命令
    return gc_execute_line(line, client);
}
```

#### Web 入口

**位置**: `Grbl_Esp32/src/WebUI/WebServer.cpp:468-469`

```cpp
// HTTP 请求处理
ESPResponseStream* espresponse = new ESPResponseStream(_webserver);
Error err = system_execute_line(line, espresponse, auth_level);
```

### 2.2 命令路由器

**位置**: `Grbl_Esp32/src/ProcessSettings.cpp:500-573`

```cpp
Error do_command_or_setting(const char* key, char* value,
                            WebUI::AuthenticationLevel auth_level,
                            WebUI::ESPResponseStream* out) {
    // 步骤 1: 搜索 Setting (运行时设置)
    for (Setting* s = Setting::List; s; s = s->next()) {
        if (strcasecmp(s->getName(), key) == 0) {
            // 处理设置...
        }
    }

    // 步骤 2: 搜索 Command (命令)
    for (Command* cp = Command::List; cp; cp = cp->next()) {
        // 匹配 "ESP410" 或 "WiFi/ListAPs"
        if ((strcasecmp(cp->getName(), key) == 0) ||
            (cp->getGrblName() && strcasecmp(cp->getGrblName(), key) == 0)) {

            // 权限验证
            if (auth_failed(cp, value, auth_level)) {
                return Error::AuthenticationFailed;
            }

            // 调用命令的 action 方法
            return cp->action(value, auth_level, out);
        }
    }

    return Error::InvalidStatement;
}
```

### 2.3 命令注册机制

**位置**: `Grbl_Esp32/src/WebUI/WebSettings.cpp:1075`

```cpp
// 在系统初始化时注册命令
new WebCommand(
    NULL,              // 描述 (可选)
    WEBCMD,            // 命令类型
    WU,                // 权限: WEBUI_USER
    "ESP410",          // GRBL 名称
    "WiFi/ListAPs",    // WebUI 名称
    listAPs            // 处理函数指针
);
```

**命令对象结构**: `Grbl_Esp32/src/Settings.h:453-478`

```cpp
class WebCommand : public Command {
private:
    Error (*_action)(char*, WebUI::AuthenticationLevel);

public:
    Error action(char* value,
                 WebUI::AuthenticationLevel auth_level,
                 WebUI::ESPResponseStream* response) {
        espresponse = response;  // 设置全局响应流
        return _action(value, auth_level);  // 调用注册的函数
    }
};
```

---

## 3️⃣ 核心处理函数 listAPs()

### 3.1 完整实现

**位置**: `Grbl_Esp32/src/WebUI/WebSettings.cpp:605-642`

```cpp
static Error listAPs(char* parameter, AuthenticationLevel auth_level) {
    // 步骤 1: 创建 JSON 编码器
    JSONencoder j(espresponse->client() != CLIENT_WEBUI);
    // ↑ WebUI 用紧凑格式，串口用可读格式

    j.begin();                    // {
    j.begin_array("AP_LIST");     // "AP_LIST":[

    // 步骤 2: 检查扫描状态（非阻塞）
    int n = WiFi.scanComplete();
    switch (n) {
        case -2:  // 未触发扫描
            WiFi.scanNetworks(true);  // 启动异步扫描
            // 注意：不等待，直接返回空列表
            break;

        case -1:  // 扫描进行中
            // 不等待，直接返回空列表
            break;

        default:  // n >= 0，扫描完成
            // 步骤 3: 遍历扫描结果
            for (int i = 0; i < n; ++i) {
                j.begin_object();     // {
                j.member("SSID", WiFi.SSID(i));
                j.member("SIGNAL", wifi_config.getSignal(WiFi.RSSI(i)));
                j.member("IS_PROTECTED", WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                j.end_object();       // }
            }

            // 步骤 4: 清理并重启扫描
            WiFi.scanDelete();
            n = WiFi.scanComplete();
            if (n == -2) {
                WiFi.scanNetworks(true);  // 重启异步扫描
            }
            break;
    }

    j.end_array();                // ]

    // 步骤 5: 输出 JSON
    webPrint(j.end());            // }

    // 步骤 6: WebUI 不需要额外换行
    if (espresponse->client() != CLIENT_WEBUI) {
        espresponse->println("");
    }

    return Error::Ok;
}
```

### 3.2 关键设计点

#### 非阻塞检查

```cpp
int n = WiFi.scanComplete();
// 立即返回，不等待
```

#### 信号强度转换

```cpp
j.member("SIGNAL", wifi_config.getSignal(WiFi.RSSI(i)));
// 将 RSSI (dBm) 转换为百分比 (0-100)
```

#### 客户端适配

```cpp
JSONencoder j(espresponse->client() != CLIENT_WEBUI);
// WebUI: 紧凑格式 {"AP_LIST":[...]}
// 串口: 可读格式（带换行和缩进）
```

---

## 4️⃣ 异步处理和阻塞避免策略

### 4.1 为什么必须异步？

Grbl_ESP32 是**实时 CNC 控制器**，有严格的实时性要求：

- **步进电机控制**: 需要精确的定时脉冲（微秒级）
- **看门狗定时器**: 长时间阻塞会触发系统重启
- **用户体验**: 命令必须快速响应

**WiFi 扫描耗时**: 通常需要 1-5 秒，绝对不能阻塞主循环。

### 4.2 异步扫描状态机

```
[未扫描 -2] ──启动扫描──> [扫描中 -1] ──完成──> [有结果 >=0]
     ↑                                              │
     └──────────────── 清理并重启 ──────────────────┘
```

**状态转换表**:

| 状态 | scanComplete() | 操作 | 返回给用户 |
|------|----------------|------|-----------|
| 未扫描 | -2 | 启动异步扫描 | 空列表 |
| 扫描中 | -1 | 不等待 | 空列表 |
| 有结果 | >=0 | 返回结果并重启扫描 | AP 列表 |

### 4.3 关键设计决策

#### 决策 1: 立即返回，不等待

```cpp
case -1:  // 扫描进行中
    // ❌ 错误做法: while (WiFi.scanComplete() == -1) { delay(100); }
    // ✅ 正确做法: 直接 break，返回空列表
    break;
```

**理由**: 用户可以稍后重试，不能阻塞系统。

#### 决策 2: 使用异步 API

```cpp
// ✅ 正确
WiFi.scanNetworks(true);   // 异步，后台扫描

// ❌ 错误
WiFi.scanNetworks(false);  // 同步，阻塞 1-5 秒
```

#### 决策 3: 看门狗喂狗

**位置**: `Grbl_Esp32/src/WebUI/Commands.cpp:36-43`

```cpp
void COMMANDS::wait(uint32_t milliseconds) {
    uint32_t timeout = millis();
    esp_task_wdt_reset();  // 喂狗

    while ((millis() - timeout) < milliseconds) {
        esp_task_wdt_reset();  // 持续喂狗
    }
}
```

---

## 5️⃣ JSON 打包机制

### 5.1 JSONencoder 类

**位置**: `Grbl_Esp32/src/WebUI/JSONEncoder.h` 和 `JSONEncoder.cpp`

这是一个轻量级的 JSON 构建器，专为 Grbl_ESP32 设计。

#### 核心 API

```cpp
class JSONencoder {
public:
    JSONencoder(bool pretty);  // pretty=true 启用格式化输出

    void begin();              // 开始 JSON 对象 {
    String end();              // 结束并返回 JSON 字符串 }

    void begin_array(const char* tag);   // "tag":[
    void end_array();                    // ]

    void begin_object();       // {
    void end_object();         // }

    void member(const char* tag, const char* value);  // "tag":"value"
    void member(const char* tag, int value);          // "tag":123
};
```

### 5.2 使用示例

#### ESP410 中的使用

```cpp
JSONencoder j(espresponse->client() != CLIENT_WEBUI);
j.begin();                          // {
j.begin_array("AP_LIST");           //   "AP_LIST":[
    j.begin_object();               //     {
    j.member("SSID", "MyWiFi");     //       "SSID":"MyWiFi",
    j.member("SIGNAL", 85);         //       "SIGNAL":85,
    j.member("IS_PROTECTED", true); //       "IS_PROTECTED":true
    j.end_object();                 //     }
j.end_array();                      //   ]
String json = j.end();              // }
```

#### 输出格式对比

**WebUI 客户端** (紧凑格式):
```json
{"AP_LIST":[{"SSID":"MyWiFi","SIGNAL":85,"IS_PROTECTED":true}]}
```

**串口客户端** (可读格式):
```json
{
  "AP_LIST":[
    {
      "SSID":"MyWiFi",
      "SIGNAL":85,
      "IS_PROTECTED":true
    }
  ]
}
```

### 5.3 响应流机制

**位置**: `Grbl_Esp32/src/WebUI/ESPResponse.cpp:75-100`

```cpp
void ESPResponseStream::print(const char* data) {
    switch (_client) {
        case CLIENT_SERIAL:
            Serial.print(data);
            break;
        case CLIENT_TELNET:
            telnet_server.print(data);
            break;
        case CLIENT_WEBUI:
            _buffer += data;  // 缓冲，等待 flush()
            break;
    }
    _header_sent = true;
}
```

**客户端类型**:
- `CLIENT_SERIAL`: 串口
- `CLIENT_TELNET`: Telnet 连接
- `CLIENT_WEBUI`: Web UI
- `CLIENT_BT`: 蓝牙

---

## 6️⃣ 关键设计模式总结

### 6.1 命令注册模式

**优点**:
- 解耦命令定义和处理逻辑
- 支持动态命令扩展
- 统一的权限管理

**实现**:
```cpp
// 注册时
new WebCommand(NULL, WEBCMD, WU, "ESP410", "WiFi/ListAPs", listAPs);

// 执行时
for (Command* cp = Command::List; cp; cp = cp->next()) {
    if (match(cp, key)) {
        return cp->action(value, auth_level, out);
    }
}
```

### 6.2 异步状态机模式

**优点**:
- 不阻塞主循环
- 保证实时性
- 用户体验友好

**状态转换**:
```
未扫描 → 启动扫描 → 扫描中 → 完成 → 清理 → 重启扫描
  ↑                                              ↓
  └──────────────────────────────────────────────┘
```

### 6.3 客户端抽象模式

**优点**:
- 统一的输出接口
- 支持多种客户端类型
- 自动格式适配

**实现**:
```cpp
ESPResponseStream* espresponse = new ESPResponseStream(client);
espresponse->print(json);  // 自动路由到正确的输出通道
```

---

## 7️⃣ 实现要点和最佳实践

### 7.1 必须遵守的原则

#### ✅ DO (正确做法)

1. **使用异步 API**
   ```cpp
   WiFi.scanNetworks(true);  // 异步模式
   ```

2. **立即返回，不等待**
   ```cpp
   case -1:  // 扫描中
       break;  // 直接返回空列表
   ```

3. **持续喂看门狗**
   ```cpp
   esp_task_wdt_reset();
   ```

4. **管理 WiFi 模式**
   ```cpp
   if (WiFi.getMode() == WIFI_AP_STA) {
       if (WiFi.scanComplete() >= 0) {
           WiFi.enableSTA(false);
       }
   }
   ```

#### ❌ DON'T (错误做法)

1. **使用同步 API**
   ```cpp
   WiFi.scanNetworks(false);  // ❌ 阻塞 1-5 秒
   ```

2. **等待扫描完成**
   ```cpp
   while (WiFi.scanComplete() == -1) {  // ❌ 阻塞主循环
       delay(100);
   }
   ```

3. **使用 delay()**
   ```cpp
   delay(5000);  // ❌ 会触发看门狗重启
   ```

4. **忽略 WiFi 模式管理**
   ```cpp
   // ❌ 不检查和切换模式，导致功耗增加
   ```

### 7.2 关键文件索引

| 文件 | 位置 | 功能 |
|------|------|------|
| WebSettings.cpp | `Grbl_Esp32/src/WebUI/WebSettings.cpp:605-642` | ESP410 核心实现 |
| WifiServices.cpp | `Grbl_Esp32/src/WebUI/WifiServices.cpp:123` | 启动时预扫描 |
| WifiServices.cpp | `Grbl_Esp32/src/WebUI/WifiServices.cpp:148-158` | WiFi 模式管理 |
| ProcessSettings.cpp | `Grbl_Esp32/src/ProcessSettings.cpp:500-573` | 命令路由器 |
| Protocol.cpp | `Grbl_Esp32/src/Protocol.cpp:77-92` | 串口入口 |
| WebServer.cpp | `Grbl_Esp32/src/WebUI/WebServer.cpp:468-469` | Web 入口 |
| JSONEncoder.h | `Grbl_Esp32/src/WebUI/JSONEncoder.h` | JSON 编码器 |
| ESPResponse.h | `Grbl_Esp32/src/WebUI/ESPResponse.h` | 响应流 |
| Settings.h | `Grbl_Esp32/src/Settings.h:453-478` | WebCommand 类 |

### 7.3 潜在问题和注意事项

#### 问题 1: 首次调用可能返回空列表

**原因**: 系统刚启动，异步扫描尚未完成。

**解决方案**:
- 用户稍后重试（1-5秒后）
- 或在 UI 中显示"扫描中..."提示

#### 问题 2: AP 模式下扫描可能不稳定

**原因**: ESP32 在 AP 模式下扫描会临时切换到混合模式。

**解决方案**:
- 主循环持续监控并及时切回 AP 模式
- 参考 `WifiServices.cpp:148-158`

#### 问题 3: 扫描结果可能不完整

**原因**: 异步扫描可能受环境干扰。

**解决方案**:
- 允许用户多次扫描
- 显示信号强度，帮助用户判断

---

## 8️⃣ 总结

### 8.1 核心设计理念

ESP410 指令的实现体现了 Grbl_ESP32 的核心设计理念：

1. **实时性优先**: 绝不阻塞主循环，保证步进电机控制的精确性
2. **异步处理**: 使用状态机模式处理长时间操作
3. **统一抽象**: 通过命令注册和响应流实现多客户端支持
4. **轻量级实现**: 自定义 JSON 编码器，避免外部依赖

### 8.2 关键技术点

| 技术点 | 实现方式 | 位置 |
|--------|---------|------|
| 异步扫描 | WiFi.scanNetworks(true) | WebSettings.cpp:614 |
| 状态检查 | WiFi.scanComplete() | WebSettings.cpp:611 |
| 模式管理 | WiFi.enableSTA(false) | WifiServices.cpp:156 |
| 命令路由 | Command::List 链表遍历 | ProcessSettings.cpp:541 |
| JSON 打包 | JSONencoder 类 | JSONEncoder.cpp |
| 响应输出 | ESPResponseStream 抽象 | ESPResponse.cpp:75 |

### 8.3 适用场景

ESP410 指令的设计模式适用于以下场景：

- **需要长时间操作的命令**: 如文件上传、固件升级等
- **需要后台处理的任务**: 如日志记录、数据采集等
- **需要多客户端支持的功能**: 如状态查询、配置管理等

---

## 9️⃣ 参考资料

### 9.1 相关文档

- [WiFi_STA配置完全指南.md](WiFi_STA配置完全指南.md) - WiFi 配置详细说明
- [配置查看命令完全指南.md](配置查看命令完全指南.md) - 其他 ESP 命令参考
- [CLAUDE.md](../../CLAUDE.md) - Grbl_ESP32 开发指南

### 9.2 相关源码

- [WebSettings.cpp](../../Grbl_Esp32/src/WebUI/WebSettings.cpp) - ESP410 实现
- [WifiServices.cpp](../../Grbl_Esp32/src/WebUI/WifiServices.cpp) - WiFi 服务管理
- [JSONEncoder.cpp](../../Grbl_Esp32/src/WebUI/JSONEncoder.cpp) - JSON 编码器
- [Commands.txt](../Commands.txt) - 完整命令列表

### 9.3 ESP32 官方文档

- [ESP32 WiFi API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- [Arduino WiFi Library](https://www.arduino.cc/en/Reference/WiFi)

---

## 🎓 结语

本文档通过深入分析 ESP410 指令的实现，揭示了 Grbl_ESP32 在处理异步操作、多客户端支持和实时性保证方面的设计智慧。

**核心要点回顾**:

1. **异步优先**: 使用 `WiFi.scanNetworks(true)` 避免阻塞
2. **状态机模式**: 通过 `scanComplete()` 轮询状态
3. **立即响应**: 即使没有结果也立即返回
4. **模式管理**: 及时切换 WiFi 模式避免干扰
5. **统一抽象**: 命令注册和响应流支持多客户端

这些设计模式不仅适用于 WiFi 扫描，也为其他需要长时间操作的功能提供了参考范例。

---

**文档维护**:
- 如有问题或建议，请参考 [CLAUDE.md](../../CLAUDE.md) 中的贡献指南
- 本文档基于 Grbl_ESP32 源码分析，随版本更新可能需要调整

**最后更新**: 2026-01-16

