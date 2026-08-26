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

#include "icon/hal/hardware_module_util.h"

#include <format>
#include <string>
#include <utility>
#include <vector>

#include "flatbuffer_definitions/icon/hal/interfaces/hardware_module_state.fbs.h"

namespace intrinsic::icon {

TransitionGuardResult HardwareModuleTransitionGuard(
    intrinsic_fbs::StateCode from, intrinsic_fbs::StateCode to) {
  using intrinsic_fbs::StateCode;
  if (from == to) {
    return TransitionGuardResult::kNoOp;
  }
  if (to == StateCode::kFatallyFaulted) {
    return TransitionGuardResult::kAllowed;
  }

  switch (from) {
    case StateCode::kPreparing:
      switch (to) {
        case StateCode::kPrepared:
        case StateCode::kFaulted:
        case StateCode::kFatallyFaulted:
          return TransitionGuardResult::kAllowed;
        default:
          return TransitionGuardResult::kProhibited;
      }
    case StateCode::kPrepared:
      switch (to) {
        case StateCode::kActivating:
        case StateCode::kDeactivating:
          return TransitionGuardResult::kAllowed;
        default:
          return TransitionGuardResult::kProhibited;
      }
    case StateCode::kDeactivating:
      switch (to) {
        case StateCode::kDeactivated:
          return TransitionGuardResult::kAllowed;
        default:
          return TransitionGuardResult::kProhibited;
      }
    case StateCode::kDeactivated:
      switch (to) {
        case StateCode::kDeactivating:
          return TransitionGuardResult::kNoOp;
        case StateCode::kPreparing:
        case StateCode::kInitFailed:
          return TransitionGuardResult::kAllowed;
        default:
          return TransitionGuardResult::kProhibited;
      }

    case StateCode::kActivating:
      switch (to) {
        case StateCode::kActivated:
          return TransitionGuardResult::kAllowed;
        default:
          return TransitionGuardResult::kProhibited;
      }

    case StateCode::kActivated:
      switch (to) {
        case StateCode::kMotionDisabling:
        case StateCode::kClearingFaults:
          return TransitionGuardResult::kNoOp;
        case StateCode::kMotionEnabling:
        case StateCode::kDeactivating:
        case StateCode::kFaulted:
          return TransitionGuardResult::kAllowed;
        default:
          return TransitionGuardResult::kProhibited;
      }

    case StateCode::kMotionEnabling:
      switch (to) {
        case StateCode::kDeactivating:
        case StateCode::kMotionEnabled:
        case StateCode::kFaulted:
          return TransitionGuardResult::kAllowed;
        default:
          return TransitionGuardResult::kProhibited;
      }

    case StateCode::kMotionEnabled:
      switch (to) {
        case StateCode::kMotionEnabling:
        case StateCode::kClearingFaults:
          return TransitionGuardResult::kNoOp;
        case StateCode::kMotionDisabling:
        case StateCode::kFaulted:
        case StateCode::kDeactivating:
          return TransitionGuardResult::kAllowed;
        default:
          return TransitionGuardResult::kProhibited;
      }

    case StateCode::kMotionDisabling:
      switch (to) {
        case StateCode::kFaulted:
        case StateCode::kActivated:
        case StateCode::kDeactivating:
          return TransitionGuardResult::kAllowed;
        default:
          return TransitionGuardResult::kProhibited;
      }

    case StateCode::kFaulted:
      switch (to) {
        case StateCode::kClearingFaults:
        case StateCode::kDeactivating:
        case StateCode::kFaulted:
          return TransitionGuardResult::kAllowed;
        default:
          return TransitionGuardResult::kProhibited;
      }

    case StateCode::kClearingFaults:
      switch (to) {
        case StateCode::kDeactivating:
        case StateCode::kFaulted:
        case StateCode::kActivated:
          return TransitionGuardResult::kAllowed;
        default:
          return TransitionGuardResult::kProhibited;
      }

    case StateCode::kInitFailed:
      return TransitionGuardResult::kProhibited;

    case StateCode::kFatallyFaulted:
      return TransitionGuardResult::kProhibited;
  }
  return TransitionGuardResult::kProhibited;
}

namespace {
std::string CreateDotGraphvizStateMachineString(
    const std::vector<std::pair<intrinsic_fbs::StateCode,
                                intrinsic_fbs::StateCode>>& transitions) {
  std::string dot_string = "digraph StateMachine {\n";
  // Create a node for each state.
  for (const auto& state : intrinsic_fbs::EnumValuesStateCode()) {
    dot_string += std::format("  {:s} [label=\"{:s}\"];\n",
                              intrinsic_fbs::EnumNameStateCode(state),
                              intrinsic_fbs::EnumNameStateCode(state));
  }

  // Add edges (transitions) between the nodes.
  for (const auto& transition : transitions) {
    dot_string += std::format(
        "  {:s} -> {:s};\n", intrinsic_fbs::EnumNameStateCode(transition.first),
        intrinsic_fbs::EnumNameStateCode(transition.second));
  }

  dot_string += "}\n";
  return dot_string;
}
}  // namespace

std::string CreateDotGraphvizStateMachineString() {
  std::vector<std::pair<intrinsic_fbs::StateCode, intrinsic_fbs::StateCode>>
      transitions;
  for (const auto& from : intrinsic_fbs::EnumValuesStateCode()) {
    for (const auto& to : intrinsic_fbs::EnumValuesStateCode()) {
      if (HardwareModuleTransitionGuard(from, to) ==
          TransitionGuardResult::kAllowed) {
        transitions.push_back({from, to});
      }
    }
  }
  return CreateDotGraphvizStateMachineString(transitions);
}

}  // namespace intrinsic::icon
