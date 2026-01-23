# ESP32 固件合并烧录完全指南

## 目录

1. [问题背景](#问题背景)
2. [ESP32 启动模式详解](#esp32-启动模式详解)
3. [ESP32 启动流程详解](#esp32-启动流程详解)
4. [Flash Mode 工作原理](#flash-mode-工作原理)
5. [问题根本原因](#问题根本原因)
6. [解决方案](#解决方案)
7. [使用方法](#使用方法)
8. [常见问题](#常见问题)

---

## 问题背景

使用 PlatformIO 编译的固件通过 `pio run --target upload` 烧录正常，但使用第三方工具（如 LaserTool）烧录单个 bin 文件后，ESP32 不断重启。

---

## ESP32 启动模式详解

### 启动模式选择机制

ESP32 通过 **BOOT (GPIO0)** 和 **EN (CHIP_PU)** 两个引脚的电平组合来决定启动模式：

| EN 引脚 | BOOT (GPIO0) | 启动模式 | 说明 |
|---------|--------------|----------|------|
| 低电平 | 任意 | **芯片复位** | 芯片处于复位状态，不工作 |
| 高电平 | 高电平 | **正常启动模式** | 从 Flash 启动应用程序 |
| 高电平 | 低电平 | **下载模式** | 进入串口下载模式，等待烧录固件 |

### 进入下载模式的时序

```
时序图：
        EN  ────┐      ┌────────────
                └──────┘
                  复位

      BOOT  ────────────┐
                        └───────────
                        保持低电平

结果：ESP32 进入串口下载模式
```

**操作步骤**：
1. 按住 BOOT 按钮（GPIO0 拉低）
2. 按下 EN 按钮（复位芯片）
3. 松开 EN 按钮（芯片启动）
4. 松开 BOOT 按钮
5. ESP32 进入下载模式，等待串口数据

### 自动下载电路（DTR/RTS 控制）

现代开发板通过 USB 转串口芯片的 **DTR** 和 **RTS** 信号自动控制 EN 和 BOOT 引脚：

```
电路原理：
USB 转串口芯片 (CH340/CP2102/FT232)
    │
    ├─ DTR ──┬─ 电容 ─┬─ EN (复位)
    │        │        │
    │        └─ 电阻 ─┴─ GND
    │
    └─ RTS ──┬─ 电容 ─┬─ BOOT (GPIO0)
             │        │
             └─ 电阻 ─┴─ GND
```

**自动下载时序**：

```
1. 准备进入下载模式：
   RTS = 低电平 → BOOT = 低电平
   DTR = 低电平 → EN = 低电平（复位）

2. 启动进入下载模式：
   DTR = 高电平 → EN = 高电平（启动）
   RTS = 低电平 → BOOT = 低电平（保持）

   结果：ESP32 进入下载模式

3. 下载完成后恢复正常：
   RTS = 高电平 → BOOT = 高电平
   DTR = 低电平 → EN = 低电平（复位）
   DTR = 高电平 → EN = 高电平（启动）

   结果：ESP32 正常启动
```

### PlatformIO 和 LaserTool 的下载方式

**本质相同**：
- 都通过串口连接 ESP32
- 都使用 DTR/RTS 控制 EN 和 BOOT 引脚
- 都使用 esptool 协议传输数据
- 都是将数据写入 Flash 芯片

**表面不同**：
- **PlatformIO**：分开烧录 4 个文件到不同地址
  ```bash
  esptool write_flash
    0x1000  bootloader.bin
    0x8000  partitions.bin
    0xe000  boot_app0.bin
    0x10000 firmware.bin
  ```

- **LaserTool**：烧录 1 个合并文件到地址 0x0
  ```bash
  esptool write_flash
    0x0     firmware_merged.bin
  ```

### 下载模式下的 Flash 操作

进入下载模式后，ESP32 的 **ROM Bootloader**（固化在芯片内部）会：

1. **初始化串口**：波特率通常为 115200（初始握手）
2. **等待命令**：监听串口，等待 esptool 发送命令
3. **执行命令**：
   - `flash_begin`：准备写入 Flash
   - `flash_data`：接收数据并写入 Flash
   - `flash_end`：完成写入
   - `mem_begin/mem_data`：加载 stub 到 RAM（提速）

**重要**：下载模式下，ROM Bootloader 使用 **默认的慢速模式** 与 Flash 通信，不依赖 bootloader.bin 中的配置。

---

## ESP32 启动流程详解

### 完整的启动流程

```
┌─────────────────────────────────────────────────────────┐
│ 阶段 0: 上电 / 复位                                      │
│   - EN 引脚从低电平变为高电平                            │
│   - ESP32 芯片开始启动                                   │
│   - 检查 BOOT (GPIO0) 引脚电平                           │
└─────────────────────────────────────────────────────────┘
                          ↓
         ┌────────────────┴────────────────┐
         │                                  │
    BOOT = 低电平                      BOOT = 高电平
         │                                  │
         ↓                                  ↓
┌─────────────────────┐      ┌─────────────────────────────┐
│  进入下载模式        │      │  正常启动流程                │
│  (串口烧录固件)      │      │                             │
└─────────────────────┘      └─────────────────────────────┘
                                            ↓
┌─────────────────────────────────────────────────────────┐
│ 阶段 1: ROM Bootloader (固化在 ESP32 芯片内)            │
│   - 自动运行，无法修改                                   │
│   - 初始化硬件（时钟、GPIO、UART）                       │
│   - 读取 Flash 地址 0x1000 的 bootloader.bin            │
│   - 解析 bootloader 头部的 Flash 配置字节               │
│                                                          │
│   Bootloader 头部结构（地址 0x1000）：                   │
│   Offset  内容        说明                               │
│   0x00    0xE9       魔术字节                            │
│   0x01    0x04       段数量                              │
│   0x02    0x02       Flash 配置字节 ← 关键！            │
│   0x03    0x20       其他配置                            │
│                                                          │
│   Flash 配置字节解析：                                   │
│   Bit 0-3: Flash Mode                                   │
│     0x0 = QIO  (Quad I/O)                               │
│     0x1 = QOUT (Quad Output)                            │
│     0x2 = DIO  (Dual I/O)  ← 本项目使用                 │
│     0x3 = DOUT (Dual Output)                            │
│   Bit 4-7: Flash Speed                                  │
│     0x0 = 40MHz                                         │
│     0xF = 80MHz  ← 本项目使用                           │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ 阶段 2: 配置 Flash 接口                                  │
│   - 根据配置字节设置 Flash SPI 接口                      │
│   - DIO 模式：使用 2 条数据线（DI, DO）                 │
│   - 80MHz 频率：设置 SPI 时钟频率                       │
│   - 后续所有 Flash 读取都使用这个配置                   │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ 阶段 3: 二级 Bootloader (地址 0x1000)                    │
│   - 使用配置好的 Flash 模式读取数据                      │
│   - 读取分区表（0x8000 的 partitions.bin）              │
│   - 解析分区表，找到应用程序分区                         │
│   - 读取 OTA 数据（0xe000 的 boot_app0.bin）            │
│   - 决定从哪个分区启动（app0 或 app1）                   │
│   - 验证应用程序的完整性（SHA256 校验）                  │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ 阶段 4: 加载应用程序                                     │
│   - 从 0x10000 读取 firmware.bin                        │
│   - 加载到 RAM（IRAM 和 DRAM）                          │
│   - 设置中断向量表                                       │
│   - 跳转到应用程序入口点（app_main）                     │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│ 阶段 5: 应用程序运行                                     │
│   - 执行用户代码（Grbl_Esp32）                          │
│   - 初始化外设（WiFi, UART, GPIO 等）                   │
│   - 进入主循环                                           │
└─────────────────────────────────────────────────────────┘
```

### 启动失败的常见原因

| 现象 | 原因 | 解决方法 |
|------|------|----------|
| `load:0xffffffff,len:-1` | Flash Mode 不匹配 | 使用正确的 bootloader (DIO/QIO) |
| `rst:0x10 (RTCWDT_RTC_RESET)` | 看门狗复位，无法找到有效程序 | 检查 boot_app0.bin 是否存在 |
| `E (xxx) boot: Invalid partition table` | 分区表损坏或地址错误 | 重新烧录 partitions.bin 到 0x8000 |
| `E (xxx) boot: No bootable app partitions` | 应用程序分区为空 | 烧录 firmware.bin 到 0x10000 |

---

## Flash Mode 工作原理

### Flash Mode 的作用

Flash Mode 决定了 ESP32 与 Flash 芯片之间的 **数据传输方式**。这个配置只在 **启动后读取 Flash** 时使用，不影响串口下载过程。

### 四种 Flash Mode 对比

| 模式 | 全称 | 数据线 | 地址传输 | 数据传输 | 读取速度 | 兼容性 |
|------|------|--------|---------|---------|---------|--------|
| **QIO** | Quad I/O | 4 条 | 4 线 | 4 线 | 最快 | 低 |
| **QOUT** | Quad Output | 4 条 | 1 线 | 4 线 | 快 | 中 |
| **DIO** | Dual I/O | 2 条 | 2 线 | 2 线 | 中等 | 高 ✓ |
| **DOUT** | Dual Output | 2 条 | 1 线 | 2 线 | 较慢 | 最高 |

### Flash 引脚连接

```
ESP32 与 Flash 芯片的连接：

标准 SPI 引脚（所有模式都需要）：
  CLK  (GPIO6)  ─────→  CLK   (时钟)
  CS   (GPIO11) ─────→  CS    (片选)
  DI   (GPIO7)  ←────→  DI/IO0 (数据输入)
  DO   (GPIO8)  ←────→  DO/IO1 (数据输出)

Quad 模式额外引脚（QIO/QOUT 需要）：
  WP   (GPIO9)  ←────→  WP/IO2 (写保护，用作数据线)
  HD   (GPIO10) ←────→  HD/IO3 (保持，用作数据线)
```

**本项目硬件**：
- ✅ 支持 DIO/DOUT（2 线模式）
- ❌ 不支持 QIO/QOUT（4 线模式）
- 原因：WP 和 HD 引脚可能未连接或用于其他功能

### Flash Mode 配置的生效时机

```
时间线：
┌──────────────────────────────────────────────────────────┐
│ 1. 串口下载阶段（烧录固件）                               │
│    - 使用 ROM Bootloader 的默认慢速模式                  │
│    - 不依赖 bootloader.bin 中的配置                      │
│    - Flash Mode 配置被写入 bootloader.bin 的头部         │
└──────────────────────────────────────────────────────────┘
                          ↓
┌──────────────────────────────────────────────────────────┐
│ 2. 正常启动阶段（运行固件）                               │
│    Step 1: ROM Bootloader 读取 0x1000 的 bootloader.bin │
│            - 解析 Flash 配置字节（0x02 = DIO + 80MHz）  │
│                                                          │
│    Step 2: 配置 Flash 接口                               │
│            - 根据配置字节设置 Flash SPI 模式             │
│            - DIO 模式：使用 2 条数据线                   │
│            - 80MHz 频率：设置时钟频率                    │
│                                                          │
│    Step 3: 使用配置的模式读取后续数据                    │
│            - 读取分区表（0x8000）                        │
│            - 读取 boot_app0.bin（0xe000）               │
│            - 读取应用程序（0x10000）                     │
└──────────────────────────────────────────────────────────┘
```

### Flash Mode 不匹配的后果

**场景：Bootloader 配置为 QIO，但硬件只支持 DIO**

```
1. ROM Bootloader 读取 0x1000 的 bootloader.bin
   ✓ 成功（使用默认慢速模式）

2. 解析 Flash 配置：QIO 模式（4 线）
   ✓ 配置完成

3. 尝试用 QIO 模式读取分区表（0x8000）
   ✗ 失败！硬件不支持 4 线模式
   ✗ WP 和 HD 引脚未连接或电平错误
   ✗ 读取到的数据是乱码或全 0xFF

4. 无法找到有效的分区表
   ✗ Bootloader 不知道应用程序在哪里
   ✗ 触发看门狗复位（RTCWDT_RTC_RESET）
   ✗ 重启，进入死循环

串口输出：
ets Jul 29 2019 12:21:46
rst:0x10 (RTCWDT_RTC_RESET),boot:0x17 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:QIO, clock div:1
load:0x3fff0018,len:4
load:0xffffffff,len:-1  ← 读取失败！
```

**正确配置（DIO 模式）**

```
1. ROM Bootloader 读取 0x1000 的 bootloader.bin
   ✓ 成功

2. 解析 Flash 配置：DIO 模式（2 线）
   ✓ 配置完成

3. 用 DIO 模式读取分区表（0x8000）
   ✓ 成功读取

4. 用 DIO 模式读取 boot_app0.bin（0xe000）
   ✓ 成功，知道从 app0 启动

5. 用 DIO 模式读取 firmware.bin（0x10000）
   ✓ 成功加载应用程序

6. 应用程序正常运行
   ✓ 启动成功！

串口输出：
ets Jul 29 2019 12:21:46
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:1
load:0x3fff0018,len:4
load:0x3fff001c,len:1044
load:0x40078000,len:10124
load:0x40080400,len:5856
entry 0x400806a8
[MSG:Grbl_ESP32 Ver 1.x.x]  ← 正常启动！
```

### 如何确定硬件支持的 Flash Mode

#### 方法 1：分析原厂固件

```bash
# 查看原厂固件的 bootloader 配置字节
od -A x -t x1z -j 0x1000 -N 4 原厂固件.bin

输出示例：
001000 e9 04 02 20
            ^
            +-- 0x02 = DIO 模式
```

#### 方法 2：查看硬件原理图

检查 Flash 芯片的引脚连接：
- 如果 WP (IO2) 和 HD (IO3) 连接到 ESP32 → 支持 QIO/QOUT
- 如果 WP 和 HD 悬空或上拉 → 只支持 DIO/DOUT

#### 方法 3：查看 Flash 芯片型号

常见 Flash 芯片及其支持的模式：
- **W25Q32** (Winbond) → 支持 QIO/QOUT/DIO/DOUT
- **GD25Q32** (GigaDevice) → 支持 QIO/QOUT/DIO/DOUT
- **某些低成本芯片** → 只支持 DIO/DOUT

**注意**：即使 Flash 芯片支持 QIO，如果硬件未连接 WP 和 HD 引脚，也只能使用 DIO/DOUT。

---

## 问题根本原因

### 1. 缺少 boot_app0.bin

本项目使用 `min_spiffs.csv` 分区表，这是 **OTA 双分区设计**：

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,   # NVS 存储
otadata,  data, ota,     0xe000,  0x2000,   # OTA 数据分区 ← 关键！
app0,     app,  ota_0,   0x10000, 0x1E0000, # 应用程序分区 0
app1,     app,  ota_1,   0x1F0000,0x1E0000, # 应用程序分区 1（OTA 更新用）
spiffs,   data, spiffs,  0x3D0000,0x30000,  # 文件系统
```

**OTA 分区的工作原理**：

```
┌─────────────────────────────────────────────────────────┐
│ OTA 数据分区（0xe000，boot_app0.bin）                    │
│   - 存储当前应该启动的分区信息                           │
│   - 内容：指向 app0 或 app1                              │
│   - 如果为空或损坏，bootloader 无法决定启动哪个分区      │
└─────────────────────────────────────────────────────────┘
                          ↓
         ┌────────────────┴────────────────┐
         │                                  │
    指向 app0                           指向 app1
         │                                  │
         ↓                                  ↓
┌──────────────────┐              ┌──────────────────┐
│ app0 (0x10000)   │              │ app1 (0x1F0000)  │
│ 当前运行的固件    │              │ OTA 更新的固件    │
└──────────────────┘              └──────────────────┘
```

**如果缺少 boot_app0.bin**：
- Bootloader 读取 0xe000 地址，发现是空的（全 0xFF）
- 无法确定应该启动哪个分区
- 触发看门狗复位
- 不断重启

### 2. Flash Mode 不匹配

通过分析原厂固件，发现硬件使用 **DIO 模式**：

```
原厂固件 bootloader 头部（地址 0x1000）：
e9 04 02 20
      ^
      +-- Flash 配置字节 0x02 = DIO 模式
```

**错误配置**：
- ❌ 使用 `bootloader_qio_80m.bin`
- ❌ 合并时指定 `--flash_mode qio`

**正确配置**：
- ✅ 使用 `bootloader_dio_80m.bin`
- ✅ 合并时指定 `--flash_mode dio`

---

## 解决方案

### 完整的 ESP32 Flash 布局

```
地址范围          内容                    说明
─────────────────────────────────────────────────────────
0x0000 - 0x0FFF   填充 0xFF (4KB)        预留给 Secure Boot
0x1000 - 0x7FFF   bootloader.bin         二级引导加载程序
0x8000 - 0x8FFF   partitions.bin         分区表
0x9000 - 0xDFFF   nvs                    NVS 存储区（运行时创建）
0xe000 - 0xFFFF   boot_app0.bin          OTA 启动选择器 ← 关键！
0x10000 - ...     firmware.bin           应用程序
```

### 自动合并脚本

`merge_firmware.bat` 脚本会自动：

1. ✅ 检查 ESP-IDF Python 环境
2. ✅ 收集 4 个必需的文件：
   - `bootloader_dio_80m.bin` (来自 PlatformIO 框架)
   - `partitions.bin` (编译生成)
   - `boot_app0.bin` (来自 PlatformIO 框架)
   - `firmware.bin` (编译生成)
3. ✅ 使用正确的 Flash Mode (DIO) 合并
4. ✅ 生成完整的固件文件

### 合并命令详解

```bash
esptool --chip esp32 merge_bin \
  -o firmware_merged.bin \
  --flash_mode dio \      # DIO 模式（2 线）
  --flash_freq 80m \      # 80MHz 时钟频率
  --flash_size 4MB \      # Flash 大小
  0x1000 bootloader_dio_80m.bin \  # Bootloader
  0x8000 partitions.bin \          # 分区表
  0xe000 boot_app0.bin \           # OTA 选择器
  0x10000 firmware.bin             # 应用程序
```

**参数说明**：
- `--flash_mode dio`：设置 Flash 工作模式为 DIO（2 线）
- `--flash_freq 80m`：设置 Flash 时钟频率为 80MHz
- `--flash_size 4MB`：设置 Flash 大小为 4MB
- `0x1000 ...`：指定每个文件的烧录地址

---

## 使用方法

### 步骤 1：编译项目

```bash
pio run -e release
```

编译完成后，生成的文件位于：
```
.pio/build/release/
├── firmware.bin      (应用程序，约 1.8 MB)
└── partitions.bin    (分区表，3 KB)
```

### 步骤 2：运行合并脚本

双击运行 `merge_firmware.bat`，或在命令行中执行：

```bash
merge_firmware.bat
```

脚本输出示例：
```
Merging firmware...

Running esptool merge_bin...
Using Python: D:\JianHao\Espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe

Merging 4 files:
  0x1000  bootloader_dio_80m.bin (DIO mode, 80MHz)
  0x8000  partitions.bin
  0xe000  boot_app0.bin (OTA selector)
  0x10000 firmware.bin

esptool.py v4.10.0
Flash params set to 0x022f
SHA digest in image updated
Wrote 0x1d70e0 bytes to file .pio\build\release\firmware_merged.bin

========================================
SUCCESS! Firmware merged successfully
Output file: .pio\build\release\firmware_merged.bin
========================================

Flash address: 0x0
File size:
2026/01/22  13:26         1,929,440 firmware_merged.bin
```

### 步骤 3：烧录固件

使用 LaserTool 或 Flash Download Tool 烧录：

**烧录参数**：
- 文件：`.pio/build/release/firmware_merged.bin`
- 地址：`0x0`
- Flash Mode：`DIO`
- Flash Freq：`80MHz`
- Flash Size：`4MB`

**使用 esptool 命令行烧录**：
```bash
esptool.py --chip esp32 --port COM3 --baud 921600 \
  write_flash --flash_mode dio --flash_freq 80m --flash_size 4MB \
  0x0 .pio/build/release/firmware_merged.bin
```

### 步骤 4：验证启动

烧录完成后，打开串口监视器（115200 波特率），应该看到正常的启动日志：

```
ets Jul 29 2019 12:21:46
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:1
load:0x3fff0018,len:4
load:0x3fff001c,len:1044
load:0x40078000,len:10124
load:0x40080400,len:5856
entry 0x400806a8
[MSG:Grbl_ESP32 Ver 1.x.x]
[MSG:Compiled with ESP32 SDK:v3.3.5-1-g85c43024c]
[MSG:Using machine:Wainlux_K11_HIM]
...
```

---

## 常见问题

### Q1: 为什么 PlatformIO 烧录正常，但合并的 bin 不行？

**A**: PlatformIO 自动包含了 4 个必需的文件：
- bootloader.bin
- partitions.bin
- **boot_app0.bin** ← 容易遗漏
- firmware.bin

手动合并时，如果缺少 `boot_app0.bin`，OTA 分区表无法工作，导致不断重启。

### Q2: 如何判断硬件使用 DIO 还是 QIO？

**A**: 查看原厂固件的 bootloader 头部（地址 0x1000），第 3 个字节：

```bash
od -A x -t x1z -j 0x1000 -N 4 原厂固件.bin

输出：
001000 e9 04 02 20
            ^
            +-- 0x02 = DIO 模式
```

Flash 配置字节对照表：
- `0x00` = QIO
- `0x01` = QOUT
- `0x02` = DIO ← 本项目
- `0x03` = DOUT

### Q3: 合并后的文件可以用于 OTA 升级吗？

**A**: 不可以。OTA 升级只需要 `firmware.bin`（应用程序部分），不需要 bootloader、分区表和 boot_app0.bin。

**OTA 升级流程**：
1. 应用程序下载新的 `firmware.bin`
2. 写入到 app1 分区（0x1F0000）
3. 更新 OTA 数据（0xe000），指向 app1
4. 重启，bootloader 从 app1 启动新固件

### Q4: 如何验证烧录是否成功？

**A**: 烧录后，通过串口监视器（115200 波特率）查看启动日志：

**失败的日志**（不断重启）：
```
ets Jul 29 2019 12:21:46
rst:0x10 (RTCWDT_RTC_RESET),boot:0x17 (SPI_FAST_FLASH_BOOT)
mode:QIO, clock div:1
load:0xffffffff,len:-1  ← 读取失败
```

**成功的日志**：
```
ets Jul 29 2019 12:21:46
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
mode:DIO, clock div:1
load:0x3fff0018,len:4
load:0x3fff001c,len:1044
[MSG:Grbl_ESP32 Ver 1.x.x]  ← 正常启动
```

### Q5: 为什么下载时不需要配置 Flash Mode，但启动时需要？

**A**:
- **下载时**：ESP32 的 ROM Bootloader 使用默认的慢速模式与 Flash 通信，不依赖 bootloader.bin 中的配置
- **启动时**：二级 Bootloader 根据配置字节设置 Flash 工作模式，以提高读取速度

这就是为什么：
- ✅ 烧录过程总是成功（使用默认模式）
- ❌ 启动时可能失败（配置模式不匹配）

### Q6: PlatformIO 烧录和脱机烧录有什么区别？

**A**: 本质上没有区别，都是通过串口下载：

| 特性 | PlatformIO 烧录 | 脱机烧录（LaserTool） |
|------|----------------|---------------------|
| **连接方式** | 串口 | 串口 |
| **下载协议** | esptool | esptool |
| **进入下载模式** | DTR/RTS 控制 | DTR/RTS 控制 |
| **文件组织** | 分开烧录 4 个文件 | 烧录 1 个合并文件 |
| **自动化程度** | 自动包含所有文件 | 需要手动准备合并文件 |
| **适用场景** | 开发调试 | 量产、现场升级 |

### Q7: 如何修改 Flash Mode 配置？

**A**: 修改 `platformio.ini` 中的配置：

```ini
[env:release]
board_build.flash_mode = dio  ; 或 qio, dout, qout
board_build.f_flash = 80000000L  ; 80MHz
```

然后重新编译和合并固件。

### Q8: 为什么原厂固件大小是 1.3MB，而我们的是 1.9MB？

**A**:
- **原厂固件**：可能使用了不同的分区表或编译优化
- **我们的固件**：包含了完整的 bootloader、分区表、boot_app0.bin 和应用程序

大小差异是正常的，只要能正常启动即可。

---

## 文件位置参考

### 编译生成的文件

```
.pio/build/release/
├── firmware.bin          (应用程序，约 1.8 MB)
├── partitions.bin        (分区表，3 KB)
├── firmware.elf          (ELF 格式，用于调试)
└── firmware_merged.bin   (合并后的完整固件，约 1.9 MB)
```

### PlatformIO 框架文件

```
%USERPROFILE%\.platformio\packages\framework-arduinoespressif32\
├── tools\sdk\bin\
│   ├── bootloader_dio_40m.bin
│   ├── bootloader_dio_80m.bin  ← 本项目使用
│   ├── bootloader_qio_40m.bin
│   ├── bootloader_qio_80m.bin
│   └── ...
└── tools\partitions\
    ├── boot_app0.bin           ← 本项目使用
    ├── min_spiffs.csv          ← 本项目使用
    ├── default.csv
    └── ...
```

---

## 参考资源

- [ESP-IDF Partition Tables](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html)
- [esptool Flashing Firmware](https://docs.espressif.com/projects/esptool/en/latest/esp32/esptool/flashing-firmware.html)
- [PlatformIO ESP32 Platform](https://github.com/platformio/platform-espressif32)
- [ESP32 Bootloader and Partition Issues](https://github.com/platformio/platform-espressif32/issues/1142)
- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)

---

**最后更新**: 2026-01-22

## 问题根本原因

### 1. 缺少 boot_app0.bin

本项目使用 `min_spiffs.csv` 分区表，这是 **OTA 双分区设计**：

```csv
nvs,      data, nvs,     0x9000,  0x5000,   # NVS 存储
otadata,  data, ota,     0xe000,  0x2000,   # OTA 数据分区
app0,     app,  ota_0,   0x10000, 0x1E0000, # 应用程序分区 0
app1,     app,  ota_1,   0x1F0000,0x1E0000, # 应用程序分区 1（OTA 更新用）
spiffs,   data, spiffs,  0x3D0000,0x30000,  # 文件系统
```

**OTA 分区需要 `boot_app0.bin`**（地址 0xe000）告诉 bootloader 从哪个分区启动。如果缺少这个文件，ESP32 会不断重启。

### 2. Flash Mode 不匹配

通过分析原厂固件，发现硬件使用 **DIO 模式**，而不是 QIO 模式：

```
原厂固件 bootloader 头部：
e9 04 02 20
      ^
      +-- Flash 配置字节 0x02 = DIO 模式
```

必须使用：
- `bootloader_dio_80m.bin`（不是 `bootloader_qio_80m.bin`）
- `--flash_mode dio`（不是 `--flash_mode qio`）

## DIO vs QIO 模式区别

| 特性 | DIO (Dual I/O) | QIO (Quad I/O) |
|------|----------------|----------------|
| 数据线数量 | 2 条（DI, DO） | 4 条（DI, DO, WP, HD） |
| 读取速度 | 较慢 | 更快 |
| 兼容性 | 好，适合大多数硬件 | 需要硬件支持 |
| 适用场景 | 通用方案 | 性能优先 |

**本项目硬件使用 DIO 模式**。

## 完整的 ESP32 Flash 布局

```
地址 0x0000:  填充 0xFF (4KB，预留给 Secure Boot)
地址 0x1000:  bootloader_dio_80m.bin (二级引导加载程序)
地址 0x8000:  partitions.bin (分区表)
地址 0x9000:  nvs (NVS 存储区，运行时创建)
地址 0xe000:  boot_app0.bin (OTA 启动选择器)
地址 0x10000: firmware.bin (应用程序)
```

## 解决方案：自动合并脚本

### 使用方法

1. **编译项目**：
   ```bash
   pio run -e release
   ```

2. **运行合并脚本**：
   ```bash
   merge_firmware.bat
   ```

3. **获得合并文件**：
   ```
   .pio/build/release/firmware_merged.bin
   ```

4. **烧录到 ESP32**：
   - 使用 LaserTool 或 Flash Download Tool
   - 烧录地址：`0x0`
   - Flash Mode：`DIO`
   - Flash Freq：`80MHz`
   - Flash Size：`4MB`

### 脚本工作原理

`merge_firmware.bat` 脚本会：

1. 检查 ESP-IDF Python 环境
2. 收集 4 个必需的文件：
   - `bootloader_dio_80m.bin` (来自 PlatformIO 框架)
   - `partitions.bin` (编译生成)
   - `boot_app0.bin` (来自 PlatformIO 框架)
   - `firmware.bin` (编译生成)
3. 使用 esptool 合并为单个文件：
   ```bash
   esptool --chip esp32 merge_bin -o firmware_merged.bin \
     --flash_mode dio --flash_freq 80m --flash_size 4MB \
     0x1000 bootloader_dio_80m.bin \
     0x8000 partitions.bin \
     0xe000 boot_app0.bin \
     0x10000 firmware.bin
   ```

## PlatformIO 烧录 vs 脱机烧录

### 相同点

- **都是串口下载**：通过 UART 串口传输数据
- **都使用 esptool**：底层都是 esptool.py 协议
- **都触发下载模式**：通过 DTR/RTS 控制 BOOT 和 EN 引脚

### 不同点

| 特性 | PlatformIO 烧录 | 脱机烧录（LaserTool） |
|------|----------------|---------------------|
| 文件组织 | 分开烧录 4 个文件到不同地址 | 烧录 1 个合并文件到 0x0 |
| 自动化程度 | 自动包含所有必需文件 | 需要手动准备合并文件 |
| 适用场景 | 开发调试 | 量产、现场升级 |

## 常见问题

### Q1: 为什么 PlatformIO 烧录正常，但合并的 bin 不行？

A: PlatformIO 自动包含了 `boot_app0.bin`，而手动合并时容易遗漏。

### Q2: 如何判断硬件使用 DIO 还是 QIO？

A: 查看原厂固件的 bootloader 头部（地址 0x1000），第 3 个字节：
- `0x00` = QIO
- `0x01` = QOUT
- `0x02` = DIO
- `0x03` = DOUT

### Q3: 合并后的文件可以用于 OTA 升级吗？

A: 不可以。OTA 升级只需要 `firmware.bin`（应用程序部分），不需要 bootloader 和分区表。

### Q4: 如何验证烧录是否成功？

A: 烧录后，通过串口监视器（115200 波特率）查看启动日志，应该看到 Grbl_Esp32 的正常启动信息，而不是：
```
load:0xffffffff,len:-1
rst:0x10 (RTCWDT_RTC_RESET)
```

## 文件位置参考

### 编译生成的文件

```
.pio/build/release/
├── firmware.bin      (应用程序，约 1.8 MB)
├── partitions.bin    (分区表，3 KB)
└── firmware_merged.bin (合并后的完整固件，约 1.9 MB)
```

### PlatformIO 框架文件

```
%USERPROFILE%\.platformio\packages\framework-arduinoespressif32\
├── tools\sdk\bin\
│   ├── bootloader_dio_80m.bin
│   ├── bootloader_qio_80m.bin
│   └── ...
└── tools\partitions\
    ├── boot_app0.bin
    ├── min_spiffs.csv
    └── ...
```

## 参考资源

- [ESP-IDF Partition Tables](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html)
- [esptool Flashing Firmware](https://docs.espressif.com/projects/esptool/en/latest/esp32/esptool/flashing-firmware.html)
- [PlatformIO ESP32 Platform](https://github.com/platformio/platform-espressif32)
- [ESP32 Bootloader and Partition Issues](https://github.com/platformio/platform-espressif32/issues/1142)

---

**最后更新**: 2026-01-22
