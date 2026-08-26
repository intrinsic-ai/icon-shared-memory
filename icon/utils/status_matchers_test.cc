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

#include "icon/utils/status_matchers.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icon/utils/status.h"
#include "tl/expected.hpp"

namespace intrinsic::testing {
namespace {

using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::Not;

TEST(StatusMatchersTest, IsOkWithStatus) {
  const Status ok_status = OkStatus();
  EXPECT_THAT(ok_status, IsOk());

  const Status err_status{.code = StatusCode::kInternal,
                          .message = "test error"};
  EXPECT_THAT(err_status, Not(IsOk()));
}

TEST(StatusMatchersTest, IsOkWithRealtimeStatus) {
  const RealtimeStatus ok_status = RtOkStatus();
  EXPECT_THAT(ok_status, IsOk());

  const RealtimeStatus err_status{.code = StatusCode::kInternal};
  EXPECT_THAT(err_status, Not(IsOk()));
}

TEST(StatusMatchersTest, IsOkWithExpected) {
  const tl::expected<int, Status> ok_val = 42;
  EXPECT_THAT(ok_val, IsOk());

  const tl::expected<int, Status> err_val =
      tl::make_unexpected(Status{.code = StatusCode::kNotFound});
  EXPECT_THAT(err_val, Not(IsOk()));
}

TEST(StatusMatchersTest, IsOkAndHolds) {
  tl::expected<int, Status> ok_val = 42;
  EXPECT_THAT(ok_val, IsOkAndHolds(Eq(42)));
  EXPECT_THAT(ok_val, IsOkAndHolds(::testing::Ge(40)));

  tl::expected<int, Status> err_val =
      tl::make_unexpected(Status{.code = StatusCode::kNotFound});
  EXPECT_THAT(err_val, Not(IsOkAndHolds(Eq(42))));
}

TEST(StatusMatchersTest, StatusIsWithStatus) {
  const Status err_status{.code = StatusCode::kInvalidArgument,
                          .message = "bad input"};
  EXPECT_THAT(err_status, StatusIs(StatusCode::kInvalidArgument));
  EXPECT_THAT(err_status,
              StatusIs(StatusCode::kInvalidArgument, HasSubstr("input")));
  EXPECT_THAT(err_status, Not(StatusIs(StatusCode::kInternal)));
}

TEST(StatusMatchersTest, StatusIsWithExpected) {
  const tl::expected<int, Status> err_val = tl::make_unexpected(
      Status{.code = StatusCode::kDeadlineExceeded, .message = "timed out"});
  EXPECT_THAT(err_val, StatusIs(StatusCode::kDeadlineExceeded));
  EXPECT_THAT(err_val,
              StatusIs(StatusCode::kDeadlineExceeded, HasSubstr("timed")));
  EXPECT_THAT(err_val, Not(StatusIs(StatusCode::kOk)));

  const tl::expected<int, Status> ok_val = 100;
  EXPECT_THAT(ok_val, Not(StatusIs(StatusCode::kDeadlineExceeded)));
}

TEST(StatusMatchersTest, StatusIsWithRealtimeStatus) {
  const RealtimeStatus err_status =
      FormatRealtimeStatus(StatusCode::kResourceExhausted, "out of memory");
  EXPECT_THAT(err_status, StatusIs(StatusCode::kResourceExhausted));
  EXPECT_THAT(err_status,
              StatusIs(StatusCode::kResourceExhausted, HasSubstr("memory")));
  EXPECT_THAT(err_status, Not(StatusIs(StatusCode::kInternal)));
}

TEST(StatusMatchersTest, StatusIsWithExpectedRealtimeStatus) {
  const tl::expected<int, RealtimeStatus> err_val = tl::make_unexpected(
      FormatRealtimeStatus(StatusCode::kCancelled, "aborted"));
  EXPECT_THAT(err_val, StatusIs(StatusCode::kCancelled));
  EXPECT_THAT(err_val, StatusIs(StatusCode::kCancelled, HasSubstr("aborted")));
  EXPECT_THAT(err_val, Not(StatusIs(StatusCode::kOk)));
}

}  // namespace
}  // namespace intrinsic::testing
