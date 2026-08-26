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

#ifndef ICON_UTILS_MOCK_LOG_SINK_H_
#define ICON_UTILS_MOCK_LOG_SINK_H_

#include <iostream>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "icon/testing/realtime_annotations.h"
#include "icon/utils/attributes.h"
#include "icon/utils/log.h"
#include "icon/utils/mutex.h"

namespace intrinsic::log {

struct LogEntryWithStorage {
  std::string msg;
  std::source_location loc;
  Logger::Severity severity;
};

// Thread-safe log sink.
class MockLogSink {
 public:
  explicit MockLogSink(std::string_view log_prefix = "")
      : log_prefix_(log_prefix) {}

  void Log(const Logger::LogEntry& entry) INTRINSIC_NON_REALTIME_ONLY {
    MutexLock lock(mutex_);
    std::cerr << log_prefix_ << entry.msg << std::endl;
    entries_.push_back(LogEntryWithStorage{
        .msg = std::string(entry.msg),
        .loc = entry.loc,
        .severity = entry.severity,
    });
  }

  void operator()(const Logger::LogEntry& entry) INTRINSIC_NON_REALTIME_ONLY {
    Log(entry);
  }

  // Implicit conversion operator to SinkCallback.
  operator Logger::SinkCallback() INTRINSIC_NON_REALTIME_ONLY {
    return [this](const Logger::LogEntry& entry) { Log(entry); };
  }

  std::vector<LogEntryWithStorage> GetEntries() const
      INTRINSIC_NON_REALTIME_ONLY {
    MutexLock lock(mutex_);
    return entries_;
  }

  void Clear() INTRINSIC_NON_REALTIME_ONLY {
    MutexLock lock(mutex_);
    entries_.clear();
  }

  size_t Size() const INTRINSIC_NON_REALTIME_ONLY {
    MutexLock lock(mutex_);
    return entries_.size();
  }

  bool Empty() const INTRINSIC_NON_REALTIME_ONLY {
    MutexLock lock(mutex_);
    return entries_.empty();
  }

 private:
  mutable Mutex mutex_;
  std::string log_prefix_;
  std::vector<LogEntryWithStorage> entries_ INTR_GUARDED_BY(mutex_);
};

}  // namespace intrinsic::log

#endif  // ICON_UTILS_MOCK_LOG_SINK_H_
