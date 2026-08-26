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

#include "platform/common/buffers/rt_promise.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <latch>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "icon/testing/malloc_test.h"
#include "icon/utils/log.h"
#include "icon/utils/mock_log_sink.h"
#include "icon/utils/status.h"
#include "icon/utils/status_and_expected_test_macros.h"
#include "icon/utils/status_matchers.h"
#include "icon/utils/time.h"

namespace intrinsic {
namespace {

using intrinsic::testing::StatusIs;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Eq;
using ::testing::HasSubstr;

constexpr std::chrono::milliseconds kWaitTimeout =
    std::chrono::milliseconds(200);

// We use a custom log prefix so we can check that our sink is used, even in
// death tests where we cannot capture the logs.
constexpr char kLogPrefix[] = "rt_promise_test: ";

template <typename T>
class FuturePromiseContextTest : public ::testing::Test {
 public:
  static T GetTestDefaultValue();

  // Helper function to allow comparisons of the Future's value, which is
  // needed for std::unique_ptr.
  template <typename V>
  V GetValueForComparison(const V& value) {
    return value;
  }
  template <typename V>
  auto GetValueForComparison(const std::unique_ptr<V>& value) {
    return *value;
  }

  // Helper function to allow copies of the Future's value, which is
  // needed for std::unique_ptr.
  template <typename V>
    requires(!std::is_same_v<V, std::unique_ptr<int>>)
  static std::function<V(const V&)> GetTestCopyFunction() {
    return [](const V& value) { return value; };
  }
  template <typename V>
    requires std::is_same_v<V, std::unique_ptr<int>>
  static std::function<V(const V&)> GetTestCopyFunction() {
    return [](const std::unique_ptr<int>& value) {
      return std::make_unique<int>(*value);
    };
  }

  log::MockLogSink mock_log_sink_{kLogPrefix};
  log::Logger logger_{log::Logger::Severity::kDebug, mock_log_sink_};
};

class NonDefaultConstructible {
 public:
  explicit NonDefaultConstructible(int value) : value_(value) {}
  int value() const { return value_; }

 private:
  int value_;
};

// Specialization for uint8_t.
template <>
uint8_t FuturePromiseContextTest<uint8_t>::GetTestDefaultValue() {
  return 42;
}
// Specialization for int.
template <>
int FuturePromiseContextTest<int>::GetTestDefaultValue() {
  return 42;
}
// Specialization for float.
template <>
float FuturePromiseContextTest<float>::GetTestDefaultValue() {
  return 42.0f;
}
// Specialization for double.
template <>
double FuturePromiseContextTest<double>::GetTestDefaultValue() {
  return 42.0;
}
// Specialization for std::string.
template <>
std::string FuturePromiseContextTest<std::string>::GetTestDefaultValue() {
  return "foo";
}
// Specialization for std::vector<int>.
template <>
std::vector<int>
FuturePromiseContextTest<std::vector<int>>::GetTestDefaultValue() {
  return {42};
}
// Specialization for std::unique_ptr<int>.
template <>
std::unique_ptr<int>
FuturePromiseContextTest<std::unique_ptr<int>>::GetTestDefaultValue() {
  return std::make_unique<int>(42);
}

// Specialization for NonDefaultConstructible.
template <>
NonDefaultConstructible
FuturePromiseContextTest<NonDefaultConstructible>::GetTestDefaultValue() {
  return NonDefaultConstructible(42);
}

// Using a combination of simple types, a string, a vector and a unique_ptr
// (non-copyable).
// Not supporting NonDefaultConstructible yet, since this is not supported by
// RealtimeQueue and the underlying RtQueueBuffer. A potential solution would
// require to change the C-style array in RtQueueBuffer to an std::array and
// then use std::generate_n with a slightly different type of `init_function` to
// initialize the elements.
using FuturePromiseContextTypes =
    ::testing::Types<uint8_t, int, float, double, std::string, std::vector<int>,
                     std::unique_ptr<int> /*, NonDefaultConstructible*/>;

TYPED_TEST_SUITE(FuturePromiseContextTest, FuturePromiseContextTypes);

TYPED_TEST(FuturePromiseContextTest, CreateWorks) {
  FuturePromiseContext<TypeParam> context(&this->logger_);
}

TYPED_TEST(FuturePromiseContextTest, DestroyWorks) {
  {
    FuturePromiseContext<TypeParam> context(&this->logger_);
    // Destroying without having a future or a promise is fine.
  }
  {
    FuturePromiseContext<TypeParam> context(&this->logger_);
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    // Destroying after a future is fine.
  }
  {
    FuturePromiseContext<TypeParam> context(&this->logger_);
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    // Destroying after a promise is fine.
  }
  {
    FuturePromiseContext<TypeParam> context(&this->logger_);
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    // Destroying after a future and a promise is fine.
  }
  {
    RealtimeFuture<TypeParam> future(&this->logger_);
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    // Destroying without a promise is fine.
  }
  {
    RealtimePromise<TypeParam> promise(&this->logger_);
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    // Destroying without a future is fine.
  }
}

TYPED_TEST(FuturePromiseContextTest, FuturePromiseWithLoggerWorks) {
  RealtimeFuture<TypeParam> future(&this->logger_);
  INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
  INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
  INTR_ASSERT_OK_AND_ASSIGN(auto val, future.Get());
  EXPECT_EQ(this->GetValueForComparison(val),
            this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
}

TYPED_TEST(FuturePromiseContextTest, FutureAfterResetTimesOut) {
  FuturePromiseContext<TypeParam> context(&this->logger_);
  {
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
  }
  INTR_ASSERT_OK(context.Reset(std::chrono::milliseconds(10)));
  INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
  EXPECT_THAT(future.WaitFor(std::chrono::milliseconds(10)),
              StatusIs(StatusCode::kDeadlineExceeded));
}

TYPED_TEST(FuturePromiseContextTest, PromiseFutureWithLoggerWorks) {
  RealtimePromise<TypeParam> promise(&this->logger_);
  INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
  INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
  INTR_ASSERT_OK_AND_ASSIGN(auto val, future.Get());
  EXPECT_EQ(this->GetValueForComparison(val),
            this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
}

TYPED_TEST(FuturePromiseContextTest,
           ContextDestructionLogsFatalErrorWhenFutureNotDestroyed) {
  EXPECT_DEATH(
      ({
        auto context = std::make_unique<FuturePromiseContext<TypeParam>>(
            std::chrono::milliseconds(10), &this->logger_);
        INTR_ASSERT_OK_AND_ASSIGN(auto future, context->GetFuture());
        context.reset();
      }),
      AllOf(HasSubstr(kLogPrefix), HasSubstr("Future was not destroyed")));
}

TYPED_TEST(FuturePromiseContextTest,
           ContextDestructionLogsFatalErrorWhenPromiseNotDestroyed) {
  EXPECT_DEATH(
      ({
        auto context = std::make_unique<FuturePromiseContext<TypeParam>>(
            std::chrono::milliseconds(10), &this->logger_);
        INTR_ASSERT_OK_AND_ASSIGN(auto promise, context->GetPromise());
        context.reset();
      }),
      AllOf(HasSubstr(kLogPrefix), HasSubstr("Promise was not destroyed")));
}

TYPED_TEST(FuturePromiseContextTest, RealtimeFuturePassesLoggerToContext) {
  EXPECT_DEATH(
      ({
        auto future = std::make_unique<RealtimeFuture<TypeParam>>(
            std::chrono::milliseconds(10), &this->logger_);
        INTR_ASSERT_OK_AND_ASSIGN(auto promise, future->GetPromise());
        future.reset();
      }),
      AllOf(HasSubstr(kLogPrefix), HasSubstr("Promise was not destroyed")));
}

TYPED_TEST(FuturePromiseContextTest, RealtimePromisePassesLoggerToContext) {
  EXPECT_DEATH(
      ({
        auto promise = std::make_unique<RealtimePromise<TypeParam>>(
            std::chrono::milliseconds(10), &this->logger_);
        INTR_ASSERT_OK_AND_ASSIGN(auto future, promise->GetFuture());
        promise.reset();
      }),
      AllOf(HasSubstr(kLogPrefix), HasSubstr("Future was not destroyed")));
}

TYPED_TEST(FuturePromiseContextTest, IsWaitFreeDestructibleWorks) {
  {
    FuturePromiseContext<TypeParam> context;
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    // Destroying a future or a promise that doesn't own the context is always
    // wait-free.
    EXPECT_TRUE(future.IsWaitFreeDestructible());
    EXPECT_TRUE(promise.IsWaitFreeDestructible());
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  }
  {
    RealtimeFuture<TypeParam> future;
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    // Destroying a future that owns the context, but doesn't have a promise
    // attached, is wait-free.
    EXPECT_TRUE(future.IsWaitFreeDestructible());
    {
      // Scope for the promise.
      INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
      // Now that the promise is attached, destroying the future is no longer
      // wait-free.
      EXPECT_FALSE(future.IsWaitFreeDestructible());
    }
    // Now that the promise is destroyed, destroying the future is wait-free
    // again.
    EXPECT_TRUE(future.IsWaitFreeDestructible());
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  }
  {
    RealtimePromise<TypeParam> promise;
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    // Destroying a promise that owns the context, but doesn't have a future
    // attached, is wait-free.
    EXPECT_TRUE(promise.IsWaitFreeDestructible());
    {
      // Scope for the future.
      INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
      // Now that the future is attached, destroying the promise is no longer
      // wait-free.
      EXPECT_FALSE(promise.IsWaitFreeDestructible());
    }
    // Now that the future is destroyed, destroying the promise is wait-free
    // again.
    EXPECT_TRUE(promise.IsWaitFreeDestructible());
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  }
}

TYPED_TEST(FuturePromiseContextTest, GetFutureMayOnlyBeCalledOnce) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    auto result = context.GetFuture();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kAlreadyExists);
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    auto result = promise.GetFuture();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kAlreadyExists);
  }
}

