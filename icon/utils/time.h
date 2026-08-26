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

#ifndef ICON_UTILS_TIME_H_
#define ICON_UTILS_TIME_H_

#include <array>
#include <chrono>
#include <cstdio>
#include <string_view>
#include <utility>

namespace intrinsic {

using Time = std::chrono::time_point<std::chrono::steady_clock>;

inline Time Now() noexcept { return std::chrono::steady_clock::now(); }

// This is not a format string. It indicates what the formatted times look like,
// so that we can allocate the right amount of memory.
constexpr char kTimeFormat[] = "yyyy-mm-ddThh:mm:ssZ";

// Writes a string representation of `t` into a fixed-size buffer and returns
// that buffer.
//
// * The return value is zero-terminated
// * The string representation is in UTC
// * If `gmtime_r()` is available, this function is thread safe.
//
// Returns an all-zero std::array in case of a string formatting error.
//
// Because `Time` is a `steady_clock` timepoint, it's not relative to a know
// epoch.
// This function determines the offset between `system_clock` and `steady_clock`
// and uses that to map the `steady_clock` time to a wall time.
// This mapping can be wrong if `system_clock` changes between the instant
// described by `t` and the instant that this function runs.
//
// So there is a slight risk that the formatted time is off. This risk is higher
// the more time passed between `t` and calling this function.
std::array<char, std::size(kTimeFormat)> FormatTime(const Time& t);

// Prints a Time to `str` as "seconds.milliseconds"
//
// (since steady_clock is not related to the "wall clock", we can't format a
// Time as date/time)
template <class Ostream>
inline Ostream&& operator<<(Ostream&& str, const Time& time) {
  // Get second/millisecond components of current time
  const auto time_seconds =
      std::chrono::time_point_cast<std::chrono::seconds>(time);
  const auto fraction = time - time_seconds;
  const auto milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(fraction);
  std::array<char, 4> ms_padded{};
  std::ignore = std::snprintf(ms_padded.data(), ms_padded.size(), "%03lld",
                              static_cast<long long int>(milliseconds.count()));
  str << time_seconds.time_since_epoch().count() << "."
      << std::string_view(ms_padded.data());
  return std::forward<Ostream>(str);
}

}  // namespace intrinsic

#endif  // ICON_UTILS_TIME_H_
