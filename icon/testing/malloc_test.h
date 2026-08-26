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

#ifndef ICON_TESTING_MALLOC_TEST_H_
#define ICON_TESTING_MALLOC_TEST_H_

#include <gtest/gtest.h>

#include <cstddef>
#include <iostream>

// Macros to check number of heap allocations.
//
// This allows writing "malloc tests", tests verify that a piece of code does
// not allocate on the heap, which is a helpful to prevent real-time safety
// bugs, or to test for a certain number of allocations.
//
// For example:
// \code
// TEST(MyTest, String) {
//     // Ensure creating a c-string doesn't call malloc()
//     IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
//     char* s = "hello, world";
//     IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS();
//
//     // Check that creating a string calls malloc() once
//     IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER();
//     string s = "hello, world";
//     IF_INTRINSIC_MALLOC_TEST_EXPECT_ALLOCATIONS_EQ(1);
// }
// \endcode
//
// Note that malloc testing has some special requirements.
// Use intrinsic/icon/testing/cc_test_and_malloc_test.bzl
// to auto-generate both a regular and malloc-counting test with the right
// settings.

// Ensure __has_feature is safe to use on non-clang compilers.
#ifndef __has_feature
#define __has_feature(x) 0
#endif

// TODO: Add back implementation once MallocGuard is available via
// BCR.

#if INTRINSIC_MALLOC_TEST
#if __linux__
#define IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER() \
  ::intrinsic::icon::MallocCounterInit()
#else
#define IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER()                               \
  std::cerr << "Ignoring IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER on non-linux " \
               "platform."                                                    \
            << std::endl;
#endif

// Checks the current number of allocations against \p expected with
// GTest's EXPECT_EQ().
#if __linux__
#define IF_INTRINSIC_MALLOC_TEST_EXPECT_ALLOCATIONS_EQ(expected) \
  EXPECT_EQ(expected, ::intrinsic::icon::MallocCounterCount())
#else
#define IF_INTRINSIC_MALLOC_TEST_EXPECT_ALLOCATIONS_EQ(expected)       \
  INTRINSIC_RT_LOG_THROTTLED(WARNING)                                  \
      << "Ignoring IF_INTRINSIC_MALLOC_TEST_EXPECT_ALLOCATIONS_EQ on " \
         "non-linux platform."
#endif

// Checks the current number of allocations equals zero with GTest's
// EXPECT_EQ().
#define IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS() \
  IF_INTRINSIC_MALLOC_TEST_EXPECT_ALLOCATIONS_EQ(0)

#else
#define IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER()
#define IF_INTRINSIC_MALLOC_TEST_EXPECT_ALLOCATIONS_EQ(expected)
#define IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS()
#endif

namespace intrinsic::icon {

// Initializes the malloc counter. This will reset the number of allocations
// back to zero.
// Do not call directly, prefer `IF_INTRINSIC_MALLOC_TEST_INIT_COUNTER`
void MallocCounterInit();

// Returns the number of allocations.
// Do not call directly, prefer `IF_INTRINSIC_MALLOC_TEST_EXPECT_NO_ALLOCATIONS`
size_t MallocCounterCount();

// Decrease the malloc counter by the given amount.
// You should probably never use this. This is expected to be called only in
// special macro functions.
void MallocCounterSubtract(size_t count);

// Print a stack trace for the next allocation. This is useful to track down
// unexpected allocations while developing. Submitted code should probably not
// call this.
void PrintNextStackTrace();
}  // namespace intrinsic::icon

#endif  // ICON_TESTING_MALLOC_TEST_H_
