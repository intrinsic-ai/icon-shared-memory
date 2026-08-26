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

#ifndef ICON_INTERPROCESS_SHARED_MEMORY_MANAGER_SEGMENT_INFO_UTILS_H_
#define ICON_INTERPROCESS_SHARED_MEMORY_MANAGER_SEGMENT_INFO_UTILS_H_
// This header file provides a few convenience functions to access general
// information about a set of shared memory data.
// It specifically provides access to a `SegmentInfo` struct as defined in
// `segment_info.fbs`.

#include <string>
#include <vector>

#include "flatbuffer_definitions/icon/interprocess/shared_memory_manager/segment_info.fbs.h"
#include "icon/utils/status.h"
#include "tl/expected.hpp"

namespace intrinsic::icon {

// Extracts the SegmentNames from the SegmentInfo struct.
tl::expected<std::vector<std::string>, Status> GetNamesFromSegmentInfo(
    const intrinsic_fbs::SegmentInfo& segment_info);

// Extracts the SegmentNames from the FileDescriptorNames struct.
tl::expected<std::vector<std::string>, Status> GetNamesFromFileDescriptorNames(
    const intrinsic_fbs::FileDescriptorNames& file_descriptor_names);

// Extracts the SegmentNames that are marked as required from the SegmentInfo
// struct.
tl::expected<std::vector<std::string>, Status>
GetRequiredInterfaceNamesFromSegmentInfo(
    const intrinsic_fbs::SegmentInfo& segment_info);

}  // namespace intrinsic::icon
#endif  // ICON_INTERPROCESS_SHARED_MEMORY_MANAGER_SEGMENT_INFO_UTILS_H_
