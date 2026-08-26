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

#include "platform/common/buffers/rt_queue_buffer.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <thread>
#include <utility>

#include "icon/utils/time.h"

namespace intrinsic {
namespace internal {
namespace {

constexpr size_t kCapacity = 10;

TEST(RtQueueBufferTest, ConstructDestruct) { RtQueueBuffer<int> queue(10); }

TEST(RtQueueBufferTest, CapacityIsCorrect) {
  RtQueueBuffer<int> queue(kCapacity);
  EXPECT_EQ(queue.Capacity(), kCapacity);
}

TEST(RtQueueBufferTest, EmptyReturnsTrueForEmptyQueue) {
  RtQueueBuffer<int> queue(kCapacity);
  EXPECT_TRUE(queue.Empty());
  EXPECT_EQ(queue.Size(), 0);
}

TEST(RtQueueBufferTest, PrepareInsertReturnsNullptrWhenFull) {
  RtQueueBuffer<int> queue(1);
  EXPECT_NE(queue.PrepareInsert(), nullptr);
  queue.FinishInsert();
  EXPECT_EQ(queue.PrepareInsert(), nullptr);
}

TEST(RtQueueBufferTest, SizeIsCorrectAfterInsertAndRemove) {
  RtQueueBuffer<int> queue(kCapacity);
  for (int i = 0; i < kCapacity; ++i) {
    std::ignore = queue.PrepareInsert();
    queue.FinishInsert();
    EXPECT_EQ(queue.Size(), i + 1);
  }

  for (int i = kCapacity; i > 0; --i) {
    std::ignore = queue.Front();
    queue.DropFront();
    EXPECT_EQ(queue.Size(), i - 1);
  }
}

TEST(RtQueueBufferTest, FullReportsFullWhenCapacityReached) {
  RtQueueBuffer<int> queue(2);
  EXPECT_NE(queue.PrepareInsert(), nullptr);
  queue.FinishInsert();
  EXPECT_NE(queue.PrepareInsert(), nullptr);
  queue.FinishInsert();
  EXPECT_TRUE(queue.Full());
}

TEST(RtQueueBufferTest, KeepFrontMaintainsFrontOfQueue) {
  RtQueueBuffer<int> queue(2);
  constexpr int kExpectedResult = 2;
  {
    int* item = queue.PrepareInsert();
    *item = kExpectedResult;
    queue.FinishInsert();
  }

  {
    int* item = queue.PrepareInsert();
    *item = kExpectedResult + 1;  // some different value
    queue.FinishInsert();
  }

  int* result = queue.Front();
  EXPECT_EQ(*result, kExpectedResult);
  queue.KeepFront();
  result = queue.Front();
  EXPECT_EQ(*result, kExpectedResult);
}

TEST(RtQueueBufferTest, DropFrontMovesFrontToNextValue) {
  RtQueueBuffer<int> queue(2);
  constexpr int kExpectedResult1 = 1;
  constexpr int kExpectedResult2 = 2;
  {
    int* item = queue.PrepareInsert();
    *item = kExpectedResult1;
    queue.FinishInsert();
  }

  {
    int* item = queue.PrepareInsert();
    *item = kExpectedResult2;
    queue.FinishInsert();
  }

  int* result = queue.Front();
  EXPECT_EQ(*result, kExpectedResult1);
  queue.DropFront();
  result = queue.Front();
  EXPECT_EQ(*result, kExpectedResult2);
}

TEST(RtQueueBufferTest, InitElementsInitializesPreparedElements) {
  RtQueueBuffer<int> queue(kCapacity);
  int n = 0;
  queue.InitElements([&n](int* item) { *item = n++; });
  for (int count = 0; count < queue.Capacity(); ++count) {
    int* item = queue.PrepareInsert();
    EXPECT_EQ(*item, count);
    queue.FinishInsert();
  }
}

TEST(RtQueueBufferTest, ConstructWithInitInitializesPreparedElements) {
  int n = 0;
  RtQueueBuffer<int> queue(kCapacity, [&n](int* item) { *item = n++; });
  for (int count = 0; count < queue.Capacity(); ++count) {
    int* item = queue.PrepareInsert();
    EXPECT_EQ(*item, count);
    queue.FinishInsert();
  }
}

TEST(RtQueueBufferTest, ThreadSafe) {
  RtQueueBuffer<int> queue(kCapacity);
  std::jthread insert_thread([&]() {
    for (int i = 0; i < kCapacity; ++i) {
      // Cover Size() in thread-safety analysis.
      std::ignore = queue.Size();
      int* item = queue.PrepareInsert();
      *item = i;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      queue.FinishInsert();
    }
  });
  for (int i = 0; i < kCapacity; ++i) {
    std::ignore = queue.Size();
    int* front;
    while ((front = queue.Front()) == nullptr) {
      std::this_thread::yield();
    }
    queue.DropFront();
    EXPECT_EQ(*front, i);
  }
}

TEST(RtQueueBufferTest, DoublePrepareInsertAborts) {
  RtQueueBuffer<int> queue(kCapacity);
  ASSERT_NE(queue.PrepareInsert(), nullptr);
  EXPECT_DEATH(std::ignore = queue.PrepareInsert(),
               "FinishInsert must be called");
}

TEST(RtQueueBufferTest, FinishInsertWithoutPrepareInsertAborts) {
  RtQueueBuffer<int> queue(kCapacity);
  EXPECT_DEATH(queue.FinishInsert(), "PrepareInsert must be called");
}

TEST(RtQueueBufferTest, DoubleFrontAborts) {
  RtQueueBuffer<int> queue(kCapacity);
  int* item = queue.PrepareInsert();
  ASSERT_NE(item, nullptr);
  *item = 42;
  queue.FinishInsert();
  ASSERT_NE(queue.Front(), nullptr);
  EXPECT_DEATH(std::ignore = queue.Front(),
               "KeepFront or DropFront must be called");
}

TEST(RtQueueBufferTest, KeepFrontWithoutFrontAborts) {
  RtQueueBuffer<int> queue(kCapacity);
  EXPECT_DEATH(queue.KeepFront(), "Front must be called");
}

TEST(RtQueueBufferTest, DropFrontWithoutFrontAborts) {
  RtQueueBuffer<int> queue(kCapacity);
  EXPECT_DEATH(queue.DropFront(), "Front must be called");
}

}  // namespace
}  // namespace internal
}  // namespace intrinsic
