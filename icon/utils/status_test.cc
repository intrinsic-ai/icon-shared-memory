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

#include "icon/utils/status.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>

#include "gtest/gtest.h"

namespace intrinsic {
namespace {

TEST(StatusTest, DefaultConstruction) {
  Status s;
  EXPECT_TRUE(s.ok());
  EXPECT_EQ(s.code, StatusCode::kOk);
  EXPECT_EQ(s.message, "");
}

TEST(StatusTest, OkStatus) {
  Status s = OkStatus();
  EXPECT_TRUE(s.ok());
  EXPECT_EQ(s.code, StatusCode::kOk);
  EXPECT_EQ(s.message, "");
}

TEST(StatusTest, NonOkStatus) {
  Status s{.code = StatusCode::kNotFound};
  EXPECT_FALSE(s.ok());
}

TEST(StatusTest, ToStringWithNoMessage) {
  EXPECT_EQ(ToString(Status{.code = StatusCode::kOk, .message = ""}), "OK");
  EXPECT_EQ(ToString(Status{.code = StatusCode::kNotFound, .message = ""}),
            "NOT_FOUND");
}

TEST(StatusTest, ToStringWithMessage) {
  EXPECT_EQ(ToString(Status{.code = StatusCode::kNotFound,
                            .message = "where did I put it?"}),
            "NOT_FOUND: where did I put it?");
}

TEST(StatusTest, StreamOperator) {
  Status s{.code = StatusCode::kInvalidArgument,
           .message =
               "expected a spherical cow in a vacuum, got a messy irregular "
               "cow in... what is this, an entire planet?"};
  std::stringstream ss;
  ss << s;
  EXPECT_EQ(ss.str(),
            "INVALID_ARGUMENT: expected a spherical cow in a vacuum, got a "
            "messy irregular cow in... what is this, an entire planet?");
}

TEST(StatusTest, FormatStatus) {
  Status s = FormatStatus(StatusCode::kInternal, "Error code: {}, message: {}",
                          404, "Not Found");
  EXPECT_EQ(s.code, StatusCode::kInternal);
  EXPECT_EQ(s.message, "Error code: 404, message: Not Found");
}

TEST(RealtimeStatusTest, DefaultConstruction) {
  RealtimeStatus s;
  EXPECT_TRUE(s.ok());
  EXPECT_EQ(s.code, StatusCode::kOk);
  EXPECT_EQ(s.GetMessage(), "");
}

TEST(RealtimeStatusTest, RtOkStatus) {
  RealtimeStatus s = RtOkStatus();
  EXPECT_TRUE(s.ok());
  EXPECT_EQ(s.code, StatusCode::kOk);
  EXPECT_EQ(s.GetMessage(), "");
}

TEST(RealtimeStatusTest, NonOk) {
  RealtimeStatus s{.code = StatusCode::kResourceExhausted};

  EXPECT_FALSE(s.ok());
}

TEST(RealtimeStatusTest, GetMessage) {
  RealtimeStatus s{.code = StatusCode::kUnknown};
  std::string_view message = "Resource needs a nap";
  std::copy(message.begin(), message.end(), s.message.begin());
  s.message[message.size()] = '\0';
  EXPECT_EQ(s.GetMessage(), "Resource needs a nap");
}

TEST(RealtimeStatusTest, ToStatus) {
  RealtimeStatus rt_status;
  rt_status.code = StatusCode::kAborted;
  std::string_view message = "Too hard, gave up";
  std::copy(message.begin(), message.end(), rt_status.message.begin());
  rt_status.message[message.size()] = '\0';

  Status s = ToStatus(rt_status);
  EXPECT_EQ(s.code, StatusCode::kAborted);
  EXPECT_EQ(s.message, "Too hard, gave up");
}

TEST(RealtimeStatusTest, ToString) {
  RealtimeStatus rt_status;
  rt_status.code = StatusCode::kDataLoss;
  std::string_view message = "I forgor :(";
  std::copy(message.begin(), message.end(), rt_status.message.begin());
  rt_status.message[message.size()] = '\0';

  EXPECT_EQ(ToString(rt_status), "DATA_LOSS: I forgor :(");
}

TEST(RealtimeStatusTest, StreamOperator) {
  RealtimeStatus rt_status;
  rt_status.code = StatusCode::kCancelled;
  std::string_view message = "we regret to inform you that milkshake duck...";
  std::copy(message.begin(), message.end(), rt_status.message.begin());
  rt_status.message[message.size()] = '\0';

  std::stringstream ss;
  ss << rt_status;
  EXPECT_EQ(ss.str(),
            "CANCELLED: we regret to inform you that milkshake duck...");
}

TEST(RealtimeStatusTest, FormatRealtimeStatus) {
  RealtimeStatus s = FormatRealtimeStatus(StatusCode::kInvalidArgument,
                                          "Index {} out of {}", 5, 2);
  EXPECT_EQ(s.code, StatusCode::kInvalidArgument);
  EXPECT_EQ(s.GetMessage(), "Index 5 out of 2");
}

TEST(RealtimeStatusTest, FormatRealtimeStatusWithExplicitSizedString) {
  constexpr std::string_view string_view_message =
      "Hello! I'm not zero-terminated!";
  RealtimeStatus s = FormatRealtimeStatus(
      StatusCode::kUnknown, "string_view says: '{:.{}s}'",
      // Only print the "Hello!" part of string_view_message.
      string_view_message.data(), 6);
  EXPECT_EQ(s.code, StatusCode::kUnknown);
  EXPECT_EQ(s.GetMessage(), "string_view says: 'Hello!'");
}

TEST(RealtimeStatusTest, FormatRealtimeStatusTruncation) {
  // Try to format a message that is longer than
  // RealtimeStatus::kMaxMessageLength to verify truncation.
  std::string long_message(RealtimeStatus::kMaxMessageLength + 1, 'a');
  RealtimeStatus s =
      FormatRealtimeStatus(StatusCode::kInternal, "{}", long_message);
  EXPECT_EQ(s.code, StatusCode::kInternal);

  std::string expected_message(RealtimeStatus::kMaxMessageLength, 'a');
  EXPECT_EQ(s.GetMessage(), expected_message);
  EXPECT_EQ(s.message[s.message.size() - 1], '\0');
}

}  // namespace
}  // namespace intrinsic
