# Grbl_Esp32 WiFi STA 配置完全指南

## 问题描述

用户配置了 STA SSID 和密码后，重启设备不会自动连接到 WiFi 网络。

```
$ESP100=MT3000
ok
$ESP101=******
ok
[ESP444]RESTART
```

**问题原因**: 缺少关键步骤 - **设置 WiFi 模式为 STA**

---

## 根本原因分析

### 1. WiFi 模式配置

Grbl_Esp32 支持多种无线电模式，通过 `[ESP110]` 命令控制：

| 模式 | 说明 | 用途 |
|------|------|------|
| `NONE` | 关闭无线电 | 节能模式 |
| `STA` | Station 模式（客户端） | 连接到现有 WiFi 网络 |
| `AP` | Access Point 模式（热点） | 创建独立 WiFi 热点 |
| `BT` | 蓝牙模式 | 使用蓝牙连接 |

### 2. 默认模式

根据 [Config.h:150-160](../../Grbl_Esp32/src/Config.h#L150-L160) 的代码：

```cpp
#ifdef ENABLE_WIFI
#    ifdef CONNECT_TO_SSID
const int DEFAULT_RADIO_MODE = ESP_WIFI_STA;  // 如果定义了 CONNECT_TO_SSID
#    else
const int DEFAULT_RADIO_MODE = ESP_WIFI_AP;   // 否则默认为 AP 模式
#    endif
#else
const int DEFAULT_RADIO_MODE = ESP_RADIO_OFF; // 或关闭
#endif
```

**开源版本默认行为**:
- 如果编译时未定义 `CONNECT_TO_SSID` 宏，默认为 **AP 模式**（热点模式）
- 即使配置了 STA SSID 和密码，系统仍然运行在 AP 模式下
- 必须手动切换到 STA 模式才能连接到外部 WiFi

**原厂固件可能的差异**:
- 编译时定义了 `CONNECT_TO_SSID` 宏
- 修改了 `DEFAULT_RADIO_MODE` 的默认值
- 在 `machine_init()` 中强制设置了 WiFi 模式

### 3. 配置存储机制

WiFi 配置通过 ESP32 的 NVS (Non-Volatile Storage) 存储：

| 配置项 | ESP 命令 | 存储位置 | 说明 |
|--------|----------|----------|------|
| STA SSID | `[ESP100]` | NVS: `Sta/SSID` | 目标网络名称 |
| STA 密码 | `[ESP101]` | NVS: `Sta/Password` | 网络密码 |
| WiFi 模式 | `[ESP110]` | NVS: `Radio/Mode` | **决定是否启用 STA** |
| 主机名 | `[ESP112]` | NVS: `Sta/Hostname` | mDNS 主机名 |

**关键点**:
- `[ESP100]` 和 `[ESP101]` 只是**保存**了 STA 的凭据
- `[ESP110]` 才是**激活** STA 模式的开关
- 重启后系统根据 `[ESP110]` 的值决定启动哪种模式

---

## 完整配置步骤

### 方法 1: 串口配置（推荐）

```bash
# 步骤 1: 设置 STA SSID
[ESP100]MT3000
# 响应: ok

# 步骤 2: 设置 STA 密码
[ESP101]your_password_here
# 响应: ok

# 步骤 3: 设置 WiFi 模式为 STA（关键步骤！）
[ESP110]STA
# 响应: ok

# 步骤 4: 重启使配置生效
[ESP444]RESTART
# 响应: [MSG:Restart ongoing]

# 步骤 5: 等待 5-10 秒后查询连接状态
[ESP111]
# 响应示例: 192.168.1.100
```

### 方法 2: 使用 Grbl 设置命令

```bash
# 使用 $ 命令（等效于 ESP 命令）
$ESP100=MT3000
$ESP101=your_password
$ESP110=STA
[ESP444]RESTART
```

### 方法 3: 立即生效（无需重启）

```bash
# 配置 SSID 和密码
[ESP100]MT3000
[ESP101]your_password

# 立即切换到 STA 模式（无需重启）
[ESP115]ON
# 注意: ESP115 是立即开关 WiFi，会尝试使用已保存的配置连接
```

---

## 验证和故障排除

### 1. 查询当前 IP 地址

```bash
[ESP111]
```

**可能的响应**:
- `192.168.x.x` - 成功连接，显示分配的 IP
- `0.0.0.0` - 未连接或 DHCP 失败
- `192.168.0.1` - 仍在 AP 模式（默认 AP IP）

### 2. 查询当前主机名

```bash
[ESP112]
```

**响应示例**: `grblesp`

### 3. 查询完整 WiFi 信息

通过串口监视器观察启动日志，会显示：

```
[MSG:Mode=STA:SSID=MT3000:Status=Connected:IP=192.168.1.100:MAC=XX-XX-XX-XX-XX-XX]
```

### 4. 常见问题排查

#### 问题 1: 配置后仍然是 AP 模式

**症状**:
```
[ESP111]
192.168.0.1  # AP 模式的默认 IP
```

**原因**: 忘记执行 `[ESP110]STA`

**解决**:
```bash
[ESP110]STA
[ESP444]RESTART
```

#### 问题 2: 重启后无法连接

**症状**:
```
[ESP111]
0.0.0.0
```

**可能原因**:
1. SSID 或密码错误
2. WiFi 信号太弱
3. 路由器 MAC 过滤
4. DHCP 服务器故障

**排查步骤**:
```bash
# 1. 确认配置已保存
[ESP100]
# 应返回: MT3000

[ESP110]
# 应返回: STA

# 2. 检查密码长度（至少 8 位）
[ESP101]
# 应返回: ******** (隐藏显示)

# 3. 尝试重新配置
[ESP100]MT3000
[ESP101]correct_password
[ESP110]STA
[ESP444]RESTART
```

#### 问题 3: 密码包含特殊字符

**症状**: 密码正确但无法连接

**原因**: 某些特殊字符可能需要转义

**解决**:
- 避免使用 `<`, `>`, `&`, `"`, `'` 等特殊字符
- 或使用 Web 界面配置（自动处理转义）

#### 问题 4: 连接后无法访问

**症状**: 获得 IP 但无法 ping 通

**可能原因**:
1. 路由器 AP 隔离已启用
2. 防火墙阻止
3. 不在同一子网

**解决**:
```bash
# 检查路由器设置，关闭 AP 隔离
# 或配置静态 IP（高级）
[ESP103]192.168.1.100  # 静态 IP
[ESP104]192.168.1.1    # 网关
[ESP105]255.255.255.0  # 子网掩码
[ESP106]STATIC         # 启用静态 IP 模式
```

---

## 相关命令参考

### WiFi STA 配置命令

| 命令 | 功能 | 参数 | 权限 | 示例 |
|------|------|------|------|------|
| `[ESP100]` | 设置/查询 STA SSID | `<SSID>` | 管理员 | `[ESP100]MyWiFi` |
| `[ESP101]` | 设置 STA 密码 | `<Password>` | 管理员 | `[ESP101]12345678` |
| `[ESP103]` | 设置 STA 静态 IP | `<IP>` | 管理员 | `[ESP103]192.168.1.100` |
| `[ESP104]` | 设置 STA 网关 | `<Gateway>` | 管理员 | `[ESP104]192.168.1.1` |
| `[ESP105]` | 设置 STA 子网掩码 | `<Netmask>` | 管理员 | `[ESP105]255.255.255.0` |
| `[ESP106]` | 设置 IP 模式 | `DHCP`/`STATIC` | 管理员 | `[ESP106]DHCP` |
| `[ESP110]` | 设置 WiFi 模式 | `STA`/`AP`/`BT`/`NONE` | 管理员 | `[ESP110]STA` |
| `[ESP111]` | 查询当前 IP | 无 | 用户 | `[ESP111]` |
| `[ESP112]` | 设置/查询主机名 | `<Hostname>` | 管理员 | `[ESP112]grblesp` |
| `[ESP115]` | 立即开关 WiFi | `ON`/`OFF` | 管理员 | `[ESP115]ON` |

### 系统控制命令

| 命令 | 功能 | 参数 | 权限 |
|------|------|------|------|
| `[ESP444]RESTART` | 重启 ESP32 | 无 | 管理员 |
| `[ESP420]` | 获取系统状态 | 无 | 用户 |
| `[ESP800]` | 获取固件信息 | 无 | 管理员 |

### 认证相关

所有管理员命令在启用认证时需要添加 `pwd=<password>` 参数：

```bash
[ESP100]MT3000 pwd=admin
[ESP101]mypassword pwd=admin
[ESP110]STA pwd=admin
```

---

## 高级配置

### 1. 静态 IP 配置

适用于需要固定 IP 地址的场景（如工业环境）：

```bash
# 步骤 1: 配置 SSID 和密码
[ESP100]MT3000
[ESP101]your_password

# 步骤 2: 配置静态 IP
[ESP103]192.168.1.100  # 设备 IP
[ESP104]192.168.1.1    # 路由器网关
[ESP105]255.255.255.0  # 子网掩码

# 步骤 3: 启用静态 IP 模式
[ESP106]STATIC

# 步骤 4: 设置为 STA 模式
[ESP110]STA

# 步骤 5: 重启
[ESP444]RESTART
```

### 2. 自定义主机名

方便通过 mDNS 访问（如 `http://mycnc.local`）：

```bash
[ESP112]mycnc
[ESP110]STA
[ESP444]RESTART

# 重启后可通过以下方式访问:
# - http://mycnc.local
# - telnet mycnc.local 8080
```

### 3. 禁用 DHCP 回退到静态 IP

如果 DHCP 失败，自动使用静态 IP：

```bash
# 先配置静态 IP 作为备用
[ESP103]192.168.1.100
[ESP104]192.168.1.1
[ESP105]255.255.255.0

# 但保持 DHCP 模式
[ESP106]DHCP

# 如果 DHCP 失败，系统会使用静态 IP 配置
```

---

## 编程接口示例

### Python 自动配置脚本

```python
import serial
import time

def configure_wifi_sta(port, ssid, password):
    """配置 Grbl_Esp32 WiFi STA 模式"""
    ser = serial.Serial(port, 115200, timeout=2)
    time.sleep(2)  # 等待串口稳定

    commands = [
        f"[ESP100]{ssid}",
        f"[ESP101]{password}",
        "[ESP110]STA",
        "[ESP444]RESTART"
    ]

    for cmd in commands:
        print(f"发送: {cmd}")
        ser.write((cmd + "\n").encode())
        time.sleep(0.5)

        # 读取响应
        response = ser.read_all().decode()
        print(f"响应: {response}")

    ser.close()
    print("\n配置完成，设备正在重启...")
    print("等待 10 秒后查询 IP...")
    time.sleep(10)

    # 重新连接查询 IP
    ser = serial.Serial(port, 115200, timeout=2)
    ser.write(b"[ESP111]\n")
    time.sleep(0.5)
    ip = ser.read_all().decode().strip()
    print(f"设备 IP: {ip}")
    ser.close()

# 使用示例
configure_wifi_sta("COM3", "MT3000", "your_password")
```

### Arduino 配置示例

```cpp
void configureWiFiSTA(const char* ssid, const char* password) {
    Serial.println("[ESP100]" + String(ssid));
    delay(500);

    Serial.println("[ESP101]" + String(password));
    delay(500);

    Serial.println("[ESP110]STA");
    delay(500);

    Serial.println("[ESP444]RESTART");
    Serial.println("设备正在重启...");
}
```

---

## 代码实现分析

### 1. 命令注册

**文件**: [WebSettings.cpp:1159](../../Grbl_Esp32/src/WebUI/WebSettings.cpp#L1159)

```cpp
wifi_radio_mode = new EnumSetting(
    "Radio mode",           // 设置名称
    WEBSET,                 // 设置类型
    WA,                     // 需要管理员权限
    "ESP110",               // 命令编号
    "Radio/Mode",           // NVS 存储路径
    DEFAULT_RADIO_MODE,     // 默认值
    &radioEnabledOptions,   // 可选值枚举
    NULL                    // 验证函数
);
```

### 2. 模式枚举定义

**文件**: [WebSettings.cpp:75-84](../../Grbl_Esp32/src/WebUI/WebSettings.cpp#L75-L84)

```cpp
enum_opt_t radioEnabledOptions = {
    { "NONE", ESP_RADIO_OFF },
#ifdef ENABLE_WIFI
    { "STA", ESP_WIFI_STA },
    { "AP", ESP_WIFI_AP },
#endif
#ifdef ENABLE_BLUETOOTH
    { "BT", ESP_BT },
#endif
};
```

### 3. WiFi 启动流程

**文件**: [WifiConfig.cpp](../../Grbl_Esp32/src/WebUI/WifiConfig.cpp)

```cpp
bool WiFiConfig::StartSTA() {
    // 1. 从 NVS 读取 SSID 和密码
    String ssid = sta_ssid->get();
    String password = sta_password->get();

    // 2. 配置 WiFi 模式
    WiFi.mode(WIFI_STA);

    // 3. 连接到 AP
    WiFi.begin(ssid.c_str(), password.c_str());

    // 4. 等待连接
    // ...
}
```

### 4. 启动时模式选择

系统启动时根据 `wifi_radio_mode` 的值决定启动哪种模式：

```cpp
void WiFiConfig::begin() {
    int mode = wifi_radio_mode->get();

    switch(mode) {
        case ESP_WIFI_STA:
            StartSTA();  // 启动 STA 模式
            break;
        case ESP_WIFI_AP:
            StartAP();   // 启动 AP 模式
            break;
        case ESP_BT:
            // 启动蓝牙
            break;
        case ESP_RADIO_OFF:
            // 关闭无线电
            break;
    }
}
```

---

## 最佳实践

### 1. 生产环境配置流程

```bash
# 1. 首次配置
[ESP100]ProductionWiFi
[ESP101]SecurePassword123
[ESP112]cnc-machine-01
[ESP110]STA
[ESP444]RESTART

# 2. 验证连接
[ESP111]  # 确认获得 IP

# 3. 修改默认密码（如果启用认证）
[ESP555]NewAdminPassword pwd=admin

# 4. 备份配置（记录所有设置）
```

### 2. 安全建议

1. **修改默认密码**: 默认管理员密码是 `admin`
2. **使用强 WiFi 密码**: 至少 12 位，包含大小写字母、数字
3. **启用认证**: 在 [Config.h](../../Grbl_Esp32/src/Config.h) 中定义 `ENABLE_AUTHENTICATION`
4. **限制网络访问**: 使用防火墙规则限制访问
5. **定期更新固件**: 关注安全更新

### 3. 故障恢复

如果完全无法连接：

```bash
# 方法 1: 恢复出厂设置
[ESP410]  # 返回扫描的AP

# 方法 2: 切换到 AP 模式
[ESP110]AP
[ESP444]RESTART
# 然后连接到 GRBL_ESP 热点（密码: 12345678）
# 通过 Web 界面 (192.168.0.1) 重新配置

# 方法 3: 串口配置
# 始终可以通过 USB 串口配置，不受 WiFi 影响
```

---

## 与原厂固件的差异

### 开源版本 (Grbl_Esp32)

- **默认模式**: AP（热点模式）
- **需要手动配置**: 必须执行 `[ESP110]STA`
- **灵活性高**: 可以随时切换模式
- **适合开发**: 便于调试和测试

### 原厂固件（可能的定制）

- **默认模式**: STA（客户端模式）
- **自动连接**: 配置 SSID/密码后自动连接
- **简化操作**: 减少用户配置步骤
- **适合生产**: 开箱即用

### 如何让开源版本表现得像原厂固件

修改 [Config.h](../../Grbl_Esp32/src/Config.h)，添加：

```cpp
#define CONNECT_TO_SSID "MT3000"
#define SSID_PASSWORD "your_password"
```

重新编译后，系统会：
1. 默认启动 STA 模式
2. 自动使用预设的 SSID 和密码
3. 无需手动执行 `[ESP110]STA`

---

## 总结

### 关键要点

1. ✅ **必须设置 WiFi 模式**: `[ESP110]STA` 是关键步骤
2. ✅ **配置顺序**: SSID → 密码 → 模式 → 重启
3. ✅ **验证连接**: 使用 `[ESP111]` 查询 IP
4. ✅ **故障排查**: 检查模式、密码长度、信号强度

### 完整命令清单

```bash
[ESP100]<SSID>        # 设置 SSID
[ESP101]<Password>    # 设置密码
[ESP110]STA           # 设置为 STA 模式（关键！）
[ESP444]RESTART       # 重启生效
[ESP111]              # 查询 IP
```

### 参考资料

- [Commands.txt](../Commands.txt) - 完整命令列表
- [WebSettings.cpp](../../Grbl_Esp32/src/WebUI/WebSettings.cpp) - 命令实现
- [WifiConfig.cpp](../../Grbl_Esp32/src/WebUI/WifiConfig.cpp) - WiFi 配置实现
- [Config.h](../../Grbl_Esp32/src/Config.h) - 编译时配置

---

**文档版本**: 1.0
**创建日期**: 2026-01-15
**适用版本**: Grbl_Esp32 主分支
**维护者**: 基于代码分析生成
