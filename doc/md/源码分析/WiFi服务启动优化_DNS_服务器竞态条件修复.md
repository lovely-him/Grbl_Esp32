# WiFi 服务启动优化 - DNS 服务器竞态条件修复

**文档版本**: 1.0
**创建日期**: 2026-02-09
**修改文件**: `Grbl_Esp32/src/WebUI/WebServer.cpp`
**关联崩溃分析**: `doc/md/崩溃分析/5_ESP32崩溃分析_WiFiUDP断言失败.md`

---

## 1. 问题描述

### 崩溃现象

系统启动时发生崩溃：

```
[E][WiFiUdp.cpp:219] parsePacket(): could not receive data: 9
assertion "false && "WiFiUDP parsePacket error - check backtrace"" failed
abort() was called at PC 0x4015cbaf on core 1
```

### 调用链

```
clientCheckTask (Serial.cpp:245)
  └─→ WiFiConfig::handle() (WifiConfig.cpp:565)
      └─→ WiFiServices::handle() (WifiServices.cpp:172)
          └─→ Web_Server::handle() (WebServer.cpp:1554)
              └─→ DNSServer::processNextRequest() (DNSServer.cpp:56)
                  └─→ WiFiUDP::parsePacket() (WiFiUdp.cpp:220)
                      └─→ recvfrom() 返回 errno=9 (EBADF)
                      └─→ 断言触发 → 系统重启
```

### 根本原因

DNS 服务器在 WiFi AP IP 分配完成前就启动，导致 UDP socket 状态异常。

---

## 2. 竞态条件分析

### 竞态流程图

```
时间轴 →
═══════════════════════════════════════════════════════════════

T0: WiFi.mode(WIFI_AP)
    └─→ WiFi.getMode() == WIFI_AP ✓
    └─→ WiFi.softAPIP() == 0.0.0.0 ✗  ← 竞态窗口开始

T1: Web_Server::begin() 被调用
    └─→ 检查 WiFi.getMode() == WIFI_AP ✓
    └─→ 调用 dnsServer.start(DNS_PORT, "*", WiFi.softAPIP())
        └─→ 传入 IP = 0.0.0.0 ✗
        └─→ DNS 服务器启动失败或 socket 无效

T2: WiFi AP IP 分配完成
    └─→ WiFi.softAPIP() == 192.168.4.1 ✓  ← 竞态窗口结束
    └─→ 但 DNS 服务器已经启动（使用错误的 IP）

T3: clientCheckTask 调用 dnsServer.processNextRequest()
    └─→ WiFiUDP::parsePacket()
        └─→ recvfrom(udp_server, ...)
            └─→ 返回 -1, errno = 9 (EBADF)
            └─→ 断言触发 → 崩溃

═══════════════════════════════════════════════════════════════
竞态窗口：T0 到 T2，通常 100-500ms
```

### 问题代码

**WebServer.cpp:165-169** (修改前)：
```cpp
if (WiFi.getMode() == WIFI_AP) {
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());  // ← WiFi.softAPIP() 可能返回 0.0.0.0
    grbl_send(CLIENT_ALL, "[MSG:Captive Portal Started]\r\n");
}
```

**WebServer.cpp:1553-1555** (修改前)：
```cpp
if (WiFi.getMode() == WIFI_AP) {
    dnsServer.processNextRequest();  // ← 不知道 DNS 是否成功启动
}
```

---

## 3. 代码修改

### 3.1 添加状态跟踪

**位置**: `WebServer.cpp:52-56`

**修改前**：
```cpp
namespace WebUI {
    const byte DNS_PORT = 53;
    DNSServer  dnsServer;
}
```

**修改后**：
```cpp
namespace WebUI {
    const byte DNS_PORT = 53;
    DNSServer  dnsServer;
    bool       dnsServerStarted = false;  // 跟踪 DNS 服务器启动状态
}
```

**说明**：添加状态标志，记录 DNS 服务器是否成功启动。

---

### 3.2 等待 AP IP 分配

**位置**: `WebServer.cpp:165-190`

**修改前**：
```cpp
#ifdef ENABLE_CAPTIVE_PORTAL
    if (WiFi.getMode() == WIFI_AP) {
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
        grbl_send(CLIENT_ALL, "[MSG:Captive Portal Started]\r\n");
        _webserver->on("/generate_204", HTTP_ANY, handle_root);
        _webserver->on("/gconnectivitycheck.gstatic.com", HTTP_ANY, handle_root);
        _webserver->on("/fwlink/", HTTP_ANY, handle_root);
    }
#endif
```

**修改后**：
```cpp
#ifdef ENABLE_CAPTIVE_PORTAL
    if (WiFi.getMode() == WIFI_AP) {
        // 等待 WiFi AP 完全初始化
        IPAddress apIP = WiFi.softAPIP();
        int retry = 0;
        while (apIP == IPAddress(0, 0, 0, 0) && retry < 50) {
            delay(100);
            apIP = WiFi.softAPIP();
            retry++;
        }

        if (apIP != IPAddress(0, 0, 0, 0)) {
            dnsServer.start(DNS_PORT, "*", apIP);
            dnsServerStarted = true;
            grbl_send(CLIENT_ALL, "[MSG:Captive Portal Started]\r\n");
            _webserver->on("/generate_204", HTTP_ANY, handle_root);
            _webserver->on("/gconnectivitycheck.gstatic.com", HTTP_ANY, handle_root);
            _webserver->on("/fwlink/", HTTP_ANY, handle_root);
        } else {
            grbl_send(CLIENT_ALL, "[MSG:Captive Portal Failed - AP IP not ready]\r\n");
            dnsServerStarted = false;
        }
    }
#endif
```

**说明**：等待 AP IP 分配完成（最多 5 秒），验证 IP 有效后再启动 DNS 服务器。

---

### 3.3 运行时状态检查

**位置**: `WebServer.cpp:1552-1568`

**修改前**：
```cpp
#ifdef ENABLE_CAPTIVE_PORTAL
    if (WiFi.getMode() == WIFI_AP) {
        dnsServer.processNextRequest();
    }
#endif
```

**修改后**：
```cpp
#ifdef ENABLE_CAPTIVE_PORTAL
    if (dnsServerStarted && WiFi.getMode() == WIFI_AP) {
        IPAddress apIP = WiFi.softAPIP();
        if (apIP != IPAddress(0, 0, 0, 0)) {
            dnsServer.processNextRequest();
        } else {
            dnsServer.stop();
            dnsServerStarted = false;
            grbl_send(CLIENT_ALL, "[MSG:Captive Portal Stopped - AP IP lost]\r\n");
        }
    }
#endif
```

**说明**：检查 DNS 服务器状态和 AP IP 有效性，自动处理异常情况。

---

### 3.4 资源清理

**位置**: `WebServer.cpp:230-237`

**修改前**：
```cpp
void Web_Server::end() {
    _setupdone = false;
#ifdef ENABLE_SSDP
    SSDP.end();
#endif
    // ...
}
```

**修改后**：
```cpp
void Web_Server::end() {
    _setupdone = false;
#ifdef ENABLE_CAPTIVE_PORTAL
    if (dnsServerStarted) {
        dnsServer.stop();
        dnsServerStarted = false;
    }
#endif
#ifdef ENABLE_SSDP
    SSDP.end();
#endif
    // ...
}
```

**说明**：停止 DNS 服务器并重置状态标志。

---

**修改总结**：
- 新增代码：+32 行
- 删除代码：-7 行
- 修改位置：4 处
- 修改完成：2026-02-09
