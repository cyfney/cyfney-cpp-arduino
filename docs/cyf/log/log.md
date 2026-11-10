# Log (`cyf/log.h`)

A lightweight, header-only logging facility for Arduino and host builds. Every log line carries a `HH.MM.SS.mmm` timestamp and a `file:line function]` prefix, then goes through a user-replaceable sink (default: `Serial` on Arduino, `stdout` elsewhere).

## Table of Contents

- [Quick Start](#quick-start)
- [Log Macros](#log-macros)
- [Log Level Filtering](#log-level-filtering)
- [Timestamp](#timestamp)
- [Configuration Functions](#configuration-functions)
- [Low-Level API](#low-level-api)
- [Platform Notes](#platform-notes)

## Quick Start

```cpp
#include "cyf.h"  // Always include the root header first (Arduino library discovery)
#include "cyf/log.h"

void setup() {
  Serial.begin(115200);
  CLOGI("boot ok, value=%d", 42);
}

void loop() {}
```

Output (serial monitor at 115200):

```text
I 00.00.03.152 main.cpp:6 setup] boot ok, value=42
```

Other levels (`CLOGV`, `CLOGD`, `CLOGW`, `CLOGE`) work the same way — see [Log Level Filtering](#log-level-filtering) for how they are enabled and disabled.

## Log Macros

| Macro | Level prefix | Typical use |
| ----- | --------------- | ----------- |
| `CLOGV(fmt, ...)` | `V` | Verbose tracing |
| `CLOGD(fmt, ...)` | `D` | Debug information |
| `CLOGI(fmt, ...)` | `I` | Informational messages |
| `CLOGW(fmt, ...)` | `W` | Warnings |
| `CLOGE(fmt, ...)` | `E` | Errors |

All macros are `printf`-style: the first argument is a format string, optional extra arguments are formatted with `vsnprintf` (AVR: `vsnprintf_P`). A newline is appended automatically.

```cpp
CLOGI("count=%u ch=%c str=%s", count, ch, str);
```

Line format:

```text
<prefix> <HH.MM.SS.mmm> <file>:<line> <function>] <message>\n
```

The file name is stripped down to its base name (everything after the last `/` or `\`) at compile time.

## Log Level Filtering

Compile-time filtering. Only macros **at or below** the configured verbosity `CYF_LOG_LEVEL` expand to code; more verbose macros compile to nothing (`void(0)`), leaving zero cost in the binary.

Log levels (higher number = more verbose, 0 disables everything):

| Level | Macro | Notes |
| ----- | ----- | ----- |
| 0 | `CYF_LOG_LEVEL_NONE` | disables all logging |
| 1 | `CYF_LOG_LEVEL_ERROR` | |
| 2 | `CYF_LOG_LEVEL_WARN` | |
| 3 | `CYF_LOG_LEVEL_INFO` | **default** |
| 4 | `CYF_LOG_LEVEL_DEBUG` | |
| 5 | `CYF_LOG_LEVEL_VERBOSE` | |

Set the level before including the header (build flag or define):

```cpp
#define CYF_LOG_LEVEL (CYF_LOG_LEVEL_DEBUG)
#include "cyf/log.h"
```

Or on the compiler command line:

```text
-DCYF_LOG_LEVEL=CYF_LOG_LEVEL_ERROR
```

At the default `INFO` level:

```cpp
CLOGV("verbose");  // compiled out
CLOGD("debug");    // compiled out
CLOGI("info");     // visible
CLOGW("warn");     // visible
CLOGE("error");    // visible
```

A runnable demo with all levels is in `examples/log/log_level/`.

## Timestamp

The timestamp is rendered as `HH.MM.SS.mmm` (12 characters, zero-padded) from a millisecond counter. Only the low 28 bits of the counter are used (wraps every ~3 days); there is no day field.

The millisecond source, in order of priority:

1. A user callback installed via `set_millis_source()`
2. ESP32: `pdTICKS_TO_MS(xTaskGetTickCount())`
3. Any Arduino core: `millis()`
4. Host / no Arduino: `0`

## Configuration Functions

All functions are in namespace `cyf::log`, `noexcept`, and inline. Each configuration can be set **only once** (enforced by `assert`); calling twice aborts in debug builds.

### `set_millis_source(MillisSource source)`

```cpp
using MillisSource = uint32_t (*)();
void set_millis_source(MillisSource source);
```

Overrides the default timestamp source. Must be a function returning elapsed milliseconds (`uint32_t`). Useful on platforms where `millis()` is unavailable, or for testing.

### `set_log_sink(LogSink sink)`

```cpp
using LogSink = void (*)(const char* message, size_t length);
void set_log_sink(LogSink sink);
```

Routes every formatted log line to a custom function. `message` points to the internal format buffer (valid only during the call — copy if you need to keep it); `length` is the number of bytes to write, including the trailing newline. When no sink is set, the default output is `Serial` on Arduino and `stdout` elsewhere.

```cpp
void MySink(const char* message, size_t length) {
  Serial1.write(message, length);  // e.g. log over a second UART
}

void setup() {
  Serial1.begin(115200);
  cyf::log::set_log_sink(MySink);
}
```

### `set_format_buffer_size(size_t size)`

Sets the size (bytes) of the internal formatting buffer. Defaults: **256** on AVR, **512** elsewhere. Must be `> 0`. Longer messages are truncated and the trailing character is replaced by `\n`. Each `Log()` call allocates this buffer on the stack, so keep it comfortably below your stack headroom.

## Low-Level API

### `cyf::log::Log(const char* fmt, ...)`

The core formatting function behind the macros. Prepends the timestamp, formats with `vsnprintf` (AVR: `vsnprintf_P`, format string read from flash), then writes through the sink. No level prefix or `file:line` decoration — the macros add those.

### `cyf::log::ExtractFileNameOffset(path)`

`constexpr` template that returns the index of the first character after the last path separator in a string literal. Used to embed only the base file name in log lines.

## Platform Notes

- **AVR (Uno, Mega, ...)**: The entire format string — including the `file:line` prefix — lives in flash via `PSTR` and is read with `vsnprintf_P`, roughly halving RAM usage. Note that `vsnprintf_P` only reads the *format string* from flash: `%s` arguments must still point to RAM, so the flash-resident file name is passed with `%S` (avr-libc extension). If you call `Log()` directly with a `%s` argument on AVR, the string must be in RAM.
- **ESP32**: Timestamps come from the FreeRTOS tick count by default; without a sink, output goes to `stdout` (visible in the serial monitor when Arduino core logging is enabled).
- **Host / unit tests**: Compiles without `Arduino.h`; timestamps default to `0` and output goes to `stdout`, so the library can be unit-tested on a PC.
- **Requirements**: C++11 or later (enforced by `static_assert`). On non-AVR targets, `<cassert>`/`<cstdarg>`/`<cstdint>`/`<cstdio>` are used; on AVR, the C forms are used.
