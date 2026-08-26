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

#include "icon/interprocess/lockable_binary_futex.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <thread>

#include "icon/testing/realtime_annotations.h"
#include "icon/utils/attributes.h"
#include "icon/utils/status_and_expected_test_macros.h"
#include "icon/utils/status_matchers.h"

using intrinsic::testing::StatusIs;
using ::testing::Eq;

namespace intrinsic {

namespace {
TEST(LockableBinaryFutexTest, LockWorks) {
  // Test that `Lock()` can get a lock when it is unlocked and not when it is
  // locked.
  LockableBinaryFutex mutex;

  INTR_EXPECT_OK(mutex.Lock());
  EXPECT_TRUE(mutex.IsHeld());
  EXPECT_FALSE(mutex.TryLock());
  INTR_EXPECT_OK(mutex.Unlock());
  INTR_EXPECT_OK(mutex.Lock());
  INTR_EXPECT_OK(mutex.Unlock());
}

TEST(LockableBinaryFutexTest, TryLockWorks) {
  // Test that `TryLock()` can get a lock when it is unlocked and not when it is
  // locked.
  LockableBinaryFutex mutex;

  auto got_lock = mutex.TryLock();
  EXPECT_TRUE(got_lock);
  if (got_lock) {  // compile time checks force us to check this and unlock
                   // only in if-branch.
    EXPECT_FALSE(mutex.TryLock());
    INTR_EXPECT_OK(mutex.Unlock());
  }
  got_lock = mutex.TryLock();
  EXPECT_TRUE(got_lock);
  if (got_lock) {
    INTR_EXPECT_OK(mutex.Unlock());
  }
}

TEST(LockableBinaryFutexTest, AvoidsRaceCondition) {
  LockableBinaryFutex mutex;
  size_t counter = 0;
  constexpr int kIterations = 100;

  auto loop_increase = [&mutex, &counter]() {
    for (int i = 0; i < kIterations; ++i) {
      {
        BinaryFutexLock lock(mutex);
        const size_t c = counter;
        std::this_thread::sleep_for(std::chrono::milliseconds(
            10));  // Sleep to trigger race condition if the lock would not work
        counter = c + 1;
      }
      std::this_thread::yield();
    }
  };
  std::jthread thread1(loop_increase);
  std::jthread thread2(loop_increase);
  thread1.join();
  thread2.join();
  EXPECT_THAT(counter, Eq(kIterations * 2));
}

TEST(LockableBinaryFutexTest, TryLockAvoidsRaceCondition) {
  LockableBinaryFutex mutex;
  size_t counter = 0;
  std::atomic_int atomic_counter = 0;
  constexpr int kIterations = 100;

  auto loop_increase = [&mutex, &counter, &atomic_counter]() {
    int i = 0;
    while (i < kIterations) {
      if (mutex.TryLock()) {
        atomic_counter++;
        const size_t c = counter;

        std::this_thread::sleep_for(std::chrono::milliseconds(
            10));  // Sleep to trigger race condition if the lock would not work
        counter = c + 1;
        INTR_ASSERT_OK(mutex.Unlock());
        ++i;
      }
      std::this_thread::yield();
    }
  };
  std::jthread thread1(loop_increase);
  std::jthread thread2(loop_increase);
  thread1.join();
  thread2.join();
  EXPECT_EQ(atomic_counter.load(), 2 * kIterations);
  EXPECT_EQ(counter, atomic_counter.load());
}

TEST(LockableBinaryFutexTest, IntrGuardedByCompiles) {
  struct Counter {
    LockableBinaryFutex mutex;
    size_t value INTR_GUARDED_BY(mutex) = 0;
  };
  Counter counter;
  BinaryFutexLock lock(counter.mutex);
  const size_t c = counter.value;
  counter.value = c + 1;
}

TEST(LockableBinaryFutexTest, IntrGuardedByCompilesWithTryLock) {
  struct Counter {
    LockableBinaryFutex mutex;
    size_t value INTR_GUARDED_BY(mutex) = 0;
  };
  Counter counter;
  const auto locked = counter.mutex.TryLock();
  // We don't care about the results, we want to test that the compile-time
  // checks handle conditionals.
  if (locked) {
    const size_t c = counter.value;
    counter.value = c + 1;
    if (true) {
      // Check that 2 return paths are also covered.
      INTR_EXPECT_OK(counter.mutex.Unlock());
      return;
    }
    INTR_EXPECT_OK(counter.mutex.Unlock());
  }
}

TEST(LockableBinaryFutexTest, UnlockReturnsFailedPreconditionWhenNotLocked) {
  auto unlock_no_thread_safety_checks = []() INTR_NO_THREAD_SAFETY_ANALYSIS {
    LockableBinaryFutex mutex;
    EXPECT_THAT(mutex.Unlock(), StatusIs(StatusCode::kFailedPrecondition));
  };
  unlock_no_thread_safety_checks();
}

TEST(LockableBinaryFutexTest, AssertHeldDiesWhenNotHeld) {
  LockableBinaryFutex mutex;
  EXPECT_DEATH(mutex.AssertHeld(), "not held");
}

TEST(LockableBinaryFutexTest, DestructorDiesWhenLocked) {
  auto lock_no_thread_safety_checks = []() INTR_NO_THREAD_SAFETY_ANALYSIS {
    EXPECT_DEATH(
        {
          LockableBinaryFutex mutex;
          (void)mutex.Lock();
        },
        "not unlocked");
  };
  lock_no_thread_safety_checks();
}

}  // namespace

}  // namespace intrinsic
