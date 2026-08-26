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

#include "icon/hal/interfaces/hardware_module_state_utils.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string_view>

#include "flatbuffers/detached_buffer.h"
#include "flatbuffers/flatbuffer_builder.h"
#include "flatbuffer_definitions/icon/hal/interfaces/hardware_module_state.fbs.h"

namespace intrinsic_fbs {

flatbuffers::DetachedBuffer BuildHardwareModuleState() {
  flatbuffers::FlatBufferBuilder builder;
  builder.ForceDefaults(true);

  builder.Finish(builder.CreateStruct(HardwareModuleState()));
  return builder.Release();
}

void SetState(HardwareModuleState* hardware_module_state, StateCode code,
              std::string_view message) {
  if (hardware_module_state == nullptr ||
      hardware_module_state->message() == nullptr) {
    return;
  }
  const size_t copy_length = std::min(
      static_cast<size_t>(hardware_module_state->message()->size() - 1),
      message.size());
  hardware_module_state->mutate_code(code);
  std::memcpy(hardware_module_state->mutable_message()->Data(), message.data(),
              copy_length);
  std::memset(hardware_module_state->mutable_message()->Data() + copy_length,
              '\0',
              hardware_module_state->mutable_message()->size() - copy_length);
}

std::string_view GetMessage(const HardwareModuleState* hardware_module_state) {
  if (hardware_module_state == nullptr ||
      hardware_module_state->message() == nullptr) {
    return "";
  }
  return reinterpret_cast<const char*>(
      hardware_module_state->message()->Data());
}

}  // namespace intrinsic_fbs
