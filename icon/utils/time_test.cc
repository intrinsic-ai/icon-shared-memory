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

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

#include "gtest/gtest.h"

namespace intrinsic {

TEST(Time, FormatTimeResultIsZeroTerminated) {
  auto t = Now();
  auto formatted = FormatTime(t);
  EXPECT_EQ(formatted[formatted.size() - 1], '\0');
}

TEST(Time, FormatTimeResultHasCorrectLength) {
  auto t = Now();
  auto formatted = FormatTime(t);
  EXPECT_EQ(std::strlen(formatted.data()), std::strlen(kTimeFormat));
}

TEST(Time, FormatTimeIsInRightBallpark) {
  auto wall_now = std::chrono::system_clock::now();
  auto intr_now = Now();
  auto formatted = FormatTime(intr_now);

  std::tm t = {};
  t.tm_isdst = 0;

  std::istringstream is(formatted.data());
  is >> std::get_time(&t, "%Y-%m-%dT%H:%M:%SZ");
  std::chrono::system_clock::time_point parsed =
      std::chrono::system_clock::from_time_t(::timegm(&t));
  EXPECT_LT(std::abs(std::chrono::duration_cast<std::chrono::milliseconds>(
                         parsed - wall_now)
                         .count()),
            1000)
      << "Formatted time and wall time diverge by more than one second. If you "
         "didn't run this test on a day leading into a leap year, or into/out "
         "of daylight savings time, this is a problem. If you did, please "
         "rerun the test.";
}

TEST(StreamingOperator, PrintsCorrectTime) {
  auto t = Now();
  std::stringstream s;
  s << t;
  std::string seconds_str;
  std::string milliseconds_str;
  std::getline(s, seconds_str, '.');
  std::getline(s, milliseconds_str);
  int seconds = std::stoi(seconds_str);
  int milliseconds = std::stoi(milliseconds_str);
  EXPECT_EQ(std::chrono::duration_cast<std::chrono::milliseconds>(
                t.time_since_epoch())
                .count(),
            seconds * 1000 + milliseconds);
}

}  // namespace intrinsic
