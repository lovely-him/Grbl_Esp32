# ESP32 WiFi 主机名设置分析与实现

## 问题背景

在 Grbl_ESP32 项目中，设备连接到路由器后，路由器管理界面显示的设备名称是 `espressif`（ESP32 默认名称），而不是机器配置文件中定义的 `MACHINE_NAME`（如 `Genmitsu_Kiosk_V07_HIM`）。

## 核心概念区分

### 1. mDNS 主机名 vs DHCP 客户端标识符

| 概念 | 用途 | 显示位置 | 设置方式 |
|------|------|----------|----------|
| **mDNS 主机名** | 本地网络服务发现 | `http://grblesp.local/` | `[ESP112]` |
| **DHCP 客户端标识符** | DHCP 协议中的设备标识 | 路由器管理界面的设备列表 | `WiFi.setHostname()` |

**关键发现**：这是两个完全独立的系统！
- mDNS 用于本地域名解析（已经正常工作）
- DHCP 主机名用于路由器识别设备（这是我们要修改的）

## 问题根源分析

### 原始代码的错误

在 `WifiConfig.cpp` 的 `StartSTA()` 函数中：

```cpp
// ❌ 错误的实现（原始代码）
WiFi.mode(WIFI_STA);
// ... 其他初始化代码 ...
String h = wifi_hostname->get();
WiFi.setHostname(h.c_str());  // ⚠️ 时机错误！
WiFi.begin(ssid, password);
```

**问题**：在 `WiFi.mode(WIFI_STA)` 之后立即调用 `WiFi.setHostname()` 会失败，因为：
1. LwIP 网络接口（netif）尚未完全初始化
2. 底层 `tcpip_adapter_set_hostname()` 返回 `ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY`

### 正确的时机

