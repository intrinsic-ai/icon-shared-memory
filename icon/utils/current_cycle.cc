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

#include "icon/utils/current_cycle.h"

#include <atomic>
#include <cstdint>

namespace intrinsic::icon {

uint64_t CycleCounter::GetCurrentCycle() noexcept {
  return current_cycle_.load(std::memory_order_acquire);
}

void CycleCounter::SetCurrentCycle(uint64_t cycle) noexcept {
  current_cycle_.store(cycle, std::memory_order_release);
}

void CycleCounter::IncrementCurrentCycle() noexcept {
  // Since current_cycle_ is unsigned, it overflows to zero automatically,
  // like one would expect (cf.
  // https://en.cppreference.com/cpp/language/operator_arithmetic#Overflows)
  current_cycle_.fetch_add(1, std::memory_order_acq_rel);
}

}  // namespace intrinsic::icon