TYPED_TEST(FuturePromiseContextTest, GetPromiseMayOnlyBeCalledOnce) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    auto result = context.GetPromise();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kAlreadyExists);
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    auto result = future.GetPromise();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kAlreadyExists);
  }
}

TYPED_TEST(FuturePromiseContextTest,
           GetFutureMayBeCalledAgainAfterDestruction) {
  FuturePromiseContext<TypeParam> context;
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();

  {
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    // Destroying after a future is fine.
  }
  {
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    // Getting a future after destruction is fine.
  }
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();

  RealtimePromise<TypeParam> promise;
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  {
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    // Destroying after a future is fine.
  }
  {
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    // Getting a future after destruction is fine.
  }
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
}

TYPED_TEST(FuturePromiseContextTest,
           GetPromiseMayBeCalledAgainAfterDestruction) {
  FuturePromiseContext<TypeParam> context;
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  {
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    // Destroying after a promise is fine.
  }
  {
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    // Getting a promise after destruction is fine.
  }
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();

  RealtimeFuture<TypeParam> future;
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();

  {
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    // Destroying after a promise is fine.
  }
  {
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    // Getting a promise after destruction is fine.
  }
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
}

TYPED_TEST(FuturePromiseContextTest, IsPromiseAttachedWorks) {
  FuturePromiseContext<TypeParam> context;
  EXPECT_FALSE(context.IsPromiseAttached());
  INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
  EXPECT_TRUE(context.IsPromiseAttached());
  // Calling `Set()` detaches the promise.
  INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
  EXPECT_FALSE(context.IsPromiseAttached());
}

TYPED_TEST(FuturePromiseContextTest, IsFutureAttachedWorks) {
  FuturePromiseContext<TypeParam> context;
  EXPECT_FALSE(context.IsFutureAttached());
  INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
  EXPECT_TRUE(context.IsFutureAttached());
  INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
  INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
  // Calling `Get()` detaches the future.
  INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
  EXPECT_EQ(this->GetValueForComparison(value),
            this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  EXPECT_FALSE(context.IsFutureAttached());
}

TYPED_TEST(FuturePromiseContextTest,
           ContextDestructionWaitsForPromiseToBeDetached) {
  auto context = std::make_unique<FuturePromiseContext<TypeParam>>();
  std::latch trigger_delayed_destruction(1);
  std::latch promise_delete_thread_started(1);
  auto destruction_delay = std::chrono::milliseconds(100);

  INTR_ASSERT_OK_AND_ASSIGN(auto promise, context->GetPromise());

  // Move the promise into a thread for delayed destruction.
  auto promise_delete_thread = std::jthread(
      [promise = std::move(promise), &promise_delete_thread_started,
       &trigger_delayed_destruction, destruction_delay]() mutable {
        promise_delete_thread_started.count_down();
        trigger_delayed_destruction.wait();
        std::this_thread::sleep_for(destruction_delay);
      });

  // Wait for the thread to start.
  promise_delete_thread_started.wait();
  auto start_time = intrinsic::Now();
  // Trigger the delayed destruction.
  trigger_delayed_destruction.count_down();
  // Reset the unique_ptr to trigger the destruction of the context, which
  // should block until the promise is destroyed.
  context.reset();
  EXPECT_GT(intrinsic::Now() - start_time, destruction_delay);
  promise_delete_thread.join();
}

TYPED_TEST(FuturePromiseContextTest,
           FutureDestructionWaitsForPromiseToBeDetached) {
  auto future = std::make_unique<RealtimeFuture<TypeParam>>();
  std::latch trigger_delayed_destruction(1);
  std::latch promise_delete_thread_started(1);
  auto destruction_delay = std::chrono::milliseconds(100);

  INTR_ASSERT_OK_AND_ASSIGN(auto promise, future->GetPromise());

  // Move the promise into a thread for delayed destruction.
  auto promise_delete_thread = std::jthread(
      [promise = std::move(promise), &promise_delete_thread_started,
       &trigger_delayed_destruction, destruction_delay]() mutable {
        promise_delete_thread_started.count_down();
        trigger_delayed_destruction.wait();
        std::this_thread::sleep_for(destruction_delay);
      });

  // Wait for the thread to start.
  promise_delete_thread_started.wait();
  auto start_time = intrinsic::Now();
  // Trigger the delayed destruction.
  trigger_delayed_destruction.count_down();
  // Reset the unique_ptr to trigger the destruction of the context, which
  // should block until the promise is destroyed.
  future.reset();
  EXPECT_GT(intrinsic::Now() - start_time, destruction_delay);
  promise_delete_thread.join();
}

TYPED_TEST(FuturePromiseContextTest,
           ContextDestructionWaitsForFutureToBeDetached) {
  auto context = std::make_unique<FuturePromiseContext<TypeParam>>();
  std::latch trigger_delayed_destruction(1);
  std::latch future_delete_thread_started(1);
  auto destruction_delay = std::chrono::milliseconds(100);

  INTR_ASSERT_OK_AND_ASSIGN(auto future, context->GetFuture());

  // Move the future into a thread for delayed destruction.
  auto future_delete_thread =
      std::jthread([future = std::move(future), &future_delete_thread_started,
                    &trigger_delayed_destruction, destruction_delay]() mutable {
        future_delete_thread_started.count_down();
        trigger_delayed_destruction.wait();
        std::this_thread::sleep_for(destruction_delay);
      });

  // Wait for the thread to start.
  future_delete_thread_started.wait();
  auto start_time = intrinsic::Now();
  // Trigger the delayed destruction.
  trigger_delayed_destruction.count_down();
  // Reset the unique_ptr to trigger the destruction of the context, which
  // should block until the promise is destroyed.
  context.reset();
  EXPECT_GT(intrinsic::Now() - start_time, destruction_delay);
  future_delete_thread.join();
}

TYPED_TEST(FuturePromiseContextTest,
           PromiseDestructionWaitsForFutureToBeDetached) {
  auto promise = std::make_unique<RealtimePromise<TypeParam>>();
  std::latch trigger_delayed_destruction(1);
  std::latch future_delete_thread_started(1);
  auto destruction_delay = std::chrono::milliseconds(100);

  INTR_ASSERT_OK_AND_ASSIGN(auto future, promise->GetFuture());

  // Move the future into a thread for delayed destruction.
  auto future_delete_thread =
      std::jthread([future = std::move(future), &future_delete_thread_started,
                    &trigger_delayed_destruction, destruction_delay]() mutable {
        future_delete_thread_started.count_down();
        trigger_delayed_destruction.wait();
        std::this_thread::sleep_for(destruction_delay);
      });

  // Wait for the thread to start.
  future_delete_thread_started.wait();
  auto start_time = intrinsic::Now();
  // Trigger the delayed destruction.
  trigger_delayed_destruction.count_down();
  // Reset the unique_ptr to trigger the destruction of the context, which
  // should block until the promise is destroyed.
  promise.reset();
  EXPECT_GT(intrinsic::Now() - start_time, destruction_delay);
  future_delete_thread.join();
}

TYPED_TEST(FuturePromiseContextTest,
           ContextDestructionWaitsForFutureAndPromiseToBeDetached) {
  auto context = std::make_unique<FuturePromiseContext<TypeParam>>();
  std::latch trigger_delayed_destruction(1);
  std::latch delete_thread_started(1);
  auto destruction_delay = std::chrono::milliseconds(100);

  INTR_ASSERT_OK_AND_ASSIGN(auto future, context->GetFuture());
  INTR_ASSERT_OK_AND_ASSIGN(auto promise, context->GetPromise());

  // Move the future and promise into a thread for delayed destruction.
  auto delete_thread =
      std::jthread([future = std::move(future), promise = std::move(promise),
                    &delete_thread_started, &trigger_delayed_destruction,
                    destruction_delay]() mutable {
        delete_thread_started.count_down();
        trigger_delayed_destruction.wait();
        std::this_thread::sleep_for(destruction_delay);
      });

  // Wait for the thread to start.
  delete_thread_started.wait();
  auto start_time = intrinsic::Now();
  // Trigger the delayed destruction.
  trigger_delayed_destruction.count_down();
  // Reset the unique_ptr to trigger the destruction of the context, which
  // should block until the future and thepromise are destroyed.
  context.reset();
  EXPECT_GT(intrinsic::Now() - start_time, destruction_delay);
  delete_thread.join();
}

TYPED_TEST(FuturePromiseContextTest,
           ResetWorksWhenNoFutureOrPromiseIsAttached) {
  FuturePromiseContext<TypeParam> context;
  INTR_EXPECT_OK(context.Reset(kWaitTimeout));
}

TYPED_TEST(FuturePromiseContextTest, ResetTimeoutsWhenFutureIsAttached) {
  FuturePromiseContext<TypeParam> context;
  INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
  EXPECT_EQ(context.Reset(kWaitTimeout).code, StatusCode::kDeadlineExceeded);
}

TYPED_TEST(FuturePromiseContextTest, ResetTimeoutsWhenPromiseIsAttached) {
  FuturePromiseContext<TypeParam> context;
  INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
  EXPECT_EQ(context.Reset(kWaitTimeout).code, StatusCode::kDeadlineExceeded);
}

TYPED_TEST(FuturePromiseContextTest,
           ResetTimeoutsWhenFutureAndPromiseAreAttached) {
  FuturePromiseContext<TypeParam> context;
  INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
  INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
  EXPECT_EQ(context.Reset(kWaitTimeout).code, StatusCode::kDeadlineExceeded);
}

TYPED_TEST(FuturePromiseContextTest, ResetWaitsForFutureToBeDetached) {
  FuturePromiseContext<TypeParam> context;
  std::latch trigger_delayed_destruction(1);
  std::latch delete_thread_started(1);
  // Wait for a time that's exactly between kWaitTimeout and 2 * kWaitTimeout,
  // which are the times at which we'll attempt the `Reset()`.
  const auto destruction_delay = kWaitTimeout * 1.5;

  INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
  INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());

  // Move the future and promise into a thread for delayed destruction.
  auto delete_thread =
      std::jthread([future = std::move(future), promise = std::move(promise),
                    &delete_thread_started, &trigger_delayed_destruction,
                    destruction_delay]() mutable {
        delete_thread_started.count_down();
        trigger_delayed_destruction.wait();
        std::this_thread::sleep_for(destruction_delay);
      });

  // Wait for the thread to start.
  delete_thread_started.wait();
  auto start_time = intrinsic::Now();
  // Trigger the delayed destruction.
  trigger_delayed_destruction.count_down();
  // Reset the context, which should block until the future and the promise are
  // destroyed.
  // This first call will time out since the future and promise are not
  // detached and the reset will wait kWaitTimeout for each.
  EXPECT_EQ(context.Reset(kWaitTimeout).code, StatusCode::kDeadlineExceeded);
  EXPECT_GT(intrinsic::Now() - start_time, kWaitTimeout);

  // Sleep for a bit such that the destruction of the future and promise
  // will fall into the wait timeout.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // This second call will not time out since the future and promise are
  // detached.
  INTR_EXPECT_OK(context.Reset(kWaitTimeout));
  // The time taken for the reset to complete must be greater than the
  // destruction delay.
  EXPECT_GT(intrinsic::Now() - start_time, destruction_delay);
  delete_thread.join();
}

