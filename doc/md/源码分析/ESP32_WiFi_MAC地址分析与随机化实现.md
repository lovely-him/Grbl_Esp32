# ESP32 WiFi MAC 地址分析与随机化实现

**日期**: 2026-01-27
**项目**: Grbl_Esp32 - Wainlux K11 HIM
**作者**: Claude Code

---

## 1. 问题背景

用户询问 ESP32 在 STA 模式下连接 AP 热点时，MAC 地址是固定的还是随机的。

### 初步分析结论

经过代码分析和官方文档查证，得出以下结论：

- ✅ **ESP32 默认使用固定的 MAC 地址**（出厂时烧录在 eFuse 中）
- ❌ **ESP-IDF 没有内置的 MAC 地址随机化功能**
- ✅ **可以通过用户代码手动实现随机 MAC 地址**

---

## 2. 官方文档确认

### 2.1 ESP-IDF 官方文档

根据 [ESP-IDF 官方文档](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/misc_system_api.html)：

> "Espressif base MAC address is used. This base MAC address **is pre-programmed into the ESP32 eFuse in the factory during production**."

**关键点**：
- ESP32 每个芯片出厂时都有唯一的固定 MAC 地址
- WiFi STA 模式直接使用 base MAC 地址
- WiFi AP 模式使用 base MAC + 1
- Bluetooth 使用 base MAC + 2
- Ethernet 使用 base MAC + 3

### 2.2 ESP32 论坛讨论

