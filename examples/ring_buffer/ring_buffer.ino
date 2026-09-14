#include "cyf.h"  // must be included first
#include "cyf/log.h"
#include "cyf/ring_buffer.h"

// Global ring buffer: buffer_size = 8 (power of two, stores max 7 elements)
static cyf::RingBuffer<uint8_t, 8> g_ring_buffer;

void setup() {
  Serial.begin(115200);

  CLOGI("=== RingBuffer Basic Demo ===");
  CLOGI("Buffer max size: %u", g_ring_buffer.max_size());

  // ----------------------------------------------------------
  // 1. Write & Read basic
  // ----------------------------------------------------------
  CLOGI("\n--- 1. Basic Write & Read ---");

  uint8_t write_data[] = {10, 20, 30, 40};
  size_t written = g_ring_buffer.Write(write_data, 4);
  CLOGI("Wrote %u elements: {10, 20, 30, 40}", written);
  CLOGI("count=%u, free=%u, empty=%d, full=%d", g_ring_buffer.count(), g_ring_buffer.free(), g_ring_buffer.empty(),
        g_ring_buffer.full());

  uint8_t read_buf[4] = {0};
  size_t read = g_ring_buffer.Read(read_buf, 4);
  CLOGI("Read %u elements:", read);
  CLOGI("  read_buf = {%u, %u, %u, %u}", read_buf[0], read_buf[1], read_buf[2], read_buf[3]);
  CLOGI("count=%u, empty=%d", g_ring_buffer.count(), g_ring_buffer.empty());

  // ----------------------------------------------------------
  // 2. Fill to full, then write without overwrite
  // ----------------------------------------------------------
  CLOGI("\n--- 2. Fill to Full (no overwrite) ---");

  g_ring_buffer.Reset();  // reset to empty

  // Fill all 7 usable slots (max_size = 7 for N=8)
  uint8_t fill_data[7] = {1, 2, 3, 4, 5, 6, 7};
  written = g_ring_buffer.Write(fill_data, 7);
  CLOGI("Filled %u elements", written);
  CLOGI("count=%u, full=%d", g_ring_buffer.count(), g_ring_buffer.full());

  // Try to write more without overwrite — should write 0
  uint8_t extra = 99;
  written = g_ring_buffer.Write(&extra, 1, /*overwrite=*/false);
  CLOGI("Write with overwrite=false when full: wrote %u (expected 0)", written);

  // ----------------------------------------------------------
  // 3. Overwrite mode — oldest data gets evicted
  // ----------------------------------------------------------
  CLOGI("\n--- 3. Overwrite Mode ---");

  g_ring_buffer.Reset();
  uint8_t init_data[] = {1, 2, 3};
  g_ring_buffer.Write(init_data, 3);
  CLOGI("Before overwrite: count=%u, front=%u", g_ring_buffer.count(), g_ring_buffer.front());

  // Write 2 more with overwrite — total 5, still fits, no eviction
  uint8_t more[] = {4, 5};
  g_ring_buffer.Write(more, 2, /*overwrite=*/true);
  CLOGI("After writing 4,5 (no eviction): count=%u", g_ring_buffer.count());

  // Now fill to full then overwrite — oldest gets dropped
  g_ring_buffer.Reset();
  uint8_t seq[] = {10, 20, 30, 40, 50, 60, 70};
  g_ring_buffer.Write(seq, 7);                         // fill all 7 slots
  g_ring_buffer.Write(&extra, 1, /*overwrite=*/true);  // evicts 10, keeps 20..99
  CLOGI("After overwrite when full:");
  CLOGI("  front=%u (10 was evicted)", g_ring_buffer.front());
  CLOGI("  [0]=%u, [1]=%u, [6]=%u", g_ring_buffer[0], g_ring_buffer[1], g_ring_buffer[6]);

  // ----------------------------------------------------------
  // 4. Peek — read without consuming
  // ----------------------------------------------------------
  CLOGI("\n--- 4. Peek (non-destructive read) ---");

  g_ring_buffer.Reset();
  uint8_t pd[] = {100, 101, 102};
  g_ring_buffer.Write(pd, 3);

  uint8_t peek_buf[3] = {0};
  size_t peeked = g_ring_buffer.Peek(peek_buf, 3);
  CLOGI("Peeked %u elements: {%u, %u, %u}", peeked, peek_buf[0], peek_buf[1], peek_buf[2]);
  CLOGI("count still = %u (data not consumed)", g_ring_buffer.count());

  // Actual read consumes data
  g_ring_buffer.Read(peek_buf, 1);
  CLOGI("After reading 1 element: count=%u, front=%u", g_ring_buffer.count(), g_ring_buffer.front());

  // ----------------------------------------------------------
  // 5. operator[] — zero-copy random access
  // ----------------------------------------------------------
  CLOGI("\n--- 5. operator[] Random Access ---");

  CLOGI("Element at index 0: %u", g_ring_buffer[0]);
  CLOGI("Element at index 1: %u", g_ring_buffer[1]);

  // ----------------------------------------------------------
  // 6. Consume — manually move read pointer (useful for DMA)
  // ----------------------------------------------------------
  CLOGI("\n--- 6. Consume (DMA-style) ---");

  CLOGI("Before Consume(2): count=%u, front=%u", g_ring_buffer.count(), g_ring_buffer.front());
  g_ring_buffer.Consume(2);  // consume 2 elements without copying
  CLOGI("After Consume(2):  count=%u, front=%u", g_ring_buffer.count(), g_ring_buffer.front());

  // ----------------------------------------------------------
  // 7. Read() — single element, convenience method
  // ----------------------------------------------------------
  CLOGI("\n--- 7. Read() Single Element ---");

  g_ring_buffer.Reset();
  g_ring_buffer.Write(seq, 3);  // {10, 20, 30}
  uint8_t val = g_ring_buffer.Read();
  CLOGI("Read() returned: %u, remaining count=%u", val, g_ring_buffer.count());

  // ----------------------------------------------------------
  // 8. External buffer usage via RingBufferCore
  // ----------------------------------------------------------
  CLOGI("\n--- 8. RingBufferCore (external buffer) ---");

  uint8_t ext_buf[16];  // 16 bytes, power of two
  cyf::RingBufferCore<uint8_t> rb(ext_buf, 16);

  rb.Write(init_data, 3);
  CLOGI("Core: wrote 3 elements, count=%u, max_size=%u", rb.count(), rb.max_size());

  CLOGI("\n=== Demo Complete ===");
}

void loop() {
  // Nothing to do here
}