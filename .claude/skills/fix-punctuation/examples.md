# fix-punctuation 使用示例

## 基本用法

### 1. 修复单个文件
```bash
/fix-punctuation src/main.cpp
```

**处理前**：
```cpp
// 初始化系统，设置参数
void init() {
    Serial.begin(115200)；
    Serial.println("系统启动成功！")；
}
```

**处理后**：
```cpp
// 初始化系统，设置参数
void init() {
    Serial.begin(115200);
    Serial.println("系统启动成功! ");
}
```

### 2. 修复目录下所有文件
```bash
/fix-punctuation Grbl_Esp32/src/**/*.cpp
```

### 3. 修复特定类型的文件
```bash
/fix-punctuation **/*.h
```

### 4. 修复当前目录所有代码文件
```bash
/fix-punctuation
```

## 转换示例

| 原始代码 | 修复后 |
|---------|--------|
| `if (x > 0)：` | `if (x > 0): ` |
| `printf("错误！")；` | `printf("错误! "); ` |
| `// 注意：这是重要的` | `// 注意: 这是重要的` |
| `int arr【10】；` | `int arr [10]; ` |
| `《头文件》` | ` <头文件> ` |

## 支持的文件类型

- **C/C++**: `.c`, `.cpp`, `.h`, `.hpp`, `.cc`, `.cxx`
- **Python**: `.py`
- **JavaScript/TypeScript**: `.js`, `.ts`, `.jsx`, `.tsx`
- **Java**: `.java`
- **C#**: `.cs`
- **Go**: `.go`
- **Rust**: `.rs`
- **文档**: `.md`, `.txt`
- **配置**: `.json`, `.yaml`, `.yml`

## 注意事项

1. **仅修改标点符号**：不会修改中文注释或字符串中的中文文字
2. **保持格式**：保持原有的缩进、换行和代码结构
3. **批量处理**：支持使用通配符批量处理多个文件
4. **安全操作**：修改前会先读取文件，确保文件存在且可读

## 常见场景

### 场景 1：修复整个项目
```bash
/fix-punctuation
```

### 场景 2：仅修复源文件
```bash
/fix-punctuation src/**/*.{c,cpp,h}
```

### 场景 3：修复特定目录
```bash
/fix-punctuation Grbl_Esp32/src/
```

## 输出示例

```
[INFO] Found 15 file(s) to process
[SUCCESS] Fixed 23 punctuation(s) in: src/main.cpp
[SUCCESS] Fixed 12 punctuation(s) in: src/config.h
[SKIP] No Chinese punctuation found in: src/utils.cpp
...
[SUMMARY] Processed 12/15 file(s)
```
