# 模块文档标准（草案 v0.1）

本文档定义 `docs/` 目录的组织方式与每篇模块文档的写作规范。状态：**草案，待评审**。定稿前 `src/cyf/log/log` 一篇已按此结构完成，可作为参照样例。

## 1. 文件组织

### 1.1 路径映射规则

文档路径由头文件路径机械推导，一网打尽、无需记忆：

```text
src/<头文件路径>.h  →  docs/<头文件路径去 .h>/<模块名>.md          （英文，canonical）
                       docs/<头文件路径去 .h>/<模块名>.zh-CN.md    （中文）
```

头文件所在目录结构与文档目录结构一一对应，`examples` 已采用同样的镜像规则（`examples/<头文件路径>/<sketch>/<sketch>.ino`），三棵树对齐。

### 1.2 全部模块映射表

| 头文件 | 英文文档 | 中文文档 | 状态 |
| ------ | -------- | -------- | ---- |
| `src/cyf.h` | `docs/cyf/cyf.md` | `docs/cyf/cyf.zh-CN.md` | 待写（库版本常量，内容少，可与 README 合并说明，定稿时再定） |
| `src/cyf/array.h` | `docs/cyf/array/array.md` | `docs/cyf/array/array.zh-CN.md` | ✅ 已完成 |
| `src/cyf/buffer_view.h` | `docs/cyf/buffer_view/buffer_view.md` | `docs/cyf/buffer_view/buffer_view.zh-CN.md` | 待写 |
| `src/cyf/byte_buffer.h` | `docs/cyf/byte_buffer/byte_buffer.md` | `docs/cyf/byte_buffer/byte_buffer.zh-CN.md` | 待写 |
| `src/cyf/crc8.h` | `docs/cyf/crc8/crc8.md` | `docs/cyf/crc8/crc8.zh-CN.md` | 待写 |
| `src/cyf/fifo.h` | `docs/cyf/fifo/fifo.md` | `docs/cyf/fifo/fifo.zh-CN.md` | 待写 |
| `src/cyf/fixed_buffer.h` | `docs/cyf/fixed_buffer/fixed_buffer.md` | `docs/cyf/fixed_buffer/fixed_buffer.zh-CN.md` | 待写 |
| `src/cyf/hysteresis_quantizer.h` | `docs/cyf/hysteresis_quantizer/hysteresis_quantizer.md` | `docs/cyf/hysteresis_quantizer/hysteresis_quantizer.zh-CN.md` | 待写 |
| `src/cyf/i2c.h` | `docs/cyf/i2c/i2c.md` | `docs/cyf/i2c/i2c.zh-CN.md` | 待写 |
| `src/cyf/i2c_wide.h` | `docs/cyf/i2c_wide/i2c_wide.md` | `docs/cyf/i2c_wide/i2c_wide.zh-CN.md` | 待写 |
| `src/cyf/lockfree/spsc/fifo.h` | `docs/cyf/lockfree/spsc/fifo/fifo.md` | `docs/cyf/lockfree/spsc/fifo/fifo.zh-CN.md` | 待写 |
| `src/cyf/log.h` | `docs/cyf/log/log.md` | `docs/cyf/log/log.zh-CN.md` | ✅ 已完成 |
| `src/cyf/robust_frame.h` | `docs/cyf/robust_frame/robust_frame.md` | `docs/cyf/robust_frame/robust_frame.zh-CN.md` | 待写 |
| `src/cyf/timed_majority_filter.h` | `docs/cyf/timed_majority_filter/timed_majority_filter.md` | `docs/cyf/timed_majority_filter/timed_majority_filter.zh-CN.md` | 待写 |

### 1.3 命名细节

- 文档文件名 = 头文件名去掉 `.h`，全小写 snake_case，与源文件保持一致。
- 子命名空间模块（如 `lockfree/spsc/fifo.h`）按目录层级展开，不拍平。
- 模块新增/改名/删除时，同步维护本表状态列。

## 2. 语言策略

- **英文版为 canonical**：以 `<模块名>.md` 命名，内容以英文为准。
- 中文版 `<模块名>.zh-CN.md` 是英文版的完整翻译，**章节结构、代码示例、表格与英文版一一对应**，不增删章节。
- 技术名词（lock-free、sink、severity 等）在中文版中保留英文原文，不强行翻译。
- 语言切换链接：两份文档开头不放互链（目录树已并排，避免链接维护成本）。

## 3. 文档结构模板

每篇模块文档按以下骨架组织，章节按需合并但**顺序不变**：

```markdown
# <模块名>（`cyf/<路径>.h`）

<一句话定位：这个模块解决什么问题>

## Table of Contents
<锚点目录，与正文标题一致>

## Quick Start
<最小可运行示例 + 预期输出；优先取材于 examples/ 下对应 sketch>

## <核心概念/工作机制>
<该模块特有：数据结构布局、内存模型、状态机、算法行为等>

## API
<按功能分组的小节或表格；每个公开接口给出签名、语义、前置条件>

## Configuration
<宏、编译期选项、可调参数（如有）>

## Platform Notes
<AVR / ESP32 / 宿主机差异；如无差异可省略>

## Examples
<指向 examples/ 下对应 sketch 的相对链接>

## See Also
<相关模块文档的相对链接（可选）>
```

写作要点：

- **Quick Start 必须可编译**：示例代码与 `examples/` 中的 sketch 保持一致或为其摘录，不写凭空代码。
- **所有示例代码的第一行必须是 `#include "cyf.h"`**：Arduino IDE / arduino-cli 依据 sketch 中的 include 语句做库发现（library discovery），只 include 子目录头文件（如 `"cyf/log.h"`）时可能索引不到本库；先 include 根头文件 `"cyf.h"` 才能保证链接到本库。本库头文件一律使用双引号形式（与 examples 保持一致）：`#include "cyf.h"`、`#include "cyf/xxx.h"`。
- **API 描述行为而非复述实现**：写"调用后 X 会发生"、"线程/中断安全性如何"，不解释内部实现步骤。
- **约束前置条件显式写出**：模板参数取值范围、`static_assert` 条件、assert 前置条件（如 `N > 0`、`levels > 0`）。
- **资源语义必须写清**：谁拥有内存（`ByteBuffer` 转移语义）、栈上分配大小（如 log 格式缓冲区）、是否涉及动态分配。
- **AVR 兼容性单独说明**：涉及 `PROGMEM`、`vsnprintf_P`、C 头文件分流、`%S` 等平台细节时，集中放在 Platform Notes。

## 4. Markdown 格式规范

仓库根目录已有 `.markdownlint.jsonc`（当前仅关闭 MD013 行长限制）。所有 `docs/**/*.md` 必须通过：

```bash
npx markdownlint-cli2 "docs/**/*.md"
```

硬性要求（markdownlint 默认规则之外的重点项）：

- 所有代码块标注语言：C++ 用 `cpp`，shell/命令用 `bash`，输出与纯文本用 `text`。
- 表格采用 padded 风格：`| xxx | yyy |`，分隔行 `| --- | --- |`。
- 相对链接使用正斜杠路径，从当前文档位置出发指向目标文件。
- 中文文档统一使用简体中文，全角标点；代码、命令、标识符内保持半角。
- 本标准文件自身与 README.md 同样受上述规则约束。

## 5. 维护流程

1. 新增/修改模块 API → 同步更新对应两份文档，并在第 1.2 节状态列打勾。
2. 文档改动随代码改动同一提交内完成，不积压。
3. 提交前跑一次 markdownlint（见第 4 节命令）。
4. 根 README 的文档索引（定稿后建立）指向各模块 `docs/.../<模块名>.md`。
