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

#include "icon/utils/status_helpers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "icon/utils/status.h"

namespace intrinsic {
namespace {

TEST(OverwriteIfError, ReturnsOkIfBothAreOk) {
  RealtimeStatus realtime_status = RtOkStatus();
  realtime_status = OverwriteIfError(realtime_status, RtOkStatus());
  EXPECT_TRUE(realtime_status.ok());
}

TEST(OverwriteIfError, ReturnsPreviousErrorIfNewStatusIsOk) {
  Status status = FormatStatus(StatusCode::kInvalidArgument, "");
  status = OverwriteIfError(status, OkStatus());
  EXPECT_EQ(status.code, StatusCode::kInvalidArgument);

  RealtimeStatus realtime_status =
      FormatRealtimeStatus(StatusCode::kInvalidArgument, "");
  realtime_status = OverwriteIfError(realtime_status, RtOkStatus());
  EXPECT_EQ(realtime_status.code, StatusCode::kInvalidArgument);
}

TEST(OverwriteIfError, ReturnsNewErrorIfPreviousIsOk) {
  Status status = OkStatus();
  status =
      OverwriteIfError(status, FormatStatus(StatusCode::kInvalidArgument, ""));
  EXPECT_EQ(status.code, StatusCode::kInvalidArgument);

  RealtimeStatus realtime_status = RtOkStatus();
  realtime_status = OverwriteIfError(
      realtime_status, FormatRealtimeStatus(StatusCode::kInvalidArgument, ""));
  EXPECT_EQ(realtime_status.code, StatusCode::kInvalidArgument);
}

TEST(OverwriteIfError, ReturnsNewErrorIfPreviousIsNotOk) {
  Status status = FormatStatus(StatusCode::kUnavailable, "");
  status =
      OverwriteIfError(status, FormatStatus(StatusCode::kInvalidArgument, ""));
  EXPECT_EQ(status.code, StatusCode::kInvalidArgument);

  RealtimeStatus realtime_status =
      FormatRealtimeStatus(StatusCode::kUnavailable, "");
  realtime_status = OverwriteIfError(
      realtime_status, FormatRealtimeStatus(StatusCode::kInvalidArgument, ""));
  EXPECT_EQ(realtime_status.code, StatusCode::kInvalidArgument);
}

TEST(OverwriteIfNotError, ReturnsOkIfBothAreOk) {
  Status status = OkStatus();
  status = OverwriteIfNotError(status, OkStatus());
  EXPECT_TRUE(status.ok());

  RealtimeStatus realtime_status = RtOkStatus();
  realtime_status = OverwriteIfNotError(realtime_status, RtOkStatus());
  EXPECT_TRUE(realtime_status.ok());
}

TEST(OverwriteIfNotError, ReturnsPreviousErrorIfNewStatusIsOk) {
  Status status = FormatStatus(StatusCode::kInvalidArgument, "");
  status = OverwriteIfNotError(status, OkStatus());
  EXPECT_EQ(status.code, StatusCode::kInvalidArgument);

  RealtimeStatus realtime_status =
      FormatRealtimeStatus(StatusCode::kInvalidArgument, "");
  realtime_status = OverwriteIfNotError(realtime_status, RtOkStatus());
  EXPECT_EQ(realtime_status.code, StatusCode::kInvalidArgument);
}

TEST(OverwriteIfNotError, ReturnsNewErrorIfPreviousIsOk) {
  Status status = OkStatus();
  status = OverwriteIfNotError(status,
                               FormatStatus(StatusCode::kInvalidArgument, ""));
  EXPECT_EQ(status.code, StatusCode::kInvalidArgument);

  RealtimeStatus realtime_status = RtOkStatus();
  realtime_status = OverwriteIfNotError(
      realtime_status, FormatRealtimeStatus(StatusCode::kInvalidArgument, ""));
  EXPECT_EQ(realtime_status.code, StatusCode::kInvalidArgument);
}

TEST(OverwriteIfNotError, ReturnsPreviousErrorIfNewStatusIsNotOk) {
  Status status = FormatStatus(StatusCode::kUnavailable, "");
  status = OverwriteIfNotError(status,
                               FormatStatus(StatusCode::kInvalidArgument, ""));
  EXPECT_EQ(status.code, StatusCode::kUnavailable);

  RealtimeStatus realtime_status =
      FormatRealtimeStatus(StatusCode::kUnavailable, "");
  realtime_status = OverwriteIfNotError(
      realtime_status, FormatRealtimeStatus(StatusCode::kInvalidArgument, ""));
  EXPECT_EQ(realtime_status.code, StatusCode::kUnavailable);
}

}  // namespace
}  // namespace intrinsic
