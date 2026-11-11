#pragma once

#ifndef CYF_TIMED_MAJORITY_FILTER_H_
#define CYF_TIMED_MAJORITY_FILTER_H_

#include <stdint.h>

#include "fixed_buffer.h"

namespace cyf {
// Accumulates distinct FrameType values (counted by equality) and, once at
// least `timeout` time units have elapsed since the first sample of the
// current window, yields the unique most frequent value via Flush().
//
// Notes:
// - The window is anchored when the buffer transitions from empty to
//   non-empty; a Flush() before the timeout does not restart the timer.
// - Once the timeout has elapsed, the next Flush() decides over all
//   accumulated samples, including recently pushed ones.
// - When more than MaxDistinct distinct values coexist, new distinct values
//   are silently dropped.
// - FrameType must be default-constructible and provide operator==.
// - SetTimeout()/SetMinCount() change only the values used by later Flush()
//   calls: buffered counts, the window anchor and pending samples are left
//   untouched. Validation runs before the single scalar store, so a rejected
//   update changes nothing and one Flush() never mixes old and new values.
//   Like Push/Flush/Reset, the setters require one execution context or
//   external synchronization; the filter itself is not thread-safe.
template <typename FrameType, typename TimeType, typename TimeGetter, uint8_t MaxDistinct = 8>
class TimedMajorityFilter {
 public:
  constexpr TimedMajorityFilter(TimeType timeout, TimeGetter getter, uint16_t min_count = 1)
      : timeout_(timeout), getter_(getter), min_count_(min_count) {}

  // Sets the window length used by subsequent Flush() calls. Rejects negative
  // values (only possible for a signed TimeType); 0 is valid and decides on
  // the next Flush(). Returns false without changing any state when rejected.
  bool SetTimeout(TimeType timeout) noexcept {
    if (IsNegative(timeout)) {
      return false;
    }
    timeout_ = timeout;
    return true;
  }

  // Sets the minimum sample count needed for a decision. Rejects 0, which
  // would remove the minimum-support requirement. Returns false without
  // changing any state when rejected.
  bool SetMinCount(uint16_t min_count) noexcept {
    if (min_count == 0) {
      return false;
    }
    min_count_ = min_count;
    return true;
  }

  void Push(const FrameType& frame) {
    for (size_t i = 0; i < entries_.size(); ++i) {
      if (entries_[i].frame == frame) {
        entries_[i].count++;
        return;
      }
    }

    if (entries_.PushBack({frame, 1}) && entries_.size() == 1) {
      last_time_ = getter_();
    }
  }

  // Writes the winning value to out; returns false when no decision was reached.
  bool Flush(FrameType& out) { return Flush(getter_(), out); }

  bool Flush(TimeType now, FrameType& out) {
    // Evaluate one snapshot: an update takes effect on the next Flush(),
    // never halfway through this one.
    const TimeType timeout = timeout_;
    const uint16_t min_count = min_count_;

    if (entries_.empty()) {
      return false;
    }

    if (Elapsed(now, last_time_) < timeout) {
      return false;
    }

    uint16_t max_count = 0;
    uint16_t second_max = 0;
    bool has_best = false;
    FrameType best{};

    for (size_t i = 0; i < entries_.size(); ++i) {
      const uint16_t c = entries_[i].count;
      if (c > max_count) {
        second_max = max_count;
        max_count = c;
        best = entries_[i].frame;
        has_best = true;
      } else if (c > second_max) {
        second_max = c;
      }
    }

    entries_.Clear();

    if (has_best && (max_count >= min_count) && (max_count > second_max)) {
      out = best;
      return true;
    }

    return false;
  }

  void Reset() { entries_.Clear(); }

 private:
  TimedMajorityFilter(const TimedMajorityFilter&) = delete;
  TimedMajorityFilter& operator=(const TimedMajorityFilter&) = delete;

  static TimeType Elapsed(TimeType now, TimeType last) { return now - last; }

  // Rejects a negative window for signed TimeType. For unsigned TimeType the
  // comparison is always false, so every value (including 0) is valid. Uses
  // no <type_traits>, keeping the header AVR/C++11 compatible.
  static constexpr bool IsNegative(TimeType value) noexcept { return value < TimeType{}; }

  struct Entry {
    FrameType frame;
    uint16_t count;
  };

  TimeType timeout_;
  const TimeGetter getter_;
  uint16_t min_count_;

  TimeType last_time_ = TimeType{0};
  FixedBuffer<Entry, MaxDistinct> entries_;
};
}  // namespace cyf

#endif  // CYF_TIMED_MAJORITY_FILTER_H_
