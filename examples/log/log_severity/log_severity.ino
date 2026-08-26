// =============================================================================
// Log Severity Control Example
// =============================================================================
// CYF_LOG_SEVERITY is a compile-time macro. Logs below the threshold are
// completely removed from the binary at compile time — zero CPU & Flash cost.
//
// Available levels (lower number = more verbose):
//   VERBOSE (1) -> CLOGV, CLOGD, CLOGI, CLOGW, CLOGE  (all enabled)
//   DEBUG   (2) -> CLOGD, CLOGI, CLOGW, CLOGE         (CLOGV stripped)
//   INFO    (3) -> CLOGI, CLOGW, CLOGE                (default)
//   WARN    (4) -> CLOGW, CLOGE
//   ERROR   (5) -> CLOGE
//   NONE    (6) -> (all stripped, zero log code)
//
// IMPORTANT: #define MUST appear BEFORE any #include of cyf.h or cyf/log.h
// =============================================================================

#include "cyf.h"  // must be included first (after macro definition)

// --- Choose one severity level below ---
#define CYF_LOG_SEVERITY CYF_LOG_SEVERITY_DEBUG
// #define CYF_LOG_SEVERITY CYF_LOG_SEVERITY_VERBOSE
// #define CYF_LOG_SEVERITY CYF_LOG_SEVERITY_INFO
// #define CYF_LOG_SEVERITY CYF_LOG_SEVERITY_WARN
// #define CYF_LOG_SEVERITY CYF_LOG_SEVERITY_NONE
#include "cyf/log.h"

void setup() {
  Serial.begin(115200);

  CLOGI("=== Log Severity Demo ===");
  CLOGI("Active level: DEBUG (change #define at top to test others)");

  // --- Each level demonstrated ---
  // With CYF_LOG_SEVERITY_DEBUG (2):
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