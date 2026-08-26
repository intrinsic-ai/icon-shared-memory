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

load("//bazel:cc_macros.bzl", "cc_library", "cc_test")
load("//flatbuffer_definitions/platform:flatbuffers.bzl", "cc_flatbuffers_library", "flatbuffers_library")

package(default_visibility = [
    "//flatbuffer_definitions/icon:__subpackages__",
    "//visibility:public",
])

cc_library(
    name = "flatbuffer_utils",
    hdrs = ["flatbuffer_utils.h"],
    deps = [
        "//icon/utils:status",
        "@com_github_google_flatbuffers//:runtime_cc",
    ],
)

cc_test(
    name = "flatbuffer_utils_test",
    srcs = ["flatbuffer_utils_test.cc"],
    deps = [
        ":flatbuffer_utils",
        "//flatbuffer_definitions/icon/flatbuffers:transform_types_fbs_cc",
        "//flatbuffer_definitions/icon/interprocess/shared_memory_manager:segment_info_fbs_cc",
        "//icon/utils:status",
        "@com_github_google_flatbuffers//:runtime_cc",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "fixed_string",
    hdrs = ["fixed_string.h"],
    deps = [
        "//icon/utils:status",
        "@com_github_google_flatbuffers//:runtime_cc",
        "@tl-expected//:tl-expected",
    ],
)

flatbuffers_library(
    name = "fixed_string_test_fbs",
    srcs = [
        "fixed_string_test.fbs",
    ],
)

cc_flatbuffers_library(
    name = "fixed_string_test_fbs_cc",
    deps = [":fixed_string_test_fbs"],
)

cc_test(
    name = "fixed_string_test",
    srcs = ["fixed_string_test.cc"],
    deps = [
        ":fixed_string",
        ":fixed_string_test_fbs_cc",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_test_macros",
        "@com_github_google_flatbuffers//:runtime_cc",
        "@com_google_googletest//:gtest_main",
    ],
)