TYPED_TEST(FuturePromiseContextTest, GetFutureWhenResetIsInProgress) {
  FuturePromiseContext<TypeParam> context;
  {
    INTR_EXPECT_OK(context.TestOnlyResetLock());
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    auto result = context.GetFuture();
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    EXPECT_FALSE(result.has_value());
    if (!result.has_value()) {
      EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    }
    INTR_ASSERT_OK(context.TestOnlyResetUnlock());
  }
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  auto result = context.GetFuture();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  INTR_ASSERT_OK(std::move(result));
}

TYPED_TEST(FuturePromiseContextTest, GetPromiseWhenResetIsInProgress) {
  FuturePromiseContext<TypeParam> context;
  {
    INTR_EXPECT_OK(context.TestOnlyResetLock());
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    auto result = context.GetPromise();
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    EXPECT_FALSE(result.has_value());
    if (!result.has_value()) {
      EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    }
    INTR_ASSERT_OK(context.TestOnlyResetUnlock());
  }
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  auto result = context.GetPromise();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  INTR_ASSERT_OK(std::move(result));
}

TYPED_TEST(FuturePromiseContextTest, CancelWorks) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    EXPECT_FALSE(context.IsCancelled());
    INTR_EXPECT_OK(context.Cancel());
    EXPECT_TRUE(context.IsCancelled());
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, promise.IsCancelled());
      EXPECT_EQ(v, true);
    }
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, future.IsCancelled());
      EXPECT_EQ(v, true);
    }
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, future.IsCancelled());
      EXPECT_EQ(v, false);
    }
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, promise.IsCancelled());
      EXPECT_EQ(v, false);
    }
    INTR_EXPECT_OK(future.Cancel());
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, future.IsCancelled());
      EXPECT_EQ(v, true);
    }
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, promise.IsCancelled());
      EXPECT_EQ(v, true);
    }
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, promise.IsCancelled());
      EXPECT_EQ(v, false);
    }
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, future.IsCancelled());
      EXPECT_EQ(v, false);
    }
    INTR_EXPECT_OK(promise.Cancel());
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, promise.IsCancelled());
      EXPECT_EQ(v, true);
    }
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, future.IsCancelled());
      EXPECT_EQ(v, true);
    }
  }
}

TYPED_TEST(FuturePromiseContextTest, CancelFailsWhenResetIsInProgress) {
  FuturePromiseContext<TypeParam> context;
  INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
  INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
  RealtimeStatus context_cancel_status;
  RealtimeStatus future_cancel_status;
  RealtimeStatus promise_cancel_status;
  {
    INTR_EXPECT_OK(context.TestOnlyResetLock());
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    context_cancel_status = context.Cancel();
    future_cancel_status = future.Cancel();
    promise_cancel_status = promise.Cancel();
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    INTR_ASSERT_OK(context.TestOnlyResetUnlock());
  }
  EXPECT_EQ(context_cancel_status.code, StatusCode::kResourceExhausted);
  EXPECT_EQ(future_cancel_status.code, StatusCode::kResourceExhausted);
  EXPECT_EQ(promise_cancel_status.code, StatusCode::kResourceExhausted);
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  context_cancel_status = context.Cancel();
  future_cancel_status = future.Cancel();
  promise_cancel_status = promise.Cancel();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  INTR_EXPECT_OK(context_cancel_status);
  INTR_EXPECT_OK(future_cancel_status);
  INTR_EXPECT_OK(promise_cancel_status);
}

TYPED_TEST(FuturePromiseContextTest, CancelFailsWhileResetInProgress) {
  FuturePromiseContext<TypeParam> context;
  std::latch trigger_delayed_reset(1);
  std::latch reset_thread_started(1);

  // Obtain a future to ensure that the reset will wait for it to be
  // destroyed.
  INTR_ASSERT_OK_AND_ASSIGN(auto temporary_future, context.GetFuture());
  std::unique_ptr<RealtimeFuture<TypeParam>> future =
      std::make_unique<RealtimeFuture<TypeParam>>(std::move(temporary_future));

  // Call `Reset()` in a thread waiting ~kWaitTimeout.
  auto reset_thread = std::jthread(
      [&context, &reset_thread_started, &trigger_delayed_reset]() mutable {
        reset_thread_started.count_down();
        trigger_delayed_reset.wait();
        INTR_EXPECT_OK(context.Reset(kWaitTimeout));
      });

  // Wait for the thread to start.
  reset_thread_started.wait();

  // Trigger the delayed reset and destruction.
  trigger_delayed_reset.count_down();
  // Wait for a bit to ensure that the reset is in progress.
  std::this_thread::sleep_for(kWaitTimeout * 0.1);
  // We now should have a little less than kWaitTimeout left where the
  // `Reset()` is in progress. Call `Cancel()` which should fail since the
  // reset is in progress.
  EXPECT_EQ(context.Cancel().code, StatusCode::kResourceExhausted);
  EXPECT_FALSE(context.IsCancelled());
  // Delete the promise to let the `Reset()` finish.
  future.reset();

  // Join the thread to make sure that the reset is done.
  reset_thread.join();

  // Call `Cancel()` which should succeed since the reset is done.
  INTR_EXPECT_OK(context.Cancel());
  EXPECT_TRUE(context.IsCancelled());
}

/// RealtimePromise tests.

TYPED_TEST(FuturePromiseContextTest, RealtimePromiseConstructorWorks) {
  {
    RealtimePromise<TypeParam> promise;
  }
  {
    RealtimePromise<TypeParam> promise(&this->logger_);
  }
  {
    RealtimePromise<TypeParam> promise(std::chrono::milliseconds(500),
                                       &this->logger_);
  }
}

TYPED_TEST(FuturePromiseContextTest, RealtimePromiseMoveConstructorWorks) {
  {
    // Scope for a default constructed promise.
    RealtimePromise<TypeParam> promise;
    {
      // Create a scope for the promise.
      {
        RealtimePromise<TypeParam> promise_move_constructed{std::move(promise)};
      }
    }
  }
  {
    // Scope for an attached promise.
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    {
      // Create a scope for the promise.
      {
        RealtimePromise<TypeParam> promise_move_constructed{std::move(promise)};
      }
    }
  }
}

TYPED_TEST(FuturePromiseContextTest, RealtimePromiseMoveAssignmentWorks) {
  {
    // Scope for a default constructed promise.
    RealtimePromise<TypeParam> promise;
    {
      // Create a scope for the promise.
      {
        RealtimePromise<TypeParam> promise_move_assigned = std::move(promise);
      }
    }
  }
  {
    // Scope for an attached promise.
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    {
      // Create a scope for the promise.
      {
        RealtimePromise<TypeParam> promise_move_assigned = std::move(promise);
      }
    }
  }
}

TYPED_TEST(FuturePromiseContextTest, SetSucceedsOnDefaultConstructedPromise) {
  RealtimePromise<TypeParam> promise;
  INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));

  // Calling `Set` again fails because the value is already set.
  // Note that default constructed promises are always reusable.
  auto set_result = promise.Set(TestFixture::GetTestDefaultValue());
  EXPECT_EQ(set_result.code, StatusCode::kResourceExhausted);
  EXPECT_THAT(set_result.GetMessage(), HasSubstr("already been set"));
}

TYPED_TEST(FuturePromiseContextTest, SetFailsOnDetachedPromise) {
  RealtimePromise<TypeParam> promise{
      RealtimePromise<TypeParam>::GetDetachedPromise()};
  auto set_result = promise.Set(TestFixture::GetTestDefaultValue());
  EXPECT_EQ(set_result.code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(set_result.GetMessage(), HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest, SetSucceedsOnAttachedPromise) {
  FuturePromiseContext<TypeParam> context;
  INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
  INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));

  // Calling `Set` again fails because the promise is no longer attached.
  auto result = promise.Set(TestFixture::GetTestDefaultValue());
  EXPECT_EQ(result.code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(result.GetMessage(), HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest, SetFailsWhenCancelled) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    INTR_ASSERT_OK(context.Cancel());
    EXPECT_TRUE(context.IsCancelled());
    auto result = promise.Set(TestFixture::GetTestDefaultValue());
    EXPECT_EQ(result.code, StatusCode::kCancelled);
    EXPECT_THAT(result.GetMessage(), HasSubstr("cancelled"));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_ASSERT_OK(promise.Cancel());
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, promise.IsCancelled());
      EXPECT_EQ(v, true);
    }
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, future.IsCancelled());
      EXPECT_EQ(v, true);
    }
    auto result = promise.Set(TestFixture::GetTestDefaultValue());
    EXPECT_EQ(result.code, StatusCode::kCancelled);
    EXPECT_THAT(result.GetMessage(), HasSubstr("cancelled"));
  }
}

