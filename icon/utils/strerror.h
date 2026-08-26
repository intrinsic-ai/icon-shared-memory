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

#ifndef ICON_UTILS_STRERROR_H_
#define ICON_UTILS_STRERROR_H_

#include <string.h>

#include <array>
#include <cstddef>
#include <cstdio>

namespace intrinsic {

namespace detail {
// Handles GNU strerror_r:
//
// char* strerror_r(int errnum, char* buf, size_t buflen)
//
// This can return either a pointer to the original buffer
// (in case the error code is unknown and it had to print a bespoke message)
// or a pointer to a static error string (if the error code is well-known).
inline void HandleStrErrorR(char* buf, size_t buflen,
                            const char* res) noexcept {
  // If strerror_r returned a pointer to our original buffer, we don't need
  // to do anything.
  if (res == buf) {
    return;
  }
  // Otherwise, i.e. if the return value from strerror_r is different from
  // the original buffer, we need to copy the result to our buffer.
  // We use `snprintf()` (rather than `memcpy()`) here because our buffer might
  // be smaller than the one that `strerror_r` returned, and we want to make
  // sure it is still zero-terminated.
  std::snprintf(buf, buflen, "%s", res);
}

// Handles POSIX strerror_r:
//
// int strerror_r(int errnum, char* buf, size_t buflen)
//
// This always writes into `buf`, so we don't need to do anything.
inline void HandleStrErrorR(char*, size_t, int) noexcept {}
}  // namespace detail

template <size_t N = 128>
std::array<char, N> StrError(const int err) noexcept {
  std::array<char, N> result{};
  // This call resolves to the correct overload, depending on whether
  // strerror_r() uses the GNU or POSIX signature (see above).
  //
  // See https://www.club.cc.cmu.edu/~cmccabe/blog_strerror.html for details.
  detail::HandleStrErrorR(result.data(), N,
                          ::strerror_r(err, result.data(), N));
  return result;
}

}  // namespace intrinsic
#endif  // ICON_UTILS_STRERROR_H_
