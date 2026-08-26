#include "cyf.h"  // must be included first
#include "cyf/log.h"

void setup() {
  Serial.begin(115200);

// CLOGx macros use printf-style format strings
// On AVR, %f is not supported by default — use integer scaling instead
#if defined(ARDUINO_ARCH_AVR)
  int temp = 314;  // represents 3.14
  CLOGI("Hello %s! count=%d, hex=0x%X, float=%d.%02d", "world", 42, 0xAB, temp / 100, temp % 100);
  // -> Hello world! count=42, hex=0xAB, float=3.14
#else
  CLOGI("Hello %s! count=%d, hex=0x%08X, float=%.2f", "world", 42, 0xAB, 3.14f);
  // -> Hello world! count=42, hex=0x000000AB, float=3.14
#endif

  // Default severity is INFO — CLOGV and CLOGD are compiled out (zero overhead)
  CLOGV("Verbose: compiled out");
  CLOGD("Debug: compiled out");
  CLOGI("Info: visible");     // ← enabled
  CLOGW("Warning: visible");  // ← enabled
  CLOGE("Error: visible");    // ← enabled
}

void loop() {
  static int s_count = 0;
  CLOGI("tick: %d", s_count++);
  delay(1000);
}