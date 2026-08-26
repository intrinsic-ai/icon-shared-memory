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

#include "util/thread/lockstep.h"

#include <atomic>
#include <chrono>
#include <latch>
#include <thread>

#include "gtest/gtest.h"
#include "icon/utils/status.h"
#include "icon/utils/status_and_expected_test_macros.h"
#include "icon/utils/time.h"

namespace intrinsic {

// Timeout for StartOperationX calls.
static constexpr auto kLockTimeout = std::chrono::milliseconds(100);
static constexpr auto kLongLockTimeout = std::chrono::milliseconds(500);

TEST(LockstepTest, StartOperationAWithTimeout) {
  Lockstep lockstep;
  INTR_EXPECT_OK(lockstep.StartOperationAWithTimeout(kLockTimeout));
}

TEST(LockstepTest, StartOperationAWithDeadline) {
  Lockstep lockstep;
  INTR_EXPECT_OK(lockstep.StartOperationAWithDeadline(Now() + kLockTimeout));
}

TEST(LockstepTest, StartOperationACancelled) {
  Lockstep lockstep;
  lockstep.Cancel(nullptr);
  constexpr int kNumIterations = 5;
  for (int i = 0; i < kNumIterations; i++) {
    EXPECT_EQ(lockstep.StartOperationAWithTimeout(kLockTimeout).code,
              StatusCode::kAborted);
  }
}

TEST(LockstepTest, StartOperationBCancelled) {
  Lockstep lockstep;
  lockstep.Cancel(nullptr);
  constexpr int kNumIterations = 5;
  for (int i = 0; i < kNumIterations; i++) {
    EXPECT_EQ(lockstep.StartOperationBWithTimeout(kLockTimeout).code,
              StatusCode::kAborted);
  }
}

TEST(LockstepTest, MismatchedEndOperationA) {
  Lockstep lockstep;

  EXPECT_EQ(lockstep.EndOperationA().code, StatusCode::kFailedPrecondition);

  INTR_EXPECT_OK(lockstep.StartOperationAWithTimeout(kLockTimeout));
  INTR_EXPECT_OK(lockstep.EndOperationA());

  EXPECT_EQ(lockstep.EndOperationA().code, StatusCode::kFailedPrecondition);
}

TEST(LockstepTest, MismatchedEndOperationB) {
  Lockstep lockstep;
  EXPECT_EQ(lockstep.EndOperationB().code, StatusCode::kFailedPrecondition);

  INTR_EXPECT_OK(lockstep.StartOperationAWithTimeout(kLockTimeout));
  INTR_EXPECT_OK(lockstep.EndOperationA());
  INTR_EXPECT_OK(lockstep.StartOperationBWithDeadline(Now() + kLockTimeout));
  INTR_EXPECT_OK(lockstep.EndOperationB());

  EXPECT_EQ(lockstep.EndOperationB().code, StatusCode::kFailedPrecondition);
}

TEST(LockstepTest, ABABABSingleThread) {
  Lockstep lockstep;
  constexpr int kNumIterations = 25000;
  for (int i = 0; i < kNumIterations; i++) {
    INTR_EXPECT_OK(lockstep.StartOperationAWithDeadline(Now() + kLockTimeout));
    INTR_EXPECT_OK(lockstep.EndOperationA());
    INTR_EXPECT_OK(lockstep.StartOperationBWithTimeout(kLockTimeout));
    INTR_EXPECT_OK(lockstep.EndOperationB());
  }
}

TEST(LockstepTest, ABABABMultiThread) {
  Lockstep lockstep;

  static constexpr int kNumIterations = 25000;
  std::atomic<int> a_count = 0;
  std::atomic<int> b_count = 0;

  // Kick off thread for Operation A.
  std::jthread operation_a_thread([&lockstep, &a_count, &b_count]() {
    for (int i = 0; i < kNumIterations; i++) {
      INTR_EXPECT_OK(lockstep.StartOperationAWithTimeout(kLockTimeout));
      ASSERT_EQ(a_count, b_count);
      a_count++;
      INTR_EXPECT_OK(lockstep.EndOperationA());
    }
  });

  // Kick off thread for Operation B.
  std::jthread operation_b_thread([&lockstep, &a_count, &b_count]() {
    for (int i = 0; i < kNumIterations; i++) {
      INTR_EXPECT_OK(
          lockstep.StartOperationBWithDeadline(Now() + kLockTimeout));
      ASSERT_EQ(a_count, b_count + 1);
      b_count++;
      INTR_EXPECT_OK(lockstep.EndOperationB());
    }
  });

  operation_a_thread.join();
  operation_b_thread.join();

  EXPECT_EQ(a_count, kNumIterations);
  EXPECT_EQ(b_count, kNumIterations);
}

TEST(LockstepTest, StartOperationABlockThenCancel) {
  Lockstep lockstep;
  // Bring the lockstep to the state, where a call to `StartOperationA` will
  // have to wait.
  INTR_EXPECT_OK(lockstep.StartOperationAWithDeadline(Now() + kLockTimeout));
  INTR_EXPECT_OK(lockstep.EndOperationA());
  INTR_EXPECT_OK(lockstep.StartOperationBWithTimeout(kLockTimeout));

  // Kick off thread that sleeps then Cancel()s the lockstep.
  std::jthread cancel_thread([&lockstep]() {
    std::this_thread::sleep_for(kLockTimeout);
    lockstep.Cancel(nullptr);
  });

  // Blocking until `Cancel` is called.
  EXPECT_EQ(
      lockstep.StartOperationAWithTimeout(/*timeout=*/kLongLockTimeout).code,
      StatusCode::kAborted);

  cancel_thread.join();
}

TEST(LockstepTest, StartOperationBBlockThenCancel) {
  Lockstep lockstep;

  // Kick off thread that sleeps then Cancel()s the lockstep.
  std::jthread cancel_thread([&lockstep]() {
    std::this_thread::sleep_for(kLockTimeout);
    lockstep.Cancel(nullptr);
  });

  // Blocking until `Cancel` is called.
  EXPECT_EQ(
      lockstep.StartOperationBWithTimeout(/*timeout=*/kLongLockTimeout).code,
      StatusCode::kAborted);

  cancel_thread.join();
}

TEST(LockstepTest, EndOperationOkWhenCancelledDuringOperationA) {
  Lockstep lockstep;
  INTR_EXPECT_OK(lockstep.StartOperationAWithTimeout(kLockTimeout));
  lockstep.Cancel(nullptr);
  INTR_EXPECT_OK(lockstep.EndOperationA());
  INTR_EXPECT_OK(lockstep.EndOperationB());
}

TEST(LockstepTest, EndOperationOkWhenCancelledDuringOperationB) {
  Lockstep lockstep;
  INTR_EXPECT_OK(lockstep.StartOperationAWithTimeout(kLockTimeout));
  INTR_EXPECT_OK(lockstep.EndOperationA());
  INTR_EXPECT_OK(lockstep.StartOperationBWithDeadline(Now() + kLockTimeout));
  lockstep.Cancel(nullptr);
  INTR_EXPECT_OK(lockstep.EndOperationB());
  INTR_EXPECT_OK(lockstep.EndOperationA());
}

TEST(LockstepTest, StartOperationASucceedsAfterReset) {
  Lockstep lockstep;
  lockstep.Cancel(nullptr);
  // We cannot start A nor B.
  EXPECT_EQ(lockstep.StartOperationAWithDeadline(Now() + kLockTimeout).code,
            StatusCode::kAborted);
  EXPECT_EQ(lockstep.StartOperationBWithTimeout(kLockTimeout).code,
            StatusCode::kAborted);

  INTR_EXPECT_OK(lockstep.Reset());
  // After reset, we can start A.
  INTR_EXPECT_OK(lockstep.StartOperationAWithTimeout(kLockTimeout));
}

TEST(LockstepTest, StartOperationBFailsAfterReset) {
  Lockstep lockstep;
  lockstep.Cancel(nullptr);
  // We cannot start A nor B.
  EXPECT_EQ(lockstep.StartOperationAWithTimeout(kLockTimeout).code,
            StatusCode::kAborted);
  EXPECT_EQ(lockstep.StartOperationBWithDeadline(Now() + kLockTimeout).code,
            StatusCode::kAborted);

  INTR_EXPECT_OK(lockstep.Reset());
  // After reset, we can still not start B.
  EXPECT_EQ(lockstep.StartOperationBWithTimeout(kLockTimeout).code,
            StatusCode::kDeadlineExceeded);
}

TEST(LockstepTest, StartOperationASucceedsAfterResetMultithread) {
  Lockstep lockstep;
  static constexpr int kCyclesUntilCancel = 100;
  std::latch operation_cancelled(2);
  std::latch lockstep_reset(2);

  // Kick off thread for operation a, that will eventually cancel and later
  // reset the lockstep.
  std::jthread operation_a_thread(
      [&lockstep, &operation_cancelled, &lockstep_reset]() {
        for (int i = 0; i < kCyclesUntilCancel; i++) {
          INTR_EXPECT_OK(
              lockstep.StartOperationAWithDeadline(Now() + kLockTimeout));
          INTR_EXPECT_OK(lockstep.EndOperationA());
        }
        // Cancel the lockstep. This will make operation b fail.
        lockstep.Cancel(nullptr);
        // Wait for `operation_b_thread` to signal, that it noticed the
        // cancellation.
        operation_cancelled.arrive_and_wait();
        // Reset the lockstep.
        INTR_EXPECT_OK(lockstep.Reset());
        lockstep_reset.count_down();
        // Run one last cycle after `Reset`.
        INTR_EXPECT_OK(lockstep.StartOperationAWithTimeout(kLockTimeout));
        INTR_EXPECT_OK(lockstep.EndOperationA());
      });

  // Kick off thread for operation b.
  std::jthread operation_b_thread([&lockstep, &operation_cancelled,
                                   &lockstep_reset]() {
    auto status = RtOkStatus();
    // Run until the `operation_a_thread` cancels the lockstep.
    while (status = lockstep.StartOperationBWithDeadline(Now() + kLockTimeout),
           status.ok()) {
      INTR_EXPECT_OK(lockstep.EndOperationB());
    }
    EXPECT_EQ(status.code, StatusCode::kAborted);

    // Tell `operation_a_thread` that we've cancelled.
    operation_cancelled.count_down();
    // Wait for `operation_a_thread` to `Reset` the lockstep.
    lockstep_reset.arrive_and_wait();
    // Run one last cycle after `Reset`.
    INTR_EXPECT_OK(lockstep.StartOperationBWithTimeout(kLockTimeout));
    INTR_EXPECT_OK(lockstep.EndOperationB());
  });

  operation_a_thread.join();
  operation_b_thread.join();
}

TEST(LockstepTest, ResetFailsWhenNotCancelled) {
  Lockstep lockstep;
  EXPECT_EQ(lockstep.Reset(kLockTimeout).code, StatusCode::kFailedPrecondition);
}

TEST(LockstepTest, StartOperationBTimesOutWhenOperationAIsRunning) {
  Lockstep lockstep;
  INTR_EXPECT_OK(lockstep.StartOperationAWithTimeout(kLockTimeout));
  EXPECT_EQ(lockstep.StartOperationBWithDeadline(Now() + kLockTimeout).code,
            StatusCode::kDeadlineExceeded);
  INTR_EXPECT_OK(lockstep.EndOperationA());
  INTR_EXPECT_OK(lockstep.StartOperationBWithTimeout(kLockTimeout));
}

TEST(LockstepTest, StartOperationBTimesOutWithoutOperationA) {
  Lockstep lockstep;
  EXPECT_EQ(lockstep.StartOperationBWithTimeout(kLockTimeout).code,
            StatusCode::kDeadlineExceeded);
}

TEST(LockstepTest, StartOperationATimesOutWhenOperationBIsRunning) {
  Lockstep lockstep;
  INTR_EXPECT_OK(lockstep.StartOperationAWithTimeout(kLockTimeout));
  INTR_EXPECT_OK(lockstep.EndOperationA());
  INTR_EXPECT_OK(lockstep.StartOperationBWithTimeout(kLockTimeout));
  EXPECT_EQ(lockstep.StartOperationAWithDeadline(Now() + kLockTimeout).code,
            StatusCode::kDeadlineExceeded);
}

TEST(LockstepTest, StartOperationATimesOutWithoutOperationB) {
  Lockstep lockstep;
  INTR_EXPECT_OK(lockstep.StartOperationAWithTimeout(kLockTimeout));
  INTR_EXPECT_OK(lockstep.EndOperationA());
  EXPECT_EQ(lockstep.StartOperationAWithDeadline(Now() + kLockTimeout).code,
            StatusCode::kDeadlineExceeded);
}

}  // namespace intrinsic
