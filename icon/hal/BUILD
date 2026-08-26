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
    name = "hardware_interface_traits",
    hdrs = ["hardware_interface_traits.h"],
)

cc_library(
    name = "hardware_interface_handle",
    hdrs = ["hardware_interface_handle.h"],
    deps = [
        ":icon_state_register",
        "//flatbuffer_definitions/icon/hal/interfaces:icon_state_fbs_cc",
        "//icon/interprocess/shared_memory_manager:memory_segment",
        "//icon/utils:current_cycle",
        "//icon/utils:status",
        "//icon/utils:time",
        "@com_github_google_flatbuffers//:flatbuffers",
    ],
)

cc_library(
    name = "hardware_interface_registry",
    srcs = ["hardware_interface_registry.cc"],
    hdrs = [
        "get_hardware_interface.h",
        "hardware_interface_registry.h",
    ],
    deps = [
        ":hardware_interface_handle",
        ":hardware_interface_traits",
        ":icon_state_register",
        "//flatbuffer_definitions/icon/hal/interfaces:icon_state_fbs_cc",
        "//flatbuffer_definitions/icon/interprocess/shared_memory_manager:segment_info_fbs_cc",
        "//icon/interprocess/shared_memory_manager",
        "//icon/interprocess/shared_memory_manager:domain_socket_utils",
        "//icon/interprocess/shared_memory_manager:memory_segment",
        "//icon/interprocess/shared_memory_manager:segment_header",
        "//icon/interprocess/shared_memory_manager:segment_info_utils",
        "//icon/utils:attributes",
        "//icon/utils:log",
        "//icon/utils:realtime_guard",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_macros",
        "@com_github_google_flatbuffers//:flatbuffers",
    ],
)

cc_library(
    name = "hardware_module_init_context",
    hdrs = [
        "hardware_module_init_context.h",
    ],
    deps = [
        "//icon/hal:hardware_interface_registry",
    ],
)

cc_library(
    name = "hardware_module_interface",
    hdrs = [
        "hardware_module_interface.h",
    ],
    deps = [
        ":hardware_module_init_context",
        "//icon/control:realtime_clock_interface",
        "//icon/utils:status",
    ],
)

cc_library(
    name = "control_period_register",
    hdrs = ["control_period_register.h"],
    deps = [
        ":hardware_interface_traits",
        "//flatbuffer_definitions/icon/hal/interfaces:control_period_fbs_cc",
        "//icon/hal/interfaces:control_period_fbs_utils",
    ],
)

cc_library(
    name = "icon_state_register",
    hdrs = ["icon_state_register.h"],
    deps = [
        ":hardware_interface_traits",
        "//flatbuffer_definitions/icon/hal/interfaces:icon_state_fbs_cc",
        "//icon/hal/interfaces:icon_state_fbs_utils",
    ],
)

cc_library(
    name = "realtime_clock",
    srcs = ["realtime_clock.cc"],
    hdrs = ["realtime_clock.h"],
    deps = [
        "//icon/control:realtime_clock_interface",
        "//icon/interprocess/shared_memory_lockstep",
        "//icon/interprocess/shared_memory_manager",
        "//icon/interprocess/shared_memory_manager:memory_segment",
        "//icon/utils:log",
        "//icon/utils:status",
        "//icon/utils:time",
        "//util/thread:lockstep",
    ],
)

