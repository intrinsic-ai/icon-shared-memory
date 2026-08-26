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

#include <cstdint>
#include <limits>

#include "gtest/gtest.h"

namespace intrinsic::icon {

class CurrentCycleTest : public ::testing::Test {
 public:
  CurrentCycleTest() {
    // Initialize the current cycle to zero before every test, since the cycle
    // count uses a static value that would otherwise persist across tests.
    CycleCounter::SetCurrentCycle(0);
  }
};

TEST_F(CurrentCycleTest, StartsAtZero) {
  EXPECT_EQ(CycleCounter::GetCurrentCycle(), 0);
}

TEST_F(CurrentCycleTest, CanIncrement) {
  while (CycleCounter::GetCurrentCycle() < 2000) {
    CycleCounter::IncrementCurrentCycle();
  }
  EXPECT_EQ(CycleCounter::GetCurrentCycle(), 2000);
}

TEST_F(CurrentCycleTest, RollsOver) {
  CycleCounter::SetCurrentCycle(std::numeric_limits<uint64_t>::max());
  CycleCounter::IncrementCurrentCycle();
  EXPECT_EQ(CycleCounter::GetCurrentCycle(), 0);
}

}  // namespace intrinsic::icon
