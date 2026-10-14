#include <Wire.h>

#include "cyf.h"
#include "cyf/i2c_wide.h"

void setup() {
  Wire.begin();

  // Read uint8_t from I2C device 0x40 at register/memory address 0x1234
  [[maybe_unused]] uint8_t uint8_read = cyf::i2c::wide::Read<uint8_t>(Wire, 0x40, 0x1234);

  // Read uint16_t from I2C device with address 0x40 and register/memory address 0x1234
  [[maybe_unused]] uint16_t uint16_read = cyf::i2c::wide::Read<uint16_t>(Wire, 0x40, 0x1234);

  // Read uint32_t from I2C device with address 0x40 and register/memory address 0x1234
  [[maybe_unused]] uint32_t uint32_read = cyf::i2c::wide::Read<uint32_t>(Wire, 0x40, 0x1234);

  // Read int32_t from I2C device with address 0x40 and register/memory address 0x1234
  [[maybe_unused]] int32_t int32_read = cyf::i2c::wide::Read<int32_t>(Wire, 0x40, 0x1234);

  // Read buffer from I2C device with address 0x40 and register/memory address 0x1234
  uint8_t buffer_read[3] = {0};
  cyf::i2c::wide::Read(Wire, 0x40, 0x1234, buffer_read, sizeof(buffer_read));

  // Write uint8_t to I2C device with address 0x40 and register/memory address 0x1234
  uint8_t uint8_write = 0x10;
  cyf::i2c::wide::Write(Wire, 0x40, 0x1234, uint8_write);

  // Write uint16_t to I2C device with address 0x40 and register/memory address 0x1234
  uint16_t uint16_write = 0x5678;
  cyf::i2c::wide::Write(Wire, 0x40, 0x1234, uint16_write);

  // Write uint32_t to I2C device with address 0x40 and register/memory address 0x1234
  uint32_t uint32_write = 11223344;
  cyf::i2c::wide::Write(Wire, 0x40, 0x1234, uint32_write);

  // Write int32_t to I2C device with address 0x40 and register/memory address 0x1234
  int32_t int32_write = -11223344;
  cyf::i2c::wide::Write(Wire, 0x40, 0x1234, int32_write);

  // Write buffer to I2C device with address 0x40 and register/memory address 0x1234
  uint8_t buffer_write[3] = {0x10, 0x20, 0x30};
  cyf::i2c::wide::Write(Wire, 0x40, 0x1234, buffer_write, sizeof(buffer_write));
}

void loop() {}