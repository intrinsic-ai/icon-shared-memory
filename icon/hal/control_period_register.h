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

#ifndef ICON_HAL_CONTROL_PERIOD_REGISTER_H_
#define ICON_HAL_CONTROL_PERIOD_REGISTER_H_

#include "flatbuffer_definitions/icon/hal/interfaces/control_period.fbs.h"
#include "icon/hal/hardware_interface_traits.h"
#include "icon/hal/interfaces/control_period_utils.h"

namespace intrinsic::icon {

// Reserved name of the control period interface.
inline constexpr char kControlPeriodInterfaceName[] = "control_period";

namespace hardware_interface_traits {

// Registers the `ControlPeriod` hardware interface.
// If you include this header, you can advertise `ControlPeriod` segments with
// HardwareInterfaceRegistry.
//
// Usage:
// #include "icon/hal/control_period_register.h"  // IWYU pragma: keep
INTRINSIC_ADD_HARDWARE_INTERFACE(intrinsic_fbs::ControlPeriod,
                                 intrinsic_fbs::BuildControlPeriod,
                                 "intrinsic_fbs.ControlPeriod")
}  // namespace hardware_interface_traits
}  // namespace intrinsic::icon

#endif  // ICON_HAL_CONTROL_PERIOD_REGISTER_H_
