// Host-side regression tests for TimedMajorityFilter runtime configuration.
//
// Build from the repository root (no Arduino core required):
//   clang++ -std=c++11 -Wall -Wextra -Werror -I src tests/timed_majority_filter_test.cpp -o /tmp/tmf_test
//   /tmp/tmf_test
//
// Covers: backward compatibility, dynamic set_timeout/set_min_count, boundary
// values, state preservation, and interleaved updates. Valid parameter values
// and any required synchronization are the caller's responsibility.

#include "cyf/timed_majority_filter.h"

#include <stdint.h>
#include <stdio.h>

#include "cyf.h"

namespace {
int g_checks = 0;
int g_failures = 0;

uint32_t g_now = 0;
uint32_t Now() { return g_now; }

int32_t g_signed_now = 0;
int32_t SignedNow() { return g_signed_now; }

using Filter = cyf::TimedMajorityFilter<uint8_t, uint32_t, uint32_t (*)()>;
using SignedFilter = cyf::TimedMajorityFilter<uint8_t, int32_t, int32_t (*)()>;

void PushN(Filter& filter, uint8_t value, int count) {
  for (int i = 0; i < count; ++i) {
    filter.Push(value);
  }
}
}  // namespace

#define CHECK(expr)                                          \
  do {                                                       \
    ++g_checks;                                              \
    if (!(expr)) {                                           \
      ++g_failures;                                          \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
    }                                                        \
  } while (false)

int main() {
  uint8_t out = 0;

  // 1. Existing constructor/API and default min_count (= 1) still behave as before.
  {
    Filter filter(2000, Now);
    g_now = 0;
    PushN(filter, 7, 3);
    out = 0;
    CHECK(!filter.Flush(1000, out));  // window not elapsed
    CHECK(out == 0);
    CHECK(filter.Flush(2000, out));
    CHECK(out == 7);
    CHECK(!filter.Flush(3000, out));  // one elapsed Flush consumes the window
    CHECK(out == 7);                  // failed Flush leaves out untouched
  }

  // 2. set_min_count() raises the threshold for later Flush() calls.
  {
    Filter filter(2000, Now, 3);
    g_now = 0;
    PushN(filter, 7, 5);
    PushN(filter, 3, 2);
    filter.set_min_count(6);
    out = 0;
    CHECK(!filter.Flush(2000, out));  // 5 votes < 6
    CHECK(out == 0);
  }

  // 3. Updating does not clear accumulated counts or pending samples.
  {
    Filter filter(2000, Now, 9);
    g_now = 0;
    PushN(filter, 7, 5);
    filter.set_min_count(5);
    CHECK(filter.Flush(2000, out));  // the 5 votes survived the update
    CHECK(out == 7);
  }

  // 4. set_timeout() shortens the window without resetting its anchor.
  {
    Filter filter(2000, Now);
    g_now = 0;
    PushN(filter, 7, 3);              // anchor = 0
    CHECK(!filter.Flush(1000, out));  // 1000 < 2000; samples retained
    filter.set_timeout(500);
    CHECK(filter.Flush(1000, out));  // 1000 - 0 >= 500, anchor still 0
    CHECK(out == 7);
  }

  // 5. set_timeout() can also extend the window.
  {
    Filter filter(2000, Now);
    g_now = 0;
    PushN(filter, 7, 3);
    filter.set_timeout(5000);
    CHECK(!filter.Flush(2000, out));
    CHECK(filter.Flush(5000, out));
    CHECK(out == 7);
  }

  // 6. timeout == 0 is valid and decides immediately.
  {
    Filter filter(2000, Now);
    g_now = 1234;
    PushN(filter, 9, 2);
    filter.set_timeout(0);
    CHECK(filter.Flush(1234, out));
    CHECK(out == 9);
  }

  // 7. The single-argument Flush() overload observes updated values too.
  {
    Filter filter(2000, Now, 2);
    filter.set_timeout(0);
    g_now = 777;
    PushN(filter, 8, 3);
    CHECK(filter.Flush(out));
    CHECK(out == 8);
  }

  // 8. Flush() still decides with the configured minimum count.
  {
    Filter filter(2000, Now, 3);
    g_now = 0;
    PushN(filter, 7, 5);
    CHECK(filter.Flush(2000, out));
    CHECK(out == 7);
  }

  // 9. min_count boundary: UINT16_MAX suppresses decisions.
  {
    Filter filter(2000, Now);
    filter.set_min_count(UINT16_MAX);
    g_now = 0;
    PushN(filter, 7, 10);
    CHECK(!filter.Flush(2000, out));
    filter.set_min_count(1);
    filter.set_timeout(0);
    g_now = 2000;
    PushN(filter, 7, 2);
    CHECK(filter.Flush(2000, out));
    CHECK(out == 7);
  }

  // 10. timeout boundary: UINT32_MAX, 0 and intermediate values.
  {
    Filter filter(10, Now);
    filter.set_timeout(UINT32_MAX);
    g_now = 0;
    PushN(filter, 7, 3);
    CHECK(!filter.Flush(1000000, out));
    filter.set_timeout(1000000);
    CHECK(filter.Flush(1000000, out));
    CHECK(out == 7);
  }

  // 11. Signed TimeType works with runtime configuration.
  {
    SignedFilter filter(2000, SignedNow, 3);
    g_signed_now = 0;
    for (int i = 0; i < 5; ++i) {
      filter.Push(7);
    }
    CHECK(filter.Flush(2000, out));
    CHECK(out == 7);
    filter.set_timeout(0);
  }

  // 12. Reset() clears samples but keeps the runtime configuration.
  {
    Filter filter(2000, Now, 3);
    filter.set_min_count(5);
    filter.set_timeout(0);
    g_now = 0;
    PushN(filter, 7, 3);
    filter.Reset();
    PushN(filter, 7, 5);
    CHECK(filter.Flush(0, out));  // 5 votes still >= min_count 5
    CHECK(out == 7);
  }

  // 13. Interleaved updates and operations keep the state machine consistent.
  {
    Filter filter(1000, Now, 2);
    for (int i = 0; i < 200; ++i) {
      g_now = static_cast<uint32_t>(i * 10);
      filter.Push(static_cast<uint8_t>(i % 3));
      if (i % 7 == 0) {
        filter.set_min_count(static_cast<uint16_t>(1 + (i % 5)));
      }
      if (i % 11 == 0) {
        filter.set_timeout(static_cast<uint32_t>(i % 50));
      }
      uint8_t value = 99;  // sentinel, outside the pushed range {0, 1, 2}
      const bool decided = filter.Flush(g_now, value);
      if (decided) {
        CHECK(value == 0 || value == 1 || value == 2);
      } else {
        CHECK(value == 99);  // failed Flush must not write out
      }
    }
  }

  printf("checks=%d failures=%d\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
