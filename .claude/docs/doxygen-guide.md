# C 语言编程规范（Doxygen 标准）

## 第一部分：编程规范

### 1. 文字规范

| **规范项** | **要求** |
|-----------|---------|
| **注释与日志** | • 必须使用中文进行说明，确保表述清晰易懂<br>• Doxygen 文档注释使用中文描述功能和参数 |
| **专业术语** | • 保留通用的专业英语缩写或单词，不强制翻译<br>• 例如：`recv`, `send`, `uart`, `buffer`, `frame`, `timeout` 等 |
| **中英混排** | • 英语单词/缩写与中文文字之间必须保留一个空格<br>• 例如："串口 buffer 已满" |
| **字符集** | • 所有代码符号仅限使用 ASCII 字符集内的符号<br>• 禁止使用全角符号或特殊字符 |

---

### 2. 注释规范（Doxygen 标准）

所有注释必须严格遵循 **Doxygen JavaDoc 风格**（使用 `@` 标签）。

#### 2.1 文件头部注释

| **规范项** | **要求** |
|-----------|---------|
| **适用范围** | • `.c` 源文件与 `.h` 头文件均需包含文件头部注释 |
| **`.h` 头文件** | • **必需标签**：`@file`, `@brief`, `@author`, `@date`, `@version`, `@copyright`<br>• **详细程度**：详细描述模块功能、使用方法、注意事项<br>• **目的**：为 Doxygen 生成完整的 API 文档 |
| **`.c` 源文件** | • **必需标签**：`@file`, `@brief`<br>• **详细程度**：仅简要说明即可，避免与 `.h` 文件重复<br>• **目的**：标识文件用途，不作为主要文档来源 |
| **格式要求** | • 使用 `/** ... */` 风格<br>• 位于文件最开头 |
| **避免冗余** | • `.c` 和 `.h` 不应包含完全相同的详细注释<br>• 详细文档只写在 `.h` 头文件中<br>• 修改文档时只需维护一处，避免同步问题 |
| **标签填写规则** | • `@author 建豪 (JianHao)` - 固定使用此标识<br>• `@date` - 如果已有则更新为当前日期，如果没有则填写今天日期<br>• `@version` - 如果已有则在原版本号基础上 +0.1 (如 1.0 → 1.1)，如果没有则从 1.0 开始，除非有特殊说明<br>• `@copyright` - 根据项目、公司或其他归属要求自由填写 |

#### 2.2 函数头部注释

| **规范项** | **要求** |
|-----------|---------|
| **添加位置** | • `.h` 头文件中的函数声明前<br>• `.c` 源文件中的函数实现前不需要重复注释 |
| **必需标签** | • `@brief` 函数简要描述<br>• `@param` 参数说明（每个参数一行）<br>• `@return` 返回值说明<br>• `@note` 注意事项（可选）<br>• `@warning` 警告信息（可选） |
| **格式要求** | • 使用 `/** ... */` 风格<br>• 参数使用 `@param[in]`, `@param[out]`, `@param[in,out]` 标注方向 |

#### 2.3 结构体与枚举注释

| **规范项** | **要求** |
|-----------|---------|
| **类型注释** | • 在类型定义前添加 `/** @brief ... */` 说明整体用途 |
| **成员注释** | • 每个成员使用行尾注释 `/**< ... */`<br>• 简洁说明成员含义、单位、取值范围 |
| **适用范围** | • 枚举类型及其成员<br>• 结构体类型及其成员 |

#### 2.4 宏定义注释

| **规范项** | **要求** |
|-----------|---------|
| **注释方式** | • 使用行尾注释 `/**< ... */` |
| **注释内容** | • 说明宏的用途、单位、取值含义 |
| **示例** | `#define MAX_BUFFER_SIZE 256 /**< 缓冲区最大字节数 */` |

#### 2.5 全局变量注释

| **规范项** | **要求** |
|-----------|---------|
| **注释方式** | • 使用行尾注释 `/**< ... */` |
| **注释内容** | • 说明变量用途、作用域、初始值含义 |
| **命名建议** | • 全局变量建议加 `g_` 前缀<br>• 静态变量建议加 `s_` 前缀 |

