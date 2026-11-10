# 固定长度数组（`cyf/array.h`）

元素内嵌在对象内部的固定长度数组容器，因此可以按值传递和返回——在没有 `<array>` 的环境（如 Arduino AVR）中作为 `std::array` 的轻量替代品。

## 目录

- [快速上手](#快速上手)
- [内存模型](#内存模型)
- [初始化](#初始化)
- [API](#api)
  - [类型别名](#类型别名)
  - [大小与数据指针](#大小与数据指针)
  - [元素访问](#元素访问)
  - [边界检查与 NDEBUG](#边界检查与-ndebug)
  - [迭代](#迭代)
  - [批量操作](#批量操作)
- [配置](#配置)
- [平台说明](#平台说明)
- [示例](#示例)
- [相关模块](#相关模块)

## 快速上手

```cpp
#include "cyf.h"  // 必须先包含根头文件（Arduino 库发现机制依赖它）
#include "cyf/array.h"

void setup() {
  Serial.begin(115200);

  cyf::Array<int, 4> samples = {12, 7, 30, 5};

  int sum = 0;
  for (int v : samples) {
    sum += v;
  }

  samples.Fill(0);      // 重置全部元素
  samples[2] = 42;      // 无边界检查的写入

  Serial.print("size=");
  Serial.println(samples.size());
  Serial.print("sum=");
  Serial.println(sum);
  Serial.print("samples[2]=");
  Serial.println(samples.at(2));
}

void loop() {}
```

输出（串口监视器 115200）：

```text
size=4
sum=54
samples[2]=42
```

## 内存模型

元素存储就是一个普通的公开成员数组 `T data[N]`：没有指针间接层，任何时刻都不做动态分配。

- **零额外开销**：`sizeof(cyf::Array<T, N>) == N * sizeof(T)`，没有额外的簿记字段。大小在编译期确定，因此对象可以放心地放在栈上、静态存储区，或作为结构体成员。
- **存储本身就是接口**：`Array` 是 `struct`，所以 `data` 和其余成员一样公开（见[大小与数据指针](#大小与数据指针)）。不提供 `data()` 访问器——成员与成员函数不能同名——而存储保持公开也正是聚合初始化得以成立的前提。
- **适合按值传递**：元素随对象一起移动，拷贝 `Array` 就是拷贝数据。从函数返回是安全的（不存在悬垂的内部指针），并可享受拷贝消除（copy elision）。
- **按值传参 = N 次拷贝**：每次拷贝都是 `N` 个元素的逐元素拷贝。`N` 较大或 `T` 构造成本高时，请改用 `const Array<T, N>&`。
- **拷贝语义为隐式生成**：拷贝构造函数与拷贝赋值运算符由编译器生成，逐元素拷贝；没有声明移动构造函数，因此"移动"`Array` 同样是拷贝。`T` 可平凡拷贝时，整个对象也可平凡拷贝，可以直接 `memcpy` 或放进 union。
- **平凡 `T` 的布局保证**：standard-layout 且 trivially copyable，`Array<char, 4>` 恰好占 4 字节，`Array<SomePod, 3>` 恰好占 `3 * sizeof(SomePod)`。

## 初始化

`Array` 没有自定义构造函数——它保持聚合类型（aggregate），这正是花括号初始化和 flash 常量表能够工作的原因。

```cpp
cyf::Array<int, 4> a = {1, 2, 3, 4};  // 全部元素
cyf::Array<int, 4> b = {1, 2};        // 其余元素值初始化：1 2 0 0
cyf::Array<int, 4> c{};               // 全部为 0
cyf::Array<int, 4> d;                 // 值不确定——写入前不要读取

a = {5, 6, 7, 8};                     // 可行：花括号列表构造临时对象后赋值
```

嵌套容器遵循普通的聚合嵌套规则——每维一层花括号：

```cpp
cyf::Array<cyf::Array<int, 2>, 3> m = {{ {1, 2}, {3, 4}, {5, 6} }};
int x = m[1][0];  // 3
```

在上例的 `d` 这类默认初始化的 `Array` 上读取元素是未定义行为：成员被默认初始化，对标量类型 `T` 而言其值不确定。需要确定的初始状态时，请使用 `{}` 或 `Fill()`。

## API

所有成员函数都定义在类内，因此隐含 `inline`；它们都不会抛出异常。

### 类型别名

| 别名 | 类型 |
| ---- | ---- |
| `value_type` | `T` |
| `reference` | `T&` |
| `const_reference` | `const T&` |
| `iterator` | `T*` |
| `const_iterator` | `const T*` |

**不提供** `size_type`、`difference_type`、`pointer`、`const_pointer`。泛型代码中写成 `typename C::size_type` 的地方无法通过 `Array` 编译；请使用 `size_t`（`size()` 的返回类型）和 `std::ptrdiff_t`。

### 大小与数据指针

#### `constexpr size_t size() const noexcept`

返回 `N`。因为是 `constexpr`，可用于常量表达式：

```cpp
constexpr cyf::Array<int, 4> kTable = {1, 2, 3, 4};
static_assert(kTable.size() == 4, "size() is a compile-time constant");
```

只有 `size()` 是 `constexpr`——`operator[]`、`at()`、`front()`、`back()` 都不是。可以声明 `constexpr cyf::Array<int, 2>` 对象并使用其 `size()`，但无法在常量表达式中通过 `operator[]` 读取元素。公开成员 `data` 是例外：对它下标属于内建数组操作，因此 `k.data[0]` 可以用在 `static_assert` 里。

#### `T data[N]`（公开成员）

原始存储是公开成员数组，不是访问器函数。这是本容器刻意偏离 `std::array` 的唯一之处：

```cpp
cyf::Array<uint8_t, 8> a = {};
a.data[0] = 0x55;                 // 元素访问，不带括号
Serial1.write(a.data, a.size());  // 退化为 uint8_t*，配合 (T*, size_t) 接口
memcpy(a.data, src, a.size());
```

由于 `N > 0`，在多数表达式中 `a.data` 退化为 `T*` 且等于 `&a[0]`；在 `const cyf::Array&` 上则退化为 `const T*`。有三点需要注意：

- 没有 `data()` 方法——`a.data()` 无法编译；只要成员用这个名字，就不可能再加同名访问器。
- `sizeof(a.data)` 是整个数组的大小（`N * sizeof(T)`），不是指针大小。
- 为 `std::array` 写的代码在这里不能直接编译，移植时需去掉括号。

### 元素访问

| 成员 | 边界检查 | 说明 |
| ---- | -------- | ---- |
| `T& operator[](size_t pos)` | 无 | 最快的路径；越界访问为未定义行为 |
| `T& at(size_t pos)` | `assert(pos < N)` | 仅在 debug 构建中检查；不抛异常 |
| `T& front()` | 无 | 始终有效：返回 `data[0]` |
| `T& back()` | 无 | 始终有效：返回 `data[N - 1]` |

每个访问接口都有 `const` 重载，返回指向 `const` 的引用，因此通过常规接口访问 `const cyf::Array&` 是只读的；`data` 成员按同样的规则通过退化为 `const T*` 保持只读。

### 边界检查与 NDEBUG

`at()` 是 `noexcept`，并用 `assert` 而非异常做校验——可以放心配合 `-fno-exceptions` 使用。检查是否保留在编译产物中取决于 `NDEBUG`：

- **Arduino IDE / arduino-cli 构建**：未定义 `NDEBUG`，检查保留。`assert` 失败会调用 `abort()`，在 AVR 上即意味着 sketch 终止。
- **使用 `-DNDEBUG` 的构建**：`assert` 展开为空，`at()` 的行为与 `operator[]` 完全一致——无检查、无开销。

因此 `at()` 是开发期的护栏，而不是运行期的校验机制。对于可能越界的输入（例如从串口收到的下标），请显式判断 `pos < a.size()` 并自行处理错误。

### 迭代

`begin()` 与 `end()` 返回裸指针，因此 `end() - begin() == N`，且容器可用于任何接受指针区间的接口，包括 `<algorithm>`：

```cpp
for (auto& v : a) {          // 可写遍历
  v *= 2;
}

for (const auto& v : a) {    // 只读遍历
  Serial.println(v);
}

// 使用容器自身边界的大小循环
for (size_t i = 0; i < a.size(); ++i) {
  a[i] = 0;
}
```

由于迭代器就是指针，`std::sort(a.begin(), a.end())` 和 `std::find` 无需任何适配即可用于 `Array`。

### 批量操作

#### `void Fill(const T& value) noexcept`

把每个元素都赋成 `value`，`O(N)`。要求 `T` 可拷贝赋值。

```cpp
cyf::Array<uint8_t, 16> buf;
buf.Fill(0x00);  // 每个字节都是 0x00

cyf::Array<int, 4> z;
z.Fill(0);       // 每个元素都是 0
```

函数体是普通的赋值循环，而不是对存储做 `memset`，因此对任何元素类型都成立：`Array<int, N>` 上的 `Fill(1)` 写入完整的 `int` 值，定义了 `operator=` 的类类型也会真正调用该运算符。代价是 `Fill` 不是 `constexpr`——见[大小与数据指针](#大小与数据指针)中的说明。

## 配置

没有宏，也没有编译期选项。两个模板参数就是全部可配置项：

| 参数 | 含义 | 约束 |
| ---- | ---- | ---- |
| `T` | 元素类型 | 若使用 `Fill`，需可拷贝赋值；此外无要求 |
| `N` | 元素个数 | 必须 `> 0`，由 `static_assert` 强制 |

`N == 0` 会在编译期报错并给出 `Array requires N > 0`。零长度数组无法表达；如果有变长需求，请改用"指针 + 长度"的形式（`buffer_view`），而不是空容器。

## 平台说明

- **不依赖标准库**：头文件仅包含 `<assert.h>` 和 `<stddef.h>`——两者都是 avr-libc 提供的 C 头文件——因此在缺少 `<array>` 及 libstdc++ 其余部分的 AVR 上也能编译。这是 Arduino 库中优先选择它而非 `std::array` 的主要原因。
- **AVR**：元素位于 RAM。定义大的 `Array` 会在定义处占用 RAM；若要把表放进 flash，请声明为 `const ... PROGMEM`，并用 `pgm_read_byte(&tbl[i])` 读取——容器不提供 PROGMEM 感知的访问接口，对 flash 对象使用 `operator[]` 只会得到垃圾值。
- **异常 / RTTI**：模块内不会抛出任何异常，可安全使用 `-fno-exceptions` 与 `-fno-rtti` 构建。
- **ESP32 / ARM / 宿主机**：思路与 `std::array` 相近，但源码级不兼容——原始存储是公开的 `data` 成员而不是 `data()`。通过指针迭代器可用于 `<algorithm>`。
- **要求**：C++11 或更高（`static_assert`、range-based `for`、`noexcept`）。在 `-Wall -Wextra` 下编译无警告。
- **包含顺序**：先包含 `"cyf.h"` 再包含 `"cyf/array.h"`，以保证 Arduino 的库发现机制链接到本库。

## 示例

上面的快速上手可直接运行，是 [`examples/array/array/array.ino`](../../../examples/array/array/array.ino) 的前半部分；该 sketch 另外演示了部分聚合初始化，并打印 `partial=1 2 0 0`。

## 相关模块

- [模块文档标准](../../README.md)
- `src/cyf/buffer_view.h` —— 对外部内存的非拥有视图，适合变长数据（文档待写）
- `src/cyf/fixed_buffer.h` —— 容量固定、长度在运行期变化的缓冲区（文档待写）
