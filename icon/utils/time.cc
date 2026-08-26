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

#include "icon/utils/time.h"

#include <time.h>

#include <array>
#include <chrono>
#include <cstring>

namespace intrinsic {

namespace {
std::chrono::system_clock::time_point to_sys(
    std::chrono::steady_clock::time_point t) {
  const auto sys_now = std::chrono::system_clock::now();
  const auto steady_now = std::chrono::steady_clock::now();
  const auto delta =
      std::chrono::duration_cast<std::chrono::system_clock::duration>(
          t - steady_now);
  return sys_now + delta;
}

}  // namespace

std::array<char, std::size(kTimeFormat)> FormatTime(const Time& t) {
  // std::size(kTimeFormat) includes a zero terminator
  std::array<char, std::size(kTimeFormat)> out{};
  const auto sys_t = to_sys(t);
  // Leave room for the zero terminator
  const auto format_result = std::format_to_n(
      std::data(out), std::size(out) - 1, "{0:%F}T{0:%T}Z", sys_t);
  // Add zero terminator
  *format_result.out = '\0';
  return out;
}

}  // namespace intrinsic
