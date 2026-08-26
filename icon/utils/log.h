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

#ifndef ICON_UTILS_LOG_H_
#define ICON_UTILS_LOG_H_

#include <algorithm>
#include <array>
#include <cstdlib>
#include <format>
#include <functional>
#include <source_location>
#include <string_view>
#include <utility>

namespace intrinsic::log {
// This class provides a `std::format`-like logging function.
//
// It requires a SinkCallback
class Logger {
 public:
  static constexpr int kMaxLogLineLength = 4096;

  enum class Severity : int {
    kNone = 0,
    kDebug = 1,
    kInfo = 2,
    kWarning = 3,
    kError = 4,
    kFatal = 5,
  };

  struct LogEntry {
    // The actual log message. This does not have a timestamp or severity level.
    // It is also not zero-terminated! Finally, it does not necessarily end with
    // a newline character, yet we should still print the next log entry on a
    // new line!
    std::string_view msg;
    std::source_location loc;
    Severity severity;
  };

  // A SinkCallback should
  // * assemble the final log line from the data in the LogEntry
  // * ensure that each output line ends with a newline character (LogEntry::msg
  //   does *not* necessarily have a newline).
  // * be thread and realtime safe
  using SinkCallback = std::function<void(const LogEntry&)>;

  // Builds a logger that sends log messages with a level <= `log_level`
  // to `sink_cb`.
  Logger(Severity log_level, SinkCallback sink_cb) noexcept
      : log_level_(log_level), sink_cb_(std::move(sink_cb)) {}

  // `std::format` style logging.
  //
  // Find information about `std::format` format specifiers here:
  // https://en.cppreference.com/cpp/utility/format/spec
  template <typename... Args>
  void Log(std::source_location loc, Severity severity,
           std::format_string<Args...> format, Args&&... args) const {
    if (severity < log_level_) {
      return;
    }
    if (!sink_cb_) {
      return;
    }
    std::array<char, kMaxLogLineLength> l;

    const auto result =
        std::format_to_n(l.data(), kMaxLogLineLength, std::move(format),
                         std::forward<Args>(args)...);
    if (result.size < 0) {
      // Possibly do something nicer here, but I'd like to avoid duplicating
      // the compile-time checks that absl::SnPrintf does...
      return;
    }
    // We can find the line length by doing pointer / iterator arithmetic –
    // `result.out` points one past the last character that `format_to_n()`
    // wrote.
    const int line_length = result.out - l.data();
    sink_cb_({.msg = std::string_view(l.data(), line_length),
              .loc = loc,
              .severity = severity});
  }

 private:
  Severity log_level_ = Severity::kNone;
  SinkCallback sink_cb_;
};

namespace internal {

// These two templates allow us to use the same macros for references and
// (nullable) pointers to a Logger.
//
// This makes it much more convenient to log from places that may or may not
// have a logger, rather than forcing callers to use if statements throughout
// their code.

template <typename... Args>
void LogWith(const Logger& logger, std::source_location loc,
             Logger::Severity severity, std::format_string<Args...> format,
             Args&&... args) {
  logger.Log(loc, severity, format, std::forward<Args>(args)...);
}

template <typename... Args>
void LogWith(const Logger* logger, std::source_location loc,
             Logger::Severity severity, std::format_string<Args...> format,
             Args&&... args) {
  if (logger == nullptr) {
    return;
  }
  logger->Log(loc, severity, format, std::forward<Args>(args)...);
}

}  // namespace internal
}  // namespace intrinsic::log

// Similar to absl logging, use token concatenation to ensure that severities
// always work without a namespace.
#define INTRINSIC_SHARED_MEMORY_LOG(SEVERITY, LOGGER, FORMAT, ...) \
  INTRINSIC_SHARED_MEMORY_LOG_##SEVERITY(LOGGER, FORMAT, __VA_ARGS__)

#define INTRINSIC_SHARED_MEMORY_LOG_DEBUG(LOGGER, FORMAT, ...) \
  ::intrinsic::log::internal::LogWith(                         \
      LOGGER, std::source_location::current(),                 \
      ::intrinsic::log::Logger::Severity::kDebug,              \
      FORMAT __VA_OPT__(, ) __VA_ARGS__)

#define INTRINSIC_SHARED_MEMORY_LOG_INFO(LOGGER, FORMAT, ...) \
  ::intrinsic::log::internal::LogWith(                        \
      LOGGER, std::source_location::current(),                \
      ::intrinsic::log::Logger::Severity::kInfo,              \
      FORMAT __VA_OPT__(, ) __VA_ARGS__)

#define INTRINSIC_SHARED_MEMORY_LOG_WARNING(LOGGER, FORMAT, ...) \
  ::intrinsic::log::internal::LogWith(                           \
      LOGGER, std::source_location::current(),                   \
      ::intrinsic::log::Logger::Severity::kWarning,              \
      FORMAT __VA_OPT__(, ) __VA_ARGS__)

#define INTRINSIC_SHARED_MEMORY_LOG_ERROR(LOGGER, FORMAT, ...) \
  ::intrinsic::log::internal::LogWith(                         \
      LOGGER, std::source_location::current(),                 \
      ::intrinsic::log::Logger::Severity::kError,              \
      FORMAT __VA_OPT__(, ) __VA_ARGS__)

// FATAL has the distinction of exiting the program (if the sink callback didn't
// already do so).
//
// NOTE: This exits **even if `logger` is nullptr**.
#define INTRINSIC_SHARED_MEMORY_LOG_FATAL(LOGGER, FORMAT, ...) \
  do {                                                         \
    ::intrinsic::log::internal::LogWith(                       \
        LOGGER, std::source_location::current(),               \
        ::intrinsic::log::Logger::Severity::kFatal,            \
        FORMAT __VA_OPT__(, ) __VA_ARGS__);                    \
    std::exit(EXIT_FAILURE);                                   \
  } while (0)
#endif  // ICON_UTILS_LOG_H_
