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

#ifndef ICON_HAL_INTERFACES_JOINT_COMMAND_UTILS_H_
#define ICON_HAL_INTERFACES_JOINT_COMMAND_UTILS_H_

#include <cstdint>

#include "flatbuffers/detached_buffer.h"
#include "flatbuffer_definitions/icon/hal/interfaces/joint_command.fbs.h"
#include "icon/utils/attributes.h"
#include "icon/utils/status.h"

namespace intrinsic_fbs {

INTR_MUST_USE_RESULT flatbuffers::DetachedBuffer BuildJointPositionCommand(
    uint32_t num_dof);

INTR_MUST_USE_RESULT flatbuffers::DetachedBuffer BuildJointVelocityCommand(
    uint32_t num_dof);

INTR_MUST_USE_RESULT flatbuffers::DetachedBuffer BuildJointTorqueCommand(
    uint32_t num_dof);

INTR_MUST_USE_RESULT flatbuffers::DetachedBuffer
BuildJointAccelerationAndTorqueCommand(uint32_t num_dof);

INTR_MUST_USE_RESULT flatbuffers::DetachedBuffer BuildHandGuidingCommand();

INTR_MUST_USE_RESULT intrinsic::RealtimeStatus CopyTo(
    const JointPositionCommand& src, JointPositionCommand& dest);

}  // namespace intrinsic_fbs
#endif  // ICON_HAL_INTERFACES_JOINT_COMMAND_UTILS_H_
