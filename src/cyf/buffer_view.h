#pragma once

#include <Print.h>
#include <stdint.h>

namespace cyf {
class BufferView {
 public:
  BufferView(const uint8_t* data, size_t length) noexcept : data_(data), length_(length) {}
  const uint8_t* data() const noexcept { return data_; }
  size_t length() const noexcept { return length_; }
  size_t WriteTo(Print& print) const noexcept { return print.write(data_, length_); }

 private:
  const uint8_t* data_;
  const size_t length_;
};
}  // namespace cyf