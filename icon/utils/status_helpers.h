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

#ifndef ICON_UTILS_STATUS_HELPERS_H_
#define ICON_UTILS_STATUS_HELPERS_H_

#include "icon/utils/status.h"

namespace intrinsic {

// Returns `new_status` if it is non-OK, `previous_status` otherwise.
//
// Example (Foo() will return the last non-Ok status returned by Bar1(), Bar2()
// or Bar3()):
//
//    RealtimeStatus Bar1();
//    RealtimeStatus Bar2();
//    RealtimeStatus Bar3();
//
//    RealtimeStatus Foo() {
//      RealtimeStatus status = Bar1();
//      status = OverwriteIfError(status, Bar2());
//      status = OverwriteIfError(status, Bar3());
//      return status;
//    }
constexpr RealtimeStatus OverwriteIfError(const RealtimeStatus& previous_status,
                                          const RealtimeStatus& new_status) {
  if (!new_status.ok()) {
    return new_status;
  }
  return previous_status;
}

// Returns `new_status` if it is non-OK, `previous_status` otherwise.
//
// Example (Foo() will return the last non-Ok status returned by Bar1(), Bar2()
// or Bar3()):
//
//    Status Bar1();
//    Status Bar2();
//    Status Bar3();
//
//    Status Foo() {
//      Status status = Bar1();
//      status = OverwriteIfError(status, Bar2());
//      status = OverwriteIfError(status, Bar3());
//      return status;
//    }
constexpr Status OverwriteIfError(const Status& previous_status,
                                  const Status& new_status) {
  if (!new_status.ok()) {
    return new_status;
  }
  return previous_status;
}

// Returns `new_status` if `previous_status` is OK.
// Used to capture the first non-OK status in a list of sequential calls.
//
// Example (Foo() will return the first non-OK status returned by Bar1(), Bar2()
// or Bar3()):
//    RealtimeStatus Bar1();
//    RealtimeStatus Bar2();
//    RealtimeStatus Bar3();
//
//    RealtimeStatus Foo() {
//      RealtimeStatus status = Bar1();
//      status = OverwriteIfNotError(status, Bar2());
//      status = OverwriteIfNotError(status, Bar3());
//      return status;
//    }
constexpr RealtimeStatus OverwriteIfNotError(
    const RealtimeStatus& previous_status, const RealtimeStatus& new_status) {
  if (previous_status.ok()) {
    return new_status;
  }
  return previous_status;
}

// Returns `new_status` if `previous_status` is OK.
// Used to capture the first non-OK status in a list of sequential calls.
//
// Example (Foo() will return the first non-OK status returned by Bar1(), Bar2()
// or Bar3()):
//    Status Bar1();
//    Status Bar2();
//    Status Bar3();
//
//    Status Foo() {
//      Status status = Bar1();
//      status = OverwriteIfNotError(status, Bar2());
//      status = OverwriteIfNotError(status, Bar3());
//      return status;
//    }
constexpr Status OverwriteIfNotError(const Status& previous_status,
                                     const Status& new_status) {
  if (previous_status.ok()) {
    return new_status;
  }
  return previous_status;
}
}  // namespace intrinsic

#endif  // ICON_UTILS_STATUS_HELPERS_H_
