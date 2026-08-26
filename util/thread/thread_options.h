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

#ifndef UTIL_THREAD_THREAD_OPTIONS_H_
#define UTIL_THREAD_THREAD_OPTIONS_H_

#include <sched.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace intrinsic {

// Builder-style struct to set thread options like scheduler and priority.
//
// A common use case is the "prelude" for a realtime thread.
struct ThreadOptions {
  static constexpr int kLowRealtimePriority = 40;
  static constexpr int kHighRealtimePriority = 45;
  static constexpr int kDefaultRealtimePriority = kLowRealtimePriority;
  static constexpr int kDefaultRealtimeScheduler = SCHED_FIFO;
  static constexpr int kDefaultNonRealtimePriority = 0;
  static constexpr int kDefaultNonRealtimeScheduler = SCHED_OTHER;

  std::string name;
  bool is_realtime = false;
  std::optional<int> priority = std::nullopt;
  std::optional<int> schedule_policy = std::nullopt;
  std::vector<int> cpu_affinity;

  ThreadOptions& SetName(std::string_view n) {
    name = n;
    return *this;
  }
  ThreadOptions& SetPriority(int priority) {
    this->priority = priority;
    return *this;
  }
  ThreadOptions& SetScheduler(int schedule_policy) {
    this->schedule_policy = schedule_policy;
    return *this;
  }
  ThreadOptions& SetIsRealtime(bool is_realtime) {
    this->is_realtime = is_realtime;
    return *this;
  }
  ThreadOptions& SetAffinity(const std::vector<int>& cpus) {
    cpu_affinity = cpus;
    return *this;
  }
  ThreadOptions& SetLowRealtimePriorityAndScheduler() {
    is_realtime = true;
    schedule_policy = kDefaultRealtimeScheduler;
    priority = kLowRealtimePriority;
    return *this;
  }
  ThreadOptions& SetHighRealtimePriorityAndScheduler() {
    is_realtime = true;
    schedule_policy = kDefaultRealtimeScheduler;
    priority = kHighRealtimePriority;
    return *this;
  }
};

}  // namespace intrinsic
#endif  // UTIL_THREAD_THREAD_OPTIONS_H_