TYPED_TEST(FuturePromiseContextTest, SetAndGetSucceedWhenTypical) {
  {
    FuturePromiseContext<TypeParam> context;
    for (int i = 0; i < 10000; ++i) {
      INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
      INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
      INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
      INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
      EXPECT_EQ(
          this->GetValueForComparison(value),
          this->GetValueForComparison(TestFixture::GetTestDefaultValue()));

      // Calling `Set` again fails because the promise is no longer attached.
      auto set_result = promise.Set(TestFixture::GetTestDefaultValue());
      EXPECT_EQ(set_result.code, StatusCode::kFailedPrecondition);
      EXPECT_THAT(set_result.GetMessage(), HasSubstr("not attached"));
      // Calling `Get` again fails because the future is no longer attached.
      auto get_result = future.Get();
      ASSERT_FALSE(get_result.has_value());
      EXPECT_EQ(get_result.error().code, StatusCode::kFailedPrecondition);
      EXPECT_THAT(get_result.error().GetMessage(), HasSubstr("not attached"));
    }
  }
  {
    RealtimePromise<TypeParam> promise;
    for (int i = 0; i < 10000; ++i) {
      INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
      INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
      INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
      EXPECT_EQ(
          this->GetValueForComparison(value),
          this->GetValueForComparison(TestFixture::GetTestDefaultValue()));

      // Calling `Set` again succeeds, because default constructed promises are
      // always reusable and the value has been consumed.
      INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
      // Calling `Get` again fails because the future is no longer attached.
      auto get_result = future.Get();
      ASSERT_FALSE(get_result.has_value());
      EXPECT_EQ(get_result.error().code, StatusCode::kFailedPrecondition);
      EXPECT_THAT(get_result.error().GetMessage(), HasSubstr("not attached"));
      // With a new future, we can get the value again.
      INTR_ASSERT_OK_AND_ASSIGN(future, promise.GetFuture());
      INTR_ASSERT_OK_AND_ASSIGN(value, future.Get());
      EXPECT_EQ(
          this->GetValueForComparison(value),
          this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    }
  }
  {
    RealtimeFuture<TypeParam> future;
    for (int i = 0; i < 10000; ++i) {
      INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
      INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
      INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
      EXPECT_EQ(
          this->GetValueForComparison(value),
          this->GetValueForComparison(TestFixture::GetTestDefaultValue()));

      // Calling `Set` again fails because the promise is no longer attached.
      auto set_result = promise.Set(TestFixture::GetTestDefaultValue());
      EXPECT_EQ(set_result.code, StatusCode::kFailedPrecondition);
      EXPECT_THAT(set_result.GetMessage(), HasSubstr("not attached"));
      // Calling `Get` will fail, but not with kFailedPrecondition for not being
      // attached, but instead with kUnavailable, because default
      // constructed futures are always reusable but setting the value already
      // failed above.
      auto get_result = future.Get();
      ASSERT_FALSE(get_result.has_value());
      EXPECT_EQ(get_result.error().code, StatusCode::kUnavailable);
      EXPECT_THAT(get_result.error().GetMessage(),
                  HasSubstr("not available yet or has already been retrieved"));
    }
  }
}

TYPED_TEST(FuturePromiseContextTest,
           SetSucceedsOnReusablePromiseOnlyOnceUntilConsumed) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise,
                              context.GetPromise(/*is_reusable=*/true));
    auto set_value_1 = TestFixture::GetTestDefaultValue();
    auto set_value_2 = TestFixture::GetTestDefaultValue();
    auto set_value_3 = TestFixture::GetTestDefaultValue();
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_EXPECT_OK(promise.Set(std::move(set_value_1)));
    // This second call is not allowed.
    RealtimeStatus status = promise.Set(std::move(set_value_2));
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    EXPECT_EQ(status.code, StatusCode::kResourceExhausted);
    // Now consume/get the value.
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // Now setting is allowed again.
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_EXPECT_OK(promise.Set(std::move(set_value_3)));
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  }
  {
    RealtimePromise<TypeParam> promise;
    auto set_value_1 = TestFixture::GetTestDefaultValue();
    auto set_value_2 = TestFixture::GetTestDefaultValue();
    auto set_value_3 = TestFixture::GetTestDefaultValue();
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_EXPECT_OK(promise.Set(std::move(set_value_1)));
    // This second call is not allowed.
    RealtimeStatus status = promise.Set(std::move(set_value_2));
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    EXPECT_EQ(status.code, StatusCode::kResourceExhausted);
    // Now consume/get the value.
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // Now setting is allowed again.
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_EXPECT_OK(promise.Set(std::move(set_value_3)));
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  }
  {
    RealtimeFuture<TypeParam> future;
    auto set_value_1 = TestFixture::GetTestDefaultValue();
    auto set_value_2 = TestFixture::GetTestDefaultValue();
    auto set_value_3 = TestFixture::GetTestDefaultValue();
    INTR_ASSERT_OK_AND_ASSIGN(auto promise,
                              future.GetPromise(/*is_reusable=*/true));
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_EXPECT_OK(promise.Set(std::move(set_value_1)));
    // This second call is not allowed.
    RealtimeStatus status = promise.Set(std::move(set_value_2));
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    EXPECT_EQ(status.code, StatusCode::kResourceExhausted);
    // Now consume/get the value.
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // Now setting is allowed again.
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_EXPECT_OK(promise.Set(std::move(set_value_3)));
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  }
}

TYPED_TEST(FuturePromiseContextTest,
           SetSucceedsAgainOnReusablePromiseIfReadInBetween) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise,
                              context.GetPromise(/*is_reusable=*/true));
    auto set_value_1 = TestFixture::GetTestDefaultValue();
    auto set_value_2 = TestFixture::GetTestDefaultValue();
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_EXPECT_OK(promise.Set(std::move(set_value_1)));
    // Read the value in between the two calls.
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));

    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_EXPECT_OK(promise.Set(std::move(set_value_2)));
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  }
  {
    RealtimePromise<TypeParam> promise;
    auto set_value_1 = TestFixture::GetTestDefaultValue();
    auto set_value_2 = TestFixture::GetTestDefaultValue();
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_EXPECT_OK(promise.Set(std::move(set_value_1)));
    // Read the value in between the two calls.
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();

    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));

    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_EXPECT_OK(promise.Set(std::move(set_value_2)));
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise,
                              future.GetPromise(/*is_reusable=*/true));
    auto set_value_1 = TestFixture::GetTestDefaultValue();
    auto set_value_2 = TestFixture::GetTestDefaultValue();
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_EXPECT_OK(promise.Set(std::move(set_value_1)));
    // Read the value in between the two calls.
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();

    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));

    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    INTR_EXPECT_OK(promise.Set(std::move(set_value_2)));
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  }
}

TYPED_TEST(FuturePromiseContextTest, CancelFailsOnDetachedPromise) {
  RealtimePromise<TypeParam> promise{
      RealtimePromise<TypeParam>::GetDetachedPromise()};
  auto result = promise.Cancel();
  EXPECT_EQ(result.code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(result.GetMessage(), HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest, IsCancelledFailsOnDetachedPromise) {
  RealtimePromise<TypeParam> promise{
      RealtimePromise<TypeParam>::GetDetachedPromise()};
  auto result = promise.IsCancelled();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(result.error().GetMessage(), HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest, HasValueFailsOnDetachedPromise) {
  RealtimePromise<TypeParam> promise{
      RealtimePromise<TypeParam>::GetDetachedPromise()};
  auto result = promise.HasValue();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(result.error().GetMessage(), HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest, GetFutureFailsOnDetachedPromise) {
  RealtimePromise<TypeParam> promise{
      RealtimePromise<TypeParam>::GetDetachedPromise()};
  auto result = promise.GetFuture();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(result.error().GetMessage(), HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest, IsWaitFreeDestructibleOnDetachedPromise) {
  RealtimePromise<TypeParam> promise{
      RealtimePromise<TypeParam>::GetDetachedPromise()};
  EXPECT_TRUE(promise.IsWaitFreeDestructible());
}

/// RealtimeFuture tests.

TYPED_TEST(FuturePromiseContextTest, RealtimeFutureConstructorWorks) {
  {
    RealtimeFuture<TypeParam> future;
  }
  {
    RealtimeFuture<TypeParam> future(&this->logger_);
  }
  {
    RealtimeFuture<TypeParam> future(std::chrono::milliseconds(500),
                                     &this->logger_);
  }
}

TYPED_TEST(FuturePromiseContextTest, RealtimeFutureMoveConstructorWorks) {
  {
    // Scope for a default constructed future.
    RealtimeFuture<TypeParam> future;
    {
      // Create a scope for the future.
      {
        RealtimeFuture<TypeParam> future_move_constructed{std::move(future)};
      }
    }
  }
  {
    // Scope for an attached future.
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    {
      // Create a scope for the future.
      {
        RealtimeFuture<TypeParam> future_move_constructed{std::move(future)};
      }
    }
  }
}

TYPED_TEST(FuturePromiseContextTest, RealtimeFutureMoveAssignmentWorks) {
  {
    // Scope for a default constructed future.
    RealtimeFuture<TypeParam> future;
    {
      // Create a scope for the future.
      {
        RealtimeFuture<TypeParam> future_move_assigned = std::move(future);
      }
    }
  }
  {
    // Scope for an attached future.
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    {
      // Create a scope for the future.
      {
        RealtimeFuture<TypeParam> future_move_assigned = std::move(future);
      }
    }
  }
}

TYPED_TEST(FuturePromiseContextTest,
           GetReturnsUnavailableOnDefaultConstructedFuture) {
  RealtimeFuture<TypeParam> future;
  {
    auto result = future.Get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("not available"));
  }

  // Calling `Get` again fails is valid, because the future only detaches after
  // a successful `Get`.
  {
    auto result = future.Get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("not available"));
  }
}

TYPED_TEST(FuturePromiseContextTest, GetReturnsUnavailableOnAttachedFuture) {
  FuturePromiseContext<TypeParam> context;
  INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());

  auto result = future.Get();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
  EXPECT_THAT(result.error().GetMessage(), HasSubstr("not available"));
}

TYPED_TEST(FuturePromiseContextTest, GetFailsOnDetachedFuture) {
  RealtimeFuture<TypeParam> future{
      RealtimeFuture<TypeParam>::GetDetachedFuture()};

  auto result = future.Get();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(result.error().GetMessage(), HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest, GetFailsWhenCancelled) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_ASSERT_OK(context.Cancel());
    EXPECT_TRUE(context.IsCancelled());
    auto result = future.Get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("cancelled"));
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK(future.Cancel());
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, future.IsCancelled());
      EXPECT_EQ(v, true);
    }
    auto result = future.Get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("cancelled"));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_ASSERT_OK(promise.Cancel());
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, promise.IsCancelled());
      EXPECT_EQ(v, true);
    }
    auto result = future.Get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("cancelled"));
  }
}

