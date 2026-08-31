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

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "shared_memory_lockstep",
    srcs = [
        "shared_memory_lockstep.cc",
    ],
    hdrs = [
        "shared_memory_lockstep.h",
    ],
    deps = [
        "//icon/interprocess/shared_memory_manager",
        "//icon/interprocess/shared_memory_manager:domain_socket_utils",
        "//icon/interprocess/shared_memory_manager:memory_segment",
        "//icon/utils:attributes",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_macros",
        "//util/thread:lockstep",
    ],
)

cc_test(
    name = "shared_memory_lockstep_test",
    srcs = ["shared_memory_lockstep_test.cc"],
    deps = [
        ":shared_memory_lockstep",
        "//icon/interprocess/shared_memory_manager",
        "//icon/interprocess/shared_memory_manager:memory_segment",
        "//icon/interprocess/shared_memory_manager/testing:unique_segment_name",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_test_macros",
        "//icon/utils:time",
        "//util/thread:lockstep",
        "@com_google_googletest//:gtest_main",
    ],
)
