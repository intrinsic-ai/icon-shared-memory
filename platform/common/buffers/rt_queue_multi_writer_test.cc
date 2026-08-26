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

#include "platform/common/buffers/rt_queue_multi_writer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_set>

#include "icon/utils/status.h"
#include "icon/utils/status_and_expected_test_macros.h"
#include "platform/common/buffers/rt_queue.h"

namespace intrinsic {
namespace {

using ::testing::Optional;

TEST(RealtimeQueueMultiWriterTest, SingleInsert) {
  RealtimeQueue<int> queue;
  RealtimeQueueMultiWriter<int> writer(*queue.writer());

  INTR_ASSERT_OK(writer.Insert(123));
  ASSERT_THAT(queue.reader()->Pop(), Optional(123));
}

TEST(RealtimeQueueMultiWriterTest, ReportsFullQueue) {
  RealtimeQueue<int> queue(/*capacity=*/1);
  RealtimeQueueMultiWriter<int> writer(*queue.writer());

  // Fill up the one available slot in the queue.
  INTR_ASSERT_OK(writer.Insert(123));
  EXPECT_TRUE(queue.Full());

  auto insert_result = writer.Insert(456);
  EXPECT_EQ(insert_result.code, StatusCode::kResourceExhausted);
}

TEST(RealtimeQueueMultiWriterTest, ConcurrentInsert) {
  constexpr size_t kIterations = 1000;
  // Use a non-trivially-copyable type to make things a bit harder.
  // The queue has room for exactly as many elements as we're planning to write.
  // This reduces the complexity of the test code below because we know that
  // Insert() operations will always succeed.
  RealtimeQueue<std::unique_ptr<int>> queue(/*capacity=*/kIterations);
  RealtimeQueueMultiWriter<std::unique_ptr<int>> writer(*queue.writer());

  // This thread inserts even integers.
  std::jthread write_thread_1([&writer]() {
    for (int i = 0; i < kIterations; i += 2) {
      INTR_ASSERT_OK(writer.Insert(std::make_unique<int>(i)));
    }
  });
  // This thread inserts odd integers.
  std::jthread write_thread_2([&writer]() {
    for (int i = 1; i < kIterations; i += 2) {
      INTR_ASSERT_OK(writer.Insert(std::make_unique<int>(i)));
    }
  });

  write_thread_1.join();
  write_thread_2.join();

  // Now the queue should be full. Assert this because otherwise the final
  // expectation has no hope of being met.
  ASSERT_TRUE(queue.Full());

  RealtimeQueue<std::unique_ptr<int>>::Reader& reader = *queue.reader();

  // Add all received values to a set. If the size of the final set is
  // `kIterations`, we know we've received all values without duplicates or data
  // loss.
  std::unordered_set<int> received_set;
  while (!queue.Empty()) {
    std::unique_ptr<int>* front = reader.Front();
    ASSERT_NE(front, nullptr);
    received_set.insert(**front);
    reader.DropFront();
  }

  // The number of received items should add up.
  EXPECT_EQ(received_set.size(), kIterations);
  // Check that we actually received each number between 0 and `kIterations`
  for (int i = 0; i < kIterations; ++i) {
    EXPECT_TRUE(received_set.contains(i));
  }
}

}  // namespace
}  // namespace intrinsic