根据 [ESP32 论坛讨论](https://www.esp32.com/viewtopic.php?t=23295)：

> "There is no option to set the ESP-IDF code to generate a random MAC address each time, but I can generate it myself in user code."

**结论**：
- ESP-IDF 没有自动随机 MAC 地址的功能
- 需要在用户代码中手动实现
- 可以使用 `esp_wifi_set_mac(WIFI_IF_STA, random_mac)` 手动设置

---

## 3. Grbl_Esp32 原始实现分析

### 3.1 原始代码位置

文件：[WifiConfig.cpp:289-349](../../Grbl_Esp32/src/WebUI/WifiConfig.cpp#L289-L349)

### 3.2 原始实现特点

```cpp
bool WiFiConfig::StartSTA() {
    // ...
    WiFi.mode(WIFI_STA);  // 设置为 STA 模式

    // 获取 STA 参数
    String h = wifi_hostname->get();
    WiFi.setHostname(h.c_str());

    // 开始连接（使用默认固定 MAC 地址）
    if (WiFi.begin(SSID.c_str(), password.c_str())) {
        // ...
    }
}
```

**关键点**：
- 没有调用任何 MAC 地址设置函数
- 直接使用 ESP32 出厂固定的 MAC 地址
- 每次连接 AP 时使用相同的 MAC 地址

---

## 4. 随机 MAC 地址实现

### 4.1 实现原理

根据 IEEE 802 标准，MAC 地址的第一个字节有两个特殊位：
- **Bit 0 (LSB)**: 单播/多播位（0=单播，1=多播）
- **Bit 1**: 全球唯一/本地管理位（0=全球唯一，1=本地管理）

随机 MAC 地址必须：
1. 设置 Bit 1 为 1（本地管理位）
2. 清除 Bit 0 为 0（单播位）
3. 随机化其他字节（通常是后三个字节）

### 4.2 实现代码

在 [WifiConfig.cpp:289-349](../../Grbl_Esp32/src/WebUI/WifiConfig.cpp#L289-L349) 的 `StartSTA()` 函数中添加：

```cpp
bool WiFiConfig::StartSTA() {
    int32_t retry_count = wifi_sta_retry_count->get();
    for (int32_t attempt = 1; attempt <= retry_count; attempt++) {
        // ... 原有代码 ...
        WiFi.mode(WIFI_STA);

        // 生成随机 MAC 地址（仅在第一次尝试时生成）
        if (attempt == 1) {
            uint8_t random_mac[6];
            esp_read_mac(random_mac, ESP_MAC_WIFI_STA);  // 读取基础 MAC

            // 设置本地管理位（bit 1 of first byte = 1）
            random_mac[0] |= 0x02;
            // 清除多播位（bit 0 of first byte = 0）
            random_mac[0] &= 0xFE;

            // 随机化后三个字节
            for (int i = 3; i < 6; i++) {
                random_mac[i] = (uint8_t)(esp_random() & 0xFF);
            }

            // 设置随机 MAC 地址
            esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, random_mac);
            if (err == ESP_OK) {
                grbl_sendf(CLIENT_ALL, "[MSG:Random MAC set: %s]\r\n", mac2str(random_mac));
            } else {
                grbl_sendf(CLIENT_ALL, "[MSG:Failed to set random MAC (error: %d)]\r\n", err);
            }
        }

        // ... 继续原有连接逻辑 ...
    }
}
```

### 4.3 实现要点

1. **仅在第一次尝试时生成随机 MAC**
   - 避免每次重试都生成新的 MAC 地址
   - 保持同一连接会话使用相同的随机 MAC

2. **基于原始 MAC 生成**
   - 保留前三个字节的厂商标识（OUI）部分
   - 仅随机化后三个字节（设备标识部分）

3. **符合 IEEE 802 标准**
   - 设置本地管理位（0x02）
   - 清除多播位（0xFE）

4. **错误处理**
   - 检查 `esp_wifi_set_mac()` 返回值
   - 打印成功或失败信息

---

## 5. machine_init 中的 WiFi 状态打印

### 5.1 实现位置

文件：[wainlux_k11_HIM.cpp:963](../../Grbl_Esp32/Custom/wainlux_k11_HIM.cpp#L963)

### 5.2 实现代码

```cpp
void machine_init() {
    LOG_INFO(TAG_INIT, "Wainlux K11 HIM Custom Init (Full Rewrite)");

    // 显式获取并打印 WiFi 状态
    wifi_mode_t wifi_mode = WiFi.getMode();
    const char* mode_str =
        (wifi_mode == WIFI_OFF) ? "OFF" :
        (wifi_mode == WIFI_STA) ? "STA" :
        (wifi_mode == WIFI_AP) ? "AP" :
        (wifi_mode == WIFI_AP_STA) ? "AP_STA" : "UNKNOWN";

    uint8_t mac[6];
    WiFi.macAddress(mac);
    LOG_INFO(TAG_INIT, "WiFi Mode: %s, MAC: %02X:%02X:%02X:%02X:%02X:%02X",
        mode_str, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // ... 继续原有初始化代码 ...
}
```

### 5.3 输出示例

```
[I][  1234][INIT] Wainlux K11 HIM Custom Init (Full Rewrite)
[I][  1235][INIT] WiFi Mode: OFF, MAC: 24:6F:28:12:34:56
```

---

## 6. 技术细节

### 6.1 MAC 地址结构

```
+--------+--------+--------+--------+--------+--------+
| Byte 0 | Byte 1 | Byte 2 | Byte 3 | Byte 4 | Byte 5 |
+--------+--------+--------+--------+--------+--------+
|<--- OUI (厂商标识) --->|<--- NIC (设备标识) --->|
```

**Byte 0 的位定义**：
```
Bit 7 6 5 4 3 2 1 0
    | | | | | | | |
    | | | | | | | +-- Bit 0: 单播/多播位 (0=单播, 1=多播)
    | | | | | | +---- Bit 1: 全球唯一/本地管理位 (0=全球唯一, 1=本地管理)
    | | | | | +------ Bit 2-7: OUI 的一部分
```

### 6.2 随机 MAC 地址示例

**原始固定 MAC**：
```
24:6F:28:12:34:56
```

**随机化后的 MAC**：
```
26:6F:28:A3:B7:C9
^                ^  ^  ^
|                |  |  |
Bit 1 设置为 1   随机化的后三个字节
(0x24 | 0x02 = 0x26)
```

### 6.3 API 调用时序

```
1. WiFi.mode(WIFI_STA)           // 设置为 STA 模式
2. esp_wifi_set_mac(...)         // 设置随机 MAC（必须在 WiFi.begin() 之前）
3. WiFi.setHostname(...)         // 设置主机名
4. WiFi.begin(SSID, password)    // 开始连接
```

**注意**：`esp_wifi_set_mac()` 必须在 `WiFi.begin()` 之前调用，否则会失败。

---

## 7. 潜在问题和注意事项

### 7.1 AP 兼容性

某些 AP 可能会拒绝本地管理的 MAC 地址连接，特别是：
- 企业级 WiFi（802.1X 认证）
- 启用了 MAC 地址过滤的 AP
- 某些路由器的安全策略

**解决方案**：
- 提供配置选项，允许用户禁用随机 MAC 功能
- 在连接失败时自动回退到固定 MAC

### 7.2 DHCP 租约问题

随机 MAC 地址会导致：
- 每次连接获得不同的 IP 地址
- DHCP 服务器租约表快速增长
- 静态 IP 绑定失效

**解决方案**：
- 在同一会话中保持相同的随机 MAC
- 提供选项在重启后保持相同的随机 MAC（存储在 NVS）

### 7.3 隐私保护效果

随机 MAC 地址可以：
- ✅ 防止基于 MAC 地址的设备跟踪
- ✅ 增强隐私保护
- ❌ 无法防止基于其他特征的指纹识别（如 DHCP 选项、HTTP User-Agent）

---

## 8. 测试建议

### 8.1 功能测试

1. **验证随机 MAC 生成**
   - 检查日志输出的随机 MAC 地址
   - 确认 Bit 1 已设置（第一个字节为偶数且 +2）
   - 确认后三个字节每次重启都不同

2. **验证连接功能**
   - 测试连接到不同类型的 AP（家用路由器、企业 WiFi）
   - 测试 DHCP 和静态 IP 配置
   - 测试重连和重试逻辑

3. **验证状态打印**
   - 检查 `machine_init()` 中的 WiFi 状态输出
   - 确认 MAC 地址格式正确

### 8.2 兼容性测试

1. **不同 AP 类型**
   - 家用路由器（TP-Link, Netgear, etc.）
   - 企业级 AP（Cisco, Ubiquiti, etc.）
   - 移动热点（手机、MiFi）

2. **不同安全模式**
   - 开放网络（无密码）
   - WPA2-PSK
   - WPA3-SAE
   - 企业认证（802.1X）

---

## 9. 代码修改总结

### 9.1 修改文件列表

1. **[WifiConfig.cpp](../../Grbl_Esp32/src/WebUI/WifiConfig.cpp)**
   - 在 `StartSTA()` 函数中添加随机 MAC 地址生成逻辑
   - 位置：第 289-349 行

2. **[wainlux_k11_HIM.cpp](../../Grbl_Esp32/Custom/wainlux_k11_HIM.cpp)**
   - 在 `machine_init()` 函数开头添加 WiFi 状态打印
   - 位置：第 963 行

### 9.2 修改行数统计

- **WifiConfig.cpp**: +28 行（随机 MAC 生成逻辑）
- **wainlux_k11_HIM.cpp**: +12 行（WiFi 状态打印）
- **总计**: +40 行

---

## 10. 参考资料

### 10.1 官方文档

- [ESP-IDF MAC Address Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/misc_system_api.html)
- [ESP32 WiFi API Reference](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_wifi.html)

### 10.2 社区讨论

- [ESP32 Forum: Can ESP-IDF use a randomised MAC address?](https://www.esp32.com/viewtopic.php?t=23295)
- [ESP32 Forum: Set MAC Address](https://esp32.com/viewtopic.php?t=2182)

### 10.3 技术标准

- [IEEE 802 MAC Address Standards](https://standards.ieee.org/products-programs/regauth/)
- [RFC 7042: IANA Considerations and IETF Protocol and Documentation Usage for IEEE 802 Parameters](https://datatracker.ietf.org/doc/html/rfc7042)

---

## 11. 后续优化建议

### 11.1 配置选项

添加编译时或运行时配置选项：

```cpp
// Config.h
#define ENABLE_RANDOM_MAC 1  // 0=禁用, 1=启用

// Settings.h
IntSetting* wifi_random_mac_enabled;  // 运行时配置
```

### 11.2 MAC 地址持久化

将随机 MAC 地址存储在 NVS 中，重启后保持不变：

```cpp
// 读取存储的随机 MAC
if (nvs_get_blob(nvs_handle, "random_mac", random_mac, &length) == ESP_OK) {
    // 使用存储的 MAC
} else {
    // 生成新的随机 MAC 并存储
}
```

### 11.3 回退机制

在连接失败时自动回退到固定 MAC：

```cpp
if (!ConnectSTA2AP()) {
    // 回退到固定 MAC 重试
    uint8_t original_mac[6];
    esp_read_mac(original_mac, ESP_MAC_WIFI_STA);
    esp_wifi_set_mac(WIFI_IF_STA, original_mac);
    // 重试连接
}
```

---

**文档版本**: 1.0
**最后更新**: 2026-01-27
