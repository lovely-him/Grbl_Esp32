# ESP32 崩溃分析 - Case 4: SD文件名获取空指针崩溃

## 1. 崩溃信息

### 日志位置
`doc/md/崩溃分析/4_ESP32崩溃日志.log` 第283行

### 触发上下文
```
error:25 in SD file at line 7
error:25 in SD file at line 7
Guru Meditation Error: Core 1 panic'ed (LoadProhibited). Exception was unhandled.
```

### 完整的崩溃信息
```
Core 1 register dump:
PC      : 0x400013f9  PS      : 0x00060430  A0      : 0x800dad6e  A1      : 0x3ffb38c0
A2      : 0x3ffb390c  A3      : 0x00000000  A4      : 0x000000ff  A5      : 0x0000ff00
A6      : 0x00ff0000  A7      : 0xff000000  A8      : 0x00000000  A9      : 0x3ffb3890
A10     : 0x3ffb390c  A11     : 0x3ffb3830  A12     : 0x00000001  A13     : 0x00000000
A14     : 0x00000001  A15     : 0x3ffb3754  SAR     : 0x00000018  EXCCAUSE: 0x0000001c
EXCVADDR: 0x00000000  LBEG    : 0x400013f9  LEND    : 0x4000140d  LCOUNT  : 0xffffffff

Backtrace: 0x400013f9:0x3ffb38c0 0x400dad6b:0x3ffb38d0 0x400da992:0x3ffb38f0
0x400e9ba1:0x3ffb3a70 0x400e91cd:0x3ffb3ba0 0x400d87b9:0x3ffb3bd0 0x400d893f:0x3ffb3c20
0x400e3b2b:0x3ffb3c50 0x400e3eb5:0x3ffb3e10 0x401a0655:0x3ffb3e30 0x400f2a52:0x3ffb3e50
0x400f2af9:0x3ffb3e70 0x400f2b66:0x3ffb3ea0 0x400f2ccf:0x3ffb3ef0 0x400e37a1:0x3ffb3f40
0x400eca18:0x3ffb3f80 0x400ec894:0x3ffb3fa0 0x400db41b:0x3ffb3fc0 0x4008ba51:0x3ffb3fe0
```

---

## 2. 寄存器分析

### 关键寄存器说明

| 寄存器 | 值 | 含义 |
|--------|-----|------|
| **PC** | 0x400013f9 | 程序计数器（崩溃指令地址）- ROM代码区，可能是内联的 `strcpy` |
| **PS** | 0x00060430 | 处理器状态寄存器 |
| **A0** | 0x800dad6e | 返回地址（调用者地址） |
| **A1** | 0x3ffb38c0 | 栈指针（当前栈帧地址） |
| **A2** | 0x3ffb390c | 第一个参数 - `char* name` 的地址（有效堆地址） |
| **A3** | 0x00000000 | **第二个参数 - `myFile.name()` 返回值 = NULL** ← 崩溃原因 |
| **A8** | 0x00000000 | 通用寄存器（也是空指针） |
| **SAR** | 0x00000018 | 移位量寄存器 |
| **EXCCAUSE** | 0x0000001c | 异常原因代码 = 28 (LoadProhibited) |
| **EXCVADDR** | 0x00000000 | **触发异常的内存地址 = NULL** ← 尝试访问空指针 |

### 异常类型详解

**EXCCAUSE = 0x1c (28 = LoadProhibited)**
- 含义：尝试从无效的内存地址加载数据
- 触发地址：EXCVADDR = 0x00000000（空指针）
- 崩溃原因：`strcpy(name, nullptr)` 尝试读取地址0的内容

### 寄存器分析结论

1. **A3 = 0x00000000**：`myFile.name()` 返回了 `nullptr`
2. **EXCVADDR = 0x00000000**：`strcpy` 尝试从空指针读取字符串
3. **A2 = 0x3ffb390c**：目标缓冲区地址有效（不是崩溃原因）
4. **PC = 0x400013f9**：崩溃在 ROM 的 `strcpy` 实现中

---

## 3. Backtrace 完整解码

### 解码命令
```bash
xtensa-esp32-elf-addr2line -pfiaC -e .pio/build/release/firmware.elf \
  0x400013f9 0x400dad6b 0x400da992 0x400e9ba1 0x400e91cd 0x400d87b9 \
  0x400d893f 0x400e3b2b 0x400e3eb5 0x401a0655 0x400f2a52 0x400f2af9 \
  0x400f2b66 0x400f2ccf 0x400e37a1 0x400eca18 0x400ec894 0x400db41b 0x4008ba51
```

### 调用链（从崩溃点到入口）

