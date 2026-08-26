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

#include "icon/hal/interfaces/control_period_utils.h"

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "flatbuffers/buffer.h"
#include "flatbuffers/detached_buffer.h"
#include "gmock/gmock.h"
#include "flatbuffer_definitions/icon/hal/interfaces/control_period.fbs.h"
#include "gtest/gtest.h"
#include "icon/hal/hardware_interface_handle.h"
#include "icon/interprocess/shared_memory_manager/memory_segment.h"
#include "icon/interprocess/shared_memory_manager/shared_memory_manager.h"
#include "icon/interprocess/shared_memory_manager/testing/unique_segment_name.h"
#include "icon/utils/mock_log_sink.h"
#include "icon/utils/status.h"
#include "icon/utils/status_and_expected_test_macros.h"
#include "icon/utils/status_matchers.h"
#include "icon/utils/time.h"

namespace intrinsic_fbs {
namespace {

using ::intrinsic::testing::StatusIs;
using ::testing::AllOf;
using ::testing::HasSubstr;

TEST(ControlPeriodUtilsTest, BuildControlPeriod) {
  flatbuffers::DetachedBuffer buffer = BuildControlPeriod();
  const auto* config = flatbuffers::GetRoot<ControlPeriod>(buffer.data());
  EXPECT_EQ(config->control_period_ns(), 0);
}

TEST(ControlPeriodUtilsTest, UpdateControlPeriodWritesValidValue) {
  flatbuffers::DetachedBuffer buffer = BuildControlPeriod();

  ::intrinsic::log::MockLogSink mock_log_sink;
  auto logger = std::make_unique<::intrinsic::log::Logger>(
      ::intrinsic::log::Logger::Severity::kDebug, mock_log_sink);

  INTR_ASSERT_OK_AND_ASSIGN(auto shared_memory_manager,
                            ::intrinsic::icon::SharedMemoryManager::Create(
                                ::intrinsic::icon::UniqueMemoryNamespace(),
                                "my_test_module", logger.get()));

  INTR_ASSERT_OK(shared_memory_manager->AddSegment("control_period", false,
                                                   buffer.size()));

  INTR_ASSERT_OK_AND_ASSIGN(
      auto segment,
      (shared_memory_manager->Get<::intrinsic::icon::ReadWriteMemorySegment<
           intrinsic_fbs::ControlPeriod>>("control_period", logger.get())));

  ::intrinsic::icon::MutableHardwareInterfaceHandle<
      intrinsic_fbs::ControlPeriod>
      handle(std::move(segment));

  EXPECT_EQ(handle.NumUpdates(), 0);
  EXPECT_EQ(handle.LastUpdatedTime(), ::intrinsic::Time{});
  EXPECT_EQ(handle->control_period_ns(), 0);

  INTR_EXPECT_OK(::intrinsic::icon::UpdateControlPeriod(
      handle, std::chrono::milliseconds(1), logger.get()));
  EXPECT_EQ(handle->control_period_ns(), 1000000);
  EXPECT_EQ(handle.NumUpdates(), 1);
  EXPECT_NE(handle.LastUpdatedTime(), ::intrinsic::Time{});
}

TEST(ControlPeriodUtilsTest,
     UpdateControlPeriodFailsWithFailedPreconditionOnNonPositivePeriod) {
  flatbuffers::DetachedBuffer buffer = BuildControlPeriod();

  ::intrinsic::log::MockLogSink mock_log_sink;
  auto logger = std::make_unique<::intrinsic::log::Logger>(
      ::intrinsic::log::Logger::Severity::kDebug, mock_log_sink);

  INTR_ASSERT_OK_AND_ASSIGN(auto shared_memory_manager,
                            ::intrinsic::icon::SharedMemoryManager::Create(
                                ::intrinsic::icon::UniqueMemoryNamespace(),
                                "my_test_module", logger.get()));

  INTR_ASSERT_OK(shared_memory_manager->AddSegment("control_period", false,
                                                   buffer.size()));

  INTR_ASSERT_OK_AND_ASSIGN(
      auto segment,
      (shared_memory_manager->Get<::intrinsic::icon::ReadWriteMemorySegment<
           intrinsic_fbs::ControlPeriod>>("control_period", logger.get())));

  ::intrinsic::icon::MutableHardwareInterfaceHandle<
      intrinsic_fbs::ControlPeriod>
      handle(std::move(segment));

  EXPECT_THAT(::intrinsic::icon::UpdateControlPeriod(
                  handle, std::chrono::nanoseconds(0), logger.get()),
              StatusIs(::intrinsic::StatusCode::kFailedPrecondition,
                       HasSubstr("Control period must be > 0, got 0 ns")));
  EXPECT_THAT(::intrinsic::icon::UpdateControlPeriod(
                  handle, std::chrono::nanoseconds(-1), logger.get()),
              StatusIs(::intrinsic::StatusCode::kFailedPrecondition,
                       HasSubstr("Control period must be > 0, got -1 ns")));
}

TEST(ControlPeriodUtilsTest, FormatMismatchError) {
  const std::string error = FormatControlPeriodMismatchError(
      "my_module", std::chrono::milliseconds(10),
      std::chrono::milliseconds(20));
  EXPECT_THAT(
      error, AllOf(HasSubstr("10000000 ns (100.0 Hz)"),
                   HasSubstr("20000000 ns (50.0 Hz)"), HasSubstr("my_module")));
}

TEST(ControlPeriodUtilsTest, FormatMismatchErrorZeroIsNaN) {
  const std::string error = FormatControlPeriodMismatchError(
      "my_module", std::chrono::milliseconds(10), std::chrono::nanoseconds(0));
  EXPECT_THAT(error, AllOf(HasSubstr("10000000 ns (100.0 Hz)"),
                           HasSubstr("0 ns (nan Hz)"), HasSubstr("my_module")));
}

TEST(ControlPeriodUtilsTest, FormatMismatchErrorNegativeIsNaN) {
  const std::string error = FormatControlPeriodMismatchError(
      "my_module", std::chrono::milliseconds(10),
      std::chrono::nanoseconds(-10000000));
  EXPECT_THAT(
      error, AllOf(HasSubstr("10000000 ns (100.0 Hz)"),
                   HasSubstr("-10000000 ns (nan Hz)"), HasSubstr("my_module")));
}

TEST(ControlPeriodUtilsTest, FormatMismatchErrorExpectedZeroIsNaN) {
  const std::string error = FormatControlPeriodMismatchError(
      "my_module", std::chrono::nanoseconds(0), std::chrono::milliseconds(20));

  EXPECT_THAT(
      error, AllOf(HasSubstr("0 ns (nan Hz)"),
                   HasSubstr("20000000 ns (50.0 Hz)"), HasSubstr("my_module")));
}

TEST(ControlPeriodUtilsTest, FormatMismatchErrorExpectedNegativeIsNaN) {
  const std::string error = FormatControlPeriodMismatchError(
      "my_module", std::chrono::nanoseconds(-10000000),
      std::chrono::milliseconds(20));

  EXPECT_THAT(
      error, AllOf(HasSubstr("-10000000 ns (nan Hz)"),
                   HasSubstr("20000000 ns (50.0 Hz)"), HasSubstr("my_module")));
}

}  // namespace
}  // namespace intrinsic_fbs