TYPED_TEST(FuturePromiseContextTest, GetFailsIfValueIsNotAvailable) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    auto result = future.Get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("is not available yet"));
  }
  {
    RealtimeFuture<TypeParam> future;
    auto result = future.Get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("is not available yet"));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    auto result = future.Get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("is not available yet"));
  }
}

TYPED_TEST(FuturePromiseContextTest,
           GetSucceedsOnReusableFutureOnlyOnceUntilSetAgain) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise,
                              context.GetPromise(/*is_reusable=*/true));
    INTR_ASSERT_OK_AND_ASSIGN(auto future,
                              context.GetFuture(/*is_reusable=*/true));
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // The future cannot be read twice.
    {
      auto result = future.Get();
      ASSERT_FALSE(result.has_value());
      EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    }
    // Setting the value again is allowed.
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    // Now the future can be read again.
    INTR_ASSERT_OK_AND_ASSIGN(value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));

    // Setting the value again is allowed.
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    // The future can be read again.
    INTR_ASSERT_OK_AND_ASSIGN(value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise,
                              future.GetPromise(/*is_reusable=*/true));
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // The future cannot be read twice.
    {
      auto result = future.Get();
      ASSERT_FALSE(result.has_value());
      EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    }
    // Setting the value again is allowed.
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    // Now the future can be read again.
    INTR_ASSERT_OK_AND_ASSIGN(value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));

    // Setting the value again is allowed.
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    // The future can be read again.
    INTR_ASSERT_OK_AND_ASSIGN(value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future,
                              promise.GetFuture(/*is_reusable=*/true));
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // The future cannot be read twice.
    {
      auto result = future.Get();
      ASSERT_FALSE(result.has_value());
      EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    }

    // Setting the value again is allowed.
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    // Now the future can be read again.
    INTR_ASSERT_OK_AND_ASSIGN(value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));

    // Setting the value again is allowed.
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    // The future can be read again.
    INTR_ASSERT_OK_AND_ASSIGN(value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
}

TYPED_TEST(FuturePromiseContextTest,
           PeekReturnsUnavailableOnDefaultConstructedFuture) {
  RealtimeFuture<TypeParam> future;

  auto result =
      future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
  EXPECT_THAT(result.error().GetMessage(), HasSubstr("not available"));
}

TYPED_TEST(FuturePromiseContextTest, PeekFailsOnDetachedFuture) {
  RealtimeFuture<TypeParam> future{
      RealtimeFuture<TypeParam>::GetDetachedFuture()};
  auto result =
      future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(result.error().GetMessage(), HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest, PeekReturnsUnavailableOnAttachedPromise) {
  FuturePromiseContext<TypeParam> context;
  INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
  auto result =
      future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
  EXPECT_THAT(result.error().GetMessage(), HasSubstr("not available"));
}

TYPED_TEST(FuturePromiseContextTest, PeekFailsWhenCancelled) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_ASSERT_OK(context.Cancel());
    EXPECT_TRUE(context.IsCancelled());
    auto result =
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("cancelled"));
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK(future.Cancel());
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, future.IsCancelled());
      EXPECT_EQ(v, true);
    }
    auto result =
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("cancelled"));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_ASSERT_OK(promise.Cancel());
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, promise.IsCancelled());
      EXPECT_EQ(v, true);
    }
    auto result =
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("cancelled"));
  }
}

TYPED_TEST(FuturePromiseContextTest, PeekFailsIfValueIsNotAvailable) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    auto result =
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("is not available yet"));
  }
  {
    RealtimeFuture<TypeParam> future;
    auto result =
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("is not available yet"));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    auto result =
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("is not available yet"));
  }
}

TYPED_TEST(FuturePromiseContextTest, PeekSucceedsEvenOnSuccessiveCalls) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // Peek can be called multiple times.
    INTR_ASSERT_OK_AND_ASSIGN(
        value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // Peek can be called multiple times.
    INTR_ASSERT_OK_AND_ASSIGN(
        value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // Peek can be called multiple times.
    INTR_ASSERT_OK_AND_ASSIGN(
        value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
}

TYPED_TEST(FuturePromiseContextTest,
           PeekSucceedsOnReusableFutureEvenOnSuccessiveCalls) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    INTR_ASSERT_OK_AND_ASSIGN(auto future,
                              context.GetFuture(/*is_reusable=*/true));
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // Peek can be called multiple times.
    INTR_ASSERT_OK_AND_ASSIGN(
        value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // Peek can be called multiple times.
    INTR_ASSERT_OK_AND_ASSIGN(
        value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future,
                              promise.GetFuture(/*is_reusable=*/true));
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // Peek can be called multiple times.
    INTR_ASSERT_OK_AND_ASSIGN(
        value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
}

TYPED_TEST(FuturePromiseContextTest, PeekDoesNotConsumeValue) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, future.HasValue());
      EXPECT_EQ(v, true);
    }
    INTR_ASSERT_OK_AND_ASSIGN(auto new_value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(new_value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, future.HasValue());
      EXPECT_EQ(v, true);
    }
    INTR_ASSERT_OK_AND_ASSIGN(auto new_value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(new_value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto v, future.HasValue());
      EXPECT_EQ(v, true);
    }
    INTR_ASSERT_OK_AND_ASSIGN(auto new_value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(new_value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
}

TYPED_TEST(FuturePromiseContextTest, PeekFailsAfterGet) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    auto result =
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kFailedPrecondition);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("not attached"));
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // Peek fails with kUnavailable, since the value has already been consumed,
    // but not with kFailedPrecondition since default constructed futures are
    // always reusable and thus don't detach.
    auto result =
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kUnavailable);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("is not available yet"));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_EXPECT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    auto result =
        future.Peek(TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kFailedPrecondition);
    EXPECT_THAT(result.error().GetMessage(), HasSubstr("not attached"));
  }
}

TYPED_TEST(FuturePromiseContextTest,
           WaitForAndGetTimeoutsOnDefaultConstructedFuture) {
  RealtimeFuture<TypeParam> future;
  auto result = future.WaitForAndGet(kWaitTimeout);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kDeadlineExceeded);
}

TYPED_TEST(FuturePromiseContextTest, WaitForAndGetFailsOnDetachedFuture) {
  RealtimeFuture<TypeParam> future{
      RealtimeFuture<TypeParam>::GetDetachedFuture()};
  auto result = future.WaitForAndGet(kWaitTimeout);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(result.error().message, HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest, WaitForAndGetTimeoutsOnAttachedFuture) {
  FuturePromiseContext<TypeParam> context;
  INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());

  auto wait_result = future.WaitForAndGet(kWaitTimeout);
  ASSERT_FALSE(wait_result.has_value());
  EXPECT_EQ(wait_result.error().code, StatusCode::kDeadlineExceeded);
}

TYPED_TEST(FuturePromiseContextTest,
           WaitForAndPeekTimeoutsOnDefaultConstructedFuture) {
  RealtimeFuture<TypeParam> future;
  auto peek_result = future.WaitForAndPeek(
      kWaitTimeout, TestFixture::template GetTestCopyFunction<TypeParam>());
  ASSERT_FALSE(peek_result.has_value());
  EXPECT_EQ(peek_result.error().code, StatusCode::kDeadlineExceeded);
}

TYPED_TEST(FuturePromiseContextTest, WaitForAndPeekTimeoutsOnAttachedFuture) {
  FuturePromiseContext<TypeParam> context;
  INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
  auto peek_result = future.WaitForAndPeek(
      kWaitTimeout, TestFixture::template GetTestCopyFunction<TypeParam>());
  ASSERT_FALSE(peek_result.has_value());
  EXPECT_EQ(peek_result.error().code, StatusCode::kDeadlineExceeded);
}

