#include "cyf.h"  // Triggers Arduino IDE to locate cyfney-cpp and add src/ to include path, making "cyf/xxx.h" includes below resolvable
#include "cyf/fifo.h"
#include "cyf/log.h"

#ifdef ARDUINO_ARCH_AVR
#error "AVR is not supported"
#endif

// Global FIFO: buffer_size = 8 (power of two, stores max 7 elements)
static cyf::Fifo<uint8_t, 8> g_fifo;

void setup() {
  Serial.begin(115200);

  CLOGI("=== Fifo Basic Demo ===");
  CLOGI("Buffer max size: %u", g_fifo.max_size());

  // ----------------------------------------------------------
  // 1. Push & Pop basic
  // ----------------------------------------------------------
  CLOGI("\n--- 1. Basic Push & Pop ---");

  uint8_t write_data[] = {10, 20, 30, 40};
  size_t written = g_fifo.Push(write_data, 4);
  CLOGI("Pushed %u elements: {10, 20, 30, 40}", written);
  CLOGI("count=%u, free=%u, empty=%d, full=%d", g_fifo.count(), g_fifo.free(), g_fifo.empty(),
        g_fifo.full());

  uint8_t read_buf[4] = {0};
  size_t read = g_fifo.Pop(read_buf, 4);
  CLOGI("Popped %u elements:", read);
  CLOGI("  read_buf = {%u, %u, %u, %u}", read_buf[0], read_buf[1], read_buf[2], read_buf[3]);
  CLOGI("count=%u, empty=%d", g_fifo.count(), g_fifo.empty());

  // ----------------------------------------------------------
  // 2. Fill to full, then push without overwrite
  // ----------------------------------------------------------
  CLOGI("\n--- 2. Fill to Full (no overwrite) ---");

  g_fifo.Clear();  // reset to empty

  // Fill all 7 usable slots (max_size = 7 for N=8)
  uint8_t fill_data[7] = {1, 2, 3, 4, 5, 6, 7};
  written = g_fifo.Push(fill_data, 7);
  CLOGI("Filled %u elements", written);
  CLOGI("count=%u, full=%d", g_fifo.count(), g_fifo.full());

  // Try to push more without overwrite — should push 0
  uint8_t extra = 99;
  written = g_fifo.Push(&extra, 1, /*overwrite=*/false);
  CLOGI("Push with overwrite=false when full: pushed %u (expected 0)", written);

  // ----------------------------------------------------------
  // 3. Overwrite mode — oldest data gets evicted
  // ----------------------------------------------------------
  CLOGI("\n--- 3. Overwrite Mode ---");

  g_fifo.Clear();
  uint8_t init_data[] = {1, 2, 3};
  g_fifo.Push(init_data, 3);
  CLOGI("Before overwrite: count=%u, front=%u", g_fifo.count(), g_fifo.front());

  // Push 2 more with overwrite — total 5, still fits, no eviction
  uint8_t more[] = {4, 5};
  g_fifo.Push(more, 2, /*overwrite=*/true);
  CLOGI("After pushing 4,5 (no eviction): count=%u", g_fifo.count());

  // Now fill to full then overwrite — oldest gets dropped
  g_fifo.Clear();
  uint8_t seq[] = {10, 20, 30, 40, 50, 60, 70};
  g_fifo.Push(seq, 7);                         // fill all 7 slots
  g_fifo.Push(&extra, 1, /*overwrite=*/true);  // evicts 10, keeps 20..99
  CLOGI("After overwrite when full:");
  CLOGI("  front=%u (10 was evicted)", g_fifo.front());
  CLOGI("  [0]=%u, [1]=%u, [6]=%u", g_fifo[0], g_fifo[1], g_fifo[6]);

  // ----------------------------------------------------------
  // 4. Peek — read without consuming
  // ----------------------------------------------------------
  CLOGI("\n--- 4. Peek (non-destructive read) ---");

  g_fifo.Clear();
  uint8_t pd[] = {100, 101, 102};
  g_fifo.Push(pd, 3);

  uint8_t peek_buf[3] = {0};
  size_t peeked = g_fifo.Peek(peek_buf, 3);
  CLOGI("Peeked %u elements: {%u, %u, %u}", peeked, peek_buf[0], peek_buf[1], peek_buf[2]);
  CLOGI("count still = %u (data not consumed)", g_fifo.count());

  // Actual pop consumes data
  g_fifo.Pop(peek_buf, 1);
  CLOGI("After popping 1 element: count=%u, front=%u", g_fifo.count(), g_fifo.front());

  // ----------------------------------------------------------
  // 5. operator[] — zero-copy random access
  // ----------------------------------------------------------
  CLOGI("\n--- 5. operator[] Random Access ---");

  CLOGI("Element at index 0: %u", g_fifo[0]);
  CLOGI("Element at index 1: %u", g_fifo[1]);

  // ----------------------------------------------------------
  // 6. Consume — manually move read pointer (useful for DMA)
  // ----------------------------------------------------------
  CLOGI("\n--- 6. Consume (DMA-style) ---");

  CLOGI("Before Consume(2): count=%u, front=%u", g_fifo.count(), g_fifo.front());
  g_fifo.Consume(2);  // consume 2 elements without copying
  CLOGI("After Consume(2):  count=%u, front=%u", g_fifo.count(), g_fifo.front());

  // ----------------------------------------------------------
  // 7. Pop() — single element, convenience method
  // ----------------------------------------------------------
  CLOGI("\n--- 7. Pop Single Element ---");

  g_fifo.Clear();
  g_fifo.Push(seq, 3);  // {10, 20, 30}
  uint8_t val = 0;
  bool ok = g_fifo.Pop(val);
  CLOGI("Pop(val) returned: %d, val=%u, remaining count=%u", ok, val, g_fifo.count());

  // ----------------------------------------------------------
  // 8. External buffer usage via FifoCore
  // ----------------------------------------------------------
  CLOGI("\n--- 8. FifoCore (external buffer) ---");

  uint8_t ext_buf[16];  // 16 bytes, power of two
  cyf::FifoCore<uint8_t> rb(ext_buf, 16);

  rb.Push(init_data, 3);
  CLOGI("Core: pushed 3 elements, count=%u, max_size=%u", rb.count(), rb.max_size());

  CLOGI("\n=== Demo Complete ===");
}

void loop() {
  // Nothing to do here
}
