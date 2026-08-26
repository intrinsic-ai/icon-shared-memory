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

#include "icon/interprocess/shared_memory_manager/testing/unique_segment_name.h"

#include <cstdint>
#include <random>
#include <sstream>
#include <string>

namespace intrinsic::icon {
namespace {
std::string UniqueName() {
  std::random_device rd;
  std::default_random_engine engine(rd());
  std::uniform_int_distribution<uint64_t> distrib;
  return (std::stringstream() << std::hex << distrib(engine)).str();
}
}  // namespace

std::string UniqueHardwareModuleName() { return UniqueName(); }

std::string UniqueMemoryNamespace() { return UniqueName(); }

}  // namespace intrinsic::icon