#### 2.6 行间注释

| **规范项** | **要求** |
|-----------|---------|
| **最少原则** | • 函数内的行间注释应尽可能少，最好不要有<br>• 代码应通过有意义的函数名、变量名自我说明 |
| **必要情况** | • 特别需要注意的事项或警告<br>• 复杂算法的关键点说明<br>• 重要的技术决策或设计考量 |
| **优先重构** | • 如果代码需要注释才能理解，应优先考虑重构代码 |
| **格式要求** | • 使用 `// ...` 单行注释<br>• 位于被注释代码的上方，与代码保持相同缩进 |
| **禁止位置** | • `.c` 源文件中尽量避免行间注释<br>• 函数内部不应有过多注释 |
| **语言规范** | • 中文描述性注释必须使用中文<br>• 保留专业英语短句不需要翻译（如 "USB Device event"）<br>• 专业术语缩写保持英文（如 `buffer`, `timeout`, `EOF`, `FID`） |

#### 2.7 回调函数注释

| **规范项** | **要求** |
|-----------|---------|
| **标准回调** | • 标准库或框架定义的回调函数（如 `*_callback`）不需要添加 Doxygen 注释<br>• 这些函数遵循已有的接口规范，注释会造成冗余 |
| **自定义函数** | • 非标准回调的静态辅助函数需要添加完整的 Doxygen 注释 |
| **示例** | • `device_descriptor_callback()` - 标准 USB 回调，无需注释<br>• `wifi_usb_sta_uvc_stream_reset()` - 自定义函数，需要注释 |

---

### 3. 代码格式规范

#### 3.1 缩进与空格

| **规范项** | **要求** |
|-----------|---------|
| **缩进** | • 统一使用 **4 个空格** 进行缩进<br>• 禁止使用 Tab 键 |
| **操作符间距** | • 双目/三目操作符两侧保留 1 个空格<br>• 例如：`=`, `+`, `-`, `==`, `!=`, `?`, `:` |
| **逗号分号** | • 逗号 `,` 和分号 `;` 后保留 1 个空格 |
| **关键字间距** | • 关键字（`if`, `while`, `for`, `switch`）与其后的括号之间保留 1 个空格 |
| **函数调用** | • 函数名与参数列表的括号之间 **不** 保留空格 |

#### 3.2 花括号规范

| **规范项** | **要求** |
|-----------|---------|
| **左花括号 `{`** | • **不** 单独另起一行，紧跟在语句行尾<br>• 与前面的语句之间保留 1 个空格 |
| **右花括号 `}`** | • 必须 **单独另起一行**<br>• 与对应的左花括号保持相同缩进层级 |
| **特殊情况** | • `do...while`：右花括号后紧跟 `while`，在同一行<br>• `if...else`：`else` 另起一行，不与右花括号同行 |

#### 3.3 Switch 语句规范

| **规范项** | **要求** |
|-----------|---------|
| **case 缩进** | • 每个 `case` 标签必须比 `switch` 多一级缩进（4 个空格） |
| **作用域** | • 每个 `case` 分支必须使用花括号 `{ }` 形成独立作用域<br>• 左花括号 `{` 与 `case` 标签在同一行 |
| **右花括号** | • 右花括号 `}` 必须单独另起一行 |
| **break 语句** | • 每个 `case` 分支的右花括号 `}` 后必须有 `break` 语句<br>• `break` 单独另起一行，与 `case` 标签对齐<br>• 即使花括号内已有 `return`，依然要在花括号外写 `break` |
| **作用** | • 确保每个 `case` 都有独立作用域，便于声明局部变量<br>• 格式统一，提高可读性和可维护性 |

---

### 4. 关键字使用规范

| **规范项** | **要求** |
|-----------|---------|
| **extern** | • `.h` 头文件中的所有全局函数声明必须显式添加 `extern` 关键字<br>• `.h` 头文件中的全局变量声明必须显式添加 `extern` 关键字 |
| **static** | • `.c` 源文件中仅限本文件使用的函数必须添加 `static` 关键字<br>• `.c` 源文件中仅限本文件使用的全局变量必须添加 `static` 关键字<br>• 建议静态变量加 `s_` 前缀以示区分 |
| **const** | • 不会被修改的参数建议添加 `const` 修饰符<br>• 常量定义优先使用 `const` 而非 `#define`（类型安全） |

