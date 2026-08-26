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

#include "platform/common/buffers/rt_queue.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

#include "icon/testing/malloc_test.h"
#include "icon/utils/time.h"

namespace intrinsic {
namespace {

using ::testing::Eq;
using ::testing::Optional;

class RealtimeQueueTest : public ::testing::Test {
 protected:
  void SetUp() override {
    queue_ = std::make_unique<RealtimeQueue<int>>(10);
    reader_ = queue_->reader();
    writer_ = queue_->writer();
  }

  std::unique_ptr<RealtimeQueue<int>> queue_ = nullptr;
  RealtimeQueue<int>::Reader* reader_;
  RealtimeQueue<int>::Writer* writer_;
};

TEST_F(RealtimeQueueTest, FreshQueueIsEmptyNotFull) {
  EXPECT_TRUE(queue_->Empty());
  EXPECT_TRUE(reader_->Empty());
  EXPECT_FALSE(queue_->Full());
  EXPECT_EQ(queue_->Size(), 0);
}

TEST_F(RealtimeQueueTest, CanInsertCapacityNumberOfElements) {
  size_t inserted_elements = 0;
  for (size_t i = 0; i < queue_->Capacity(); ++i) {
    EXPECT_FALSE(queue_->Full());
    EXPECT_TRUE(queue_->writer()->Insert(0));
    ++inserted_elements;
    EXPECT_EQ(queue_->Size(), inserted_elements);
  }
  EXPECT_TRUE(queue_->Full());
  EXPECT_FALSE(reader_->Empty());
  EXPECT_EQ(queue_->Size(), inserted_elements);
}

TEST_F(RealtimeQueueTest, EmptyIsCorrectAfterInsertAndPop) {
  ASSERT_TRUE(writer_->Insert(5));
  EXPECT_FALSE(queue_->Empty());
  EXPECT_FALSE(reader_->Empty());
  EXPECT_EQ(queue_->Size(), 1);

  EXPECT_THAT(reader_->Pop(), Optional(5));
  EXPECT_TRUE(queue_->Empty());
  EXPECT_TRUE(reader_->Empty());
  EXPECT_EQ(queue_->Size(), 0);
}

TEST_F(RealtimeQueueTest, NonRealtimeFillAndEmpty) {
  int write_value = 0;
  int read_value = 0;
  // Completely fill and empty the queue multiple times, using the potentially
  // non-realtime-safe `Insert()` and `Pop()` methods.
  //
  // ASSERT instead of EXPECT because errors after the first one are unlikely to
  // provide more helpful information, but rather spam the test log.
  for (int loops = 0; loops < 5; ++loops) {
    while (!queue_->Full()) {
      ASSERT_TRUE(writer_->Insert(write_value++));
    }

    while (!queue_->Empty()) {
      ASSERT_THAT(reader_->Pop(), Optional(read_value++));
    }
  }
}

TEST_F(RealtimeQueueTest, RealtimeFillAndEmpty) {
  int write_value = 0;
  int read_value = 0;
  // Completely fill and empty the queue multiple times, using the realtime-safe
  // `PrepareInsert()`/`FinishInsert()` and `Front()`/`DropFront()` methods.
  //
  // ASSERT instead of EXPECT because errors after the first one are unlikely to
  // provide more helpful information, but rather spam the test log.
  for (int loops = 0; loops < 5; ++loops) {
    while (!queue_->Full()) {
      int* v = writer_->PrepareInsert();
      ASSERT_NE(v, nullptr);
      *v = write_value++;
      writer_->FinishInsert();
    }

    while (!queue_->Empty()) {
      const int* v = reader_->Front();
      ASSERT_NE(v, nullptr);
      ASSERT_EQ(*v, read_value++);
      reader_->DropFront();
    }
  }
}

TEST_F(RealtimeQueueTest, KeepFront) {
  std::ignore = writer_->Insert(5);
  const int* v1 = reader_->Front();
  ASSERT_NE(v1, nullptr);
  EXPECT_EQ(*v1, 5);
  reader_->KeepFront();
  const int* v2 = reader_->Front();
  EXPECT_EQ(v1, v2);
}

TEST_F(RealtimeQueueTest, InsertFailsWhenFull) {
  while (!queue_->Full()) {
    std::ignore = writer_->Insert(1);
  }
  EXPECT_FALSE(writer_->Insert(5));
}

TEST_F(RealtimeQueueTest, PrepareInsertReturnsNullptrWhenFull) {
  while (!queue_->Full()) {
    std::ignore = writer_->Insert(1);
  }
  EXPECT_EQ(writer_->PrepareInsert(), nullptr);
}

TEST_F(RealtimeQueueTest, PopReturnsNulloptWhenEmpty) {
  EXPECT_EQ(reader_->Pop(), std::nullopt);
}

TEST_F(RealtimeQueueTest, FrontReturnsNullptrWhenEmpty) {
  EXPECT_EQ(reader_->Front(), nullptr);
}

TEST_F(RealtimeQueueTest, InitElements) {
  int i = 0;
  queue_->InitElements([&i](int* v) { *v = i++; });
  for (int j = 0; j < queue_->Capacity(); ++j) {
    int* v = writer_->PrepareInsert();
    EXPECT_EQ(*v, j);
    writer_->FinishInsert();
  }
}

TEST_F(RealtimeQueueTest, ElementResetFunction) {
  queue_->InitElements([](int* v) { *v = 5; });
  writer_->SetElementResetFunction([](int* v) { *v = 0; });
  while (!queue_->Full()) {
    int* v = writer_->PrepareInsert();
    EXPECT_EQ(*v, 0);
    writer_->FinishInsert();
  }

  while (!queue_->Empty()) {
    EXPECT_THAT(reader_->Pop(), Optional(0));
  }
}

TEST_F(RealtimeQueueTest, CheckBufferWriteReadStressTest) {
  // Push a lot of data through the buffer using multiple threads.
  constexpr size_t kIterations = 10000;

  std::jthread writer([this](std::stop_token stop_token) {
    for (size_t i = 0; i < kIterations; i++) {
      while (!writer_->Insert(i) && !stop_token.stop_requested()) {
        // If full, hold-on and try again.
        std::this_thread::sleep_for(std::chrono::nanoseconds(10));
      }
    }
  });

  std::jthread reader([this] {
    for (size_t i = 0; i < kIterations; i++) {
      std::optional<int> optional_value;
      while ((optional_value = reader_->Pop()) == std::nullopt) {
        // If empty, hold-on and try again.
        std::this_thread::sleep_for(std::chrono::nanoseconds(10));
      }
      // Assert instead of expect because 10000 messages in the test log are
      // unlikely to be more helpful than one.
      ASSERT_THAT(optional_value, Optional(i));
    }
  });

  // Only wait for the reader thread. The writer thread cleans up automatically
  // when it's destroyed.
  reader.join();

  ASSERT_TRUE(queue_->Empty());
}

using RealtimeQueueDeathTest = RealtimeQueueTest;

TEST_F(RealtimeQueueDeathTest, MultiplePrepareInsertCalls) {
  std::ignore = writer_->PrepareInsert();
  ASSERT_DEATH(std::ignore = writer_->PrepareInsert(),
               "FinishInsert must be called .*");
}

TEST_F(RealtimeQueueDeathTest, FinishInsertWithoutPrepareInsert) {
  ASSERT_DEATH(writer_->FinishInsert(), "PrepareInsert must be called .*");
}

TEST_F(RealtimeQueueDeathTest, MultipleFrontCalls) {
  ASSERT_TRUE(writer_->Insert(5));
  std::ignore = reader_->Front();
  ASSERT_DEATH(std::ignore = reader_->Front(),
               "KeepFront or DropFront must be called .*");
}

TEST_F(RealtimeQueueDeathTest, KeepFrontWithoutFront) {
  ASSERT_DEATH(reader_->KeepFront(), "Front must be called .*");
}

TEST_F(RealtimeQueueDeathTest, DropFrontWithoutFront) {
  ASSERT_DEATH(reader_->DropFront(), "Front must be called .*");
}

using ::testing::ElementsAre;
using ::testing::Optional;

TEST(RealtimeQueueMallocTest, InsertDoesNotMallocForTriviallyCopyableT) {
  RealtimeQueue<int> queue(10);

  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  const bool insert_result = queue.writer()->Insert(42);
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  EXPECT_TRUE(insert_result);
}

TEST(RealtimeQueueMallocTest, PopDoesNotMallocForTriviallyCopyableT) {
  RealtimeQueue<int> queue(10);

  ASSERT_TRUE(queue.writer()->Insert(42));
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  const auto value = queue.reader()->Pop();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();

  EXPECT_THAT(value, Optional(42));
}

TEST(RealtimeQueueMallocTest, PrepareInsertDoesNotMallocForTriviallyCopyableT) {
  RealtimeQueue<int> queue(10);

  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  auto* queue_head = queue.writer()->PrepareInsert();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  ASSERT_NE(queue_head, nullptr);
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  *queue_head = 42;
  queue.writer()->FinishInsert();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
}

TEST(RealtimeQueueMallocTest, FrontDoesNotMallocForTriviallyCopyableT) {
  RealtimeQueue<int> queue(10);
  ASSERT_TRUE(queue.writer()->Insert(42));

  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  auto* queue_head = queue.reader()->Front();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  ASSERT_NE(queue_head, nullptr);
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  const int value = *queue_head;
  queue.reader()->DropFront();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();

  EXPECT_EQ(value, 42);
}

TEST(RealtimeQueueMallocTest, InsertCausesMallocForNonTriviallyCopyableT) {
  RealtimeQueue<std::vector<int>> queue(10);

  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  const bool insert_result = queue.writer()->Insert({42});
  IF_INTRINSIC_MALLOC_TEST_EXPECT_ALLOCATIONS_EQ(2);

  EXPECT_TRUE(insert_result);
}

TEST(RealtimeQueueMallocTest, PopCausesMallocForNonTriviallyCopyableT) {
  RealtimeQueue<std::vector<int>> queue(10);

  ASSERT_TRUE(queue.writer()->Insert({42}));
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  const auto value = queue.reader()->Pop();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_ALLOCATIONS_EQ(1);

  EXPECT_THAT(value, Optional(ElementsAre(42)));
}

TEST(RealtimeQueueMallocTest,
     PrepareInsertAndMoveDoesNotMallocForNonTriviallyCopyableT) {
  RealtimeQueue<std::vector<int>> queue(10);

  std::vector<int> value{42};
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  auto* queue_head = queue.writer()->PrepareInsert();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  ASSERT_NE(queue_head, nullptr);
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  *queue_head = std::move(value);
  queue.writer()->FinishInsert();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
}

TEST_F(RealtimeQueueTest, MoveFront) {
  std::ignore = writer_->Insert(5);
  {
    const std::optional<int> v1 = reader_->MoveFront();
    EXPECT_THAT(v1, Optional(Eq(5)));
  }

  reader_->KeepFront();
  // Primitive types are not moved.
  const int* v2 = reader_->Front();
  EXPECT_EQ(*v2, 5);
}

TEST_F(RealtimeQueueTest, MoveFrontWithEmptyQueue) {
  const std::optional<int> v1 = reader_->MoveFront();
  EXPECT_EQ(v1, std::nullopt);
}

TEST(RealtimeQueueMallocTest,
     FrontAndMoveDoesNotMallocForNonTriviallyCopyableT) {
  RealtimeQueue<std::vector<int>> queue(10);
  ASSERT_TRUE(queue.writer()->Insert({42}));

  std::optional<std::vector<int>> value;
  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  value = queue.reader()->MoveFront();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  EXPECT_THAT(value, Optional(ElementsAre(42)));

  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  queue.reader()->DropFront();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  EXPECT_TRUE(queue.reader()->Empty());

  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  const auto* queue_head = queue.reader()->Front();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  ASSERT_EQ(queue_head, nullptr);
}

TEST(RealtimeQueueMallocTest,
     MoveFrontAndKeepFrontDoesNotMallocForNonTriviallyCopyableT) {
  RealtimeQueue<std::vector<int>> queue(10);
  ASSERT_TRUE(queue.writer()->Insert({42}));

  {
    std::vector<int> value;
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    value = *queue.reader()->MoveFront();
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    EXPECT_THAT(value, ElementsAre(42));
  }

  IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
  queue.reader()->KeepFront();
  IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();

  {
    // Non trivial type is moved, head is an empty vector.
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    const auto* queue_head = queue.reader()->Front();
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
    ASSERT_NE(queue_head, nullptr);
    EXPECT_TRUE(queue_head->empty());
    IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
    queue.reader()->DropFront();
    IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
  }
}

}  // namespace
}  // namespace intrinsic