cc_test(
    name = "hardware_interface_registry_test",
    srcs = ["hardware_interface_registry_test.cc"],
    deps = [
        ":hardware_interface_handle",
        ":hardware_interface_registry",
        ":hardware_interface_traits",
        ":icon_state_register",
        "//flatbuffer_definitions/icon/hal/interfaces:icon_state_fbs_cc",
        "//flatbuffer_definitions/icon/hal/interfaces:joint_limits_fbs_cc",
        "//icon/hal/interfaces:joint_limits_fbs_utils",
        "//icon/interprocess/shared_memory_manager",
        "//icon/interprocess/shared_memory_manager:segment_header",
        "//icon/interprocess/shared_memory_manager/testing:unique_segment_name",
        "//icon/utils:current_cycle",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_test_macros",
        "//icon/utils:time",
        "@com_github_google_flatbuffers//:runtime_cc",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "get_hardware_interface_test",
    srcs = ["get_hardware_interface_test.cc"],
    deps = [
        ":hardware_interface_registry",
        ":icon_state_register",
        "//flatbuffer_definitions/icon/hal/interfaces:icon_state_fbs_cc",
        "//flatbuffer_definitions/icon/hal/interfaces:joint_limits_fbs_cc",
        "//flatbuffer_definitions/icon/interprocess/shared_memory_manager:segment_info_fbs_cc",
        "//icon/flatbuffers:flatbuffer_utils",
        "//icon/hal/interfaces:icon_state_fbs_utils",
        "//icon/hal/interfaces:joint_limits_fbs_utils",
        "//icon/interprocess/shared_memory_manager:segment_header",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_test_macros",
        "@com_github_google_flatbuffers//:runtime_cc",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "hardware_module_runtime",
    srcs = ["hardware_module_runtime.cc"],
    hdrs = ["hardware_module_runtime.h"],
    deps = [
        ":control_period_register",
        ":hardware_interface_handle",
        ":hardware_interface_registry",
        ":hardware_interface_traits",
        ":hardware_module_init_context",
        ":hardware_module_interface",
        ":hardware_module_util",
        ":icon_state_register",
        "//flatbuffer_definitions/icon/hal/interfaces:control_period_fbs_cc",
        "//flatbuffer_definitions/icon/hal/interfaces:hardware_module_state_fbs_cc",
        "//flatbuffer_definitions/icon/hal/interfaces:icon_state_fbs_cc",
        "//icon/hal/interfaces:control_period_fbs_utils",
        "//icon/hal/interfaces:hardware_module_state_fbs_utils",
        "//icon/interprocess/remote_trigger:remote_trigger_server",
        "//icon/interprocess/shared_memory_manager",
        "//icon/interprocess/shared_memory_manager:domain_socket_server",
        "//icon/interprocess/shared_memory_manager:domain_socket_utils",
        "//icon/testing:realtime_annotations",
        "//icon/utils:async_buffer",
        "//icon/utils:async_request",
        "//icon/utils:attributes",
        "//icon/utils:cleanup",
        "//icon/utils:log",
        "//icon/utils:mutex",
        "//icon/utils:realtime_guard",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_macros",
        "//icon/utils:time",
        "//platform/common/buffers:rt_promise",
        "//platform/common/buffers:rt_queue",
        "//platform/common/buffers:rt_queue_multi_writer",
        "//util/thread:thread_options",
        "@com_github_google_flatbuffers//:flatbuffers",
        "@tl-expected//:tl-expected",
    ],
)

cc_test(
    name = "hardware_module_runtime_test",
    srcs = ["hardware_module_runtime_test.cc"],
    tags = [
        "notsan",  # 
    ],
    deps = [
        ":control_period_register",
        ":hardware_interface_handle",
        ":hardware_interface_registry",
        ":hardware_interface_traits",
        ":hardware_module_init_context",
        ":hardware_module_interface",
        ":hardware_module_runtime",
        ":hardware_module_util",
        ":icon_state_register",
        "//flatbuffer_definitions/icon/hal/interfaces:control_period_fbs_cc",
        "//flatbuffer_definitions/icon/hal/interfaces:hardware_module_state_fbs_cc",
        "//flatbuffer_definitions/icon/hal/interfaces:icon_state_fbs_cc",
        "//flatbuffer_definitions/icon/hal/interfaces:joint_command_fbs_cc",
        "//flatbuffer_definitions/icon/hal/interfaces:joint_limits_fbs_cc",
        "//flatbuffer_definitions/icon/hal/interfaces:joint_state_fbs_cc",
        "//icon/hal/interfaces:control_period_fbs_utils",
        "//icon/hal/interfaces:hardware_module_state_fbs_utils",
        "//icon/hal/interfaces:joint_command_fbs_utils",
        "//icon/hal/interfaces:joint_limits_fbs_utils",
        "//icon/hal/interfaces:joint_state_fbs_utils",
        "//icon/interprocess:binary_futex",
        "//icon/interprocess/remote_trigger:remote_trigger_server",
        "//icon/interprocess/shared_memory_manager",
        "//icon/interprocess/shared_memory_manager:domain_socket_utils",
        "//icon/interprocess/shared_memory_manager:memory_segment",
        "//icon/interprocess/shared_memory_manager/testing:unique_segment_name",
        "//icon/utils:log",
        "//icon/utils:mock_log_sink",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_macros",
        "//icon/utils:status_and_expected_test_macros",
        "//icon/utils:status_matchers",
        "@com_github_google_flatbuffers//:flatbuffers",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@tl-expected//:tl-expected",
    ],
)

cc_library(
    name = "hardware_module_util",
    srcs = ["hardware_module_util.cc"],
    hdrs = [
        "hardware_module_util.h",
    ],
    deps = [
        "//flatbuffer_definitions/icon/hal/interfaces:hardware_module_state_fbs_cc",
        "//icon/utils:attributes",
        "//icon/utils:mutex",
        "//icon/utils:status",
    ],
)

cc_test(
    name = "hardware_module_util_test",
    srcs = ["hardware_module_util_test.cc"],
    deps = [
        ":hardware_module_util",
        "//flatbuffer_definitions/icon/hal/interfaces:hardware_module_state_fbs_cc",
        "//icon/utils:status",
        "//icon/utils:status_and_expected_test_macros",
        "//icon/utils:status_matchers",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
