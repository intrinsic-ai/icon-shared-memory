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

#include "icon/interprocess/binary_futex.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <latch>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icon/utils/attributes.h"
#include "icon/utils/status.h"
#include "icon/utils/status_and_expected_test_macros.h"

using ::testing::Each;
using ::testing::Eq;
using ::testing::Field;
using ::testing::Optional;

namespace intrinsic::icon {
namespace {

TEST(BinaryFutexTest, DefaultConstructorInitializesValueCorrectly) {
  {
    BinaryFutex f;
    EXPECT_THAT(f.Value(), Eq(0));
  }
  {
    BinaryFutex f(true);
    EXPECT_THAT(f.Value(), Eq(1));
  }
}

TEST(BinaryFutexTest, PostIncreasesValueCorrectly) {
  BinaryFutex f;
  EXPECT_THAT(f.Value(), Eq(0));
  INTR_EXPECT_OK(f.Post());
  EXPECT_THAT(f.Value(), Eq(1));
  // A repeated call to Post does not change the value.
  INTR_EXPECT_OK(f.Post());
  EXPECT_THAT(f.Value(), Eq(1));
}

TEST(BinaryFutexTest, TryWaitDecreasesValueCorrectly) {
  BinaryFutex f(true);
  EXPECT_THAT(f.Value(), Eq(1));
  EXPECT_THAT(f.TryWait(), Optional(true));
  EXPECT_THAT(f.Value(), Eq(0));
}

TEST(BinaryFutexTest, WaitUntilDecreasesValueCorrectly) {
  BinaryFutex f(true);
  EXPECT_THAT(f.Value(), Eq(1));
  INTR_EXPECT_OK(f.WaitUntil(/*deadline=*/Time::max()));
  EXPECT_THAT(f.Value(), Eq(0));
}

TEST(BinaryFutexTest, WaitUntilTimesoutCorrectly) {
  BinaryFutex f;
  EXPECT_THAT(f.Value(), Eq(0));
  EXPECT_EQ(f.WaitUntil(Now() + std::chrono::milliseconds(1)).code,
            StatusCode::kDeadlineExceeded);
  EXPECT_THAT(f.Value(), Eq(0));
  // Any repeated wait call will still time out when no Post() happens.
  EXPECT_EQ(f.WaitUntil(Now() + std::chrono::milliseconds(1)).code,
            StatusCode::kDeadlineExceeded);
  EXPECT_THAT(f.Value(), Eq(0));
}

TEST(BinaryFutexTest, WaitForDecreasesValueCorrectly) {
  BinaryFutex f(true);
  EXPECT_THAT(f.Value(), Eq(1));
  INTR_EXPECT_OK(f.WaitFor(/*timeout=*/std::chrono::nanoseconds::max()));
  EXPECT_THAT(f.Value(), Eq(0));
}

TEST(BinaryFutexTest, TryWaitReturnsTrueIffValuePosted) {
  BinaryFutex f(false);
  EXPECT_THAT(f.TryWait(), Optional(false));
  INTR_EXPECT_OK(f.Post());
  EXPECT_THAT(f.TryWait(), Optional(true));
  EXPECT_THAT(f.TryWait(), Optional(false));
  INTR_EXPECT_OK(f.Post());
  EXPECT_THAT(f.TryWait(), Optional(true));
  EXPECT_THAT(f.TryWait(), Optional(false));
}

TEST(BinaryFutexTest, TryWaitReturnsTrueIfInitializedWithPostedValue) {
  BinaryFutex f(true);
  EXPECT_THAT(f.TryWait(), Optional(true));
  EXPECT_THAT(f.TryWait(), Optional(false));
}

TEST(BinaryFutexTest, WaitForTimesoutCorrectly) {
  BinaryFutex f;
  EXPECT_THAT(f.Value(), Eq(0));
  EXPECT_EQ(f.WaitFor(std::chrono::milliseconds(1)).code,
            StatusCode::kDeadlineExceeded);
  EXPECT_THAT(f.Value(), Eq(0));
  // Any repeated wait call will still time out when no Post() happens.
  EXPECT_EQ(f.WaitFor(std::chrono::milliseconds(1)).code,
            StatusCode::kDeadlineExceeded);
  EXPECT_THAT(f.Value(), Eq(0));
}

TEST(BinaryFutexTest, WaitWithNegativeTimeoutButPostedFutexSucceeds) {
  BinaryFutex f;
  EXPECT_THAT(f.Value(), Eq(0));
  INTR_EXPECT_OK(f.Post());
  INTR_EXPECT_OK(f.WaitFor(std::chrono::milliseconds(-1)));
}

TEST(BinaryFutexTest, WaitWithPastDeadlineButPostedFutexSucceeds) {
  BinaryFutex f;
  EXPECT_THAT(f.Value(), Eq(0));
  INTR_EXPECT_OK(f.Post());
  INTR_EXPECT_OK(f.WaitUntil(Time::min()));
}

TEST(BinaryFutexTest, OnlyOneWaitForSucceeds) {
  std::atomic<int> wait_for_ok = 0;
  std::atomic<int> wait_for_deadline_exceeded = 0;
  BinaryFutex futex;
  constexpr int num_threads = 10;
  std::latch barrier(num_threads);
  std::function worker = [&]() -> void {
    barrier.arrive_and_wait();
    auto status = futex.WaitFor(std::chrono::seconds(1));
    if (status.ok()) {
      wait_for_ok++;
    }
    if (status.code == StatusCode::kDeadlineExceeded) {
      wait_for_deadline_exceeded++;
    }
  };
  std::vector<std::jthread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker);
  }
  INTR_EXPECT_OK(futex.Post());
  for (auto& thread : threads) {
    thread.join();
  }
  EXPECT_EQ(wait_for_ok, 1);
  EXPECT_EQ(wait_for_deadline_exceeded, num_threads - 1);
}

