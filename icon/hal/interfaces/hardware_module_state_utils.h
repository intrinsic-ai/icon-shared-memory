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

#ifndef ICON_HAL_INTERFACES_HARDWARE_MODULE_STATE_UTILS_H_
#define ICON_HAL_INTERFACES_HARDWARE_MODULE_STATE_UTILS_H_

#include <string_view>

#include "flatbuffers/detached_buffer.h"
#include "flatbuffer_definitions/icon/hal/interfaces/hardware_module_state.fbs.h"
#include "icon/utils/attributes.h"

namespace intrinsic_fbs {

INTR_MUST_USE_RESULT flatbuffers::DetachedBuffer BuildHardwareModuleState();

// Updates the code and message of the state.
//
// Is a no-op if `hardware_module_state` is a nullptr, or its `message` member
// is a nullptr!
//
// Truncates `message` if it is longer than the underlying flatbuffer.
void SetState(HardwareModuleState* hardware_module_state, StateCode code,
              std::string_view message);

// Returns the message associated with the given state.
//
// Returns an empty string if `hardware_module_state` or
// `hardware_module_state->message()` is nullptr.
std::string_view GetMessage(const HardwareModuleState* hardware_module_state);

}  // namespace intrinsic_fbs

#endif  // ICON_HAL_INTERFACES_HARDWARE_MODULE_STATE_UTILS_H_
