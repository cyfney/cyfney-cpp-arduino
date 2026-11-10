// =============================================================================
// Log Severity Control Example
// =============================================================================
// CYF_LOG_LEVEL is a compile-time macro. Log macros above the configured
// verbosity are completely removed from the binary at compile time —
// zero CPU & Flash cost.
//
// Available levels (higher number = more verbose, NONE disables all):
//   NONE    (0) -> (all stripped, zero log code)
//   ERROR   (1) -> CLOGE
//   WARN    (2) -> CLOGW, CLOGE
//   INFO    (3) -> CLOGI, CLOGW, CLOGE                (default)
//   DEBUG   (4) -> CLOGD, CLOGI, CLOGW, CLOGE         (CLOGV stripped)
//   VERBOSE (5) -> CLOGV, CLOGD, CLOGI, CLOGW, CLOGE  (all enabled)
//
// IMPORTANT: #define MUST appear BEFORE any #include of cyf.h or cyf/log.h
// =============================================================================

// --- Choose one log level below ---
#define CYF_LOG_LEVEL CYF_LOG_LEVEL_DEBUG
// #define CYF_LOG_LEVEL CYF_LOG_LEVEL_VERBOSE
// #define CYF_LOG_LEVEL CYF_LOG_LEVEL_INFO
// #define CYF_LOG_LEVEL CYF_LOG_LEVEL_WARN
// #define CYF_LOG_LEVEL CYF_LOG_LEVEL_ERROR
// #define CYF_LOG_LEVEL CYF_LOG_LEVEL_NONE

#include "cyf.h"  // Triggers Arduino IDE to locate cyfney-cpp and add src/ to include path, making "cyf/xxx.h" includes below resolvable
#include "cyf/log.h"

void setup() {
  Serial.begin(115200);

  CLOGI("=== Log Severity Demo ===");
  CLOGI("Active level: DEBUG (change #define at top to test others)");

  // --- Each level demonstrated ---
  // With CYF_LOG_LEVEL_DEBUG (4):
  CLOGV("CLOGV: compiled out (requires VERBOSE)");  // ✂️ stripped
  CLOGD("CLOGD: visible");                          // ✅ enabled
  CLOGI("CLOGI: visible");                          // ✅ enabled
  CLOGW("CLOGW: visible");                          // ✅ enabled
  CLOGE("CLOGE: visible");                          // ✅ enabled

  CLOGI("---");
  CLOGI("Tip: check the compiled binary size with different severity levels.");
  CLOGI("Going to loop()...");
}

void loop() {}