TEST(BinaryFutexTest, ConcurrentPostWakesOnlyOne) {
  BinaryFutex futex;
  constexpr int num_threads = 10;
  std::latch barrier(num_threads);
  std::function worker = [&]() -> void {
    barrier.arrive_and_wait();
    INTR_EXPECT_OK(futex.Post());
  };
  std::vector<std::jthread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker);
  }
  for (auto& thread : threads) {
    thread.join();
  }
  INTR_EXPECT_OK(futex.WaitFor(std::chrono::seconds(1)));
  EXPECT_EQ(futex.WaitFor(std::chrono::seconds(1)).code,
            StatusCode::kDeadlineExceeded)
      << "Multiple concurrent Post should have same effect as one Post.";
}

TEST(BinaryFutexTest, FutexSignalsThreadCorrectly) {
  BinaryFutex f_request;
  BinaryFutex f_response;
  int counter = 0;
  std::atomic<bool> server_started = false;
  std::atomic<bool> shutdown_requested = false;

  std::jthread server_thread([&]() {
    server_started = true;
    while (!shutdown_requested) {
      INTR_EXPECT_OK(
          f_request.WaitFor(/*timeout=*/std::chrono::nanoseconds::max()));
      ++counter;
      INTR_EXPECT_OK(f_response.Post());
    }
  });

  // Wait for the thread to come up.
  while (!server_started) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  for (int i = 0; i < 10; ++i) {
    INTR_EXPECT_OK(f_request.Post());
    INTR_EXPECT_OK(
        f_response.WaitUntil(Now() + std::chrono::milliseconds(100)));
  }
  EXPECT_THAT(counter, Eq(10));

  shutdown_requested = true;
  // Wake up the thread once more to cleanly exit.
  INTR_EXPECT_OK(f_request.Post());
}

TEST(BinaryFutexTest, PrivateFutexSignalsThreadCorrectly) {
  BinaryFutex f_request(/*posted=*/false, /*private_futex=*/true);
  BinaryFutex f_response(/*posted=*/false, /*private_futex=*/true);
  int counter = 0;
  std::atomic<bool> server_started = false;
  std::atomic<bool> shutdown_requested = false;

  std::jthread server_thread([&]() {
    server_started = true;
    while (!shutdown_requested) {
      INTR_EXPECT_OK(
          f_request.WaitFor(/*timeout=*/std::chrono::nanoseconds::max()));
      ++counter;
      INTR_EXPECT_OK(f_response.Post());
    }
  });

  // Wait for the thread to come up.
  while (!server_started) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  for (int i = 0; i < 10; ++i) {
    INTR_EXPECT_OK(f_request.Post());
    INTR_EXPECT_OK(
        f_response.WaitUntil(Now() + std::chrono::milliseconds(100)));
  }
  EXPECT_THAT(counter, Eq(10));

  shutdown_requested = true;
  // Wake up the thread once more to cleanly exit.
  INTR_EXPECT_OK(f_request.Post());
}

using BinaryFutexPrivateNonPrivateTest = ::testing::TestWithParam<bool>;

TEST_P(BinaryFutexPrivateNonPrivateTest, CloseWakesThreadBlockedOnWaitUntil) {
  BinaryFutex f(/*posted=*/false, /*private_futex=*/GetParam());
  RealtimeStatus wait_result;
  std::jthread wait{[&]() {
    // This should return almost instantly (`f.Close()` may execute even
    // _before_ this thread starts running), but it's not safe to set test
    // expectations based on that, because CI tests run on highly-loaded
    // machines and can be preempted at any time for any amount of time.
    //
    // Instead, we just expect that this thread finishes at all. If it doesn't,
    // then the overall test binary will time out.
    wait_result = f.WaitUntil(Time::max());
  }};
  f.Close();
  wait.join();
  EXPECT_EQ(wait_result.code, StatusCode::kAborted);
}

