#pragma once

#ifndef CYF_BYTE_BUFFER_H_
#define CYF_BYTE_BUFFER_H_

#include <Print.h>
#include <stddef.h>
#include <stdint.h>

namespace cyf {
class ByteBuffer {
 public:
  explicit ByteBuffer(size_t capacity) noexcept : data_(new uint8_t[capacity]), capacity_(capacity), length_(0) {}
  ~ByteBuffer() noexcept { delete[] data_; }
  ByteBuffer(ByteBuffer&& other) noexcept : data_(other.data_), capacity_(other.capacity_), length_(other.length_) {
    other.data_ = nullptr;
    other.capacity_ = 0;
    other.length_ = 0;
  }

  ByteBuffer& operator=(ByteBuffer&& other) noexcept {
    if (this != &other) {
      delete[] data_;
      data_ = other.data_;
      capacity_ = other.capacity_;
      length_ = other.length_;
      other.data_ = nullptr;
    }
    return *this;
  }

  bool empty() const noexcept { return length_ == 0; }
  void clear() noexcept { length_ = 0; }
  uint8_t* data() noexcept { return data_; }
  const uint8_t* data() const noexcept { return data_; }
  size_t capacity() const noexcept { return capacity_; }
  size_t length() const noexcept { return length_; }
  void set_length(size_t length) noexcept { length_ = length; }
  size_t WriteTo(Print& print) const noexcept { return print.write(data_, length_); }
  const uint8_t& operator[](size_t pos) const noexcept { return *(data_ + pos); }
  uint8_t& operator[](size_t pos) noexcept { return *(data_ + pos); }

 private:
  ByteBuffer(const ByteBuffer&) = delete;
  ByteBuffer& operator=(const ByteBuffer&) = delete;

  uint8_t* data_ = nullptr;
  size_t capacity_ = 0;
  size_t length_ = 0;
};
}  // namespace cyf
#endif