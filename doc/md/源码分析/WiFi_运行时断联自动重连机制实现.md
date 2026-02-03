# WiFi 运行时断联自动重连机制实现

## 文档信息
- **创建日期**: 2026-02-02
- **修改类型**: 功能增强
- **涉及模块**: `Grbl_Esp32/src/WebUI/WifiConfig.cpp`
- **修改文件**: WifiConfig.h, WifiConfig.cpp

---

## 一、功能概述

### 1.1 原代码的问题

Grbl_Esp32 原有代码在 WiFi 连接管理上存在不对称的处理逻辑：

| 场景 | 是否有重连机制 | 实现位置 |
|------|---------------|---------|
| **启动时连接失败** | ✅ 有 | `StartSTA()` 函数，支持配置重试次数 |
| **运行时意外断联** | ❌ 无 | `WiFiEvent()` 函数，仅打印日志 |

**核心问题**：
- 运行时断联后，代码仅打印 `[MSG:Disconnected]`，不做任何重连尝试
- 完全依赖 ESP32 WiFi 库的 `setAutoReconnect()` 功能
- 该功能在某些断联场景下不可靠（详见 2.3 节）

### 1.2 本次实现的功能

在 WiFi 事件处理中添加运行时自动重连机制：

- ✅ 运行时断联自动触发重连
- ✅ 复用现有配置 `wifi_sta_retry_count`（0 = 无限重试）
- ✅ 区分"用户主动断开"和"意外断联"
- ✅ 不影响原有启动连接流程

---

## 二、源码分析

### 2.1 WiFi 事件处理机制

Grbl_Esp32 使用 ESP32 WiFi 事件系统监听 WiFi 状态变化：

```cpp
// WifiConfig.cpp:475
WiFi.onEvent(WiFiConfig::WiFiEvent);
```

关键事件：
- `SYSTEM_EVENT_STA_START` - WiFi STA 启动
- `SYSTEM_EVENT_STA_GOT_IP` - 获取 IP 地址（连接成功）
- `SYSTEM_EVENT_STA_DISCONNECTED` - 断开连接

### 2.2 原有代码的局限

#### 启动时连接逻辑（有重试）

```cpp
// WifiConfig.cpp:303-399
bool WiFiConfig::StartSTA() {
    int32_t retry_count = wifi_sta_retry_count->get();
    bool infinite_retry = (retry_count == 0);

    while (infinite_retry || attempt <= retry_count) {
        if (WiFi.begin(...)) {
            if (ConnectSTA2AP()) {
                return true;  // ✅ 连接成功
            }
        }
        // ✅ 重试逻辑
    }
    return false;
}
```

#### 运行时断联处理（无重试）

```cpp
// WifiConfig.cpp:228-230（修改前）
case SYSTEM_EVENT_STA_DISCONNECTED:
    grbl_send(CLIENT_ALL, "[MSG:Disconnected]\r\n");
    break;  // ❌ 仅打印日志，无重连逻辑
```

#### 差异总结

| 特性 | 启动时连接 | 运行时断联 |
|------|-----------|-----------|
| 重试机制 | ✅ 有 | ❌ 无 |
| 配置支持 | ✅ 支持 | ❌ 不支持 |
| 日志输出 | ✅ 详细 | ⚠️ 仅一条 |

### 2.3 ESP32 WiFi 自动重连的局限性

ESP32 WiFi 库的 `setAutoReconnect()` 功能在以下场景下**不可靠**：

**会自动重连**：
- ✅ 路由器重启导致的短暂断联
- ✅ 信号暂时丢失后恢复

**不会自动重连**：
- ❌ 路由器主动发送 deauth 帧断开连接
- ❌ 长时间信号丢失（超过内部超时）
- ❌ WiFi 驱动内部状态异常

