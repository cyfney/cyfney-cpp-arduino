# 日志（`cyf/log.h`）

面向 Arduino 与宿主机构建的轻量级、仅头文件日志组件。每条日志自带 `HH.MM.SS.mmm` 时间戳和 `文件:行号 函数名]` 前缀，输出经由可替换的 sink（默认：Arduino 上为 `Serial`，其他平台为 `stdout`）。

## 目录

- [快速上手](#快速上手)
- [日志宏](#日志宏)
- [日志级别过滤](#日志级别过滤)
- [时间戳](#时间戳)
- [配置函数](#配置函数)
- [底层 API](#底层-api)
- [平台说明](#平台说明)

## 快速上手

```cpp
#include "cyf.h"  // 必须先包含根头文件（Arduino 库发现机制依赖它）
#include "cyf/log.h"

void setup() {
  Serial.begin(115200);
  CLOGI("boot ok, value=%d", 42);
}

void loop() {}
```

输出（串口监视器 115200）：

```text
I 00.00.03.152 main.cpp:6 setup] boot ok, value=42
```

其他级别（`CLOGV`、`CLOGD`、`CLOGW`、`CLOGE`）用法相同——如何启用与关闭见[日志级别过滤](#日志级别过滤)。

## 日志宏

| 宏 | 级别前缀 | 典型用途 |
| -- | ------------ | -------- |
| `CLOGV(fmt, ...)` | `V` | 详细追踪 |
| `CLOGD(fmt, ...)` | `D` | 调试信息 |
| `CLOGI(fmt, ...)` | `I` | 一般信息 |
| `CLOGW(fmt, ...)` | `W` | 警告 |
| `CLOGE(fmt, ...)` | `E` | 错误 |

所有宏均为 `printf` 风格：第一个参数是格式化字符串，可选的附加参数经 `vsnprintf`（AVR 上为 `vsnprintf_P`）格式化，末尾自动追加换行符。

```cpp
CLOGI("count=%u ch=%c str=%s", count, ch, str);
```

日志行格式：

```text
<前缀> <HH.MM.SS.mmm> <文件>:<行号> <函数名>] <消息>\n
```

文件名在编译期被裁剪为去掉目录的基本名（最后一个 `/` 或 `\` 之后的部分）。

## 日志级别过滤

编译期过滤。只有详细度 **不高于** `CYF_LOG_LEVEL` 的宏才展开为代码，更详细的宏展开为空（`void(0)`），不占用任何二进制空间。

日志级别（数字越大越详细，0 表示全部关闭）：

| 级别 | 宏 | 说明 |
| ---- | -- | ---- |
| 0 | `CYF_LOG_LEVEL_NONE` | 关闭全部日志 |
| 1 | `CYF_LOG_LEVEL_ERROR` | |
| 2 | `CYF_LOG_LEVEL_WARN` | |
| 3 | `CYF_LOG_LEVEL_INFO` | **默认值** |
| 4 | `CYF_LOG_LEVEL_DEBUG` | |
| 5 | `CYF_LOG_LEVEL_VERBOSE` | |

在包含头文件之前设置级别（编译选项或宏定义）：

```cpp
#define CYF_LOG_LEVEL (CYF_LOG_LEVEL_DEBUG)
#include "cyf/log.h"
```

或在编译器命令行上：

```text
-DCYF_LOG_LEVEL=CYF_LOG_LEVEL_ERROR
```

默认 `INFO` 级别下：

```cpp
CLOGV("verbose");  // 编译期移除
CLOGD("debug");    // 编译期移除
CLOGI("info");     // 可见
CLOGW("warn");     // 可见
CLOGE("error");    // 可见
```

包含全部级别的可运行示例见 `examples/log/log_level/`。

## 时间戳

时间戳由毫秒计数器渲染为 `HH.MM.SS.mmm`（12 个字符，前导零补齐）。仅使用计数器的低 28 位（约 3 天回绕一次），不含"天"字段。

毫秒来源按优先级依次为：

1. 通过 `set_millis_source()` 注册的用户回调
2. ESP32：`pdTICKS_TO_MS(xTaskGetTickCount())`
3. 其他 Arduino 核心：`millis()`
4. 宿主机 / 非 Arduino 环境：`0`

## 配置函数

以下函数均位于命名空间 `cyf::log`，为 `noexcept` 的 inline 函数。每项配置**只能设置一次**（由 `assert` 保证，debug 构建下重复调用会中止）。

### `set_millis_source(MillisSource source)`

```cpp
using MillisSource = uint32_t (*)();
void set_millis_source(MillisSource source);
```

覆盖默认的时间戳来源，须传入一个返回已流逝毫秒数（`uint32_t`）的函数。适用于 `millis()` 不可用的平台或测试场景。

### `set_log_sink(LogSink sink)`

```cpp
using LogSink = void (*)(const char* message, size_t length);
void set_log_sink(LogSink sink);
```

将每条格式化后的日志路由到自定义函数。`message` 指向内部格式化缓冲区（仅在本次调用期间有效，需要保留请自行拷贝）；`length` 为需写入的字节数（含末尾换行符）。未设置 sink 时，默认输出为 Arduino 上的 `Serial`，其他平台为 `stdout`。

```cpp
void MySink(const char* message, size_t length) {
  Serial1.write(message, length);  // 例如从第二个串口输出日志
}

void setup() {
  Serial1.begin(115200);
  cyf::log::set_log_sink(MySink);
}
```

### `set_format_buffer_size(size_t size)`

设置内部格式化缓冲区的大小（字节）。默认值：AVR 上 **256**，其他平台 **512**。必须 `> 0`。超长消息会被截断，末尾字符替换为 `\n`。每次 `Log()` 调用都会在栈上分配该缓冲区，请保持其大小明显小于你的栈余量。

## 底层 API

### `cyf::log::Log(const char* fmt, ...)`

所有宏背后的核心格式化函数。先写入时间戳，再用 `vsnprintf`（AVR 上为 `vsnprintf_P`，格式化字符串存于 flash）格式化，最后经 sink 输出。不含级别前缀和 `文件:行号` 装饰——这些由宏补充。

### `cyf::log::ExtractFileNameOffset(path)`

`constexpr` 模板，返回字符串字面量中最后一个路径分隔符之后首个字符的下标，用于在日志行中只嵌入文件基本名。

## 平台说明

- **AVR（Uno、Mega 等）**：整个格式化字符串（含 `文件:行号` 前缀）通过 `PSTR` 存放于 flash，用 `vsnprintf_P` 读取，可大致减半 RAM 占用。注意 `vsnprintf_P` 只从 flash 读取*格式化字符串本身*：`%s` 参数仍须指向 RAM，因此存于 flash 的文件名以 `%S`（avr-libc 扩展）传入。若在 AVR 上直接调用 `Log()` 并使用 `%s` 参数，对应字符串必须位于 RAM 中。
- **ESP32**：时间戳默认取自 FreeRTOS tick 计数；未设置 sink 时输出到 `stdout`（在启用 Arduino 核心日志时可见于串口监视器）。
- **宿主机 / 单元测试**：无需 `Arduino.h` 即可编译；时间戳默认为 `0`，输出到 `stdout`，因此可以在 PC 上对库做单元测试。
- **要求**：C++11 或更高（由 `static_assert` 保证）。非 AVR 目标使用 `<cassert>`/`<cstdarg>`/`<cstdint>`/`<cstdio>`；AVR 上使用对应的 C 头文件。