TYPED_TEST(FuturePromiseContextTest, WaitForAndGetFailsWhenCancelled) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_ASSERT_OK(context.Cancel());
    EXPECT_TRUE(context.IsCancelled());
    auto result = future.WaitForAndGet(kWaitTimeout);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);
    EXPECT_THAT(result.error().message, HasSubstr("cancelled"));
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK(future.Cancel());
    INTR_ASSERT_OK_AND_ASSIGN(bool is_cancelled, future.IsCancelled());
    ASSERT_TRUE(is_cancelled);
    auto wait_result = future.WaitForAndGet(kWaitTimeout);
    ASSERT_FALSE(wait_result.has_value());
    EXPECT_EQ(wait_result.error().code, StatusCode::kCancelled);
    EXPECT_THAT(wait_result.error().message, HasSubstr("cancelled"));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_ASSERT_OK(promise.Cancel());
    auto wait_result = future.WaitForAndGet(kWaitTimeout);
    ASSERT_FALSE(wait_result.has_value());
    EXPECT_EQ(wait_result.error().code, StatusCode::kCancelled);
    EXPECT_THAT(wait_result.error().message, HasSubstr("cancelled"));
  }
}

TYPED_TEST(FuturePromiseContextTest, WaitForAndPeekFailsWhenCancelled) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_ASSERT_OK(context.Cancel());
    EXPECT_TRUE(context.IsCancelled());
    auto result = future.WaitForAndPeek(
        kWaitTimeout, TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);
    EXPECT_THAT(result.error().message, HasSubstr("cancelled"));
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK(future.Cancel());
    INTR_ASSERT_OK_AND_ASSIGN(bool is_cancelled, future.IsCancelled());
    EXPECT_TRUE(is_cancelled);
    auto result = future.WaitForAndPeek(
        kWaitTimeout, TestFixture::template GetTestCopyFunction<TypeParam>());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);
    EXPECT_THAT(result.error().message, HasSubstr("cancelled"));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_ASSERT_OK(promise.Cancel());
    auto result = future.WaitForAndPeek(
        kWaitTimeout, TestFixture::template GetTestCopyFunction<TypeParam>());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);
    EXPECT_THAT(result.error().message, HasSubstr("cancelled"));
  }
}

TYPED_TEST(FuturePromiseContextTest, WaitForAndGetFailsIfValueIsNotAvailable) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    auto result = future.WaitForAndGet(kWaitTimeout);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kDeadlineExceeded);
  }
  {
    RealtimeFuture<TypeParam> future;
    auto result = future.WaitForAndGet(kWaitTimeout);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kDeadlineExceeded);
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    auto result = future.WaitForAndGet(kWaitTimeout);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kDeadlineExceeded);
  }
}

TYPED_TEST(FuturePromiseContextTest, WaitForAndPeekFailsIfValueIsNotAvailable) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    auto result = future.WaitForAndPeek(
        kWaitTimeout, TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kDeadlineExceeded);
  }
  {
    RealtimeFuture<TypeParam> future;
    auto result = future.WaitForAndPeek(
        kWaitTimeout, TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kDeadlineExceeded);
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    auto result = future.WaitForAndPeek(
        kWaitTimeout, TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kDeadlineExceeded);
  }
}

TYPED_TEST(FuturePromiseContextTest, WaitForAndGetSucceeds) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.WaitForAndGet(kWaitTimeout));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.WaitForAndGet(kWaitTimeout));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.WaitForAndGet(kWaitTimeout));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
}

TYPED_TEST(FuturePromiseContextTest, WaitForAndPeekSucceeds) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.WaitForAndPeek(
            kWaitTimeout,
            TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.WaitForAndPeek(
            kWaitTimeout,
            TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.WaitForAndPeek(
            kWaitTimeout,
            TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
}

TYPED_TEST(FuturePromiseContextTest,
           WaitForAndGetSucceedsOnResuableFutureOnlyOnceUntilSetAgain) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    INTR_ASSERT_OK_AND_ASSIGN(auto future,
                              context.GetFuture(/*is_reusable=*/true));
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.WaitForAndGet(kWaitTimeout));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // The future cannot be read twice.
    auto result = future.WaitForAndGet(kWaitTimeout);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kDeadlineExceeded);
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.WaitForAndGet(kWaitTimeout));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // The future cannot be read twice.
    auto result = future.WaitForAndGet(kWaitTimeout);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kDeadlineExceeded);
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future,
                              promise.GetFuture(/*is_reusable=*/true));
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.WaitForAndGet(kWaitTimeout));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    // The future cannot be read twice.
    auto result = future.WaitForAndGet(kWaitTimeout);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kDeadlineExceeded);
  }
}

TYPED_TEST(FuturePromiseContextTest,
           WaitForAndPeekSucceedsEvenOnSuccessiveCalls) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.WaitForAndPeek(
            kWaitTimeout,
            TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));

    // WaitForAndPeek can be called multiple times.
    INTR_ASSERT_OK_AND_ASSIGN(
        value, future.WaitForAndPeek(
                   kWaitTimeout,
                   TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.WaitForAndPeek(
            kWaitTimeout,
            TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));

    // WaitForAndPeek can be called multiple times.
    INTR_ASSERT_OK_AND_ASSIGN(
        value, future.WaitForAndPeek(
                   kWaitTimeout,
                   TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    INTR_ASSERT_OK_AND_ASSIGN(
        auto value,
        future.WaitForAndPeek(
            kWaitTimeout,
            TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));

    // WaitForAndPeek can be called multiple times.
    INTR_ASSERT_OK_AND_ASSIGN(
        value, future.WaitForAndPeek(
                   kWaitTimeout,
                   TestFixture::template GetTestCopyFunction<TypeParam>()));
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
  }
}

TYPED_TEST(FuturePromiseContextTest,
           WaitForAndGetFailsWhenCancelledWhileWaiting) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    std::latch cancel_thread_started(1);

    auto cancel_thread =
        std::jthread([&context, &cancel_thread_started]() mutable {
          cancel_thread_started.count_down();
          // Wait for a bit to ensure that `WaitFor()` is already waiting.
          std::this_thread::sleep_for(kWaitTimeout);
          // Cancel the context.
          INTR_EXPECT_OK(context.Cancel());
        });

    // Wait for the thread to start.
    cancel_thread_started.wait();

    auto result = future.WaitForAndGet(kWaitTimeout * 2);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);

    cancel_thread.join();
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    std::latch cancel_thread_started(1);

    auto cancel_thread = std::jthread(
        [promise = std::move(promise), &cancel_thread_started]() mutable {
          cancel_thread_started.count_down();
          // Wait for a bit to ensure that `WaitFor()` is already waiting.
          std::this_thread::sleep_for(kWaitTimeout);
          // Cancel the context.
          INTR_EXPECT_OK(promise.Cancel());
        });

    // Wait for the thread to start.
    cancel_thread_started.wait();
    auto result = future.WaitForAndGet(kWaitTimeout * 2);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);

    cancel_thread.join();
  }
  {
    RealtimePromise<TypeParam> promise;
    std::jthread cancel_thread;
    {
      // Scope for the future to ensure it is destroyed before the promise.
      INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
      std::latch cancel_thread_started(1);
      cancel_thread = std::jthread(
          [promise = std::move(promise), &cancel_thread_started]() mutable {
            cancel_thread_started.count_down();
            // Wait for a bit to ensure that `WaitFor()` is already waiting.
            std::this_thread::sleep_for(kWaitTimeout);
            // Cancel the context.
            INTR_EXPECT_OK(promise.Cancel());
          });

      // Wait for the thread to start.
      cancel_thread_started.wait();
      auto result = future.WaitForAndGet(kWaitTimeout * 2);
      ASSERT_FALSE(result.has_value());
      EXPECT_EQ(result.error().code, StatusCode::kCancelled);
    }

    cancel_thread.join();
  }
}

TYPED_TEST(FuturePromiseContextTest,
           WaitForAndPeekFailsWhenCancelledWhileWaiting) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    std::latch cancel_thread_started(1);

    auto cancel_thread =
        std::jthread([&context, &cancel_thread_started]() mutable {
          cancel_thread_started.count_down();
          // Wait for a bit to ensure that `WaitFor()` is already waiting.
          std::this_thread::sleep_for(kWaitTimeout);
          // Cancel the context.
          INTR_EXPECT_OK(context.Cancel());
        });

    // Wait for the thread to start.
    cancel_thread_started.wait();

    auto result = future.WaitForAndPeek(
        kWaitTimeout * 2,
        TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);

    cancel_thread.join();
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    std::latch cancel_thread_started(1);

    auto cancel_thread = std::jthread(
        [promise = std::move(promise), &cancel_thread_started]() mutable {
          cancel_thread_started.count_down();
          // Wait for a bit to ensure that `WaitFor()` is already waiting.
          std::this_thread::sleep_for(kWaitTimeout);
          // Cancel the context.
          INTR_EXPECT_OK(promise.Cancel());
        });

    // Wait for the thread to start.
    cancel_thread_started.wait();
    auto result = future.WaitForAndPeek(
        kWaitTimeout * 2,
        TestFixture::template GetTestCopyFunction<TypeParam>());
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, StatusCode::kCancelled);

    cancel_thread.join();
  }
  {
    RealtimePromise<TypeParam> promise;
    std::jthread cancel_thread;
    {
      // Scope for the future to ensure it is destroyed before the promise.
      INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
      std::latch cancel_thread_started(1);
      cancel_thread = std::jthread(
          [promise = std::move(promise), &cancel_thread_started]() mutable {
            cancel_thread_started.count_down();
            // Wait for a bit to ensure that `WaitFor()` is already waiting.
            std::this_thread::sleep_for(kWaitTimeout);
            // Cancel the context.
            INTR_EXPECT_OK(promise.Cancel());
          });

      // Wait for the thread to start.
      cancel_thread_started.wait();
      auto result = future.WaitForAndPeek(
          kWaitTimeout * 2,
          TestFixture::template GetTestCopyFunction<TypeParam>());
      ASSERT_FALSE(result.has_value());
      EXPECT_EQ(result.error().code, StatusCode::kCancelled);
    }

    cancel_thread.join();
  }
}

