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

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "realtime_annotations",
    hdrs = ["realtime_annotations.h"],
)

# This is a dummy implementation to make it easier to port code.
# TODO: Add back the implementation once MallocGuard is
#                    available via BCR.
bzl_library(
    name = "cc_test_and_malloc_test_bzl",
    srcs = ["cc_test_and_malloc_test.bzl"],
    deps = [
        "@bazel_skylib//lib:new_sets",
        "@rules_cc//cc:bzl_srcs",
    ],
)

cc_library(
    name = "malloc_test",
    testonly = True,
    hdrs = ["malloc_test.h"],
    deps = [
        "@com_google_googletest//:gtest",
    ],
)

cc_binary(
    name = "malloc_test_main",
    testonly = True,
    srcs = ["malloc_test_main.cc"],
    deps = [
        "@com_google_googletest//:gtest",
    ],
)
