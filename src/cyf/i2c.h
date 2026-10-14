#pragma once

#ifndef CYF_I2C_H_
#define CYF_I2C_H_

#include <Wire.h>
#include <stdint.h>

namespace cyf {
namespace i2c {
namespace internal {
inline void Check(bool success) {
  if (!success) {
    abort();
  }
}
}  // namespace internal

/**
 * @brief Reads raw bytes from the I2C device at the specified register/memory address.
 *
 * @param wire The TwoWire instance (e.g., Wire).
 * @param dev_addr The 7-bit I2C device address.
 * @param addr The 8-bit register/memory address to read from.
 * @param data Buffer to store the read data.
 * @param length Number of bytes to read.
 */
inline void Read(TwoWire& wire, uint8_t dev_addr, uint8_t addr, uint8_t* data, size_t length) {
  wire.beginTransmission(dev_addr);
  wire.write(addr);
  internal::Check(wire.endTransmission() == 0);
  internal::Check(wire.requestFrom(dev_addr, length) == length);

  size_t offset = 0;

  while (offset < length) {
    offset += wire.readBytes(data + offset, length - offset);
  }
}

/**
 * @brief Writes raw bytes to the I2C device at the specified register/memory address.
 *
 * @param wire The TwoWire instance.
 * @param dev_addr The 7-bit I2C device address.
 * @param addr The 8-bit register/memory address to write to.
 * @param data Buffer containing the data to write.
 * @param length Number of bytes to write.
 */
inline void Write(TwoWire& wire, uint8_t dev_addr, uint8_t addr, const uint8_t* data, size_t length) {
  wire.beginTransmission(dev_addr);
  wire.write(addr);
  wire.write(data, length);
  internal::Check(wire.endTransmission() == 0);
}

/**
 * @brief Reads a value of type T from the I2C device at the specified register/memory address.
 *
 * @tparam T The type of data to read (e.g., uint8_t, int32_t, float, struct).
 * @param wire The TwoWire instance.
 * @param dev_addr The 7-bit I2C device address.
 * @param addr The 8-bit register/memory address to read from.
 * @return The value read from the device.
 */
template <typename T>
inline T Read(TwoWire& wire, uint8_t dev_addr, uint8_t addr) {
  T value{};
  Read(wire, dev_addr, addr, reinterpret_cast<uint8_t*>(&value), sizeof(T));
  return value;
}

/**
 * @brief Writes a value of type T to the I2C device at the specified register/memory address.
 *
 * @tparam T The type of data to write.
 * @param wire The TwoWire instance.
 * @param dev_addr The 7-bit I2C device address.
 * @param addr The 8-bit register/memory address to write to.
 * @param value The value to write.
 */
template <typename T>
inline void Write(TwoWire& wire, uint8_t dev_addr, uint8_t addr, T& value) {
  Write(wire, dev_addr, addr, reinterpret_cast<const uint8_t*>(&value), sizeof(T));
}
}  // namespace i2c
}  // namespace cyf

#endif