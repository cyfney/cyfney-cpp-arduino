#include "cyf.h"  // Triggers Arduino IDE to locate cyfney-cpp and add src/ to include path, making "cyf/xxx.h" includes below resolvable
#include "cyf/log.h"
#include "cyf/robust_frame.h"

namespace {
constexpr size_t kMaxPayloadSize = 20;
cyf::robust_frame::Parser g_parser(kMaxPayloadSize);
}  // namespace

void setup() {
  Serial.begin(115200);
  CLOGI("Robust Frame Example");
  randomSeed(analogRead(0));
}

void loop() {
  static uint32_t s_count = 0;
  CLOGI("---");
  CLOGI("count: %" PRIu32, s_count++);

  const size_t data_length = random(1, kMaxPayloadSize);
  CLOGI("data length: %u", data_length);
  uint8_t data[data_length] = {0};
  {
    String data_str;
    for (size_t i = 0; i < data_length; ++i) {
      data[i] = random(0xFF);
      data_str += "0x";
      data_str += String(data[i], HEX);
      data_str += " ";
    }

    CLOGI("data: %s", data_str.c_str());
  }

  auto buffer = cyf::robust_frame::Serialize(data, data_length);
  size_t index = 0;
  bool got_decoded_data = false;

  for (; index < buffer.size(); index++) {
    auto decoded_data = g_parser.Process(buffer[index]);

    if (decoded_data.data() != nullptr) {
      if (got_decoded_data) {
        CLOGE("Error: got decoded data twice");
        while (true);
      }

      got_decoded_data = true;

      if (decoded_data.length() != data_length) {
        CLOGE("Error: data length mismatch");
        while (true);
      }

      if (0 != memcmp(data, decoded_data.data(), decoded_data.length())) {
        CLOGE("Error: data mismatch");
        while (true);
      }
    }
  }

  if (index != buffer.size()) {
    CLOGE("Error: incomplete frame");
    while (true);
  }

  if (!got_decoded_data) {
    CLOGE("Error: no decoded data");
    while (true);
  }

  CLOGI("Success");

  delay(1000);
}