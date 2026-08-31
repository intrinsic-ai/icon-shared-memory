# Copyright 2026 Intrinsic Innovation LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = [
    "//visibility:public",
])

cc_library(
    name = "binary_futex",
    srcs = ["binary_futex.cc"],
    hdrs = [
        "binary_futex.h",
    ],
    deps = [
        "//icon/utils:attributes",
        "//icon/utils:status",
        "//icon/utils:strerror",
        "//icon/utils:time",
    ],
)

cc_test(
    name = "binary_futex_test",
    size = "small",
    srcs = ["binary_futex_test.cc"],
    deps = [
        ":binary_futex",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_test_macros",
        "//icon/utils:time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "lockable_binary_futex",
    hdrs = [
        "lockable_binary_futex.h",
    ],
    deps = [
        ":binary_futex",
        "//icon/utils:attributes",
        "//icon/utils:check",
        "//icon/utils:status",
        "//icon/utils:strerror",
        "//icon/utils:time",
    ],
)

cc_test(
    name = "lockable_binary_futex_test",
    size = "small",
    srcs = ["lockable_binary_futex_test.cc"],
    deps = [
        ":lockable_binary_futex",
        "//icon/utils:attributes",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_test_macros",
        "//icon/utils:status_matchers",
        "//icon/utils:time",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
