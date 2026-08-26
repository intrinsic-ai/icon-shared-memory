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

#include "icon/utils/log.h"

#include <source_location>
#include <string>
#include <string_view>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "icon/utils/mock_log_sink.h"

namespace intrinsic::log {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Field;

TEST(Logger, CallsSink) {
  MockLogSink sink;
  Logger l(Logger::Severity::kDebug, sink);
  std::source_location loc = std::source_location::current();
  std::string verb = "walk";
  l.Log(loc, Logger::Severity::kWarning, "Eyyy, I'm {:s}ing here!", verb);

  EXPECT_EQ(sink.Size(), 1);
  const auto entries = sink.GetEntries();
  const auto& e = entries.front();
  EXPECT_EQ(e.msg, "Eyyy, I'm walking here!");
  EXPECT_EQ(e.severity, Logger::Severity::kWarning);
  EXPECT_EQ(e.loc.file_name(), loc.file_name());
  EXPECT_EQ(e.loc.line(), loc.line());
  EXPECT_EQ(e.loc.column(), loc.column());
  EXPECT_EQ(e.loc.function_name(), loc.function_name());
}

TEST(Logger, LogsStringWithSize) {
  constexpr std::string_view kLongString = "lorem ipsum dolor sit amet";
  MockLogSink sink;
  Logger l(Logger::Severity::kDebug, sink);
  std::source_location loc = std::source_location::current();
  l.Log(loc, Logger::Severity::kWarning, "Just a bit of lipsum: {:.{}s}",
        kLongString, 5);

  EXPECT_EQ(sink.Size(), 1);
  const auto entries = sink.GetEntries();
  const auto& e = entries.front();
  EXPECT_EQ(e.msg, "Just a bit of lipsum: lorem");
  EXPECT_EQ(e.severity, Logger::Severity::kWarning);
  EXPECT_EQ(e.loc.file_name(), loc.file_name());
  EXPECT_EQ(e.loc.line(), loc.line());
  EXPECT_EQ(e.loc.column(), loc.column());
  EXPECT_EQ(e.loc.function_name(), loc.function_name());
}

TEST(Logger, DoesNotCallSinkIfLogEntryLevelIsLow) {
  MockLogSink sink;
  Logger l(Logger::Severity::kError, sink);
  l.Log(std::source_location::current(), Logger::Severity::kWarning,
        "You shouldn't see me");

  EXPECT_EQ(sink.Size(), 0);
}

TEST(LogMacro, WorksWithReference) {
  MockLogSink sink;
  Logger l(Logger::Severity::kDebug, sink);
  INTRINSIC_SHARED_MEMORY_LOG(DEBUG, l, "Hello Debug!");
  INTRINSIC_SHARED_MEMORY_LOG(INFO, l, "Hello Info!");
  INTRINSIC_SHARED_MEMORY_LOG(WARNING, l, "Hello Warning!");
  INTRINSIC_SHARED_MEMORY_LOG(ERROR, l, "Hello Error!");
  EXPECT_THAT(
      sink.GetEntries(),
      ElementsAre(Field("msg", &LogEntryWithStorage::msg, Eq("Hello Debug!")),
                  Field("msg", &LogEntryWithStorage::msg, Eq("Hello Info!")),
                  Field("msg", &LogEntryWithStorage::msg, Eq("Hello Warning!")),
                  Field("msg", &LogEntryWithStorage::msg, Eq("Hello Error!"))));
  // EXPECT_DEATH forks off, so the message logged in there doesn't end up in
  // `entries`.
  EXPECT_DEATH({ INTRINSIC_SHARED_MEMORY_LOG(FATAL, l, "Hello!"); }, "");
}

TEST(LogMacro, WorksWithPointer) {
  MockLogSink sink;
  Logger l(Logger::Severity::kDebug, sink);
  INTRINSIC_SHARED_MEMORY_LOG(DEBUG, &l, "Hello Debug!");
  INTRINSIC_SHARED_MEMORY_LOG(INFO, &l, "Hello Info!");
  INTRINSIC_SHARED_MEMORY_LOG(WARNING, &l, "Hello Warning!");
  INTRINSIC_SHARED_MEMORY_LOG(ERROR, &l, "Hello Error!");

  EXPECT_THAT(
      sink.GetEntries(),
      ElementsAre(Field("msg", &LogEntryWithStorage::msg, Eq("Hello Debug!")),
                  Field("msg", &LogEntryWithStorage::msg, Eq("Hello Info!")),
                  Field("msg", &LogEntryWithStorage::msg, Eq("Hello Warning!")),
                  Field("msg", &LogEntryWithStorage::msg, Eq("Hello Error!"))));
  // EXPECT_DEATH forks off, so the message logged in there doesn't end up in
  // `entries`.
  EXPECT_DEATH({ INTRINSIC_SHARED_MEMORY_LOG(FATAL, &l, "Hello!"); }, "");
}

TEST(LogMacro, SkipsLoggingWithNullptr) {
  INTRINSIC_SHARED_MEMORY_LOG(DEBUG, nullptr, "Hello!");
  INTRINSIC_SHARED_MEMORY_LOG(INFO, nullptr, "Hello!");
  INTRINSIC_SHARED_MEMORY_LOG(WARNING, nullptr, "Hello!");
  INTRINSIC_SHARED_MEMORY_LOG(ERROR, nullptr, "Hello!");
  // Even if `logger` is a nullptr, a fatal log should kill the process.
  EXPECT_DEATH({ INTRINSIC_SHARED_MEMORY_LOG(FATAL, nullptr, "Hello!"); }, "");
}

}  // namespace intrinsic::log
