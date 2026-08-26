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

#ifndef ICON_INTERPROCESS_REMOTE_TRIGGER_REMOTE_TRIGGER_TEST_COMMON_H_
#define ICON_INTERPROCESS_REMOTE_TRIGGER_REMOTE_TRIGGER_TEST_COMMON_H_

#include <chrono>
#include <thread>

#include "icon/interprocess/remote_trigger/remote_trigger_server.h"

namespace remote_trigger_test_common {

inline bool WaitForServer(intrinsic::icon::RemoteTriggerServer& server) {
  constexpr int kMaxWaitCycles = 10;
  int wait_cycles = 0;
  while (!server.IsStarted() && wait_cycles <= kMaxWaitCycles) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ++wait_cycles;
  }
  return server.IsStarted();
}
}  // namespace remote_trigger_test_common

#endif  // ICON_INTERPROCESS_REMOTE_TRIGGER_REMOTE_TRIGGER_TEST_COMMON_H_
