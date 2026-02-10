# ESP32 Arduino mDNS 已知问题汇总

## 概述

经过对 GitHub Issues、ESP32 论坛和技术社区的广泛调研，**ESP32 的 mDNS 功能确实存在多个已知的可靠性问题**。这些问题在社区中被广泛报告，影响范围从间歇性失败到完全无响应。

---

## 🔴 主要问题列表

### 1. mDNS 在 2 分钟后停止响应

**问题描述**:
- ESP32 启动后，mDNS 正常工作约 2 分钟
- 2 分钟后，设备停止响应 mDNS 查询
- 设备本身仍在线且功能正常，只是 mDNS 失效

**根本原因**:
- ESP32 在初始化时广播 mDNS 记录，TTL 设置为 120 秒（2 分钟）
- TTL 过期后，ESP32 不再主动刷新 mDNS 记录
- 客户端缓存过期后，应该重新查询，但 ESP32 不响应新查询

**影响范围**:
- 多个 ESP32 开发板（LOLIN32, MH-ET-LIVE, DOIT devkit v1 等）
- ESP8266 也存在类似问题

**状态**: ⚠️ **未解决** (Issue #4406 仍然开放)

**相关链接**:
- [mDNS stops working after two minutes · Issue #4406](https://github.com/espressif/arduino-esp32/issues/4406)

**临时解决方案**:
```cpp
// 方案 1: 定期重启 mDNS (不推荐，可能导致崩溃)
void loop() {
    static unsigned long lastRefresh = 0;
    if (millis() - lastRefresh > 60000) {  // 每 60 秒
        MDNS.end();
        delay(100);
        MDNS.begin("hostname");
        MDNS.addService("http", "tcp", 80);
        lastRefresh = millis();
    }
}

// 方案 2: 使用静态 IP + DNS 记录（绕过 mDNS）
WiFi.config(staticIP, gateway, subnet);
```

---

### 2. WiFi 睡眠模式导致 mDNS 不可靠

**问题描述**:
- 主机名解析 (`hostname.local`) 正常工作
- 服务发现 (`_http._tcp.local`) 极不可靠
- 查询响应时断时续

**根本原因**:
- ESP32 的 WiFi 睡眠模式会导致设备错过 mDNS 查询数据包
- 当 WiFi 进入睡眠状态时，UDP 多播数据包可能被丢弃
- mDNS 查询通常是短暂的，设备唤醒时可能已经错过

**影响范围**:
- 所有启用 WiFi 睡眠模式的 ESP32 设备
- 特别影响服务发现功能

**状态**: ✅ **已修复** (在 Arduino-ESP32 v2.0.7 中修复)

**相关链接**:
- [mDNS service responses are unreliable unless toggling WiFi.setSleep(false) · Issue #7156](https://github.com/espressif/arduino-esp32/issues/7156)
- [ESP-IDF issue #9099](https://github.com/espressif/esp-idf/issues/9099)

**解决方案**:
```cpp
// 方案 1: 升级到 Arduino-ESP32 v2.0.7 或更高版本
// platformio.ini
[env:esp32]
platform = espressif32@6.0.0  ; 对应 Arduino-ESP32 v2.0.7+

// 方案 2: 禁用 WiFi 睡眠模式（旧版本）
WiFi.setSleep(false);  // 注意：会影响 WiFi/Bluetooth 共存
```

---

### 3. mDNS 大部分时间失败（57% 失败率）

**问题描述**:
- 连续 30 次连接测试：前 7 次失败，中间 13 次成功，最后 10 次失败
- 总体失败率 57%
- 同样的代码在 ESP8266 上 100% 成功

**根本原因**:
- 可能与 Web 服务器实现有关（"request handler not found" 错误）
- 也可能是 mDNS 响应时序问题

**影响范围**:
- Arduino-ESP32 v1.0.6
- ESP32 Dev Module

**状态**: ⚠️ **未确认** (Issue #5446 因缺乏后续信息而关闭)

**相关链接**:
- [MDNS fails the majority of the time (HelloServer included) · Issue #5446](https://github.com/espressif/arduino-esp32/issues/5446)

---

### 4. Android 应用的特殊 mDNS 数据包导致 ESP32 崩溃

**问题描述**:
- 使用 Android "Service Browser" 应用浏览 `_http._tcp` 服务时
- 选择 ESP32 设备名称后，ESP32 立即崩溃重启
- 错误：`Guru Meditation Error: Core 0 panic'ed (LoadProhibited)`

**根本原因**:
- mDNS 数据包解析代码中的空指针解引用
- `_mdns_get_service_item_instance` 函数中的 `strcasecmp` 调用失败
- EXCVADDR: 0x00000000（尝试访问空指针）

**影响范围**:
- ESP32 和 ESP32-S2
- Arduino-ESP32 v2.0.2, v2.0.3-rc1

**状态**: ✅ **已修复** (在 ESP-IDF master 分支修复，需升级固件)

**相关链接**:
- [MDNS special packet from Android App crashes and reboots ESP32/ESP32-S2 · Issue #6368](https://github.com/espressif/arduino-esp32/issues/6368)
- [ESP-IDF issue #8307](https://github.com/espressif/esp-idf/issues/8307)

**解决方案**:
```cpp
// 升级到包含修复的版本
// platformio.ini
[env:esp32]
platform = espressif32@6.0.0 或更高版本
```

---

### 5. mDNS 在长时间运行后停止工作

**问题描述**:
- ESP32 运行数小时或数天后，mDNS 突然停止响应
- 需要重启设备才能恢复
- 设备其他功能正常

**根本原因**:
- 可能是内存泄漏
- 可能是 mDNS 任务崩溃或死锁
- 可能与 WiFi 重连事件处理有关

**影响范围**:
- 长时间运行的 ESP32 设备
- 特别是有频繁 WiFi 重连的环境

**状态**: ⚠️ **部分解决** (某些情况下通过固件升级改善)

**相关链接**:
- [ESP32 stops responding after a week](https://stackoverflow.com/questions/73177638/esp32-stops-responding-after-a-week)
- [MDNS transmit failure](https://esp32.com/viewtopic.php?t=47055)
- [ESP32 devices show offline, mDNS not working correctly · Issue #6311](https://github.com/esphome/issues/issues/6311)

**临时解决方案**:
```cpp
// 监控 WiFi 事件并重启 mDNS
void WiFiEvent(WiFiEvent_t event) {
    switch(event) {
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.println("WiFi connected");
            // 重启 mDNS
            MDNS.end();
            delay(100);
            MDNS.begin("hostname");
            MDNS.addService("http", "tcp", 80);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi disconnected");
            break;
    }
}

void setup() {
    WiFi.onEvent(WiFiEvent);
}
```

---

### 6. OTA 更新后 mDNS 停止工作

**问题描述**:
- 通过 OTA (Over-The-Air) 更新固件后
- mDNS 功能失效
- 需要硬重启才能恢复

**根本原因**:
- OTA 更新过程中 mDNS 服务未正确关闭
- 更新后 mDNS 初始化失败

**影响范围**:
- 使用 OTA 更新的 ESP32 项目

**状态**: ⚠️ **未完全解决**

**相关链接**:
- [Esp32 mDNS stop working with OTA](https://community.platformio.org/t/esp32-mdns-stop-working-with-ota/30881)
- [ESP32 Over-The-Air network port wont appear](https://forum.arduino.cc/t/solved-esp32-over-the-air-network-port-wont-appear/1330635)

**解决方案**:
```cpp
// OTA 更新前关闭 mDNS
void performOTA() {
    MDNS.end();  // 关闭 mDNS

    // 执行 OTA 更新
    ArduinoOTA.begin();
    // ...

    // 更新完成后会自动重启
}

// 启动时确保 mDNS 正确初始化
void setup() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    delay(1000);  // 等待 WiFi 稳定

    if (!MDNS.begin("hostname")) {
        Serial.println("Error starting mDNS");
        ESP.restart();  // 重启重试
    }
}
```

---

### 7. BLE 和 mDNS 冲突

**问题描述**:
- 同时使用 BLE (Bluetooth Low Energy) 和 mDNS 时
- mDNS 响应变得不稳定
- WiFi 连接也可能受影响

**根本原因**:
- ESP32 的 WiFi 和 Bluetooth 共享同一个 2.4GHz 射频
- BLE 扫描会干扰 WiFi 接收
- mDNS 多播数据包可能在 BLE 活动期间丢失

**影响范围**:
- 同时使用 WiFi 和 BLE 的项目

**状态**: ⚠️ **已知限制** (硬件限制，无完美解决方案)

**相关链接**:
- [esp32_ble_tracker breaks mDNS and wifi stability · Issue #2690](https://github.com/esphome/issues/issues/2690)

**解决方案**:
```cpp
// 方案 1: 时分复用 BLE 和 WiFi
void loop() {
    // BLE 扫描阶段
    if (blePhase) {
        MDNS.end();  // 暂停 mDNS
        // 执行 BLE 操作
        delay(5000);
        blePhase = false;
    }
    // WiFi/mDNS 阶段
    else {
        MDNS.begin("hostname");
        // 执行 WiFi 操作
        delay(10000);
        blePhase = true;
    }
}

// 方案 2: 降低 BLE 扫描频率
BLEScan* pBLEScan = BLEDevice::getScan();
pBLEScan->setInterval(1000);  // 增加扫描间隔
pBLEScan->setWindow(100);     // 减少扫描窗口
```

---

## 📊 问题统计

| 问题 | 严重程度 | 状态 | 影响版本 | 修复版本 |
|------|---------|------|---------|---------|
| 2 分钟后停止响应 | 🔴 高 | 未解决 | 所有版本 | - |
| WiFi 睡眠模式干扰 | 🟡 中 | 已修复 | < v2.0.7 | v2.0.7+ |
| 高失败率 (57%) | 🟡 中 | 未确认 | v1.0.6 | - |
| Android 数据包崩溃 | 🔴 高 | 已修复 | v2.0.2-2.0.3 | v2.0.4+ |
| 长时间运行失效 | 🟡 中 | 部分解决 | 所有版本 | 部分改善 |
| OTA 后失效 | 🟡 中 | 未完全解决 | 所有版本 | - |
| BLE 冲突 | 🟢 低 | 已知限制 | 所有版本 | 硬件限制 |

---

## 🛠️ 通用解决方案和最佳实践

### 1. 升级到最新版本

```ini
; platformio.ini
[env:esp32]
platform = espressif32@6.5.0  ; 使用最新稳定版本
framework = arduino
board = esp32dev
```

### 2. 禁用 WiFi 睡眠模式

```cpp
void setup() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    WiFi.setSleep(false);  // 禁用 WiFi 睡眠

    if (!MDNS.begin("hostname")) {
        Serial.println("Error starting mDNS");
    }
}
```

### 3. 监控和自动恢复

```cpp
unsigned long lastMdnsCheck = 0;
const unsigned long MDNS_CHECK_INTERVAL = 60000;  // 60 秒

void loop() {
    // 定期检查 mDNS 是否正常
    if (millis() - lastMdnsCheck > MDNS_CHECK_INTERVAL) {
        if (!pingMdns()) {
            Serial.println("mDNS not responding, restarting...");
            MDNS.end();
            delay(100);
            MDNS.begin("hostname");
            MDNS.addService("http", "tcp", 80);
        }
        lastMdnsCheck = millis();
    }
}

bool pingMdns() {
    // 实现 mDNS 健康检查
    // 可以通过查询自己的主机名来验证
    return true;  // 简化示例
}
```

### 4. 使用静态 IP 作为备选方案

```cpp
void setup() {
    // 配置静态 IP
    IPAddress local_IP(192, 168, 1, 100);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);

    if (!WiFi.config(local_IP, gateway, subnet)) {
        Serial.println("STA Failed to configure");
    }

    WiFi.begin(ssid, password);

    // 同时启用 mDNS（作为便利功能）
    MDNS.begin("hostname");

    // 在 Web 界面显示两种访问方式
    Serial.println("Access via:");
    Serial.println("  http://hostname.local");
    Serial.println("  http://192.168.1.100");
}
```

### 5. 添加详细日志

```cpp
void setup() {
    Serial.begin(115200);

    // 启用 ESP32 核心调试日志
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set("mdns", ESP_LOG_VERBOSE);

    WiFi.begin(ssid, password);

    if (!MDNS.begin("hostname")) {
        Serial.println("Error starting mDNS");
        Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
    } else {
        Serial.println("mDNS started successfully");
    }
}
```

---

## 🔍 调试技巧

### 1. 使用 Wireshark 抓包

```bash
# 过滤 mDNS 流量
mdns

# 查看特定主机的 mDNS 流量
mdns and ip.addr == 192.168.1.100
```

### 2. 使用命令行工具测试

```bash
# Linux/Mac
avahi-browse -a -t -r

# 查询特定主机
avahi-resolve -n hostname.local

# Windows (需要安装 Bonjour)
dns-sd -B _http._tcp local
```

### 3. 使用 Python 测试脚本

```python
from zeroconf import ServiceBrowser, Zeroconf

class MyListener:
    def add_service(self, zeroconf, type, name):
        print(f"Service {name} added")
        info = zeroconf.get_service_info(type, name)
        print(f"  Address: {info.parsed_addresses()}")
        print(f"  Port: {info.port}")

zeroconf = Zeroconf()
listener = MyListener()
browser = ServiceBrowser(zeroconf, "_http._tcp.local.", listener)

try:
    input("Press enter to exit...\n\n")
finally:
    zeroconf.close()
```

---

## 📚 参考资源

### GitHub Issues
- [mDNS stops working after two minutes · Issue #4406](https://github.com/espressif/arduino-esp32/issues/4406)
- [MDNS fails the majority of the time · Issue #5446](https://github.com/espressif/arduino-esp32/issues/5446)
- [mDNS service responses are unreliable · Issue #7156](https://github.com/espressif/arduino-esp32/issues/7156)
- [MDNS special packet crashes ESP32 · Issue #6368](https://github.com/espressif/arduino-esp32/issues/6368)
- [mDNS responding to requests unreliably · Issue #3121](https://github.com/espressif/esp-idf/issues/3121)

### 论坛讨论
- [MDNS transmit failure - ESP32 Forum](https://esp32.com/viewtopic.php?t=47055)
- [mDNS does not work - ESP32 Forum](https://esp32.com/viewtopic.php?t=7142)
- [Esp32 mDNS stop working with OTA - PlatformIO Community](https://community.platformio.org/t/esp32-mdns-stop-working-with-ota/30881)

### 技术文档
- [ESP-IDF mDNS API Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mdns.html)
- [RFC 6762 - Multicast DNS](https://tools.ietf.org/html/rfc6762)
- [RFC 6763 - DNS-Based Service Discovery](https://tools.ietf.org/html/rfc6763)

---

## 💡 结论

**ESP32 的 mDNS 功能确实存在可靠性问题**，主要表现为：

1. ✅ **部分问题已修复**：WiFi 睡眠模式干扰、Android 数据包崩溃等问题在新版本中已解决
2. ⚠️ **部分问题仍存在**：2 分钟 TTL 问题、长时间运行失效等问题尚未完全解决
3. 🔧 **有可用的解决方案**：通过禁用 WiFi 睡眠、定期重启 mDNS、使用静态 IP 备选等方法可以缓解问题

**建议**：
- 升级到 Arduino-ESP32 v2.0.7 或更高版本
- 在生产环境中同时提供静态 IP 和 mDNS 两种访问方式
- 实现 mDNS 健康检查和自动恢复机制
- 对于关键应用，考虑使用其他服务发现机制（如 SSDP、自定义 UDP 广播等）

---

**文档更新日期**: 2026-02-10
