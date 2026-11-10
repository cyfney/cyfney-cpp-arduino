#include "cyf.h"
#include "cyf/hysteresis_quantizer.h"
#include "cyf/log.h"

cyf::HysteresisQuantizer<int> g_hysteresis_quantizer(0, 100, 10, 2);

void setup() {
  Serial.begin(115200);
  CLOGI("Hysteresis Quantizer Demo");
  randomSeed(analogRead(0));
}

void loop() {
  int input = random(0, 100);
  int pre_level = g_hysteresis_quantizer.level();
  int level = g_hysteresis_quantizer.Update(input);
  CLOGI("intput: %d, pre_level: %d, level: %d", input, pre_level, level);
  delay(500);
}