#include "cyf.h"  // Triggers Arduino IDE to locate cyfney-cpp and add src/ to include path, making "cyf/xxx.h" includes below resolvable
#include "cyf/log.h"
#include "cyf/timed_majority_filter.h"

namespace {
constexpr uint32_t kWindowMs = 2000;
constexpr uint16_t kMinCount = 5;

cyf::TimedMajorityFilter<uint8_t, uint32_t, uint32_t (*)()> g_filter(kWindowMs, millis, kMinCount);

uint8_t g_true_state = 0;
}  // namespace

void setup() {
  Serial.begin(115200);
  CLOGI("Timed Majority Filter Demo");
  randomSeed(analogRead(0));
}

void loop() {
  // Simulate a noisy sensor reading: 70% of samples match the true state,
  // 30% are flipped. The filter should recover the true state as the
  // majority value once the 2 s window elapses.
  const uint8_t noisy = (random(100) < 70) ? g_true_state : (uint8_t)(1 - g_true_state);
  g_filter.Push(noisy);

  // Flip the true state occasionally so the filter's debounce is visible.
  static uint32_t s_next_flip_ms = 8000;
  if (millis() >= s_next_flip_ms) {
    g_true_state ^= 1;
    s_next_flip_ms = millis() + 8000;
    CLOGI("true state flipped to %u", g_true_state);
  }

  uint8_t decision = 0;
  if (g_filter.Flush(decision)) {
    CLOGI("majority decision: %u (true state: %u)", decision, g_true_state);
  }

  delay(200);
}
