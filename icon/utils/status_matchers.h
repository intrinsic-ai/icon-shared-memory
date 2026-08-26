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

#ifndef ICON_UTILS_STATUS_MATCHERS_H_
#define ICON_UTILS_STATUS_MATCHERS_H_

#include <ostream>
#include <string>
#include <type_traits>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icon/utils/status.h"
#include "tl/expected.hpp"

namespace intrinsic {
// Allow gtest to pretty-print Status.
inline void PrintTo(const Status& status, std::ostream* os) {
  *os << ToString(status);
}
// Allow gtest to pretty-print RealtimeStatus.
inline void PrintTo(const RealtimeStatus& rt_status, std::ostream* os) {
  *os << ToString(rt_status);
}
// Allow gtest to pretty-print StatusCode.
inline void PrintTo(const StatusCode& code, std::ostream* os) {
  *os << StatusCodeName(code);
}
// Allow gtest to pretty-print tl::expected (uses the pretty-printers for
// RealtimeStatus and Status defined above, if applicable).
template <typename T>
inline void PrintTo(const tl::expected<T, Status>& expected, std::ostream* os) {
  if (expected.has_value()) {
    *os << "tl::expected with value "
        << ::testing::PrintToString(expected.value());
  } else {
    *os << "tl::expected with error "
        << ::testing::PrintToString(expected.error());
  }
}
}  // namespace intrinsic

namespace intrinsic::testing {

namespace internal_status {

template <typename T>
struct StatusExtractor;

template <>
struct StatusExtractor<::intrinsic::Status> {
  static constexpr bool IsOk(const ::intrinsic::Status& s) { return s.ok(); }
  static constexpr ::intrinsic::StatusCode GetCode(
      const ::intrinsic::Status& s) {
    return s.code;
  }
  static std::string GetMessage(const ::intrinsic::Status& s) {
    return s.message;
  }
};

template <>
struct StatusExtractor<::intrinsic::RealtimeStatus> {
  static constexpr bool IsOk(const ::intrinsic::RealtimeStatus& s) {
    return s.ok();
  }
  static constexpr ::intrinsic::StatusCode GetCode(
      const ::intrinsic::RealtimeStatus& s) {
    return s.code;
  }
  static std::string GetMessage(const ::intrinsic::RealtimeStatus& s) {
    return std::string{s.GetMessage()};
  }
};

template <typename T, typename E>
struct StatusExtractor<tl::expected<T, E>> {
  static constexpr bool IsOk(const tl::expected<T, E>& e) {
    return e.has_value();
  }
  static constexpr ::intrinsic::StatusCode GetCode(
      const tl::expected<T, E>& e) {
    if (e.has_value()) {
      return ::intrinsic::StatusCode::kOk;
    }
    return StatusExtractor<E>::GetCode(e.error());
  }
  static std::string GetMessage(const tl::expected<T, E>& e) {
    if (e.has_value()) {
      return "";
    }
    return StatusExtractor<E>::GetMessage(e.error());
  }
};

}  // namespace internal_status

// Matches a `tl::expected<T, E>` that has a value, and verifies that the held
// value matches `inner_matcher`.
//
// Example:
// ```cpp
// tl::expected<int, Status> result = CalculateValue();
// EXPECT_THAT(result, IsOkAndHolds(Eq(42)));
// EXPECT_THAT(result, IsOkAndHolds(Ge(40)));
// ```
MATCHER_P(IsOkAndHolds, inner_matcher, "") {
  if (!arg.has_value()) {
    *result_listener << "is unexpected status: "
                     << ::intrinsic::ToString(arg.error());
    return false;
  }
  return ::testing::ExplainMatchResult(inner_matcher, arg.value(),
                                       result_listener);
}

// Matches any `intrinsic::Status`, `intrinsic::RealtimeStatus`, or
// `tl::expected<T, E>` that represents a successful / OK state.
//
// Example:
// ```cpp
// Status status = DoWork();
// EXPECT_THAT(status, IsOk());
//
// tl::expected<MyType, Status> result = CreateObject();
// EXPECT_THAT(result, IsOk());
// ```
MATCHER(IsOk, "") {
  using CleanT = std::remove_cvref_t<decltype(arg)>;
  if constexpr (requires {
                  internal_status::StatusExtractor<CleanT>::IsOk(arg);
                }) {
    if (!internal_status::StatusExtractor<CleanT>::IsOk(arg)) {
      *result_listener
          << "is error: "
          << StatusCodeName(
                 internal_status::StatusExtractor<CleanT>::GetCode(arg))
          << " " << internal_status::StatusExtractor<CleanT>::GetMessage(arg);
      return false;
    }
    return true;
  } else {
    static_assert(sizeof(arg) == 0, "Unsupported type for IsOk matcher");
  }
}

// Matches an `intrinsic::Status`, `intrinsic::RealtimeStatus`, or
// `tl::expected<T, E>` whose status code matches `code_matcher` and whose
// message matches `message_matcher`.
//
// Example:
// ```cpp
// Status status = ProcessInput("");
// EXPECT_THAT(status, StatusIs(StatusCode::kInvalidArgument,
// HasSubstr("empty")));
//
// tl::expected<int, Status> val = GetValue(-1);
// EXPECT_THAT(val, StatusIs(StatusCode::kInvalidArgument, Eq("Negative
// index")));
// ```
MATCHER_P2(StatusIs, code_matcher, message_matcher,
           "has a status code that " +
               ::testing::DescribeMatcher<::intrinsic::StatusCode>(code_matcher,
                                                                   negation) +
               (negation ? " or " : " and ") + "has an error message that " +
               ::testing::DescribeMatcher<const std::string&>(message_matcher,
                                                              negation)) {
  using CleanT = std::remove_cvref_t<decltype(arg)>;
  const auto code = internal_status::StatusExtractor<CleanT>::GetCode(arg);
  const auto message =
      internal_status::StatusExtractor<CleanT>::GetMessage(arg);
  return ::testing::ExplainMatchResult(code_matcher, code, result_listener) &&
         ::testing::ExplainMatchResult(message_matcher, message,
                                       result_listener);
}

// Matches an `intrinsic::Status`, `intrinsic::RealtimeStatus`, or
// `tl::expected<T, E>` whose status code matches `code_matcher`.
//
// Example:
// ```cpp
// Status status = ConnectToServer();
// EXPECT_THAT(status, StatusIs(StatusCode::kUnavailable));
// ```
MATCHER_P(StatusIs, code_matcher, "") {
  return ::testing::ExplainMatchResult(StatusIs(code_matcher, ::testing::_),
                                       arg, result_listener);
}

}  // namespace intrinsic::testing

namespace intrinsic::icon {
using ::intrinsic::testing::IsOk;
using ::intrinsic::testing::IsOkAndHolds;
using ::intrinsic::testing::StatusIs;
}  // namespace intrinsic::icon

#endif  // ICON_UTILS_STATUS_MATCHERS_H_