根据 [ESP32 官方文档](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html) 和 [腾讯云开发者笔记](https://cloud.tencent.com/developer/ask/sof/116024168)：

> **"Upon receiving this event (`WIFI_EVENT_STA_START`), the event task will initialize the LwIP network interface (netif)."**

**关键时机**：必须在 `SYSTEM_EVENT_STA_START` 事件触发**之后**设置主机名。

## 解决方案实现

### 修改 1：在 WiFi 事件处理器中设置主机名

**文件**：`Grbl_Esp32/src/WebUI/WifiConfig.cpp`

**位置**：`WiFiEvent()` 函数（第 210-236 行）

```cpp
void WiFiConfig::WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_START:
            // ✅ 在 WiFi STA 启动后设置主机名（关键时机！）
            // 此时 LwIP 网络接口已初始化，可以安全设置主机名
            // 参考：https://cloud.tencent.com/developer/ask/sof/116024168
            {
                const char* c_str = MACHINE_NAME;
                if (WiFi.setHostname(c_str)) {
                    grbl_sendf(CLIENT_ALL, "[MSG:Hostname set to: %s]\r\n", c_str);
                } else {
                    grbl_sendf(CLIENT_ALL, "[MSG:Failed to set hostname: %s]\r\n", c_str);
                }
            }
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            grbl_sendf(CLIENT_ALL, "[MSG:Connected with %s]\r\n", WiFi.localIP().toString().c_str());
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            grbl_send(CLIENT_ALL, "[MSG:Disconnected]\r\n");
            break;
        default:
            break;
    }
}
```

**关键点**：
1. 使用 `SYSTEM_EVENT_STA_START` 事件（而不是 `SYSTEM_EVENT_STA_GOT_IP`）
2. 此时 LwIP 网络接口已初始化，但尚未开始 DHCP 协商
3. 主机名会被包含在 DHCP 请求中（DHCP Option 12）

### 修改 2：移除 StartSTA() 中的主机名设置

**文件**：`Grbl_Esp32/src/WebUI/WifiConfig.cpp`

**位置**：`StartSTA()` 函数（第 304-388 行）

```cpp
bool WiFiConfig::StartSTA() {
    // ... 省略其他代码 ...

    WiFi.mode(WIFI_STA);

    // ❌ 已删除（时机不对，会失败）
    // String h = wifi_hostname->get();
    // WiFi.setHostname(h.c_str());

    // ... 继续其他初始化 ...
    WiFi.begin(SSID.c_str(), password.c_str());
}
```

**说明**：原始代码在 `WiFi.mode()` 之后立即调用 `WiFi.setHostname()`，但此时 LwIP 网络接口尚未初始化，导致设置失败。

## 关键发现：DEFAULT_HOSTNAME 的作用

在调试过程中发现，`DEFAULT_HOSTNAME` 主要用于：
1. **mDNS 服务**：`http://grblesp.local/` 访问
2. **Web UI 显示**：设置页面中的默认主机名
3. **NVS 存储初始化**：首次启动时的默认值

**重要**：修改 `DEFAULT_HOSTNAME` **不会直接影响**路由器显示的设备名称！
- 路由器显示的名称来自 DHCP 协议中的 Option 12
- 必须在 `SYSTEM_EVENT_STA_START` 事件中调用 `WiFi.setHostname()` 才能生效

因此，**唯一有效的修改是在 WiFi 事件处理器中设置主机名**（修改 1 和 2）。

## WiFi 事件序列图

```
用户调用 WiFi.begin()
    ↓
SYSTEM_EVENT_STA_START 触发
    ↓
LwIP 网络接口初始化完成
    ↓
【此时调用 WiFi.setHostname()】✅
    ↓
开始 DHCP 协商
    ↓
发送 DHCP DISCOVER（包含主机名 Option 12）
    ↓
接收 DHCP OFFER
    ↓
发送 DHCP REQUEST
    ↓
接收 DHCP ACK
    ↓
SYSTEM_EVENT_STA_GOT_IP 触发
    ↓
连接成功，路由器显示自定义主机名 ✅
```

## 常见误区

### 误区 1：在 WiFi.begin() 之前设置主机名

```cpp
// ❌ 错误：网络接口未初始化
WiFi.mode(WIFI_STA);
WiFi.setHostname("hostname");  // 失败！
WiFi.begin(ssid, password);
```

**问题**：`WiFi.mode()` 只是设置模式，不会初始化 LwIP 网络接口。

### 误区 2：在 SYSTEM_EVENT_STA_GOT_IP 事件中设置

```cpp
// ❌ 错误：DHCP 协商已完成
case SYSTEM_EVENT_STA_GOT_IP:
    WiFi.setHostname("hostname");  // 太晚了！
    break;
```

**问题**：此时 DHCP 协商已完成，主机名已经发送给路由器，修改无效。

## 验证方法

### 1. 查看串口日志

```
[MSG:Hostname set to: Genmitsu_Kiosk_V07_HIM]
[MSG:Connecting YourSSID (attempt 1/3)]
[MSG:Connected with 192.168.1.100]
```

### 2. 检查路由器管理界面

- 登录路由器管理页面
- 查看 DHCP 客户端列表或已连接设备列表
- 应该显示：`Genmitsu_Kiosk_V07_HIM`（而不是 `espressif`）

**注意**：
- 路由器可能有缓存，首次生效可能需要等待几分钟或重启路由器
- 不同品牌路由器的显示位置可能不同（通常在"已连接设备"、"DHCP 客户端列表"等菜单中）

## 技术细节

### DHCP Option 12 (Hostname)

DHCP 协议中，客户端可以在 DHCP DISCOVER 和 DHCP REQUEST 消息中包含 Option 12（主机名）：

```
DHCP DISCOVER
├── Option 53: DHCP Message Type = DISCOVER
├── Option 12: Host Name = "Genmitsu_Kiosk_V07_HIM"  ⭐
├── Option 55: Parameter Request List
└── ...
```

路由器收到后，会将这个主机名与分配的 IP 地址关联，并显示在设备列表中。

### ESP32 底层实现

Arduino ESP32 的 `WiFi.setHostname()` 最终调用：

```c
// ESP-IDF API
esp_err_t tcpip_adapter_set_hostname(tcpip_adapter_if_t tcpip_if, const char *hostname);
```

**要求**：
1. 网络接口必须已初始化（`TCPIP_ADAPTER_IF_STA` 存在）
2. 主机名长度 ≤ 32 字符
3. 只能包含字母、数字和连字符（`-`）
4. 不能以连字符开头或结尾

### 为什么 SYSTEM_EVENT_STA_START 是正确时机？

根据 ESP-IDF 文档：

> **WIFI_EVENT_STA_START**: The station has been started, and the LwIP network interface has been initialized.

此时：
- ✅ LwIP 网络接口已创建
- ✅ `tcpip_adapter_set_hostname()` 可以成功调用
- ✅ DHCP 客户端尚未启动，主机名会被包含在 DHCP 请求中
- ❌ 如果在此之前调用，会返回 `ESP_ERR_TCPIP_ADAPTER_IF_NOT_READY`
- ❌ 如果在 DHCP 完成后调用，主机名不会发送给路由器

## 相关资源

### 官方文档
- [ESP-IDF WiFi API Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html)
- [Arduino ESP32 WiFi API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html)

### GitHub Issues
- [#2537 - Hostname not sent via DHCP request](https://github.com/espressif/arduino-esp32/issues/2537)
- [#3398 - How to set DHCP Client Name for ESP?](https://github.com/espressif/arduino-esp32/issues/3398)

### 参考文章
- [腾讯云开发者 - ESP32 WiFi 主机名设置](https://cloud.tencent.com/developer/ask/sof/116024168) ⭐ 关键参考
- [Random Nerd Tutorials - ESP32 Custom Hostname](https://randomnerdtutorials.com/esp32-set-custom-hostname-arduino/)
- [Deep Blue Embedded - ESP32 Hostname Change](https://deepbluembedded.com/esp32-hostname-change-arduino/)

## 总结

### 问题本质
ESP32 设置 WiFi 主机名的关键在于**时机**，而不是 API 调用方式。

### 解决方案核心
在 `SYSTEM_EVENT_STA_START` 事件中调用 `WiFi.setHostname()`，确保：
1. LwIP 网络接口已初始化
2. DHCP 协商尚未开始
3. 主机名会被包含在 DHCP 请求中

### 最佳实践
1. 使用 WiFi 事件处理器设置主机名
2. 不要在 `WiFi.begin()` 之前或之后直接调用 `WiFi.setHostname()`
3. 主机名应符合 RFC 952 规范（字母、数字、连字符，≤32 字符）
4. 路由器可能有缓存，首次生效可能需要等待或重启路由器

---

**文档版本**：1.0
**最后更新**：2026-01-28
**作者**：Claude (Anthropic)
**项目**：Grbl_ESP32 - Wainlux K11 HIM 定制版