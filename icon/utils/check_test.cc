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

#include "icon/utils/check.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

TEST(IntrTestMacro, TerminatesWithMessageIfConditionIsTrue) {
  EXPECT_DEATH({ INTR_CHECK(false, "Ack!"); }, "Ack!");
}
TEST(IntrTestMacro, WorksWithMultiPartStatement) {
  const int val = 5;
  EXPECT_DEATH({ INTR_CHECK(val == 1, "Ack!"); }, "Ack!");
}

TEST(IntrTestMacro, NoopIfConditionIsFalse) {
  INTR_CHECK(true, "Nothing to see here, move along folks");
}

}  // namespace
