// Cross-core SPSC FIFO consistency test for ESP32.
//
// Two FreeRTOS tasks pinned to different cores exercise the FIFO from both
// sides at the same time:
//   - producer (core 0): keeps pushing a monotonically increasing sequence,
//     waits (randomized vTaskDelay) whenever the FIFO is full.
//   - consumer (core 1): keeps popping, waits (randomized vTaskDelay) whenever
//     the FIFO is empty. Random delays make both sides periodically outpace
//     each other, so full/empty paths are exercised continuously.
//
// Every pushed/popped value is recorded into mutex-protected lists. loop()
// periodically verifies that the read list is a prefix of the written list
// (same order, same values, nothing lost or duplicated), and the consumer
// additionally checks sequence continuity on the fly.

#include <Arduino.h>

#include <atomic>
#include <list>
#include <mutex>

#include "cyf.h"
#include "cyf/lockfree/spsc/fifo.h"
#include "cyf/log.h"

namespace {
constexpr size_t kFifoSize = 512;
constexpr TickType_t kMaxDelayTicks = 10;  // wait delays are uniform in [1, kMaxDelayTicks]
using Value = uint32_t;

cyf::lockfree::spsc::Fifo<Value, kFifoSize> g_fifo;

std::mutex g_data_mutex;
std::list<Value> g_written;
std::list<Value> g_read;

std::atomic<uint32_t> g_pushed{0};
std::atomic<uint32_t> g_popped{0};
std::atomic<uint32_t> g_full_waits{0};
std::atomic<uint32_t> g_empty_waits{0};
std::atomic<uint32_t> g_errors{0};

// xorshift32: tiny PRNG, avoids pulling in <random> on embedded targets.
uint32_t NextRand(uint32_t& state) {
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

TickType_t RandomDelay(uint32_t& state) { return static_cast<TickType_t>(1 + NextRand(state) % kMaxDelayTicks); }

void ProducerTask(void* /*arg*/) {
  uint32_t rand = xTaskGetTickCount() ^ 0x9E3779B9u;
  Value next = 0;
  for (;;) {
    {
      // Record BEFORE pushing: the consumer can only pop a value after Push has
      // published it, so at any instant g_read is a prefix of g_written.
      // Recording afterwards would race with loop()'s snapshot (the consumer
      // may pop and record a value the producer has pushed but not yet logged),
      // producing spurious CONSISTENCY VIOLATION reports.
      std::lock_guard<std::mutex> lock(g_data_mutex);
      g_written.push_back(next);
      ++g_pushed;
    }
    if (g_fifo.Push(next)) {
      ++next;
    } else {
      ++g_full_waits;
      // Sleep WITHOUT holding the mutex, then undo the speculative record.
      // Sleeping under the lock starves the consumer and loop() (it froze the
      // whole test on device; confirmed by a PC stress-test replica).
      vTaskDelay(RandomDelay(rand));
      std::lock_guard<std::mutex> lock(g_data_mutex);
      g_written.pop_back();  // undo the speculative record
      --g_pushed;
    }
  }
}

void ConsumerTask(void* /*arg*/) {
  uint32_t rand = xTaskGetTickCount() ^ 0x85EBCA6Bu;
  Value expected = 0;
  for (;;) {
    Value v = 0;
    if (g_fifo.Pop(v)) {
      {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        g_read.push_back(v);
      }
      if (v != expected) {
        ++g_errors;
        expected = v + 1;  // resync after a discontinuity
      } else {
        ++expected;
      }
      ++g_popped;
    } else {
      ++g_empty_waits;
      vTaskDelay(RandomDelay(rand));
    }
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);

  CLOGI("starting producer on core 0, consumer on core 1");

  xTaskCreatePinnedToCore(ProducerTask, "fifo_producer", 4096, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(ConsumerTask, "fifo_consumer", 4096, nullptr, 1, nullptr, 1);
}

void loop() {
  uint32_t errors = 0;
  size_t pending_written = 0;
  size_t pending_read = 0;
  {
    std::lock_guard<std::mutex> lock(g_data_mutex);
    if (g_read.size() > g_written.size()) {
      ++errors;  // consumer recorded more than producer: invariant broken
    }
    // The read list must be a prefix of the written list (SPSC FIFO order).
    while (!g_read.empty() && !g_written.empty()) {
      if (g_read.front() != g_written.front()) {
        ++errors;
      }
      g_read.pop_front();
      g_written.pop_front();
    }
    pending_written = g_written.size();
    pending_read = g_read.size();
  }

  const uint32_t pushed = g_pushed.load();
  const uint32_t popped = g_popped.load();
  const uint32_t total_errors = g_errors.fetch_add(errors);

  CLOGI("pushed=%u popped=%u verified=%u pending(w/r)=%u/%u", (unsigned)pushed, (unsigned)popped,
        (unsigned)(pushed - pending_written), (unsigned)pending_written, (unsigned)pending_read);
  CLOGI("full_waits=%u empty_waits=%u errors=%u", (unsigned)g_full_waits.load(), (unsigned)g_empty_waits.load(),
        (unsigned)(total_errors + errors));
  if (errors != 0 || total_errors != 0) {
    CLOGE("FIFO CONSISTENCY VIOLATION DETECTED");
  }

  delay(50);
}