TYPED_TEST(FuturePromiseContextTest, WaitForAndPeekFailsOnDetachedFuture) {
  RealtimeFuture<TypeParam> future{
      RealtimeFuture<TypeParam>::GetDetachedFuture()};
  auto result = future.WaitForAndPeek(
      kWaitTimeout, TestFixture::template GetTestCopyFunction<TypeParam>());
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(result.error().message, HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest,
           HasValueReturnsFalseOnDefaultConstructedFuture) {
  RealtimeFuture<TypeParam> future;
  INTR_ASSERT_OK_AND_ASSIGN(bool has_value, future.HasValue());
  EXPECT_FALSE(has_value);
}

TYPED_TEST(FuturePromiseContextTest, HasValueReturnsFalseOnAttachedFuture) {
  FuturePromiseContext<TypeParam> context;
  INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
  INTR_ASSERT_OK_AND_ASSIGN(bool has_value, future.HasValue());
  EXPECT_FALSE(has_value);
}

TYPED_TEST(FuturePromiseContextTest, HasValueFailsIfCancelled) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_ASSERT_OK(context.Cancel());
    auto has_value_result = future.HasValue();
    ASSERT_FALSE(has_value_result.has_value());
    EXPECT_EQ(has_value_result.error().code, StatusCode::kCancelled);
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    INTR_ASSERT_OK(future.Cancel());
    auto has_value_result = future.HasValue();
    ASSERT_FALSE(has_value_result.has_value());
    EXPECT_EQ(has_value_result.error().code, StatusCode::kCancelled);
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    INTR_ASSERT_OK(promise.Cancel());
    auto has_value_result = future.HasValue();
    ASSERT_FALSE(has_value_result.has_value());
    EXPECT_EQ(has_value_result.error().code, StatusCode::kCancelled);
  }
}

TYPED_TEST(FuturePromiseContextTest, HasValueWorks) {
  {
    FuturePromiseContext<TypeParam> context;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, context.GetFuture());
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, context.GetPromise());
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto future_has_value, future.HasValue());
      INTR_ASSERT_OK_AND_ASSIGN(auto promise_has_value, promise.HasValue());
      EXPECT_FALSE(future_has_value);
      EXPECT_FALSE(promise_has_value);
    }
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto future_has_value, future.HasValue());
      EXPECT_TRUE(future_has_value);
    }
    {
      // HasValue no longer works, since the promise is detached.
      auto has_value_result = promise.HasValue();
      ASSERT_FALSE(has_value_result.has_value());
      EXPECT_EQ(has_value_result.error().code, StatusCode::kFailedPrecondition);
      EXPECT_THAT(has_value_result.error().GetMessage(),
                  HasSubstr("not attached"));
    }
    // Now consume the value.
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    {
      // HasValue still doesn't work.
      auto has_value_result = promise.HasValue();
      ASSERT_FALSE(has_value_result.has_value());
      EXPECT_EQ(has_value_result.error().code, StatusCode::kFailedPrecondition);
      EXPECT_THAT(has_value_result.error().GetMessage(),
                  HasSubstr("not attached"));
    }
  }
  {
    RealtimeFuture<TypeParam> future;
    INTR_ASSERT_OK_AND_ASSIGN(auto promise, future.GetPromise());
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto future_has_value, future.HasValue());
      INTR_ASSERT_OK_AND_ASSIGN(auto promise_has_value, promise.HasValue());
      EXPECT_FALSE(future_has_value);
      EXPECT_FALSE(promise_has_value);
    }
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto future_has_value, future.HasValue());
      EXPECT_TRUE(future_has_value);
    }
    // HasValue no longer works, since the promise is detached.
    {
      // HasValue no longer works, since the promise is detached.
      auto has_value_result = promise.HasValue();
      ASSERT_FALSE(has_value_result.has_value());
      EXPECT_EQ(has_value_result.error().code, StatusCode::kFailedPrecondition);
      EXPECT_THAT(has_value_result.error().GetMessage(),
                  HasSubstr("not attached"));
    }

    // Now consume the value.
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    {
      // HasValue works, since default-constructed futures are always reusable.
      INTR_ASSERT_OK_AND_ASSIGN(auto future_has_value, future.HasValue());
      EXPECT_FALSE(future_has_value);
    }
  }
  {
    RealtimePromise<TypeParam> promise;
    INTR_ASSERT_OK_AND_ASSIGN(auto future, promise.GetFuture());
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto future_has_value, future.HasValue());
      INTR_ASSERT_OK_AND_ASSIGN(auto promise_has_value, promise.HasValue());
      EXPECT_FALSE(future_has_value);
      EXPECT_FALSE(promise_has_value);
    }
    INTR_ASSERT_OK(promise.Set(TestFixture::GetTestDefaultValue()));
    {
      INTR_ASSERT_OK_AND_ASSIGN(auto future_has_value, future.HasValue());
      EXPECT_TRUE(future_has_value);
    }
    // HasValue still works, since default-constructed promises are always
    // reusable.
    {
      // HasValue works, since default-constructed futures are always reusable.
      INTR_ASSERT_OK_AND_ASSIGN(auto promise_has_value, promise.HasValue());
      EXPECT_TRUE(promise_has_value);
    }

    // Now consume the value.
    INTR_ASSERT_OK_AND_ASSIGN(auto value, future.Get());
    EXPECT_EQ(this->GetValueForComparison(value),
              this->GetValueForComparison(TestFixture::GetTestDefaultValue()));
    {
      // HasValue no longer works, since the future is detached.
      auto has_value_result = future.HasValue();
      ASSERT_FALSE(has_value_result.has_value());
      EXPECT_EQ(has_value_result.error().code, StatusCode::kFailedPrecondition);
      EXPECT_THAT(has_value_result.error().GetMessage(),
                  HasSubstr("not attached"));
    }
  }
}