---

## 第二部分：完整代码示例

以下是一个 **串口通信模块** 的完整示例，展示了所有编程规范的应用。

### uart_driver.h（头文件）

```c
/**
 * @file uart_driver.h
 * @brief 串口通信驱动模块
 *
 * 提供串口初始化、数据发送和接收功能，支持多种波特率配置。
 * 本模块适用于嵌入式系统中的 UART 硬件抽象层。
 *
 * @author Zhang San (zhangsan@example.com)
 * @date 2025-01-02
 * @version 1.0
 * @copyright Copyright (c) 2025 Company Name
 */

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 串口端口号定义
 */
typedef enum {
    UART_PORT_0 = 0,        /**< 串口 0 */
    UART_PORT_1 = 1,        /**< 串口 1 */
    UART_PORT_2 = 2,        /**< 串口 2 */
    UART_PORT_MAX           /**< 串口数量上限 */
} uart_port_t;

/**
 * @brief 串口波特率定义
 */
typedef enum {
    UART_BAUD_9600 = 9600,      /**< 9600 bps */
    UART_BAUD_115200 = 115200,  /**< 115200 bps */
    UART_BAUD_921600 = 921600   /**< 921600 bps */
} uart_baud_t;

/**
 * @brief 串口状态定义
 */
typedef enum {
    UART_STATUS_IDLE = 0,       /**< 空闲状态 */
    UART_STATUS_BUSY,           /**< 忙碌状态 */
    UART_STATUS_ERROR           /**< 错误状态 */
} uart_status_t;

/**
 * @brief 串口配置结构体
 */
typedef struct {
    uart_port_t port;       /**< 串口端口号 */
    uart_baud_t baud_rate;  /**< 波特率 */
    uint8_t data_bits;      /**< 数据位 (5-8) */
    uint8_t stop_bits;      /**< 停止位 (1-2) */
    bool parity_enable;     /**< 是否启用奇偶校验 */
} uart_config_t;

#define UART_TX_BUFFER_SIZE     256     /**< 发送缓冲区大小 (字节) */
#define UART_RX_BUFFER_SIZE     512     /**< 接收缓冲区大小 (字节) */
#define UART_TIMEOUT_MS         1000    /**< 默认超时时间 (毫秒) */

/**
 * @brief 初始化串口
 *
 * 根据配置参数初始化指定的串口硬件，包括波特率、数据位、停止位等。
 * 初始化成功后，串口进入空闲状态，可以进行数据收发。
 *
 * @param[in]   config      串口配置参数指针
 * @return 0 表示成功，-1 表示失败
 *
 * @note 调用此函数前需确保硬件时钟已正确配置
 * @warning 重复初始化同一个串口会导致之前的配置丢失
 */
extern int uart_init(const uart_config_t *config);

/**
 * @brief 发送数据
 *
 * 通过指定串口发送数据缓冲区中的内容。函数会阻塞直到所有数据发送完成或超时。
 *
 * @param[in]   port        串口端口号
 * @param[in]   data        待发送数据的缓冲区指针
 * @param[in]   length      待发送数据的字节数
 * @return 实际发送的字节数，失败返回 -1
 *
 * @note 如果 length 为 0，函数直接返回 0
 * @warning 发送过程中请勿修改 data 指向的内存内容
 */
extern int uart_send(uart_port_t port, const uint8_t *data, uint16_t length);

/**
 * @brief 接收数据
 *
 * 从指定串口接收数据到缓冲区。函数会阻塞直到接收到指定长度的数据或超时。
 *
 * @param[in]   port        串口端口号
 * @param[out]  buffer      接收数据的缓冲区指针
 * @param[in]   max_length  缓冲区最大容量（字节数）
 * @param[in]   timeout_ms  超时时间（毫秒），0 表示非阻塞
 * @return 实际接收的字节数，超时或失败返回 -1
 *
 * @note 如果 timeout_ms 为 0，函数立即返回当前可用数据
 */
extern int uart_receive(uart_port_t port, uint8_t *buffer, uint16_t max_length, uint32_t timeout_ms);

/**
 * @brief 获取串口状态
 *
 * @param[in]   port        串口端口号
 * @return 当前串口状态
 */
extern uart_status_t uart_get_status(uart_port_t port);

/**
 * @brief 清空接收缓冲区
 *
 * @param[in]   port        串口端口号
 * @return 0 表示成功，-1 表示失败
 */
extern int uart_flush_rx_buffer(uart_port_t port);

#ifdef __cplusplus
}
#endif

#endif // UART_DRIVER_H
```

