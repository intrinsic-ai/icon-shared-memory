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

#include "icon/utils/status_and_expected_macros.h"

#include "gtest/gtest.h"
#include "icon/utils/status.h"

namespace intrinsic {
Status ErrorIfNegative(int x) {
  if (x < 0) {
    return FormatStatus(StatusCode::kOutOfRange, "Input '{}' is negative", x);
  }
  return OkStatus();
}

RealtimeStatus RtErrorIfNegative(int x) {
  if (x < 0) {
    return FormatRealtimeStatus(StatusCode::kOutOfRange,
                                "Input '{}' is negative", x);
  }
  return RtOkStatus();
}

tl::expected<int, Status> Halve(int x) {
  if (x % 2 != 0) {
    return tl::unexpected(FormatStatus(StatusCode::kInvalidArgument,
                                       "Input '{}' is odd, cannot halve", x));
  }
  return x / 2;
}

tl::expected<int, RealtimeStatus> RtHalve(int x) {
  if (x % 2 != 0) {
    return tl::unexpected(FormatRealtimeStatus(
        StatusCode::kInvalidArgument, "Input '{}' is odd, cannot halve", x));
  }
  return x / 2;
}

TEST(StatusMacros, ReturnStatusIfError) {
  auto double_unless_negative = [](int& x) -> Status {
    INTR_RETURN_STATUS_IF_ERROR(ErrorIfNegative(x));
    x *= 2;
    return OkStatus();
  };

  {
    int x = 21;
    Status non_negative_status = double_unless_negative(x);
    EXPECT_TRUE(non_negative_status.ok()) << ToString(non_negative_status);
    EXPECT_EQ(x, 42);
  }
  {
    int x = -1;
    Status negative_status = double_unless_negative(x);
    EXPECT_EQ(negative_status.code, StatusCode::kOutOfRange);
    EXPECT_EQ(negative_status.message, "Input '-1' is negative");
  }
}

TEST(StatusMacros, ReturnRealtimeStatusIfError) {
  auto double_unless_negative = [](int& x) -> RealtimeStatus {
    INTR_RETURN_STATUS_IF_ERROR(RtErrorIfNegative(x));
    x *= 2;
    return RtOkStatus();
  };

  {
    int x = 21;
    RealtimeStatus non_negative_status = double_unless_negative(x);
    EXPECT_TRUE(non_negative_status.ok()) << ToString(non_negative_status);
    EXPECT_EQ(x, 42);
  }
  {
    int x = -1;
    RealtimeStatus negative_status = double_unless_negative(x);
    EXPECT_EQ(negative_status.code, StatusCode::kOutOfRange);
    EXPECT_EQ(negative_status.GetMessage(), "Input '-1' is negative");
  }
}

TEST(StatusMacros, ReturnUnexpectedStatusIfError) {
  auto double_unless_negative = [](int x) -> tl::expected<int, Status> {
    INTR_RETURN_UNEXPECTED_IF_ERROR(ErrorIfNegative(x));
    return x * 2;
  };

  {
    int x = 21;
    auto non_negative_result = double_unless_negative(x);
    ASSERT_TRUE(non_negative_result.has_value())
        << ToString(non_negative_result.error());
    EXPECT_EQ(non_negative_result.value(), 42);
  }
  {
    int x = -1;
    auto negative_result = double_unless_negative(x);
    ASSERT_FALSE(negative_result.has_value());
    EXPECT_EQ(negative_result.error().code, StatusCode::kOutOfRange);
    EXPECT_EQ(negative_result.error().message, "Input '-1' is negative");
  }
}

TEST(StatusMacros, ReturnUnexpectedRealtimeStatusIfError) {
  auto double_unless_negative = [](int x) -> tl::expected<int, RealtimeStatus> {
    INTR_RETURN_UNEXPECTED_IF_ERROR(RtErrorIfNegative(x));
    return x * 2;
  };

  {
    int x = 21;
    auto non_negative_result = double_unless_negative(x);
    ASSERT_TRUE(non_negative_result.has_value())
        << ToString(non_negative_result.error());
    EXPECT_EQ(non_negative_result.value(), 42);
  }
  {
    int x = -1;
    auto negative_result = double_unless_negative(x);
    ASSERT_FALSE(negative_result.has_value());
    EXPECT_EQ(negative_result.error().code, StatusCode::kOutOfRange);
    EXPECT_EQ(negative_result.error().GetMessage(), "Input '-1' is negative");
  }
}

TEST(StatusMacros, AssignOrReturnStatus) {
  auto halve_then_add = [](int& x, int y) -> Status {
    INTR_ASSIGN_OR_RETURN_STATUS(int x_half, Halve(x));
    x = x_half + y;
    return OkStatus();
  };

  {
    int x = 2;
    int y = 5;
    auto even_status = halve_then_add(x, y);
    EXPECT_TRUE(even_status.ok()) << ToString(even_status);
    // (2 / 2) + 5 = 6
    EXPECT_EQ(x, 6);
  }
  {
    int x = 3;
    int y = 5;
    auto odd_status = halve_then_add(x, y);
    EXPECT_EQ(odd_status.code, StatusCode::kInvalidArgument);
    EXPECT_EQ(odd_status.message, "Input '3' is odd, cannot halve");
  }
}

TEST(StatusMacros, AssignOrReturnRealtimeStatus) {
  auto halve_then_add = [](int& x, int y) -> RealtimeStatus {
    INTR_ASSIGN_OR_RETURN_STATUS(int x_half, RtHalve(x));
    x = x_half + y;
    return RtOkStatus();
  };

  {
    int x = 2;
    int y = 5;
    auto even_status = halve_then_add(x, y);
    EXPECT_TRUE(even_status.ok()) << ToString(even_status);
    // (2 / 2) + 5 = 6
    EXPECT_EQ(x, 6);
  }
  {
    int x = 3;
    int y = 5;
    auto odd_status = halve_then_add(x, y);
    EXPECT_EQ(odd_status.code, StatusCode::kInvalidArgument);
    EXPECT_EQ(odd_status.GetMessage(), "Input '3' is odd, cannot halve");
  }
}

TEST(StatusMacros, AssignOrReturnUnexpectedStatus) {
  auto halve_then_add = [](int x, int y) -> tl::expected<int, Status> {
    INTR_ASSIGN_OR_RETURN_UNEXPECTED(int x_half, Halve(x));
    return x_half + y;
  };

  {
    int x = 2;
    int y = 5;
    auto even_result = halve_then_add(x, y);
    ASSERT_TRUE(even_result.has_value()) << ToString(even_result.error());
    // (2 / 2) + 5 = 6
    EXPECT_EQ(even_result.value(), 6);
  }
  {
    int x = 3;
    int y = 5;
    auto odd_result = halve_then_add(x, y);
    ASSERT_FALSE(odd_result.has_value());
    EXPECT_EQ(odd_result.error().code, StatusCode::kInvalidArgument);
    EXPECT_EQ(odd_result.error().message, "Input '3' is odd, cannot halve");
  }
}

TEST(StatusMacros, AssignOrReturnUnexpectedRealtimeStatus) {
  auto halve_then_add = [](int x, int y) -> tl::expected<int, RealtimeStatus> {
    INTR_ASSIGN_OR_RETURN_UNEXPECTED(int x_half, RtHalve(x));
    return x_half + y;
  };

  {
    int x = 2;
    int y = 5;
    auto even_result = halve_then_add(x, y);
    ASSERT_TRUE(even_result.has_value()) << ToString(even_result.error());
    // (2 / 2) + 5 = 6
    EXPECT_EQ(even_result.value(), 6);
  }
  {
    int x = 3;
    int y = 5;
    auto odd_result = halve_then_add(x, y);
    ASSERT_FALSE(odd_result.has_value());
    EXPECT_EQ(odd_result.error().code, StatusCode::kInvalidArgument);
    EXPECT_EQ(odd_result.error().GetMessage(),
              "Input '3' is odd, cannot halve");
  }
}

}  // namespace intrinsic