TYPED_TEST(FuturePromiseContextTest, CancelFailsOnDetachedFuture) {
  RealtimeFuture<TypeParam> future{
      RealtimeFuture<TypeParam>::GetDetachedFuture()};
  auto result = future.Cancel();
  EXPECT_EQ(result.code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(result.GetMessage(), HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest, IsCancelledFailsOnDetachedFuture) {
  RealtimeFuture<TypeParam> future{
      RealtimeFuture<TypeParam>::GetDetachedFuture()};
  auto result = future.IsCancelled();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(result.error().GetMessage(), HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest, HasValueFailsOnDetachedFuture) {
  RealtimeFuture<TypeParam> future{
      RealtimeFuture<TypeParam>::GetDetachedFuture()};
  auto result = future.HasValue();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(result.error().GetMessage(), HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest, GetPromiseFailsOnDetachedFuture) {
  RealtimeFuture<TypeParam> future{
      RealtimeFuture<TypeParam>::GetDetachedFuture()};
  auto result = future.GetPromise();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code, StatusCode::kFailedPrecondition);
  EXPECT_THAT(result.error().GetMessage(), HasSubstr("not attached"));
}

TYPED_TEST(FuturePromiseContextTest, IsWaitFreeDestructibleOnDetachedFuture) {
  RealtimeFuture<TypeParam> future{
      RealtimeFuture<TypeParam>::GetDetachedFuture()};
  EXPECT_TRUE(future.IsWaitFreeDestructible());
}

// This test attempts to trigger race conditions by running multiple threads
// that perform random operations on a FuturePromiseContext, Future and Promise.
// The operations are selected randomly from a set of possible operations.
// The test checks that the FuturePromiseContext, Future and Promise remain in a
// valid state and that only expected errors are returned and no internal errors
// occur.
TYPED_TEST(FuturePromiseContextTest, HammerTest) {
  constexpr int kNumIterations = 10000;
  FuturePromiseContext<TypeParam> context;

  std::size_t successful_set_count = 0;
  std::size_t producer_cycles = 0;
  std::size_t successful_get_count = 0;
  std::size_t consumer_cycles = 0;

  std::latch producer_thread_started(1);
  std::latch consumer_thread_started(1);
  std::latch start_tasks(1);

  // The producer thread continuously tries to set a value if one is not
  // present.
  auto producer_task = [&](std::stop_token stop_token) {
    std::mt19937 producer_gen(42);  // Fixed seed for determinism.
    // While the producer has 8 actions, we draw numbers between 0 and 20 to
    // make it more likely to set/get a value.
    std::uniform_int_distribution<> producer_action_dist(0, 20);
    std::uniform_int_distribution<> sleep_millis_100_dist(1, 100);

    std::array<RealtimePromise<TypeParam>, 2> promises;
    uint8_t current_promise = 0;
    INTR_ASSERT_OK_AND_ASSIGN(promises[current_promise], context.GetPromise());
    producer_thread_started.count_down();
    start_tasks.wait();

    while (!stop_token.stop_requested()) {
      producer_cycles++;
      switch (producer_action_dist(producer_gen)) {
        default:
          [[fallthrough]];
        case 0: {  // Set a value
          auto value = TestFixture::GetTestDefaultValue();
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          auto status = promises[current_promise].Set(std::move(value));
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          if (!status.ok()) {
            // Status should only be cancelled or resource exhausted.
            EXPECT_THAT(status.code,
                        AnyOf(Eq(StatusCode::kCancelled),
                              Eq(StatusCode::kResourceExhausted),
                              Eq(StatusCode::kFailedPrecondition)));
          } else {
            successful_set_count++;
          }
          break;
        }
        case 1: {  // Move itself
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          std::swap(promises[0], promises[1]);
          current_promise = (current_promise + 1) % 2;
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          break;
        }
        case 2: {  // Destroy the promise.
          promises[current_promise] = RealtimePromise<TypeParam>();
          break;
        }
        case 3: {  // Create a new promise from the context
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          auto status_or_promise = context.GetPromise();
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          if (!status_or_promise.has_value()) {
            // Status should only be unavailable or already exists.
            EXPECT_THAT(status_or_promise.error().code,
                        AnyOf(Eq(StatusCode::kUnavailable),
                              Eq(StatusCode::kAlreadyExists)));

            break;
          } else {
            promises[current_promise] = std::move(status_or_promise.value());
          }
          break;
        }
        case 4: {  // Cancel the promise
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          auto status = promises[current_promise].Cancel();
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          if (!status.ok()) {
            // Status should only be resource exhausted.
            EXPECT_THAT(status.code,
                        AnyOf(Eq(StatusCode::kResourceExhausted),
                              Eq(StatusCode::kFailedPrecondition)));
          }
          break;
        }
        case 5: {  // Check if cancelled
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          auto status = promises[current_promise].IsCancelled();
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          // IsCancelled should always be okay, or FailedPrecondition
          if (!status.has_value()) {
            EXPECT_EQ(status.error().code, StatusCode::kFailedPrecondition);
          }
          break;
        }
        case 6: {  // Check if has value
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          auto status = promises[current_promise].HasValue();
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          // HasValue should always be okay, or be
          // cancelled or FailedPrecondition.
          if (!status.has_value()) {
            EXPECT_THAT(status.error().code,
                        AnyOf(Eq(StatusCode::kCancelled),
                              Eq(StatusCode::kFailedPrecondition)));
          }
          break;
        }
        case 7: {  // Get a future from the promise.
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          auto status = promises[current_promise].GetFuture();
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          if (!status.has_value()) {
            // Status should only be unavailable or already exists.
            EXPECT_THAT(status.error().code,
                        AnyOf(Eq(StatusCode::kUnavailable),
                              Eq(StatusCode::kAlreadyExists),
                              Eq(StatusCode::kFailedPrecondition)));
          }
          break;
        }
        case 8: {  // Check if wait free destructible
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          bool is_wait_free_destructible =
              promises[current_promise].IsWaitFreeDestructible();
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          // Could be true or false.
          EXPECT_THAT(is_wait_free_destructible, AnyOf(true, false));
          break;
        }
      };
      // Sleep for a random, short amount of time to yield.
      std::this_thread::sleep_for(
          std::chrono::microseconds(sleep_millis_100_dist(producer_gen)));
    }
  };

  auto consumer_task = [&, copy_fn = TestFixture::template GetTestCopyFunction<
                               TypeParam>()](std::stop_token stop_token) {
    std::mt19937 consumer_gen(42);  // Fixed seed for determinism.
    // While the consumer has 11 actions, we draw numbers between 0 and 20
    // to make it more likely to `Get()` a value.
    std::uniform_int_distribution<> consumer_action_dist(0, 20);
    std::uniform_int_distribution<> sleep_millis_100_dist(1, 100);
    std::array<RealtimeFuture<TypeParam>, 2> futures;
    uint8_t current_future = 0;
    INTR_ASSERT_OK_AND_ASSIGN(futures[current_future], context.GetFuture());

    consumer_thread_started.count_down();
    start_tasks.wait();

    while (!stop_token.stop_requested()) {
      consumer_cycles++;
      switch (consumer_action_dist(consumer_gen)) {
        default:
          [[fallthrough]];
        case 0: {  // Get a value
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          auto status = futures[current_future].Get();
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          if (!status.has_value()) {
            // Status should only be cancelled or unavailable.
            EXPECT_THAT(status.error().code,
                        AnyOf(StatusCode::kCancelled, StatusCode::kUnavailable,
                              StatusCode::kFailedPrecondition));
          } else {
            successful_get_count++;
          }
          break;
        }
        case 1: {  // Move itself
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          std::swap(futures[0], futures[1]);
          current_future = (current_future + 1) % 2;
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          break;
        }
        case 2: {  // Peek a value
          auto status = futures[current_future].Peek(copy_fn);
          if (!status.has_value()) {
            // Status should only be cancelled or unavailable.
            EXPECT_THAT(status.error().code,
                        AnyOf(StatusCode::kCancelled, StatusCode::kUnavailable,
                              StatusCode::kFailedPrecondition));
          }
          break;
        }
        case 3: {  // WaitForAndGet a value
          auto status = futures[current_future].WaitForAndGet(
              std::chrono::milliseconds(sleep_millis_100_dist(consumer_gen)));
          if (!status.has_value()) {
            // Status should only be cancelled or deadline exceeded.
            EXPECT_THAT(
                status.error().code,
                AnyOf(StatusCode::kCancelled, StatusCode::kDeadlineExceeded,
                      StatusCode::kUnavailable,
                      StatusCode::kFailedPrecondition));
          } else {
            successful_get_count++;
          }
          break;
        }
        case 4: {  // WaitForAndPeek a value
          auto status = futures[current_future].WaitForAndPeek(
              std::chrono::milliseconds(sleep_millis_100_dist(consumer_gen)),
              copy_fn);
          if (!status.has_value()) {
            // Status should only be cancelled or deadline exceeded.
            EXPECT_THAT(
                status.error().code,
                AnyOf(StatusCode::kCancelled, StatusCode::kDeadlineExceeded,
                      StatusCode::kUnavailable,
                      StatusCode::kFailedPrecondition));
          }
          break;
        }
        case 5: {  // Destroy the future.
          futures[current_future] = RealtimeFuture<TypeParam>();
          break;
        }
        case 6: {  // Create a new future from the context
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          auto status_or_future = context.GetFuture();
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          if (!status_or_future.has_value()) {
            // Status should only be unavailable or already exists.
            EXPECT_THAT(
                status_or_future.error().code,
                AnyOf(StatusCode::kUnavailable, StatusCode::kAlreadyExists));

            break;
          } else {
            futures[current_future] = std::move(status_or_future.value());
          }
          break;
        }
        case 7: {  // Cancel the future
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          auto status = futures[current_future].Cancel();
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          if (!status.ok()) {
            // Status should only be resource exhausted.
            EXPECT_THAT(status.code, AnyOf(StatusCode::kResourceExhausted,
                                           StatusCode::kFailedPrecondition));
          }
          break;
        }
        case 8: {  // Check if cancelled
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          auto status = futures[current_future].IsCancelled();
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          // IsCancelled should always be okay and hold either okay, or
          // FailedPrecondition.
          if (!status.has_value()) {
            EXPECT_EQ(status.error().code, StatusCode::kFailedPrecondition);
          }
          break;
        }
        case 9: {  // Check if has value
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          auto status = futures[current_future].HasValue();
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          // HasValue should always be okay, FailedPrecondition, or
          // Cancelled.
          if (!status.has_value()) {
            EXPECT_THAT(
                status.error().code,
                AnyOf(StatusCode::kCancelled, StatusCode::kFailedPrecondition));
          }
          break;
        }
        case 10: {  // Get a promise from the future.
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          auto status = futures[current_future].GetPromise();
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          if (!status.has_value()) {
            // Status should only be unavailable or already exists.
            EXPECT_THAT(
                status.error().code,
                AnyOf(StatusCode::kUnavailable, StatusCode::kAlreadyExists,
                      StatusCode::kFailedPrecondition));
          }
          break;
        }
        case 11: {  // Check if wait free destructible
          IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
          bool is_wait_free_destructible =
              futures[current_future].IsWaitFreeDestructible();
          IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
          // Could be true or false, we don't care which.
          EXPECT_THAT(is_wait_free_destructible, AnyOf(true, false));
          break;
        }
      };
      // Sleep for a random, short amount of time to yield.
      std::this_thread::sleep_for(
          std::chrono::microseconds(sleep_millis_100_dist(consumer_gen)));
    }
  };

  std::jthread producer_thread(producer_task);
  std::jthread consumer_thread(consumer_task);

  producer_thread_started.wait();
  consumer_thread_started.wait();

  start_tasks.count_down();

  std::mt19937 main_gen(42);  // Fixed seed for determinism.
  // For the context we have a total of 5 actions, including a no-op.
  std::uniform_int_distribution<> context_action_dist(0, 4);
  std::uniform_int_distribution<> sleep_millis_100_dist(1, 100);

  for (int i = 0; i < kNumIterations; ++i) {
    switch (context_action_dist(main_gen)) {
      default:
        [[fallthrough]];
      case 0: {  // No-op
        break;
      }
      case 1: {  // Reset
        auto status = context.Reset(
            std::chrono::milliseconds(sleep_millis_100_dist(main_gen)));
        if (!status.ok()) {
          // Status should only be deadline exceeded..
          EXPECT_EQ(status.code, StatusCode::kDeadlineExceeded);
        }
        break;
      }
      case 2: {  // Cancel
        IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
        auto status = context.Cancel();
        IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
        if (!status.ok()) {
          // Status should only be resource exhausted.
          EXPECT_EQ(status.code, StatusCode::kResourceExhausted);
        }
        break;
      }
      case 3: {  // Check if cancelled
        IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
        bool is_cancelled = context.IsCancelled();
        IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
        // IsCancelled should always hold either true or false.
        EXPECT_THAT(is_cancelled, AnyOf(true, false));
        break;
      }
      case 4: {  // Check if has value
        IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
        auto status = context.HasValue();
        IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
        // HasValue should always be okay and hold either okay, or be
        // cancelled.
        if (!status.has_value()) {
          EXPECT_EQ(status.error().code, StatusCode::kCancelled);
        }
        break;
      }
    }
    // Sleep for a random, short amount of time to yield.
    std::this_thread::sleep_for(
        std::chrono::microseconds(sleep_millis_100_dist(main_gen)));
  }
  producer_thread.request_stop();
  consumer_thread.request_stop();
  producer_thread.join();
  consumer_thread.join();

  std::cout << "producer_cycles: " << producer_cycles << std::endl;
  std::cout << "consumer_cycles: " << consumer_cycles << std::endl;
  std::cout << "successful_set_count: " << successful_set_count << std::endl;
  std::cout << "successful_get_count: " << successful_get_count << std::endl;
}
}  // namespace
}  // namespace intrinsic
