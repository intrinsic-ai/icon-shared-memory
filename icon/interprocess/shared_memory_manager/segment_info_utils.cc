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

#include "icon/interprocess/shared_memory_manager/segment_info_utils.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "flatbuffer_definitions/icon/interprocess/shared_memory_manager/segment_info.fbs.h"
#include "icon/flatbuffers/fixed_string.h"
#include "icon/utils/status.h"
#include "tl/expected.hpp"

namespace intrinsic::icon {

namespace {

inline static tl::expected<std::string, Status> InterfaceNameFromSegment(
    const intrinsic_fbs::SegmentName& name) {
  auto status_or_name = intrinsic_fbs::StringView(name.value());
  if (!status_or_name) {
    return tl::unexpected(ToStatus(status_or_name.error()));
  }
  return std::string(*status_or_name);
}

}  // namespace

tl::expected<std::vector<std::string>, Status> GetNamesFromSegmentInfo(
    const intrinsic_fbs::SegmentInfo& segment_info) {
  std::vector<std::string> names;
  names.reserve(segment_info.size());

  for (uint32_t i = 0; i < segment_info.size(); ++i) {
    auto name = InterfaceNameFromSegment(*segment_info.names()->Get(i));
    if (!name) {
      return tl::unexpected(name.error());
    }
    names.emplace_back(*name);
  }

  return names;
}

tl::expected<std::vector<std::string>, Status> GetNamesFromFileDescriptorNames(
    const intrinsic_fbs::FileDescriptorNames& file_descriptor_names) {
  std::vector<std::string> names;
  names.reserve(file_descriptor_names.size());

  for (uint32_t i = 0; i < file_descriptor_names.size(); ++i) {
    auto name =
        InterfaceNameFromSegment(*file_descriptor_names.names()->Get(i));
    if (!name) {
      return tl::unexpected(name.error());
    }
    names.emplace_back(*name);
  }

  return names;
}

tl::expected<std::vector<std::string>, Status>
GetRequiredInterfaceNamesFromSegmentInfo(
    const intrinsic_fbs::SegmentInfo& segment_info) {
  std::vector<std::string> names;
  names.reserve(segment_info.size());

  for (uint32_t i = 0; i < segment_info.size(); ++i) {
    const intrinsic_fbs::SegmentName* name = segment_info.names()->Get(i);
    if (!name->must_be_used()) {
      continue;
    }
    auto interface_name = InterfaceNameFromSegment(*name);
    if (!interface_name) {
      return tl::unexpected(interface_name.error());
    }
    names.emplace_back(*interface_name);
  }

  return names;
}

}  // namespace intrinsic::icon