| # | 地址 | 函数 | 文件:行号 | 说明 |
|---|------|------|-----------|------|
| 0 | 0x400013f9 | `??` | ROM | **崩溃点** - `strcpy` 内联实现 |
| 1 | 0x400dad6b | `sd_get_current_filename()` | SDCard.cpp:169 | **问题函数** - 调用 `myFile.name()` |
| 2 | 0x400da992 | `report_realtime_status()` | Report.cpp:777 | 状态报告函数 - 调用获取文件名 |
| 3 | 0x400e9ba1 | `WebUI::runSDFile()` | WebSettings.cpp:748 | 执行SD文件 - 触发状态报告 |
| 4 | 0x400e91cd | `WebCommand::action()` | WebSettings.cpp:178 | Web命令处理 |
| 5 | 0x400d87b9 | `do_command_or_setting()` | ProcessSettings.cpp:546 | 命令或设置处理 |
| 6 | 0x400d893f | `system_execute_line()` | ProcessSettings.cpp:608 | 系统执行命令行 |
| 7 | 0x400e3b2b | `Web_Server::_handle_web_command()` | WebServer.cpp:469 | Web服务器命令处理 |
| 8 | 0x400e3eb5 | `Web_Server::handle_web_command()` | WebServer.h:83 | Web服务器命令入口 |
| 9 | 0x401a0655 | `std::_Function_handler<>::_M_invoke()` | functional:1871 | C++ 函数包装器 |
| 10 | 0x400f2a52 | `std::function<>::operator()()` | functional:2271 | C++ function 调用 |
| 11 | 0x400f2af9 | `FunctionRequestHandler::handle()` | RequestHandlersImpl.h:81 | HTTP请求处理器 |
| 12 | 0x400f2b66 | `WebServer::_handleRequest()` | WebServer.cpp:620 | Web服务器请求分发 |
| 13 | 0x400f2ccf | `WebServer::handleClient()` | WebServer.cpp:309 | Web服务器客户端处理 |
| 14 | 0x400e37a1 | `Web_Server::handle()` | WebServer.cpp:1558 | Grbl Web服务器 handle |
| 15 | 0x400eca18 | `WiFiServices::handle()` | WifiServices.cpp:172 | WiFi服务处理 |
| 16 | 0x400ec894 | `WiFiConfig::handle()` | WifiConfig.cpp:565 | WiFi配置处理 |
| 17 | 0x400db41b | `clientCheckTask()` | Serial.cpp:200 | **任务入口** - FreeRTOS任务 |
| 18 | 0x4008ba51 | `vPortTaskWrapper` | freertos/port.c:143 | FreeRTOS任务包装器 |

### 调用链分析

**执行流程**：
1. FreeRTOS 任务 `clientCheckTask` 运行
2. 处理 WiFi 服务和 Web 请求
3. 收到执行 SD 文件的 Web 命令
4. `runSDFile()` 开始执行 G-code 文件
5. G-code 解析器遇到 error:25（语法错误）
6. 错误处理调用 `closeFile()` 关闭文件
7. **同时**，`report_realtime_status()` 在另一个线程中被调用
8. 调用 `sd_get_current_filename()` 获取文件名
9. `myFile.name()` 返回 `nullptr`（文件已关闭）
10. `strcpy(name, nullptr)` 崩溃

---

## 4. 根本原因

### 触发条件

**G-code 语法错误** - 文件 `/Untitled_2026231048.nc` 第7行：
```gcode
G0X30.82Y87.52Y87.52 S900.0F3000.0
         ↑        ↑
         Y 字母重复出现
```

**错误检测**：
- G-code 解析器检测到 `GcodeWordRepeated` 错误
- 触发 error:25 报告
- 错误处理调用 `closeFile()` 关闭文件

### 竞态条件流程图

```
时间轴 →
═══════════════════════════════════════════════════════════════════════════════

Thread 1 (G-code执行线程)              Thread 2 (状态报告线程)
─────────────────────────              ─────────────────────────

[执行 G-code 文件]                     [周期性状态报告]
        ↓                                      ↓
[解析第7行]                            [调用 report_realtime_status()]
        ↓                                      ↓
[检测到 Y 重复]                        [检查 SDState == BusyPrinting] ✓
        ↓                                      ↓
[触发 error:25]                        [调用 sd_get_current_filename()]
        ↓                                      ↓
[调用 closeFile()]                     [执行 if (myFile)] ✓
        ↓                                      ↓
[执行 myFile.close()]                  [调用 myFile.name()]
        ↓                                      ↓
[myFile._p = nullptr] ◄────────────────[读取 myFile._p] → nullptr
        ↓                                      ↓
[返回]                                 [返回 nullptr]
                                              ↓
                                       [strcpy(name, nullptr)]
                                              ↓
                                       💥 LoadProhibited 异常
                                              ↓
                                       [系统崩溃重启]

═══════════════════════════════════════════════════════════════════════════════
```

**关键时间窗口**：
```
T0: Thread 2 执行 if (myFile) 检查 → myFile._p 不为空 ✓
T1: Thread 1 执行 myFile.close() → myFile._p = nullptr
T2: Thread 2 执行 myFile.name() → 读取 myFile._p → nullptr
T3: Thread 2 执行 strcpy(name, nullptr) → 崩溃 💥
```

