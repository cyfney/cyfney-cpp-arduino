# Array (`cyf/array.h`)

A fixed-size array container whose elements are stored inside the object itself, so it can be passed and returned by value — a light alternative to `std::array` for environments where `<array>` is unavailable (e.g. Arduino AVR).

## Table of Contents

- [Quick Start](#quick-start)
- [Memory Model](#memory-model)
- [Initialization](#initialization)
- [API](#api)
  - [Type Aliases](#type-aliases)
  - [Size and Data](#size-and-data)
  - [Element Access](#element-access)
  - [Bounds Checking and NDEBUG](#bounds-checking-and-ndebug)
  - [Iteration](#iteration)
  - [Bulk Operations](#bulk-operations)
- [Configuration](#configuration)
- [Platform Notes](#platform-notes)
- [Examples](#examples)
- [See Also](#see-also)

## Quick Start

```cpp
#include "cyf.h"  // Always include the root header first (Arduino library discovery)
#include "cyf/array.h"

void setup() {
  Serial.begin(115200);

  cyf::Array<int, 4> samples = {12, 7, 30, 5};

  int sum = 0;
  for (int v : samples) {
    sum += v;
  }

  samples.Fill(0);      // reset all elements
  samples[2] = 42;      // unchecked write

  Serial.print("size=");
  Serial.println(samples.size());
  Serial.print("sum=");
  Serial.println(sum);
  Serial.print("samples[2]=");
  Serial.println(samples.at(2));
}

void loop() {}
```

Output (serial monitor at 115200):

```text
size=4
sum=54
samples[2]=42
```

## Memory Model

The element storage is a plain public member array `T data[N]`; there is no pointer indirection and no dynamic allocation at any point.

- **No overhead**: `sizeof(cyf::Array<T, N>) == N * sizeof(T)`, with no extra bookkeeping fields. The size is known at compile time, so the object can live on the stack, in static storage, or as a struct member without surprises.
- **Storage is part of the interface**: `Array` is a `struct`, so `data` is public like every other member (see [Size and Data](#size-and-data)). There is no `data()` accessor — a member and a member function cannot share a name — and keeping the storage public is also what preserves aggregate initialization.
- **By-value friendly**: because the elements travel with the object, copying `Array` copies the data. Returning one from a function is safe (no dangling interior pointer) and is subject to copy elision.
- **Pass by value = N copies**: each copy is an element-wise copy of `N` elements. For large `N` or expensive `T`, take `const Array<T, N>&` instead.
- **Copy semantics are implicit**: the copy constructor and copy assignment operator are compiler-generated and copy element-wise; no move constructor is declared, so "moving" an `Array` copies as well. For a trivially copyable `T` the whole object is trivially copyable and can be `memcpy`'d or placed in a union.
- **Layout guarantees for trivial `T`**: standard-layout and trivially copyable, so `Array<char, 4>` occupies exactly 4 bytes and `Array<SomePod, 3>` exactly `3 * sizeof(SomePod)`.

## Initialization

There is no user-declared constructor — `Array` stays an aggregate, which is what keeps brace initialization (and therefore flash-resident constant tables) working.

```cpp
cyf::Array<int, 4> a = {1, 2, 3, 4};  // all elements
cyf::Array<int, 4> b = {1, 2};        // rest are value-initialized: 1 2 0 0
cyf::Array<int, 4> c{};               // all zero
cyf::Array<int, 4> d;                 // indeterminate — do not read before writing

a = {5, 6, 7, 8};                     // works: braced list builds a temporary
```

Nested containers follow normal aggregate nesting — one brace level per dimension:

```cpp
cyf::Array<cyf::Array<int, 2>, 3> m = {{ {1, 2}, {3, 4}, {5, 6} }};
int x = m[1][0];  // 3
```

Reading an element of a default-initialized `Array` (case `d` above) before assigning to it is undefined behaviour: the members are default-initialized, and for scalar `T` that leaves them unspecified. Use `{}` or `Fill()` when you need a known starting state.

## API

All members are `inline` by virtue of being defined in-class; none of them throw.

### Type Aliases

| Alias | Type |
| ----- | ---- |
| `value_type` | `T` |
| `reference` | `T&` |
| `const_reference` | `const T&` |
| `iterator` | `T*` |
| `const_iterator` | `const T*` |

`size_type`, `difference_type`, `pointer`, `const_pointer` are **not** provided. Generic code that spells `typename C::size_type` will not compile against `Array`; use `size_t` (what `size()` returns) and `std::ptrdiff_t`.

### Size and Data

#### `constexpr size_t size() const noexcept`

Returns `N`. `constexpr`, so it is usable in constant expressions:

```cpp
constexpr cyf::Array<int, 4> kTable = {1, 2, 3, 4};
static_assert(kTable.size() == 4, "size() is a compile-time constant");
```

Only `size()` is `constexpr` — `operator[]`, `at()`, `front()` and `back()` are not. A `constexpr cyf::Array<int, 2>` object can be declared and its `size()` used, but elements cannot be read through `operator[]` in a constant expression. The public `data` member is the exception: subscripting it is a built-in array operation, so `k.data[0]` *is* usable in `static_assert`.

#### `T data[N]` (public member)

The raw storage is a public member array, not an accessor function. This is the one place where the container deliberately departs from `std::array`:

```cpp
cyf::Array<uint8_t, 8> a = {};
a.data[0] = 0x55;                 // element access, no parentheses
Serial1.write(a.data, a.size());  // decays to uint8_t*, pairs with (T*, size_t) APIs
memcpy(a.data, src, a.size());
```

Because `N > 0`, `a.data` decays to `T*` in most expressions and equals `&a[0]`; on a `const cyf::Array&` it decays to `const T*`. Three consequences worth knowing:

- There is no `data()` method — `a.data()` does not compile, and no such accessor can be added while the member keeps this name.
- `sizeof(a.data)` is the size of the whole array (`N * sizeof(T)`), not of a pointer.
- Code written for `std::array` is not source-compatible here; drop the parentheses when porting.

### Element Access

| Member | Bounds checking | Notes |
| ------ | --------------- | ----- |
| `T& operator[](size_t pos)` | none | Fastest path; out-of-range access is undefined behaviour |
| `T& at(size_t pos)` | `assert(pos < N)` | Checked in debug builds only; never throws |
| `T& front()` | none | Always valid: returns `data[0]` |
| `T& back()` | none | Always valid: returns `data[N - 1]` |

Each accessor has a `const` overload returning a reference to `const`, so a `const cyf::Array&` is read-only through the usual interface; the `data` member follows the same rule by decaying to `const T*`.

### Bounds Checking and NDEBUG

`at()` is `noexcept` and validates with `assert`, not an exception — it is safe to use with `-fno-exceptions`. Whether the check survives compilation depends on `NDEBUG`:

- **Arduino IDE / arduino-cli builds**: `NDEBUG` is not defined, so the check stays in. A failed `assert` calls `abort()`, which on AVR ends the sketch.
- **Builds with `-DNDEBUG`**: the `assert` expands to nothing and `at()` behaves exactly like `operator[]` — no check, no cost.

So `at()` is a development-time guard, not a runtime validation mechanism. For input that can be out of range (e.g. an index received over a serial link), test `pos < a.size()` explicitly and handle the error yourself.

### Iteration

`begin()` and `end()` return raw pointers, so `end() - begin() == N` and the container works with anything that accepts a pointer range, including `<algorithm>`:

```cpp
for (auto& v : a) {          // mutating
  v *= 2;
}

for (const auto& v : a) {    // read-only
  Serial.println(v);
}

// size_t loop with the container's own bound
for (size_t i = 0; i < a.size(); ++i) {
  a[i] = 0;
}
```

Because the iterators are pointers, `std::sort(a.begin(), a.end())` and `std::find` work on an `Array` without any adaptor.

### Bulk Operations

#### `void Fill(const T& value) noexcept`

Assigns `value` to every element, `O(N)`. Requires `T` to be copy-assignable.

```cpp
cyf::Array<uint8_t, 16> buf;
buf.Fill(0x00);  // every byte is 0x00

cyf::Array<int, 4> z;
z.Fill(0);       // every element is 0
```

The body is a plain assignment loop rather than a `memset` of the storage, so it stays correct for every element type: `Fill(1)` on an `Array<int, N>` writes whole `int` values, and a class type with a user-defined `operator=` runs that operator. As a consequence `Fill` is not `constexpr` — see the note in [Size and Data](#size-and-data).

## Configuration

No macros and no build options. The two template parameters are the entire configuration surface:

| Parameter | Meaning | Constraint |
| --------- | ------- | ---------- |
| `T` | element type | copy-assignable if you use `Fill`; no other requirement |
| `N` | element count | must be `> 0`, enforced by `static_assert` |

`N == 0` fails at compile time with `Array requires N > 0`. A zero-length array is not representable; use a pointer plus length (`buffer_view`) instead of an empty container.

## Platform Notes

- **No standard library dependency**: the header includes only `<assert.h>` and `<stddef.h>` — C headers that avr-libc provides — so it compiles on AVR where `<array>` and the rest of libstdc++ are absent. That is the reason the library ships its own container instead of relying on `std::array`.
- **AVR**: elements live in RAM. A large `Array` consumes RAM at the point of definition; to keep a table in flash, declare it `const ... PROGMEM` and read it back with `pgm_read_byte(&tbl[i])` — the container offers no PROGMEM-aware accessors, so `operator[]` on a flash-resident object returns garbage.
- **Exceptions / RTTI**: nothing in the module throws, so `-fno-exceptions` and `-fno-rtti` builds are fine.
- **ESP32 / ARM / host**: close to `std::array` in spirit but not source-compatible — the raw storage is the public `data` member rather than `data()`. Usable with `<algorithm>` through the pointer iterators.
- **Requirements**: C++11 or later (`static_assert`, range-based `for`, `noexcept`). Compiles warning-free with `-Wall -Wextra`.
- **Include order**: include `"cyf.h"` before `"cyf/array.h"` so Arduino's library discovery links this library.

## Examples

The Quick Start above runs as-is and is the first part of [`examples/array/array/array.ino`](../../../examples/array/array/array.ino); the sketch adds a partial-initialization demo and prints it as `partial=1 2 0 0`.

## See Also

- [Module documentation standard](../../README.md)
- `src/cyf/buffer_view.h` — non-owning view over externally owned memory, for variable-length data (documentation pending)
- `src/cyf/fixed_buffer.h` — fixed-capacity buffer with a runtime length (documentation pending)
