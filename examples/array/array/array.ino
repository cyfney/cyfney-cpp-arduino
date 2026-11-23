#include "cyf.h"  // Triggers Arduino IDE to locate cyfney-cpp and add src/ to include path, making "cyf/xxx.h" includes below resolvable
#include "cyf/array.h"
#include "cyf/log.h"

void setup() {
  Serial.begin(115200);

  // Elements live inside the object: this array is 4 * sizeof(int) bytes on the stack
  cyf::Array<int, 4> samples = {12, 7, 30, 5};

  // Iterators are plain pointers, so range-based for just works
  int sum = 0;
  for (int v : samples) {
    sum += v;
  }

  samples.Fill(0);    // element-wise reset, O(N)
  samples[2] = 42;    // operator[]: no bounds check
  samples.at(3) = 7;  // at(): assert(pos < N), no exception

  CLOGI("size=%u", (unsigned)samples.size());
  CLOGI("sum=%d", sum);
  CLOGI("samples[2]=%d", samples[2]);

  // Aggregate initialization: remaining elements are value-initialized to 0
  cyf::Array<int, 4> partial = {1, 2};
  CLOGI("partial=%d %d %d %d", partial[0], partial[1], partial[2], partial[3]);
}

void loop() {}
