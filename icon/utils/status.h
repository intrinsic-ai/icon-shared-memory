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

#ifndef ICON_UTILS_STATUS_H_
#define ICON_UTILS_STATUS_H_

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "icon/testing/realtime_annotations.h"
#include "icon/utils/attributes.h"
#include "icon/utils/realtime_guard.h"

namespace intrinsic {

// Same status codes as Abseil, for compatibility.
enum class StatusCode : int {
  kOk = 0,
  kCancelled = 1,
  kUnknown = 2,
  kInvalidArgument = 3,
  kDeadlineExceeded = 4,
  kNotFound = 5,
  kAlreadyExists = 6,
  kPermissionDenied = 7,
  kResourceExhausted = 8,
  kFailedPrecondition = 9,
  kAborted = 10,
  kOutOfRange = 11,
  kUnimplemented = 12,
  kInternal = 13,
  kUnavailable = 14,
  kDataLoss = 15,
  kUnauthenticated = 16,
};

constexpr std::string_view StatusCodeName(StatusCode c) noexcept
    INTRINSIC_CHECK_REALTIME_SAFE {
  switch (c) {
    case StatusCode::kOk:
      return "OK";
    case StatusCode::kCancelled:
      return "CANCELLED";
    case StatusCode::kUnknown:
      return "UNKNOWN";
    case StatusCode::kInvalidArgument:
      return "INVALID_ARGUMENT";
    case StatusCode::kDeadlineExceeded:
      return "DEADLINE_EXCEEDED";
    case StatusCode::kNotFound:
      return "NOT_FOUND";
    case StatusCode::kAlreadyExists:
      return "ALREADY_EXISTS";
    case StatusCode::kPermissionDenied:
      return "PERMISSION_DENIED";
    case StatusCode::kResourceExhausted:
      return "RESOURCE_EXHAUSTED";
    case StatusCode::kFailedPrecondition:
      return "FAILED_PRECONDITION";
    case StatusCode::kAborted:
      return "ABORTED";
    case StatusCode::kOutOfRange:
      return "OUT_OF_RANGE";
    case StatusCode::kUnimplemented:
      return "UNIMPLEMENTED";
    case StatusCode::kInternal:
      return "INTERNAL";
    case StatusCode::kUnavailable:
      return "UNAVAILABLE";
    case StatusCode::kDataLoss:
      return "DATA_LOSS";
    case StatusCode::kUnauthenticated:
      return "UNAUTHENTICATED";
    default:
      return "INVALID_ERROR_CODE";
  }
}

struct INTR_MUST_USE_RESULT Status {
  StatusCode code = StatusCode::kOk;
  std::string message = "";

  constexpr bool ok() const noexcept { return code == StatusCode::kOk; }
};

inline std::string ToString(const Status& s) {
  const std::string_view code_name = StatusCodeName(s.code);
  if (s.message.empty()) {
    return std::string(code_name);
  }

  std::string result;
  result.reserve(code_name.size() + 2 + s.message.size());
  result.append(code_name).append(": ").append(s.message);
  return result;
}

// Allows streaming `Status` to `str`.
//
// This is a template, rather than a regular function that takes `const
// std::ostream&`, to preserve the type of `str`.
//
// Otherwise, it would not be possible to, for instance, use `Status`
// with an "inline" stringstream like so:
//
// ```c++
// Status s = DoNonRealtimeThing();
// if (!s.ok()) {
//   std::string output = (std::stringstream()
//       << "Oh no, something went wrong! " << s).str();
// }
// ```
template <class Ostream>
inline Ostream&& operator<<(Ostream&& str, const Status& status) {
  str << ToString(status);
  return std::forward<Ostream>(str);
}

constexpr Status OkStatus() noexcept { return {}; }

struct INTR_MUST_USE_RESULT RealtimeStatus {
  // The maximum length of a `RealtimeStatus`'s message, not counting the final
  // zero terminator.
  static constexpr size_t kMaxMessageLength = 100;
  // Expect this to be zero terminated.
  using MessageType = std::array<char, kMaxMessageLength + 1>;
  StatusCode code = StatusCode::kOk;
  MessageType message{};

  constexpr bool ok() const noexcept INTRINSIC_CHECK_REALTIME_SAFE {
    return code == StatusCode::kOk;
  }

  std::string_view GetMessage() const noexcept INTRINSIC_CHECK_REALTIME_SAFE {
    // Short-circuit if we're okay.
    if (ok()) {
      return std::string_view{};
    }
    // Find terminator, if any
    return std::string_view(message.begin(),
                            std::find(message.begin(), message.end(), '\0'));
  }
};

constexpr RealtimeStatus RtOkStatus() noexcept INTRINSIC_CHECK_REALTIME_SAFE {
  return {};
}

inline Status ToStatus(const RealtimeStatus& s) {
  INTRINSIC_ASSERT_NON_REALTIME();
  return Status{
      .code = s.code,
      .message = std::string(s.GetMessage()),
  };
}

inline std::string ToString(const RealtimeStatus& s) {
  INTRINSIC_ASSERT_NON_REALTIME();
  return ToString(ToStatus(s));
}

// Allows streaming `RealtimeStatus` to `str`.
//
// This is a template, rather than a regular function that takes `const
// std::ostream&`, to preserve the type of `str`.
//
// Otherwise, it would not be possible to, for instance, use `RealtimeStatus`
// with an "inline" stringstream like so:
//
// ```c++
// RealtimeStatus s = DoRealtimeThing();
// if (!s.ok()) {
//   std::string output = (std::stringstream()
//       << "Oh no, something went wrong! " << s).str();
// }
// ```
template <class Ostream>
inline Ostream&& operator<<(Ostream&& str, const RealtimeStatus& status) {
  INTRINSIC_ASSERT_NON_REALTIME();
  str << ToString(status);
  return std::forward<Ostream>(str);
}

// Convenience methods that make it easier to construct (Realtime)Status objects
// with formatted messages.

// Returns a Status object with the given code, and a message that is equivalent
// to `std::format(format_string, args...)`.
template <class... Args>
Status FormatStatus(StatusCode code, std::format_string<Args...> format_string,
                    Args&&... args) {
  return {.code = code,
          .message = std::format(format_string, std::forward<Args>(args)...)};
}

// Returns a Status object with the given code, and a message that is equivalent
// to `std::format(format_string, args...)`.
//
// Respects the static maximum size of `RealtimeStatus::message`, and
// zero-terminates the message.
template <class... Args>
RealtimeStatus FormatRealtimeStatus(
    StatusCode code, std::format_string<Args...> format_string,
    Args&&... args) INTRINSIC_CHECK_REALTIME_SAFE {
  RealtimeStatus s;
  s.code = code;
  // Leave room for, and add, a zero terminator.
  auto result = std::format_to_n(s.message.data(), s.message.size() - 1,
                                 format_string, std::forward<Args>(args)...);
  *result.out = '\0';
  return s;
}

}  // namespace intrinsic

#endif  // ICON_UTILS_STATUS_H_