---

### uart_driver.c（源文件）

```c
/**
 * @file uart_driver.c
 * @brief 串口驱动实现文件
 */

#include "uart_driver.h"
#include <string.h>

static uint8_t s_tx_buffer[UART_PORT_MAX][UART_TX_BUFFER_SIZE];
static uint8_t s_rx_buffer[UART_PORT_MAX][UART_RX_BUFFER_SIZE];
static uart_status_t s_uart_status[UART_PORT_MAX] = {UART_STATUS_IDLE};
static uint16_t s_rx_data_count[UART_PORT_MAX] = {0};

static bool is_port_valid(uart_port_t port)
{
    return port < UART_PORT_MAX;
}

static void hardware_uart_init(uart_port_t port, uart_baud_t baud_rate)
{
    (void)port;
    (void)baud_rate;
}

static void hardware_uart_send_byte(uart_port_t port, uint8_t byte)
{
    (void)port;
    (void)byte;
}

static bool hardware_uart_is_tx_ready(uart_port_t port)
{
    (void)port;
    return true;
}

int uart_init(const uart_config_t *config)
{
    if (config == NULL || !is_port_valid(config->port)) {
        return -1;
    }

    hardware_uart_init(config->port, config->baud_rate);
    s_uart_status[config->port] = UART_STATUS_IDLE;
    s_rx_data_count[config->port] = 0;
    memset(s_rx_buffer[config->port], 0, UART_RX_BUFFER_SIZE);

    return 0;
}

int uart_send(uart_port_t port, const uint8_t *data, uint16_t length)
{
    if (!is_port_valid(port) || data == NULL || length == 0) {
        return -1;
    }

    if (s_uart_status[port] == UART_STATUS_ERROR) {
        return -1;
    }

    s_uart_status[port] = UART_STATUS_BUSY;

    for (uint16_t i = 0; i < length; i++) {
        while (!hardware_uart_is_tx_ready(port)) {
        }
        hardware_uart_send_byte(port, data[i]);
    }

    s_uart_status[port] = UART_STATUS_IDLE;
    return length;
}

int uart_receive(uart_port_t port, uint8_t *buffer, uint16_t max_length, uint32_t timeout_ms)
{
    if (!is_port_valid(port) || buffer == NULL || max_length == 0) {
        return -1;
    }

    uint16_t available = s_rx_data_count[port];
    uint16_t copy_length = (available < max_length) ? available : max_length;

    if (copy_length == 0 && timeout_ms == 0) {
        return 0;
    }

    memcpy(buffer, s_rx_buffer[port], copy_length);
    s_rx_data_count[port] -= copy_length;

    if (s_rx_data_count[port] > 0) {
        memmove(s_rx_buffer[port], s_rx_buffer[port] + copy_length, s_rx_data_count[port]);
    }

    return copy_length;
}

uart_status_t uart_get_status(uart_port_t port)
{
    if (!is_port_valid(port)) {
        return UART_STATUS_ERROR;
    }

    switch (s_uart_status[port]) {
        case UART_STATUS_IDLE: {
            return UART_STATUS_IDLE;
        }
        break;
        case UART_STATUS_BUSY: {
            return UART_STATUS_BUSY;
        }
        break;
        case UART_STATUS_ERROR: {
            return UART_STATUS_ERROR;
        }
        break;
        default: {
            return UART_STATUS_ERROR;
        }
        break;
    }
}

int uart_flush_rx_buffer(uart_port_t port)
{
    if (!is_port_valid(port)) {
        return -1;
    }

    s_rx_data_count[port] = 0;
    memset(s_rx_buffer[port], 0, UART_RX_BUFFER_SIZE);

    return 0;
}
```