**参考**：[ESP32 Forum - WiFi.setAutoReconnect function](https://esp32.com/viewtopic.php?t=39116)
> "If you manually turn it off with a disconnect, it doesn't reconnect. It is equivalent to ending the session."

**结论**：不能依赖 ESP32 WiFi 库的自动重连作为唯一机制。

---

## 三、实现方案

### 3.1 技术选型

#### 事件驱动 vs 轮询

**选择**：事件驱动

**理由**：
- ✅ 响应及时（事件触发即处理）
- ✅ 无额外 CPU 开销
- ✅ 符合现有代码架构

#### WiFi.reconnect() vs WiFi.begin()

**选择**：`WiFi.reconnect()`

**理由**：
- ✅ 无需参数，使用之前保存的 SSID 和密码
- ✅ 更轻量级，开销更小
- ✅ 语义更符合"重连"场景

#### 状态标志的作用

**设计**：添加 `_auto_reconnect_enabled` 标志

**理由**：
- 区分"用户主动断开"（不应重连）和"意外断联"（应该重连）
- 避免在 AP 模式或 WiFi OFF 状态下尝试 STA 重连
- 确保重连行为与系统状态一致

### 3.2 核心设计

#### 状态变量

```cpp
static bool   _auto_reconnect_enabled;  // 是否启用运行时自动重连
static int32_t _reconnect_attempt;      // 当前重连尝试次数
```

#### 关键修改点

| 函数 | 修改内容 | 目的 |
|------|---------|------|
| `WiFiEvent()` | 在 `SYSTEM_EVENT_STA_DISCONNECTED` 中添加重连逻辑 | 运行时自动重连 |
| `StartSTA()` | 连接成功后设置 `_auto_reconnect_enabled = true` | 启用自动重连 |
| `StopWiFi()` | 设置 `_auto_reconnect_enabled = false` | 用户主动关闭时禁用 |
| `begin()` | 在切换模式时设置 `_auto_reconnect_enabled = false` | 状态切换时禁用 |

#### 启用/禁用时机

**启用时机**：
- ✅ STA 模式连接成功后（`StartSTA()` 返回 true）

**禁用时机**：
- ❌ 用户主动关闭 WiFi（`StopWiFi()`）
- ❌ 切换到 AP 模式（`begin()` 中）
- ❌ WiFi 模式设为 OFF（`begin()` 中）
- ❌ 达到最大重试次数（`WiFiEvent()` 中）

---

## 四、代码实现

### 4.1 修改文件清单

| 文件 | 修改类型 | 行数变化 |
|------|---------|---------|
| `WifiConfig.h` | 新增成员变量 | +2 |
| `WifiConfig.cpp` | 初始化静态变量 | +2 |
| `WifiConfig.cpp` | 增强 `WiFiEvent()` | +28 |
| `WifiConfig.cpp` | 修改 `StartSTA()` | +3 |
| `WifiConfig.cpp` | 修改 `StopWiFi()` | +4 |
| `WifiConfig.cpp` | 修改 `begin()` | +2 |

### 4.2 关键代码片段

#### 状态变量声明与初始化

**WifiConfig.h:132-133**
```cpp
static bool   _auto_reconnect_enabled;  // 是否启用运行时自动重连
static int32_t _reconnect_attempt;      // 当前重连尝试次数
```

**WifiConfig.cpp:36-37**
```cpp
bool   WiFiConfig::_auto_reconnect_enabled = false;
int32_t WiFiConfig::_reconnect_attempt = 0;
```

#### WiFiEvent() 断联处理增强

**WifiConfig.cpp:228-259**
```cpp
case SYSTEM_EVENT_STA_DISCONNECTED:
    grbl_send(CLIENT_ALL, "[MSG:Disconnected]\r\n");

    // 如果启用了自动重连，尝试重新连接
    if (_auto_reconnect_enabled) {
        int32_t max_retry = wifi_sta_retry_count->get();
        bool infinite_retry = (max_retry == 0);

        // 检查是否还可以重试
        if (infinite_retry || _reconnect_attempt < max_retry) {
            _reconnect_attempt++;

            if (infinite_retry) {
                grbl_sendf(CLIENT_ALL, "[MSG:Auto-reconnecting (attempt %d)...]\r\n",
                          _reconnect_attempt);
            } else {
                grbl_sendf(CLIENT_ALL, "[MSG:Auto-reconnecting (attempt %d/%d)...]\r\n",
                          _reconnect_attempt, max_retry);
            }

            WiFi.reconnect();
        } else {
            grbl_sendf(CLIENT_ALL, "[MSG:Auto-reconnect failed after %d attempts]\r\n",
                      max_retry);
            _auto_reconnect_enabled = false;
        }
    }
    break;

case SYSTEM_EVENT_STA_GOT_IP:
    grbl_sendf(CLIENT_ALL, "[MSG:Connected with %s]\r\n", WiFi.localIP().toString().c_str());
    _reconnect_attempt = 0;  // 连接成功后重置计数器
    break;
```

#### 启用自动重连

**WifiConfig.cpp:380-383**
```cpp
if (ConnectSTA2AP()) {
    // 连接成功后启用运行时自动重连
    _auto_reconnect_enabled = true;
    _reconnect_attempt = 0;
    return true;
}
```

#### 禁用自动重连

**WifiConfig.cpp:453-455（StopWiFi）**
```cpp
void WiFiConfig::StopWiFi() {
    // 禁用运行时自动重连
    _auto_reconnect_enabled = false;
    _reconnect_attempt = 0;
    // ...
}
```

**WifiConfig.cpp:488, 495（begin）**
```cpp
// 切换到 AP 模式时
_auto_reconnect_enabled = false;

// WiFi 模式为 OFF 时
_auto_reconnect_enabled = false;
```

---

## 五、使用说明

### 5.1 功能行为

#### 正常连接流程
```
[ESP115]ON
  ↓
[MSG:Connecting MT3000 (attempt 1)]
  ↓
[MSG:Connected with 192.168.8.177]
  ↓
_auto_reconnect_enabled = true  // 自动启用
```

#### 运行时断联（无限重试）
```
[MSG:Disconnected]
  ↓
[MSG:Auto-reconnecting (attempt 1)...]
  ↓
[MSG:Disconnected]
  ↓
[MSG:Auto-reconnecting (attempt 2)...]
  ↓
...（持续重试直到成功）
  ↓
[MSG:Connected with 192.168.8.177]
```

#### 运行时断联（有限重试）
```
[MSG:Disconnected]
  ↓
[MSG:Auto-reconnecting (attempt 1/5)...]
  ↓
...
  ↓
[MSG:Auto-reconnecting (attempt 5/5)...]
  ↓
[MSG:Auto-reconnect failed after 5 attempts]
  ↓
_auto_reconnect_enabled = false  // 自动禁用
```

#### 用户主动关闭
```
[ESP115]OFF
  ↓
[MSG:WiFi Off]
  ↓
_auto_reconnect_enabled = false  // 不会重连
```

### 5.2 配置项

| 配置项 | 默认值 | 说明 | 查看命令 |
|--------|--------|------|---------|
| `wifi_sta_retry_count` | 0 | 重试次数（0 = 无限重试） | `[ESP400]` |
| `wifi_sta_connect_timeout` | 10000ms | 每次连接的超时时间 | `[ESP400]` |

**修改配置**：
```
[ESP401]P=Sta/RetryCount T=I V=5
```

### 5.3 控制命令

| 命令 | 功能 | 说明 |
|------|------|------|
| `[ESP115]ON` | 开启 WiFi | 根据 `wifi_radio_mode` 启动 STA 或 AP 模式 |
| `[ESP115]OFF` | 关闭 WiFi | 停止所有 WiFi 服务，禁用自动重连 |
| `[ESP115]` | 查询状态 | 显示当前 WiFi 是 ON 还是 OFF |
| `[ESP420]` | 系统状态 | 显示 CPU、内存、WiFi 连接信息 |
| `[ESP444]RESTART` | 重启系统 | 重启整个 ESP32 |

---

## 六、设计决策说明

### Q1: 为什么不在 handle() 中轮询？

**考虑过的方案**：在 `WiFiConfig::handle()` 中定期检查 WiFi 状态并重连

**放弃原因**：
- ❌ 增加 CPU 开销（每次循环都要检查）
- ❌ 响应延迟（取决于 handle() 调用频率）
- ❌ 可能与事件处理冲突
- ✅ 事件驱动更高效、更及时

### Q2: 为什么使用 WiFi.reconnect()？

**WiFi.reconnect() vs WiFi.begin()**：

| 特性 | WiFi.reconnect() | WiFi.begin() |
|------|------------------|--------------|
| 参数 | 无需参数 | 需要 SSID 和密码 |
| 配置 | 使用之前保存的配置 | 需要重新传入配置 |
| 开销 | 轻量级 | 较重 |
| 适用场景 | 运行时重连 | 首次连接或更换网络 |

**选择理由**：
- ✅ 更符合"重连"语义
- ✅ 不需要重新读取配置
- ✅ 代码更简洁

### Q3: 为什么需要 _auto_reconnect_enabled 标志？

**问题**：为什么不直接在断联事件中无条件重连？

**原因**：
1. **区分用户意图**：
   - 用户主动关闭 WiFi（`[ESP115]OFF`）不应该重连
   - 意外断联才需要重连

2. **避免无意义重连**：
   - 切换到 AP 模式后不应该尝试 STA 重连
   - WiFi 模式为 OFF 时不应该重连

3. **状态一致性**：
   - 确保重连行为与系统状态一致
   - 避免与其他模块冲突

### Q4: 为什么默认无限重试？

**默认值为 0（无限重试）的原因**：
1. ✅ 符合大多数用户期望（WiFi 应该自动恢复）
2. ✅ 适合长时间运行的设备（如 CNC 机器）
3. ✅ 避免因临时网络问题导致功能失效

**支持有限重试的原因**：
1. ✅ 某些场景需要快速失败（如生产测试）
2. ✅ 避免无限重连消耗资源
3. ✅ 给用户更多控制权

---

## 七、总结

### 核心改进

**问题**：原代码启动时有重连机制，运行时没有

**原因**：运行时断联处理仅打印日志，完全依赖不可靠的 ESP32 WiFi 自动重连

**方案**：在 WiFi 事件处理中添加主动重连逻辑，使用状态标志区分不同场景

### 修改范围

- **文件数量**：2 个（WifiConfig.h, WifiConfig.cpp）
- **代码行数**：+41 行
- **修改位置**：5 处

### 关键特性

- ✅ **配置灵活**：复用现有 `wifi_sta_retry_count` 配置
- ✅ **状态清晰**：使用标志位区分用户主动断开和意外断联
- ✅ **不影响原有流程**：启动连接逻辑保持不变
- ✅ **日志完善**：重连过程有详细日志输出

---

## 附录

### A. 完整修改对比

#### WifiConfig.h

```diff
 private:
     static bool   ConnectSTA2AP();
     static void   WiFiEvent(WiFiEvent_t event);
     static String _hostname;
     static bool   _events_registered;
+    static bool   _auto_reconnect_enabled;  // 是否启用运行时自动重连
+    static int32_t _reconnect_attempt;      // 当前重连尝试次数
 };
```

#### WifiConfig.cpp - 静态变量初始化

```diff
 namespace WebUI {
     WiFiConfig wifi_config;

     String WiFiConfig::_hostname          = "";
     bool   WiFiConfig::_events_registered = false;
+    bool   WiFiConfig::_auto_reconnect_enabled = false;
+    int32_t WiFiConfig::_reconnect_attempt = 0;

     WiFiConfig::WiFiConfig() {}
```

#### WifiConfig.cpp - WiFiEvent() 函数

```diff
     void WiFiConfig::WiFiEvent(WiFiEvent_t event) {
         switch (event) {
             case SYSTEM_EVENT_STA_START:
                 // ...
                 break;
             case SYSTEM_EVENT_STA_GOT_IP:
                 grbl_sendf(CLIENT_ALL, "[MSG:Connected with %s]\r\n", WiFi.localIP().toString().c_str());
+                _reconnect_attempt = 0;  // 连接成功后重置计数器
                 break;
             case SYSTEM_EVENT_STA_DISCONNECTED:
                 grbl_send(CLIENT_ALL, "[MSG:Disconnected]\r\n");
+
+                // 如果启用了自动重连，尝试重新连接
+                if (_auto_reconnect_enabled) {
+                    int32_t max_retry = wifi_sta_retry_count->get();
+                    bool infinite_retry = (max_retry == 0);
+
+                    // 检查是否还可以重试
+                    if (infinite_retry || _reconnect_attempt < max_retry) {
+                        _reconnect_attempt++;
+
+                        if (infinite_retry) {
+                            grbl_sendf(CLIENT_ALL, "[MSG:Auto-reconnecting (attempt %d)...]\r\n",
+                                      _reconnect_attempt);
+                        } else {
+                            grbl_sendf(CLIENT_ALL, "[MSG:Auto-reconnecting (attempt %d/%d)...]\r\n",
+                                      _reconnect_attempt, max_retry);
+                        }
+
+                        WiFi.reconnect();
+                    } else {
+                        grbl_sendf(CLIENT_ALL, "[MSG:Auto-reconnect failed after %d attempts]\r\n",
+                                  max_retry);
+                        _auto_reconnect_enabled = false;
+                    }
+                }
                 break;
             default:
                 break;
         }
     }
```

#### WifiConfig.cpp - StartSTA() 函数

```diff
             if (WiFi.begin(SSID.c_str(), (password.length() > 0) ? password.c_str() : NULL)) {
                 COMMANDS::wait(200);

                 if (ConnectSTA2AP()) {
+                    // 连接成功后启用运行时自动重连
+                    _auto_reconnect_enabled = true;
+                    _reconnect_attempt = 0;
                     return true;
                 }
             } else {
                 grbl_send(CLIENT_ALL, "[MSG:WiFi.begin() failed]\r\n");
             }
```

#### WifiConfig.cpp - StopWiFi() 函数

```diff
     void WiFiConfig::StopWiFi() {
+        // 禁用运行时自动重连
+        _auto_reconnect_enabled = false;
+        _reconnect_attempt = 0;
+
         //Sanity check
         if ((WiFi.getMode() == WIFI_STA) || (WiFi.getMode() == WIFI_AP_STA)) {
             WiFi.disconnect(true);
         }
         // ...
     }
```

#### WifiConfig.cpp - begin() 函数

```diff
         } else if (wifiMode == ESP_WIFI_STA) {
             if (!StartSTA()) {
                 // STA connection failed after all retries, switch to AP mode
                 grbl_send(CLIENT_ALL, "[MSG:Switching to AP mode]\r\n");
+                _auto_reconnect_enabled = false;  // 切换到 AP 模式时禁用
                 StartAP();
             }
             //start services
             wifi_services.begin();
         } else {
             WiFi.mode(WIFI_OFF);
+            _auto_reconnect_enabled = false;  // WiFi 关闭时禁用
         }
```

### B. 参考资料

- [ESP32 Forum - WiFi.setAutoReconnect function](https://esp32.com/viewtopic.php?t=39116)
- [ESP32 Forum - Autoreconnect not working](https://esp32.com/viewtopic.php?t=39558)
- [ESP-IDF WiFi Driver Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- [Arduino-ESP32 WiFi API](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi)

### C. 相关源码文件路径

- `Grbl_Esp32/src/WebUI/WifiConfig.h`
- `Grbl_Esp32/src/WebUI/WifiConfig.cpp`
- `Grbl_Esp32/src/WebUI/WebSettings.cpp`

---

**文档版本**: 1.0
**最后更新**: 2026-02-02
**维护者**: 集成商/二次开发方
