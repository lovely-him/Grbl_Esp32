# Web 文件管理 API 详解

## 概述

Grbl_Esp32 提供了完整的 HTTP Web API，用于管理 ESP32 上的文件系统。支持两种存储介质：
- **SPIFFS (Flash)**: ESP32 内部 Flash 存储
- **SD Card**: 外部 SD 卡存储

这些 API 可以被移动应用、PC 应用或 Web 界面调用，实现文件的查询、上传、删除等操作。

---

## API 端点

### 1. SPIFFS 文件系统 API

**端点**: `/files`

**用途**: 管理 ESP32 内部 Flash (SPIFFS) 上的文件

**代码位置**: [WebServer.cpp:695](../../../Grbl_Esp32/src/WebUI/WebServer.cpp#L695) `handleFileList()`

**API 测试示例**:

![SPIFFS 文件列表 API 测试](images/screenshot_2026-01-29_14-00-11.png)

*图：使用 Postman 测试 `/files` 端点，查询 SPIFFS 文件系统*

### 2. SD 卡文件系统 API

**端点**: `/upload`

**用途**: 管理 SD 卡上的文件

**代码位置**: [WebServer.cpp:1216](../../../Grbl_Esp32/src/WebUI/WebServer.cpp#L1216) `handle_direct_SDFileList()`

**API 测试示例**:

![SD 卡文件列表 API 测试](images/screenshot_2026-01-29_14-00-56.png)

*图：使用 Postman 测试 `/upload` 端点，查询 SD 卡文件系统*

---

## 请求参数

### 查询文件列表

```
GET /upload?action=list&filename=all&path=/
```

或

```
GET /files?action=list&filename=all&path=/
```

#### 参数说明

| 参数 | 类型 | 必填 | 说明 | 示例 |
|------|------|------|------|------|
| `action` | string | 否 | 操作类型 | `list`, `delete`, `deletedir`, `createdir` |
| `filename` | string | 视情况 | 文件名或目录名 | `test.gcode`, `all` (列表查询时) |
| `path` | string | 否 | 目标路径，默认为 `/` | `/`, `/subdir/` |
| `dontlist` | string | 否 | 设置为 `yes` 时不返回文件列表 | `yes` |

---

## 响应格式

### 成功响应 (HTTP 200)

```json
{
  "files": [
    {
      "name": "test.gcode",
      "size": "1.2 KB"
    },
    {
      "name": "config.txt",
      "size": "512 B"
    },
    {
      "name": "subdir",
      "size": "-1"
    }
  ],
  "path": "/",
  "status": "Ok",
  "total": "4.0 MB",
  "used": "1.5 MB",
  "occupation": "37"
}
```

#### 字段说明

| 字段 | 类型 | 说明 |
|------|------|------|
| `files` | array | 文件和目录列表 |
| `files[].name` | string | 文件或目录名 |
| `files[].size` | string | 文件大小（格式化后），`-1` 表示目录 |
| `path` | string | 当前路径 |
| `status` | string | 操作状态信息 |
| `total` | string | 存储总容量（格式化后） |
| `used` | string | 已使用容量（格式化后） |
| `occupation` | string | 使用百分比 (0-100) |
| `mode` | string | 仅 SD 卡 API 返回，值为 `"direct"` |

### 错误响应

#### 认证失败 (HTTP 401)

```json
{
  "status": "Authentication failed!"
}
```

#### SD 卡不可用 (HTTP 200)

```json
{
  "status": "No SD Card"
}
```

或

```json
{
  "status": "Busy"
}
```

#### 路径不存在 (HTTP 200)

```json
{
  "status": "/invalid/path does not exist on SD Card"
}
```

---

## 支持的操作

### 1. 列出文件 (list)

**请求示例**:
```
GET /upload?path=/
```

**说明**: 不指定 `action` 参数时，默认列出文件

---

### 2. 删除文件 (delete)

**请求示例**:
```
GET /upload?action=delete&filename=test.gcode&path=/
```

**响应**:
```json
{
  "files": [...],
  "status": "test.gcode deleted"
}
```

---

### 3. 删除目录 (deletedir)

**请求示例**:
```
GET /upload?action=deletedir&filename=subdir&path=/
```

**说明**:
- 会递归删除目录下所有文件和子目录
- 不能删除根目录 `/`

**响应**:
```json
{
  "files": [...],
  "status": "subdir deleted"
}
```

---

### 4. 创建目录 (createdir)

**请求示例**:
```
GET /upload?action=createdir&filename=newdir&path=/
```

**响应**:
```json
{
  "files": [...],
  "status": "newdir created"
}
```

---

## 文件上传

### SPIFFS 文件上传

**端点**: `POST /files`

**Content-Type**: `multipart/form-data`

**代码位置**: [WebServer.cpp:153](../../../Grbl_Esp32/src/WebUI/WebServer.cpp#L153) `SPIFFSFileupload()`

### SD 卡文件上传

**端点**: `POST /upload`

**Content-Type**: `multipart/form-data`

**代码位置**: [WebServer.cpp:160](../../../Grbl_Esp32/src/WebUI/WebServer.cpp#L160) `SDFile_direct_upload()`

**上传响应**:
```json
{
  "status": "Ok"
}
```

---

## 认证要求

所有文件管理 API 都需要认证：

| 操作 | 最低权限 |
|------|----------|
| 列出文件 | User |
| 查看文件 | User |
| 上传文件 | User |
| 删除文件 | User |
| 创建目录 | User |
| 删除目录 | User |

**Guest 用户无法访问文件管理功能**，会返回 HTTP 401 错误。

---

## 代码实现位置

### 核心文件

| 文件 | 说明 |
|------|------|
| [WebServer.cpp](../../../Grbl_Esp32/src/WebUI/WebServer.cpp) | Web 服务器主文件 |
| [WebServer.h](../../../Grbl_Esp32/src/WebUI/WebServer.h) | Web 服务器头文件 |

### 关键函数

| 函数 | 行号 | 说明 |
|------|------|------|
| `handleFileList()` | 695 | SPIFFS 文件列表处理 |
| `SPIFFSFileupload()` | 923 | SPIFFS 文件上传处理 |
| `handle_direct_SDFileList()` | 1216 | SD 卡文件列表处理 |
| `SDFile_direct_upload()` | 1416 | SD 卡文件上传处理 |
| `deleteRecursive()` | 1180 | 递归删除目录 |

### 路由注册

```cpp
// WebServer.cpp:153
_webserver->on("/files", HTTP_ANY, handleFileList, SPIFFSFileupload);

// WebServer.cpp:160
_webserver->on("/upload", HTTP_ANY, handle_direct_SDFileList, SDFile_direct_upload);
```

---

## 使用示例

### 示例 1: 查询 SD 卡根目录文件列表

**请求**:
```
GET http://192.168.1.100/upload?path=/
```

**响应**:
```json
{
  "files": [
    {
      "name": "test.gcode",
      "shortname": "test.gcode",
      "size": "2.5 KB",
      "datetime": ""
    },
    {
      "name": "projects",
      "shortname": "projects",
      "size": "-1",
      "datetime": ""
    }
  ],
  "path": "/",
  "total": "7.4 GB",
  "used": "1.2 GB",
  "occupation": "16",
  "mode": "direct",
  "status": "Ok"
}
```

### 示例 2: 查询子目录

**请求**:
```
GET http://192.168.1.100/upload?path=/projects/
```

### 示例 3: 删除文件

**请求**:
```
GET http://192.168.1.100/upload?action=delete&filename=test.gcode&path=/
```

**响应**:
```json
{
  "files": [...],
  "path": "/",
  "status": "test.gcode deleted",
  ...
}
```

### 示例 4: 创建目录

**请求**:
```
GET http://192.168.1.100/upload?action=createdir&filename=backup&path=/
```

### 示例 5: 仅执行操作不返回文件列表

**请求**:
```
GET http://192.168.1.100/upload?action=delete&filename=old.gcode&path=/&dontlist=yes
```

**说明**: 添加 `dontlist=yes` 参数可以加快响应速度，适合只需要执行操作不需要获取文件列表的场景。

---

## 注意事项

### 1. 路径格式

- 路径必须以 `/` 开头
- 目录路径建议以 `/` 结尾（代码会自动处理）
- 路径中的 `//` 会被自动替换为 `/`

### 2. 文件名处理

- 文件名中的 `/` 会被移除
- 上传文件时，如果文件名不以 `/` 开头，会自动添加

### 3. SD 卡状态

SD 卡 API 会检查 SD 卡状态：
- `SDState::Idle` - 可用
- `SDState::NotPresent` - 未插入
- `SDState::BusyParsing` - 正在解析
- `SDState::BusyUploading` - 正在上传
- `SDState::BusyPrinting` - 正在打印

**只有在 `Idle` 状态下才能进行文件操作**。

### 4. 目录识别

- 文件大小为 `-1` 表示这是一个目录
- SPIFFS 使用 `/.` 文件来标记空目录
- SD 卡使用原生目录结构

### 5. 权限控制

- Admin 用户可以访问所有路径
- User 用户被限制在 `/user` 目录下（仅 SPIFFS）
- Guest 用户无法访问文件管理功能

### 6. 并发控制

- SD 卡操作会设置状态锁，防止并发访问
- 上传、打印、解析等操作会互斥
- 操作完成后会自动释放状态锁

### 7. 性能优化

- 使用 `dontlist=yes` 可以跳过文件列表生成，加快响应
- 大目录列表可能需要较长时间
- 代码中使用 `COMMANDS::wait(0)` 防止看门狗超时

---

## 与 FileTransferServer 的区别

| 特性 | Web API (`/upload`, `/files`) | FileTransferServer |
|------|-------------------------------|-------------------|
| 协议 | HTTP (RESTful) | TCP Socket (二进制) |
| 端口 | 80 (Web服务器) | 8080 (可配置) |
| 数据格式 | JSON | 自定义二进制协议 (0xAB 魔术字节) |
| 用途 | 文件管理、查询、删除 | 高效文件传输 |
| 适用场景 | Web界面、App轻量级操作 | 大文件快速上传 |
| 认证 | Web 认证系统 | 无认证 |
| 文件列表 | ✅ 支持 | ❌ 不支持 |
| 文件上传 | ✅ 支持 (multipart) | ✅ 支持 (分包传输) |
| 文件删除 | ✅ 支持 | ❌ 不支持 |
| 目录操作 | ✅ 支持 | ❌ 不支持 |

**选择建议**:
- **文件管理操作**: 使用 Web API (`/upload`, `/files`)
- **大文件上传**: 使用 FileTransferServer (更高效，支持断点续传)

---

## 相关文档

- [Settings配置系统设计详解.md](./Settings配置系统设计详解.md)
- [WiFi_STA配置完全指南.md](./WiFi_STA配置完全指南.md)
- [FileTransferServer.cpp](../../../Grbl_Esp32/src/WebUI/FileTransferServer.cpp) - TCP文件传输服务器

---

**最后更新**: 2026-01-29
**作者**: Claude Code
**版本**: 1.0
