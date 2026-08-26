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

#include "icon/hal/interfaces/control_period_utils.h"

#include <chrono>
#include <format>
#include <limits>
#include <ratio>
#include <string>
#include <string_view>

#include "flatbuffers/flatbuffer_builder.h"
#include "flatbuffer_definitions/icon/hal/interfaces/control_period.fbs.h"
#include "icon/hal/hardware_interface_handle.h"
#include "icon/utils/log.h"
#include "icon/utils/status.h"
#include "icon/utils/time.h"

namespace intrinsic_fbs {

flatbuffers::DetachedBuffer BuildControlPeriod() {
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(builder.CreateStruct(ControlPeriod(0)));
  return builder.Release();
}

std::string FormatControlPeriodMismatchError(std::string_view module_name,
                                             std::chrono::nanoseconds expected,
                                             std::chrono::nanoseconds actual) {
  double expected_hz = std::numeric_limits<double>::quiet_NaN();
  if (expected > std::chrono::nanoseconds::zero()) {
    expected_hz = static_cast<double>(std::nano::den) / expected.count();
  }

  double actual_hz = std::numeric_limits<double>::quiet_NaN();
  if (actual > std::chrono::nanoseconds::zero()) {
    actual_hz = static_cast<double>(std::nano::den) / actual.count();
  }

  return std::format(
      "Inconsistent configuration with Hardware Module '{}'."
      " ICON ('control_frequency_hz'): {} ns ({:.1f} Hz), Hardware Module "
      "reports: {} ns ({:.1f} Hz). Check your configuration.",
      module_name, expected.count(), expected_hz, actual.count(), actual_hz);
}

}  // namespace intrinsic_fbs

namespace intrinsic::icon {

Status UpdateControlPeriod(
    MutableHardwareInterfaceHandle<intrinsic_fbs::ControlPeriod>& handle,
    std::chrono::nanoseconds duration, const log::Logger* logger) {
  if (duration <= std::chrono::nanoseconds::zero()) {
    return FormatStatus(StatusCode::kFailedPrecondition,
                        "Control period must be > 0, got {:d} ns. Check your "
                        "configuration.",
                        duration.count());
  }
  handle->mutate_control_period_ns(duration.count());
  handle.UpdatedAt(Now(), logger);
  return OkStatus();
}

}  // namespace intrinsic::icon
