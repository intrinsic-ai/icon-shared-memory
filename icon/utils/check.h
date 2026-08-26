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

#ifndef ICON_UTILS_CHECK_H_
#define ICON_UTILS_CHECK_H_

#include <cstdlib>
#include <iostream>

// Terminates the program with std::abort if `condition` evaluates to `false`.
//
// Also prints `message` (plus an additional newline) to stderr.
#define INTR_CHECK(condition, message) \
  if (!(condition)) {                  \
    std::cerr << message << std::endl; \
    std::abort();                      \
  }

#endif  // ICON_UTILS_CHECK_H_
