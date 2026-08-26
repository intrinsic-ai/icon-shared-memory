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

#include "icon/hal/hardware_module_util.h"

#include <chrono>
#include <string>
#include <thread>

#include "gmock/gmock.h"
#include "flatbuffer_definitions/icon/hal/interfaces/hardware_module_state.fbs.h"
#include "gtest/gtest.h"
#include "icon/utils/status.h"
#include "icon/utils/status_and_expected_test_macros.h"
#include "icon/utils/status_matchers.h"

namespace intrinsic::icon {
namespace {

using intrinsic::testing::StatusIs;
using intrinsic_fbs::StateCode;
using ::testing::HasSubstr;
using ::testing::Not;

TEST(HardwareModuleUtilTest, TransitionGuardTransitions) {
  EXPECT_EQ(HardwareModuleTransitionGuard(StateCode::kPreparing,
                                          StateCode::kPrepared),
            TransitionGuardResult::kAllowed);
  EXPECT_EQ(HardwareModuleTransitionGuard(StateCode::kPreparing,
                                          StateCode::kActivated),
            TransitionGuardResult::kProhibited);
  EXPECT_EQ(HardwareModuleTransitionGuard(StateCode::kDeactivated,
                                          StateCode::kDeactivating),
            TransitionGuardResult::kNoOp);
  EXPECT_EQ(HardwareModuleTransitionGuard(StateCode::kMotionEnabled,
                                          StateCode::kFatallyFaulted),
            TransitionGuardResult::kAllowed);
}

TEST(HardwareModuleUtilTest, TransitionTosameStateIsNoOp) {
  for (const auto state : intrinsic_fbs::EnumValuesStateCode()) {
    EXPECT_EQ(HardwareModuleTransitionGuard(state, state),
              TransitionGuardResult::kNoOp)
        << "state=" << intrinsic_fbs::EnumNameStateCode(state);
  }
}

TEST(HardwareModuleUtilTest, TransitionGuardCanNeverLeaveFatallyFaulted) {
  for (const auto to : intrinsic_fbs::EnumValuesStateCode()) {
    if (to == StateCode::kFatallyFaulted) {
      // self transitions are always no-ops
      continue;
    }
    EXPECT_EQ(HardwareModuleTransitionGuard(StateCode::kFatallyFaulted, to),
              TransitionGuardResult::kProhibited)
        << "to=" << intrinsic_fbs::EnumNameStateCode(to);
  }
}

TEST(HardwareModuleUtilTest,
     TransitionGuardCanOnlyGoFromInitFailedToFatallyFaulted) {
  for (const auto to : intrinsic_fbs::EnumValuesStateCode()) {
    if (to == StateCode::kInitFailed) {
      // self transitions are always no-ops
      continue;
    }
    if (to == StateCode::kFatallyFaulted) {
      // We can always transition to `kFatallyFaulted`
      continue;
    }
    EXPECT_EQ(HardwareModuleTransitionGuard(StateCode::kInitFailed, to),
              TransitionGuardResult::kProhibited)
        << "to=" << intrinsic_fbs::EnumNameStateCode(to);
  }
}

TEST(HardwareModuleUtilTest, TransitionGuardAlwaysAllowsFatallyFaulted) {
  for (const auto from : intrinsic_fbs::EnumValuesStateCode()) {
    if (from == StateCode::kFatallyFaulted) {
      // self transitions are always no-ops
      continue;
    }
    if (from == StateCode::kInitFailed) {
      // `kInitFailed` is a terminal state, so we can't transition from it
      continue;
    }
    EXPECT_EQ(HardwareModuleTransitionGuard(from, StateCode::kFatallyFaulted),
              TransitionGuardResult::kAllowed)
        << "from=" << intrinsic_fbs::EnumNameStateCode(from);
  }
}

TEST(HardwareModuleUtilTest, SharedPromiseWrapperBasic) {
  SharedPromiseWrapper<int> wrapper;
  EXPECT_FALSE(wrapper.HasBeenSet());

  auto future = wrapper.GetSharedFuture();
  INTR_EXPECT_OK(wrapper.SetValue(42));
  EXPECT_TRUE(wrapper.HasBeenSet());

  EXPECT_EQ(future.get(), 42);

  // Setting again should return an error
  EXPECT_THAT(wrapper.SetValue(100),
              StatusIs(intrinsic::StatusCode::kFailedPrecondition));
}

TEST(HardwareModuleUtilTest, SharedPromiseWrapperMultiThreaded) {
  SharedPromiseWrapper<HardwareModuleExitCode> wrapper;
  auto f1 = wrapper.GetSharedFuture();
  auto f2 = wrapper.GetSharedFuture();

  std::thread t([&wrapper]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    INTR_EXPECT_OK(wrapper.SetValue(HardwareModuleExitCode::kRestartRequested));
  });

  EXPECT_EQ(f1.get(), HardwareModuleExitCode::kRestartRequested);
  EXPECT_EQ(f2.get(), HardwareModuleExitCode::kRestartRequested);

  t.join();
}

TEST(HardwareModuleUtilTest, CreateDotGraphvizStateMachineString) {
  const std::string dot = CreateDotGraphvizStateMachineString();
  EXPECT_THAT(dot, HasSubstr("digraph StateMachine"));
  EXPECT_THAT(dot, HasSubstr("kPreparing"));
  EXPECT_THAT(dot, HasSubstr("kPrepared"));
  EXPECT_THAT(dot, HasSubstr("->"));
}

}  // namespace
}  // namespace intrinsic::icon
