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

#ifndef ICON_INTERPROCESS_SHARED_MEMORY_MANAGER_TESTING_UNIQUE_SEGMENT_NAME_H_
#define ICON_INTERPROCESS_SHARED_MEMORY_MANAGER_TESTING_UNIQUE_SEGMENT_NAME_H_

#include <string>

#include "icon/interprocess/shared_memory_manager/memory_segment.h"

namespace intrinsic::icon {

// Generates a unique hardware module name. This helps avoid memory segment
// naming collisions in tests.
std::string UniqueHardwareModuleName();

// Generates a unique shared memory namespace. This helps avoid memory segment
// naming collisions in tests.
std::string UniqueMemoryNamespace();

}  // namespace intrinsic::icon
#endif  // ICON_INTERPROCESS_SHARED_MEMORY_MANAGER_TESTING_UNIQUE_SEGMENT_NAME_H_
