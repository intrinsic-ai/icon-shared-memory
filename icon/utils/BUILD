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
load("//icon/testing:cc_test_and_malloc_test.bzl", "cc_test_and_malloc_test")

package(default_visibility = [
    "//visibility:public",
])

cc_library(
    name = "attributes",
    hdrs = ["attributes.h"],
)

cc_library(
    name = "async_buffer",
    hdrs = ["async_buffer.h"],
    deps = [
        ":attributes",
        ":check",
    ],
)

cc_test(
    name = "async_buffer_test",
    srcs = ["async_buffer_test.cc"],
    deps = [
        ":async_buffer",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "async_request",
    hdrs = ["async_request.h"],
    deps = [
        "//icon/testing:realtime_annotations",
        "//icon/utils:status",
        "//platform/common/buffers:rt_promise",
    ],
)

cc_test_and_malloc_test(
    name = "async_request_test",
    srcs = ["async_request_test.cc"],
    deps = [
        ":async_request",
        ":log",
        ":mock_log_sink",
        ":status",
        ":status_and_expected_test_macros",
        "//icon/testing:malloc_test",
        "//platform/common/buffers:rt_promise",
    ],
)

cc_library(
    name = "cleanup",
    hdrs = ["cleanup.h"],
    deps = [":attributes"],
)

cc_test(
    name = "cleanup_test",
    size = "small",
    srcs = ["cleanup_test.cc"],
    deps = [
        ":cleanup",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "current_cycle",
    srcs = ["current_cycle.cc"],
    hdrs = ["current_cycle.h"],
)

cc_test(
    name = "current_cycle_test",
    size = "small",
    srcs = ["current_cycle_test.cc"],
    deps = [
        ":current_cycle",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "format",
    hdrs = ["format.h"],
)

cc_test(
    name = "format_test",
    size = "small",
    srcs = ["format_test.cc"],
    deps = [
        ":format",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "log",
    hdrs = ["log.h"],
)

cc_test(
    name = "log_test",
    size = "small",
    srcs = ["log_test.cc"],
    deps = [
        ":log",
        ":mock_log_sink",
        ":time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "mock_log_sink",
    testonly = True,
    hdrs = ["mock_log_sink.h"],
    deps = [
        ":attributes",
        ":log",
        ":mutex",
        "//icon/testing:realtime_annotations",
    ],
)

cc_library(
    name = "mutex",
    hdrs = ["mutex.h"],
    deps = [
        ":attributes",
        ":check",
    ],
)

cc_test(
    name = "mutex_test",
    size = "small",
    srcs = ["mutex_test.cc"],
    deps = [
        ":attributes",
        ":mutex",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "realtime_guard",
    hdrs = ["realtime_guard.h"],
)

cc_library(
    name = "status_helpers",
    hdrs = ["status_helpers.h"],
    deps = [
        ":status",
    ],
)

cc_test(
    name = "status_helpers_test",
    srcs = ["status_helpers_test.cc"],
    deps = [
        ":status_helpers",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "status",
    hdrs = ["status.h"],
    deps = [
        ":attributes",
        ":realtime_guard",
        "//icon/testing:realtime_annotations",
    ],
)

cc_test(
    name = "status_test",
    srcs = ["status_test.cc"],
    deps = [
        ":status",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "status_and_expected_macros",
    hdrs = ["status_and_expected_macros.h"],
    deps = [
        ":status",
        "@tl-expected//:tl-expected",
    ],
)

cc_test(
    name = "status_and_expected_macros_test",
    size = "small",
    srcs = ["status_and_expected_macros_test.cc"],
    deps = [
        ":status_and_expected_macros",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "status_and_expected_test_macros",
    testonly = True,
    hdrs = ["status_and_expected_test_macros.h"],
    deps = [
        ":status",
        ":status_and_expected_macros",
        "@tl-expected//:tl-expected",
    ],
)

cc_library(
    name = "status_matchers",
    testonly = True,
    hdrs = ["status_matchers.h"],
    deps = [
        ":status",
        "@com_google_googletest//:gtest",
        "@tl-expected//:tl-expected",
    ],
)

cc_test(
    name = "status_matchers_test",
    size = "small",
    srcs = ["status_matchers_test.cc"],
    deps = [
        ":status",
        ":status_matchers",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@tl-expected//:tl-expected",
    ],
)

cc_library(
    name = "strerror",
    hdrs = ["strerror.h"],
)

cc_test(
    name = "strerror_test",
    size = "small",
    srcs = ["strerror_test.cc"],
    deps = [
        ":strerror",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "time",
    srcs = ["time.cc"],
    hdrs = ["time.h"],
)

cc_test(
    name = "time_test",
    size = "small",
    srcs = ["time_test.cc"],
    deps = [
        ":time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "check",
    hdrs = ["check.h"],
)

cc_test(
    name = "check_test",
    size = "small",
    srcs = ["check_test.cc"],
    deps = [
        ":check",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
