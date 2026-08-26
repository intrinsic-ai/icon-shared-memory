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

package(default_visibility = [
    "//incode/icon/no_absl:__subpackages__",
])

cc_library(
    name = "control_period_fbs_utils",
    srcs = ["control_period_utils.cc"],
    hdrs = [
        "control_period_utils.h",
    ],
    deps = [
        "//flatbuffer_definitions/icon/hal/interfaces:control_period_fbs_cc",
        "//icon/hal:hardware_interface_handle",
        "//icon/utils:attributes",
        "//icon/utils:format",
        "//icon/utils:log",
        "//icon/utils:status",
        "//icon/utils:time",
        "@com_github_google_flatbuffers//:flatbuffers",
    ],
)

cc_test(
    name = "control_period_utils_test",
    srcs = ["control_period_utils_test.cc"],
    deps = [
        ":control_period_fbs_utils",
        "//flatbuffer_definitions/icon/hal/interfaces:control_period_fbs_cc",
        "//icon/hal:hardware_interface_handle",
        "//icon/interprocess/shared_memory_manager",
        "//icon/interprocess/shared_memory_manager:memory_segment",
        "//icon/interprocess/shared_memory_manager/testing:unique_segment_name",
        "//icon/utils:mock_log_sink",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_test_macros",
        "//icon/utils:status_matchers",
        "//icon/utils:time",
        "@com_github_google_flatbuffers//:runtime_cc",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "hardware_module_state_fbs_utils",
    srcs = ["hardware_module_state_utils.cc"],
    hdrs = [
        "hardware_module_state_utils.h",
    ],
    deps = [
        "//flatbuffer_definitions/icon/hal/interfaces:hardware_module_state_fbs_cc",
        "//icon/utils:attributes",
        "@com_github_google_flatbuffers//:flatbuffers",
    ],
)

cc_test(
    name = "hardware_module_state_utils_test",
    srcs = ["hardware_module_state_utils_test.cc"],
    deps = [
        ":hardware_module_state_fbs_utils",
        "//flatbuffer_definitions/icon/hal/interfaces:hardware_module_state_fbs_cc",
        "@com_github_google_flatbuffers//:runtime_cc",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "icon_state_fbs_utils",
    srcs = ["icon_state_utils.cc"],
    hdrs = [
        "icon_state_utils.h",
    ],
    deps = [
        "//flatbuffer_definitions/icon/hal/interfaces:icon_state_fbs_cc",
        "//icon/utils:attributes",
        "@com_github_google_flatbuffers//:flatbuffers",
    ],
)

cc_test(
    name = "icon_state_utils_test",
    srcs = ["icon_state_utils_test.cc"],
    deps = [
        ":icon_state_fbs_utils",
        "//flatbuffer_definitions/icon/hal/interfaces:icon_state_fbs_cc",
        "@com_github_google_flatbuffers//:runtime_cc",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "joint_limits_fbs_utils",
    srcs = ["joint_limits_utils.cc"],
    hdrs = [
        "joint_limits_utils.h",
    ],
    deps = [
        "//flatbuffer_definitions/icon/hal/interfaces:joint_limits_fbs_cc",
        "//icon/utils:attributes",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_macros",
        "//kinematics/types:joint_limits",
        "@com_github_google_flatbuffers//:flatbuffers",
    ],
)

cc_library(
    name = "joint_command_fbs_utils",
    srcs = ["joint_command_utils.cc"],
    hdrs = [
        "joint_command_utils.h",
    ],
    deps = [
        "//flatbuffer_definitions/icon/hal/interfaces:joint_command_fbs_cc",
        "//icon/flatbuffers:flatbuffer_utils",
        "//icon/utils:attributes",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_macros",
        "@com_github_google_flatbuffers//:runtime_cc",
    ],
)

cc_test(
    name = "joint_limits_utils_test",
    srcs = ["joint_limits_utils_test.cc"],
    deps = [
        ":joint_limits_fbs_utils",
        "//flatbuffer_definitions/icon/hal/interfaces:joint_limits_fbs_cc",
        "//icon/hal:hardware_interface_handle",
        "//icon/hal:hardware_interface_registry",
        "//icon/hal:hardware_interface_traits",
        "//icon/interprocess/shared_memory_manager",
        "//icon/interprocess/shared_memory_manager/testing:unique_segment_name",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_test_macros",
        "//kinematics/types:joint_limits",
        "@com_github_google_flatbuffers//:runtime_cc",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "joint_state_fbs_utils",
    srcs = ["joint_state_utils.cc"],
    hdrs = [
        "joint_state_utils.h",
    ],
    deps = [
        "//flatbuffer_definitions/icon/hal/interfaces:joint_state_fbs_cc",
        "//icon/utils:attributes",
        "@com_github_google_flatbuffers//:flatbuffers",
    ],
)

cc_test(
    name = "joint_state_utils_test",
    srcs = ["joint_state_utils_test.cc"],
    deps = [
        ":joint_state_fbs_utils",
        "//flatbuffer_definitions/icon/hal/interfaces:joint_state_fbs_cc",
        "@com_github_google_flatbuffers//:runtime_cc",
        "@com_google_googletest//:gtest_main",
    ],
)
