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

#ifndef ICON_HAL_INTERFACES_ICON_STATE_UTILS_H_
#define ICON_HAL_INTERFACES_ICON_STATE_UTILS_H_

#include "flatbuffers/detached_buffer.h"
#include "icon/utils/attributes.h"

namespace intrinsic_fbs {

// Initializes `current_cycle` with `std::numeric_limits<uint64_t>::max()`, so
// that the IconState flatbuffer is invalid/inconsistent until it receives its
// first update.
INTR_MUST_USE_RESULT flatbuffers::DetachedBuffer BuildIconState();

}  // namespace intrinsic_fbs
#endif  // ICON_HAL_INTERFACES_ICON_STATE_UTILS_H_