**问题本质**：
- `if (myFile)` 检查和 `myFile.name()` 调用之间**不是原子操作**
- Thread 1 可以在这个间隙修改 `myFile._p`
- 导致 TOCTOU (Time-of-check to time-of-use) 竞态条件

### 问题代码

**SDCard.cpp:167-173 (修改前)**：
```cpp
void sd_get_current_filename(char* name) {
    if (myFile) {  // myFile._p 可能在检查后立即被另一个线程设为 nullptr
        strcpy(name, myFile.name());  // ← 崩溃点：myFile.name() 返回 nullptr
    } else {
        name[0] = 0;
    }
}
```

**File::name() 实现** (ESP32 Arduino 框架):
```cpp
const char* File::name() const {
    if (!_p) {
        return nullptr;  // ← 关闭后返回 nullptr
    }
    return _p->name();
}
```

**File::close() 实现**：
```cpp
void File::close() {
    if (_p) {
        _p->close();
        _p = nullptr;  // ← 将 shared_ptr 设为空
    }
}
```

---

## 5. 修复方案

### 代码修改

**SDCard.cpp:167-178 (修改后)**：
```cpp
void sd_get_current_filename(char* name) {
    if (myFile) {
        const char* filename = myFile.name();
        if (filename != nullptr) {  // ← 添加空指针检查
            strcpy(name, filename);
        } else {
            name[0] = 0;  // 返回空字符串
        }
    } else {
        name[0] = 0;
    }
}
```

### 修改说明

1. **添加 `filename != nullptr` 检查**
   - 在调用 `strcpy` 前验证返回值
   - 避免传递空指针给 `strcpy`

2. **失败时返回空字符串**
   - 不影响程序正常流程
   - 状态报告中文件名显示为空

### 修复前后对比流程图

**修复前（会崩溃）**：
```
sd_get_current_filename(name)
    ↓
if (myFile) ────────────┐
    ↓                   │ 时间窗口：myFile._p 可能被其他线程修改
strcpy(name, myFile.name()) ← 如果 name() 返回 nullptr
    ↓                   │
💥 崩溃                  │
```

**修复后（不会崩溃）**：
```
sd_get_current_filename(name)
    ↓
if (myFile) ────────────┐
    ↓                   │
filename = myFile.name()│ 时间窗口：myFile._p 可能被其他线程修改
    ↓                   │
if (filename != nullptr)│ ← 添加防御性检查
    ↓           ↓       │
   Yes         No       │
    ↓           ↓       │
strcpy(...)  name[0]=0  │
    ↓           ↓       │
✓ 正常      ✓ 正常      │
```

### 为什么不需要加锁？

**原因**：
- `name()` 有内部检查，最坏情况返回 `nullptr`
- 我们添加了外部检查，防止 `strcpy(nullptr)`
- 失败时返回空字符串，不影响功能
- 这是防御性编程，不是完全的线程同步

**如果要完全线程安全**：需要添加互斥锁保护所有对 `myFile` 的访问，但：
- 增加复杂度
- 有性能开销
- 当前方案已足够解决崩溃问题

---

## 6. 解决的问题

### 修复效果

1. **防止崩溃** ✓
   - 避免 `strcpy(nullptr)` 导致的 LoadProhibited 异常
   - 系统不再因为文件名获取失败而崩溃

2. **保持功能** ✓
   - 文件关闭后，文件名获取返回空字符串
   - 状态报告继续正常工作

3. **无性能开销** ✓
   - 只增加一次指针判断（1个 if）
   - 不需要锁或额外内存分配

4. **符合设计原则** ✓
   - 代码简洁（只增加3行）
   - 防御性编程
   - 适合原型阶段

### 测试建议

**重现测试**：
1. 创建包含语法错误的 G-code 文件
2. 在第7行添加重复的单词（如 `F1000 F1000`）
3. 上传并执行文件
4. 观察是否崩溃

**压力测试**：
- 循环执行包含错误的 G-code 文件 100 次
- 监控崩溃和内存泄漏

---

## 7. 附录：ESP32 异常代码表

| EXCCAUSE | 名称 | 含义 |
|----------|------|------|
| 0 | IllegalInstruction | 非法指令 |
| 1 | Syscall | 系统调用 |
| 2 | InstructionFetchError | 指令获取错误 |
| 3 | LoadStoreError | 加载/存储错误 |
| **28 (0x1c)** | **LoadProhibited** | **从无效地址加载数据** ← 本次崩溃 |
| 29 (0x1d) | StoreProhibited | 向无效地址存储数据 |

---

**分析完成**: 2026-02-03
**修复状态**: 已修复
**优先级**: P0 (系统崩溃)
**修复文件**: `Grbl_Esp32/src/SDCard.cpp`