---

## 规范体现说明

上述示例完整体现了以下规范要点：

### 注释规范
- ✅ 文件头部使用 Doxygen 标准格式（`@file`, `@brief`, `@author` 等）
- ✅ 函数注释在 `.h` 文件中详细说明，`.c` 文件中无重复注释
- ✅ 枚举、结构体成员使用行尾注释 `/**< ... */`
- ✅ 宏定义使用行尾注释
- ✅ `.c` 文件中无行间注释，代码自说明

### 格式规范
- ✅ 4 个空格缩进
- ✅ 操作符、逗号、关键字的空格规范
- ✅ 花括号规范（左花括号不另起一行，右花括号单独一行）
- ✅ `if...else` 的 `else` 单独起一行
- ✅ `switch` 语句每个 `case` 都有 `{}` 作用域和独立的 `break`

### 关键字规范
- ✅ `.h` 文件中所有函数声明使用 `extern`
- ✅ `.c` 文件中内部函数使用 `static`
- ✅ 静态变量使用 `s_` 前缀
- ✅ 函数参数使用 `const` 修饰

### 中英混排
- ✅ 注释中"串口 buffer"、"串口 0" 等保持空格

---

## 使用 Doxygen 生成文档

配置 `Doxyfile` 后运行以下命令：

```bash
doxygen Doxyfile
```

即可生成标准的 HTML/LaTeX 格式的 API 文档。

---

## 附录：参考资料

**重要说明：** 以下参考资料仅供规范制定者参考，用于解决本文档未明确规定的边界情况。日常开发中**严格遵循本文档规定即可**，无需查阅这些外部资料。只有在遇到本文档未涵盖的特殊场景时，才需要查阅以下资料进行决策。

### Doxygen 官方文档

- **[Doxygen: Documenting the code](https://www.doxygen.nl/manual/docblocks.html)**
  - 官方文档块标记规范
  - 明确了 Doxygen 文档注释（`/**`, `///`）与普通注释的区别
  - **关键结论**：Doxygen 只规定文档注释格式，普通代码注释（`//` vs `/* */`）由项目自行规定

### C 语言注释风格参考

- **[GitHub - MaJerle/c-code-style](https://github.com/MaJerle/c-code-style)**
  - 严格派规范：禁止使用 `//`，统一使用 `/* */`
  - 理由：C89 兼容性、风格统一

- **[C Code Style Guidelines - Swarthmore College](https://www.cs.swarthmore.edu/~newhall/unixhelp/c_codestyle.html)**
  - 灵活派规范：允许两种注释风格
  - 核心原则：**代码自说明** > 注释
  - 避免冗余注释，优先通过好的命名减少注释需求

### 项目级 Doxygen 实践

- **[Pigweed Doxygen Style Guide](https://pigweed.dev/docs/style/doxygen.html)**
  - Google 项目的 Doxygen 实践，推荐使用 `///` 单行文档注释

- **[OpenOCD Doxygen Style Guide](https://openocd.org/doc-release/doxygen/styledoxygen.html)**
  - 嵌入式项目的 Doxygen 实践
  - 推荐 `///` 用于单行文档，`/** */` 用于多行文档

### 关键知识点总结

1. **Doxygen 文档注释 vs 普通注释**
   - Doxygen **只管文档注释**（`/**`, `/*!`, `///`, `//!`）
   - 普通注释（`//` 和 `/* */`）被 Doxygen 完全忽略
   - 普通注释的格式规范属于 **C 语言编码规范范畴**，非 Doxygen 标准

2. **代码自说明原则**
   - 所有主流规范一致认同：优先通过有意义的函数名、变量名减少注释
   - 注释应解释 "为什么"（设计决策），而非 "是什么"（代码已说明）

3. **本项目的选择**
   - 统一使用 `//` 作为普通代码注释
   - `/* */` 仅用于临时注释掉代码块（调试用途）
   - 理由：C99+ 标准支持、简洁明了、与 Doxygen 文档注释视觉区分度高

---

**文档版本：** 1.1
**最后更新：** 2025-01-03
**维护者：** 建豪 (JianHao)