TEST_P(BinaryFutexPrivateNonPrivateTest, CloseWakesThreadBlockedOnWaitFor) {
  BinaryFutex f(/*posted=*/false, /*private_futex=*/GetParam());
  RealtimeStatus wait_result;
  std::jthread wait{[&]() {
    // This should return almost instantly (`f.Close()` may execute even
    // _before_ this thread starts running), but it's not safe to set test
    // expectations based on that, because CI tests run on highly-loaded
    // machines and can be preempted at any time for any amount of time.
    //
    // Instead, we just expect that this thread finishes at all. If it doesn't,
    // then the overall test binary will time out.
    wait_result = f.WaitFor(std::chrono::nanoseconds::max());
  }};
  f.Close();
  wait.join();
  EXPECT_EQ(wait_result.code, StatusCode::kAborted);
}

TEST_P(BinaryFutexPrivateNonPrivateTest, CloseWakesAllBlockedThreads) {
  BinaryFutex f(/*posted=*/false, /*private_futex=*/GetParam());
  struct ResultList {
    std::mutex result_list_mutex;
    std::vector<RealtimeStatus> result_list INTR_GUARDED_BY(result_list_mutex);
  };
  constexpr size_t kNumThreads = 10;

  ResultList results;
  {
    std::lock_guard<std::mutex> l(results.result_list_mutex);
    results.result_list.reserve(kNumThreads);
  }
  std::vector<std::jthread> threads;
  threads.reserve(kNumThreads);
  for (size_t i = 0; i < kNumThreads; ++i) {
    threads.push_back(std::jthread{[&]() {
      // This should return almost instantly (`f.Close()` may execute even
      // _before_ this thread starts running), but it's not safe to set test
      // expectations based on that, because CI tests run on highly-loaded
      // machines and can be preempted at any time for any amount of time.
      //
      // Instead, we just expect that this thread finishes at all. If it
      // doesn't, then the overall test binary will time out.
      RealtimeStatus wait_result = f.WaitFor(std::chrono::nanoseconds::max());
      std::lock_guard<std::mutex> l(results.result_list_mutex);
      results.result_list.push_back(wait_result);
    }});
  }

  // This should wake _all_ waiting threads, and cause them to return
  // AbortedError.
  f.Close();
  std::for_each(threads.begin(), threads.end(),
                [](auto& thread) { thread.join(); });
  std::lock_guard<std::mutex> l(results.result_list_mutex);
  EXPECT_THAT(results.result_list, Each(Field("code", &RealtimeStatus::code,
                                              Eq(StatusCode::kAborted))));
}

TEST_P(BinaryFutexPrivateNonPrivateTest, ValueIsClosedConstantAfterClose) {
  BinaryFutex f(/*posted=*/false, /*private_futex=*/GetParam());
  EXPECT_EQ(f.Value(), BinaryFutex::kReady);
  f.Close();

  EXPECT_EQ(f.Value(), BinaryFutex::kClosed);
}

TEST_P(BinaryFutexPrivateNonPrivateTest, PostAfterCloseFails) {
  BinaryFutex f(/*posted=*/false, /*private_futex=*/GetParam());
  f.Close();

  EXPECT_EQ(f.Post().code, StatusCode::kAborted);
}

TEST_P(BinaryFutexPrivateNonPrivateTest, TryWaitFailsAfterClose) {
  BinaryFutex f(/*posted=*/false, /*private_futex=*/GetParam());
  f.Close();

  EXPECT_THAT(f.TryWait(), std::nullopt);
}

TEST_P(BinaryFutexPrivateNonPrivateTest,
       TryWaitFailsAfterCloseEvenIfPostedBefore) {
  BinaryFutex f(/*posted=*/true, /*private_futex=*/GetParam());
  f.Close();

  EXPECT_THAT(f.TryWait(), std::nullopt);
}

TEST_P(BinaryFutexPrivateNonPrivateTest, WaitUntilFailsAfterClose) {
  BinaryFutex f(/*posted=*/false, /*private_futex=*/GetParam());
  f.Close();

  EXPECT_EQ(f.WaitUntil(Time::max()).code, StatusCode::kAborted);
}

TEST_P(BinaryFutexPrivateNonPrivateTest, WaitForFailsAfterClose) {
  BinaryFutex f(/*posted=*/false, /*private_futex=*/GetParam());
  f.Close();

  EXPECT_EQ(f.WaitFor(std::chrono::nanoseconds::max()).code,
            StatusCode::kAborted);
}

INSTANTIATE_TEST_SUITE_P(Both, BinaryFutexPrivateNonPrivateTest,
                         ::testing::Values(true, false));

}  // namespace
}  // namespace intrinsic::icon
