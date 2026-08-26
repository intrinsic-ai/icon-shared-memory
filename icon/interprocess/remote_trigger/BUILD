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

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//bazel:cc_macros.bzl", "cc_library")

package(default_visibility = [
    "//incode/icon/no_absl:__subpackages__",
])

cc_library(
    name = "remote_trigger_server",
    srcs = [
        "remote_trigger_server.cc",
    ],
    hdrs = [
        "remote_trigger_constants.h",
        "remote_trigger_server.h",
    ],
    deps = [
        "//icon/interprocess:binary_futex",
        "//icon/interprocess/shared_memory_manager",
        "//icon/interprocess/shared_memory_manager:memory_segment",
        "//icon/utils:log",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_macros",
        "//icon/utils:time",
        "@tl-expected//:tl-expected",
    ],
)

cc_library(
    name = "remote_trigger_test_common",
    testonly = True,
    hdrs = ["remote_trigger_test_common.h"],
    deps = [
        ":remote_trigger_server",
    ],
)

cc_test(
    name = "remote_trigger_server_test",
    srcs = ["remote_trigger_server_test.cc"],
    deps = [
        ":remote_trigger_server",
        ":remote_trigger_test_common",
        "//icon/interprocess/shared_memory_manager",
        "//icon/interprocess/shared_memory_manager/testing:unique_segment_name",
        "//icon/utils:status_and_expected_test_macros",
        "@com_google_googletest//:gtest_main",
        "@tl-expected//:tl-expected",
    ],
)
