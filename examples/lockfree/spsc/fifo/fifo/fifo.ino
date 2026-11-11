// Basic demo of the lock-free SPSC FIFO (single task, API walkthrough).
//
// Demonstrates Push/Pop (single + block), front(), empty(), DiscardAll() and
// ConsumeAll(f).
// Push never blocks and never overwrites: when the FIFO is full it returns
// false (or pushes fewer elements for the block form). This demo exercises
// the API from one task; for a two-core concurrency stress test see the
// fifo_sync example.

#include "cyf.h"
#include "cyf/lockfree/spsc/fifo.h"
#include "cyf/log.h"

namespace {
// N = 8 -> usable capacity 7 (one slot is reserved to tell full from empty).
cyf::lockfree::spsc::Fifo<uint8_t, 8> g_fifo;
}  // namespace

void setup() {
  Serial.begin(115200);

  CLOGI("=== SPSC FIFO Basic Demo ===");
  CLOGI("Lock-free single-producer/single-consumer ring buffer.");
  CLOGI("Capacity: 7 elements (Fifo<uint8_t, 8>, one slot reserved).");
  CLOGI("For a two-core concurrency stress test, see the fifo_sync example.");

  // ----------------------------------------------------------
  // 1. Push & Pop single elements
  // ----------------------------------------------------------
  CLOGI("\n--- 1. Push / Pop single elements ---");

  CLOGI("empty() at start: %d", g_fifo.empty());

  CLOGI("Push(10) -> %d", g_fifo.Push(static_cast<uint8_t>(10)));
  CLOGI("Push(20) -> %d", g_fifo.Push(static_cast<uint8_t>(20)));
  CLOGI("empty() after 2 pushes: %d", g_fifo.empty());
  CLOGI("front() (next to pop): %u", (unsigned)g_fifo.front());

  uint8_t v = 0;
  CLOGI("Pop(v) -> %d, v=%u", g_fifo.Pop(v), (unsigned)v);
  CLOGI("Pop(v) -> %d, v=%u", g_fifo.Pop(v), (unsigned)v);
  CLOGI("empty() after popping both: %d", g_fifo.empty());

  // ----------------------------------------------------------
  // 2. Block Push & Pop
  // ----------------------------------------------------------
  CLOGI("\n--- 2. Block Push / Pop ---");

  const uint8_t tx[] = {1, 2, 3, 4, 5};
  const size_t pushed = g_fifo.Push(tx, sizeof(tx));
  CLOGI("Push(tx, 5) -> pushed %u", (unsigned)pushed);

  uint8_t rx[8] = {0};
  const size_t popped = g_fifo.Pop(rx, sizeof(rx));
  CLOGI("Pop(rx, 8) -> popped %u: {%u, %u, %u, %u, %u}", (unsigned)popped, (unsigned)rx[0], (unsigned)rx[1],
        (unsigned)rx[2], (unsigned)rx[3], (unsigned)rx[4]);

  // ----------------------------------------------------------
  // 3. Full behavior: Push never blocks, never overwrites
  // ----------------------------------------------------------
  CLOGI("\n--- 3. Full behavior ---");

  g_fifo.DiscardAll();
  for (uint8_t i = 1; i <= 7; ++i) {
    g_fifo.Push(i);  // fill the whole usable capacity
  }
  CLOGI("Filled 7 elements (full capacity), front()=%u", (unsigned)g_fifo.front());

  CLOGI("Push(99) when full -> %d (false: rejected, no overwrite)", g_fifo.Push(static_cast<uint8_t>(99)));

  const uint8_t more[] = {91, 92, 93};
  CLOGI("Push(more, 3) when full -> pushed %u", (unsigned)g_fifo.Push(more, sizeof(more)));
  CLOGI("front() still %u (nothing was dropped)", (unsigned)g_fifo.front());

  // ----------------------------------------------------------
  // 4. DiscardAll — drop everything (consumer side)
  // ----------------------------------------------------------
  CLOGI("\n--- 4. DiscardAll ---");

  g_fifo.DiscardAll();
  CLOGI("DiscardAll() -> empty()=%d", g_fifo.empty());

  // ----------------------------------------------------------
  // 5. ConsumeAll(f) — visit every remaining element, then drop them
  // ----------------------------------------------------------
  CLOGI("\n--- 5. ConsumeAll ---");

  for (uint8_t i = 1; i <= 3; ++i) {
    g_fifo.Push(i);
  }
  uint16_t sum = 0;
  const size_t visited = g_fifo.ConsumeAll([&sum](uint8_t& v) { sum += v; });
  CLOGI("ConsumeAll() visited %u elements, sum=%u, empty()=%d", (unsigned)visited, (unsigned)sum,
        g_fifo.empty());

  CLOGI("\n=== Demo Complete ===");
}

void loop() {
  // Nothing to do here
}
