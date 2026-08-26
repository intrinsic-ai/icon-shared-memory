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

#include "icon/hal/hardware_interface_registry.h"

#include <stdint.h>

#include <cstring>
#include <string>
#include <string_view>

#include "flatbuffers/detached_buffer.h"
#include "icon/interprocess/shared_memory_manager/shared_memory_manager.h"
#include "icon/utils/status.h"
#include "icon/utils/status_and_expected_macros.h"

namespace intrinsic::icon {

HardwareInterfaceRegistry::HardwareInterfaceRegistry(
    SharedMemoryManager& shared_memory_manager)
    : shm_manager_(&shared_memory_manager) {}

Status HardwareInterfaceRegistry::AdvertiseInterfaceT(
    std::string_view interface_name, bool must_be_used,
    const flatbuffers::DetachedBuffer& buffer, std::string_view type_id) {
  // Create a shared memory segment that is big enough to hold the SegmentHeader
  // and the flatbuffer payload.
  // TODO: Make `AddSegment` return a pointer to the allocated
  // data when being created successfully.

  INTR_RETURN_STATUS_IF_ERROR(shm_manager_->AddSegment(
      interface_name, must_be_used, buffer.size(), std::string(type_id)));
  uint8_t* const shm_data = shm_manager_->GetRawValue(interface_name);
  std::memcpy(shm_data, buffer.data(), buffer.size());

  return OkStatus();
}
}  // namespace intrinsic::icon
