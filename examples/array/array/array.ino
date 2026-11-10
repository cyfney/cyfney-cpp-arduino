#include "cyf.h"  // Triggers Arduino IDE to locate cyfney-cpp and add src/ to include path, making "cyf/xxx.h" includes below resolvable
#include "cyf/array.h"

void setup() {
  Serial.begin(115200);

  // Elements live inside the object: this array is 4 * sizeof(int) bytes on the stack
  cyf::Array<int, 4> samples = {12, 7, 30, 5};

  // Iterators are plain pointers, so range-based for just works
  int sum = 0;
  for (int v : samples) {
    sum += v;
  }

  samples.Fill(0);      // element-wise reset, O(N)
  samples[2] = 42;      // operator[]: no bounds check
  samples.at(3) = 7;    // at(): assert(pos < N), no exception

  Serial.print("size=");
  Serial.println(samples.size());
  Serial.print("sum=");
  Serial.println(sum);
  Serial.print("samples[2]=");
  Serial.println(samples[2]);

  // Aggregate initialization: remaining elements are value-initialized to 0
  cyf::Array<int, 4> partial = {1, 2};
  Serial.print("partial=");
  for (size_t i = 0; i < partial.size(); ++i) {
    Serial.print(partial[i]);
    Serial.print(i + 1 < partial.size() ? " " : "\n");
  }
}

void loop() {}
