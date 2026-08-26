// Copyright 2026 Intrinsic Innovation LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ICON_UTILS_CURRENT_CYCLE_H_
#define ICON_UTILS_CURRENT_CYCLE_H_

#include <atomic>
#include <cstdint>

namespace intrinsic::icon {

// Singleton.
// Allows accessing the current cycle count without passing references through
// the ICON stack. The three static methods are thread safe, but it's a bad idea
// to call `SetCurrentCycle()` and `IncrementCurrentCycle()` from multiple
// threads. They will overwrite each other and produce unpredictable results
// based on scheduling.
//
// It is assumed that there is
// * one main control loop in charge calling `SetCurrentCycle()` /
//   `IncrementCurrentCycle()`
// * any number of readers using `GetCurrentCycle()`
//
// The value of current cycle is not guaranteed to be static or continuous.
// It can overflow, or jump (if `SetCurrentCycle()` is called).
//
// Expected Usage:
// * During Init: (optionally) Initial value is set using `SetCurrentCycle()`.
// * During realtime operation:
//   * In the main control loop call `IncrementCurrentCycle()`.
//   * Use `GetCurrentCycle()` to get the current cycle where required.
class CycleCounter final {
 public:
  CycleCounter() = delete;
  CycleCounter(CycleCounter& other) = delete;
  CycleCounter(const CycleCounter& other) = delete;
  CycleCounter& operator=(const CycleCounter&) = delete;
  CycleCounter(CycleCounter&& other) = delete;
  CycleCounter& operator=(const CycleCounter&&) = delete;

  // Returns the current cycle.
  static uint64_t GetCurrentCycle() noexcept;

  // Adjusts the value of current cycle.
  static void SetCurrentCycle(uint64_t cycle) noexcept;

  // Increments the value of current cycle while handling overruns.
  static void IncrementCurrentCycle() noexcept;

 private:
  inline static std::atomic_uint64_t current_cycle_ = 0;
};

}  // namespace intrinsic::icon

#endif  // ICON_UTILS_CURRENT_CYCLE_H_
