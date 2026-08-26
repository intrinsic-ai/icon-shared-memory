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

#ifndef ICON_HAL_INTERFACES_JOINT_LIMITS_UTILS_H_
#define ICON_HAL_INTERFACES_JOINT_LIMITS_UTILS_H_

#include <cstdint>

#include "flatbuffers/detached_buffer.h"
#include "flatbuffer_definitions/icon/hal/interfaces/joint_limits.fbs.h"
#include "icon/utils/attributes.h"
#include "icon/utils/status.h"
#include "kinematics/types/joint_limits.h"

namespace intrinsic_fbs {

INTR_MUST_USE_RESULT flatbuffers::DetachedBuffer BuildJointLimits(
    uint32_t num_dof);

}  // namespace intrinsic_fbs

namespace intrinsic::icon {

// Copies a JointLimits struct to a JointLimits flatbuffer. Fails if the number
// of joints in the struct does not match the size of the flatbuffer.
INTR_MUST_USE_RESULT RealtimeStatus
CopyTo(const JointLimits& limits, intrinsic_fbs::JointLimits& fb_limits);

// Copies a JointLimits flatbuffer to a JointLimits struct.
INTR_MUST_USE_RESULT RealtimeStatus
CopyTo(const intrinsic_fbs::JointLimits& fb_limits, JointLimits& limits);

}  // namespace intrinsic::icon
#endif  // ICON_HAL_INTERFACES_JOINT_LIMITS_UTILS_H_
