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

load("@rules_cc//cc:defs.bzl", "cc_library")
load("//flatbuffer_definitions/platform:flatbuffers.bzl", "cc_flatbuffers_library", "flatbuffers_library")

package(default_visibility = ["//visibility:public"])

flatbuffers_library(
    name = "control_period_fbs",
    srcs = ["control_period.fbs"],
)

cc_flatbuffers_library(
    name = "control_period_fbs_cc",
    deps = [":control_period_fbs"],
)

flatbuffers_library(
    name = "hardware_module_state_fbs",
    srcs = ["hardware_module_state.fbs"],
)

cc_flatbuffers_library(
    name = "hardware_module_state_fbs_cc",
    deps = [":hardware_module_state_fbs"],
)

flatbuffers_library(
    name = "icon_state_fbs",
    srcs = ["icon_state.fbs"],
)

cc_flatbuffers_library(
    name = "icon_state_fbs_cc",
    deps = [":icon_state_fbs"],
)

flatbuffers_library(
    name = "joint_command_fbs",
    srcs = ["joint_command.fbs"],
)

cc_flatbuffers_library(
    name = "joint_command_fbs_cc",
    deps = [":joint_command_fbs"],
)

flatbuffers_library(
    name = "joint_limits_fbs",
    srcs = ["joint_limits.fbs"],
)

cc_flatbuffers_library(
    name = "joint_limits_fbs_cc",
    deps = [":joint_limits_fbs"],
)

flatbuffers_library(
    name = "joint_state_fbs",
    srcs = ["joint_state.fbs"],
)

cc_flatbuffers_library(
    name = "joint_state_fbs_cc",
    deps = [":joint_state_fbs"],
)
