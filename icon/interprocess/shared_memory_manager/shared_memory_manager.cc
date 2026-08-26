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

#include "icon/interprocess/shared_memory_manager/shared_memory_manager.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "icon/flatbuffers/flatbuffer_utils.h"
#include "icon/utils/cleanup.h"
#include "icon/utils/status.h"
#include "icon/utils/status_and_expected_macros.h"
#include "icon/utils/strerror.h"
#include "tl/expected.hpp"

namespace intrinsic::icon {

using ::intrinsic_fbs::FlatbufferArrayNumElements;

// Max string size as defined in `segment_info.fbs`
inline constexpr size_t kMaxSegmentStringSize =
    FlatbufferArrayNumElements(&intrinsic_fbs::SegmentName::value);

inline constexpr size_t kMaxNumberOfSegments =
    FlatbufferArrayNumElements(&intrinsic_fbs::SegmentInfo::names);

namespace {
Status VerifyName(std::string_view name) {
  if (name.empty()) {
    return {
        .code = StatusCode::kInvalidArgument,
        .message = {"Shm segment name cannot be empty."},
    };
  }
  if (name.size() >= kMaxSegmentStringSize) {
    return FormatStatus(StatusCode::kInvalidArgument,
                        "Shm segment name '{}' can't exceed {} characters.",
                        name, kMaxSegmentStringSize - 1);
  }

  if (std::find(name.begin(), name.end(), '/') != name.end()) {
    return FormatStatus(StatusCode::kInvalidArgument,
                        "Shm segment name '{}' can't have forward slashes.",
                        name);
  }

  return OkStatus();
}

intrinsic_fbs::SegmentInfo SegmentInfoFromHashMap(
    const std::unordered_map<
        std::string, SharedMemoryManager::MemorySegmentInfo>& segments) {
  intrinsic_fbs::SegmentInfo segment_info(segments.size());
  uint32_t index = 0;
  for (const auto& [memory_name, buf] : segments) {
    intrinsic_fbs::SegmentName segment;
    segment.mutate_must_be_used(buf.must_be_used);
    // fbs doesn't have char as datatype, only int8_t which is byte compatible.
    auto* data = reinterpret_cast<char*>(segment.mutable_value()->Data());
    std::memset(data, '\0', kMaxSegmentStringSize);
    std::snprintf(data, kMaxSegmentStringSize, "%s", memory_name.c_str());
    segment_info.mutable_names()->Mutate(index, segment);
    ++index;
  }

  return segment_info;
}
}  // namespace

SharedMemoryManager::SharedMemoryManager(
    std::string_view module_name, std::string_view shared_memory_namespace,
    const log::Logger* logger)
    : logger_(logger),
      module_name_(std::string(module_name)),
      shared_memory_namespace_(std::string(shared_memory_namespace)) {}

// static
tl::expected<std::unique_ptr<SharedMemoryManager>, Status>
SharedMemoryManager::Create(std::string_view shared_memory_namespace,
                            std::string_view module_name,
                            const log::Logger* logger) {
  if (module_name.empty()) {
    return tl::unexpected(Status{
        .code = StatusCode::kInvalidArgument,
        .message = {"Module name can't be empty."},
    });
  }

  return std::unique_ptr<SharedMemoryManager>(new SharedMemoryManager(
      /*module_name=*/module_name,
      /*shared_memory_namespace=*/shared_memory_namespace,
      /*logger=*/logger));
}

SharedMemoryManager::~SharedMemoryManager() {
  // unlink all created shm segments
  for (const auto& segment : memory_segments_) {
    auto* header = reinterpret_cast<SegmentHeader*>(segment.second.data);
    int fd = segment.second.fd;
    const std::string segment_name = segment.first;

    // We've used placement new during the initialization. We have to call the
    // destructor explicitly to cleanup.
    header->~SegmentHeader();

    if (::close(fd) == -1) {
      INTRINSIC_SHARED_MEMORY_LOG(WARNING, logger_,
                                  "Failed to close shm_fd for '{:s}' with "
                                  "error: {:s}. Continuing anyways.",
                                  segment_name,
                                  intrinsic::StrError(errno).data());
    }
    if (segment.second.data != nullptr) {
      if (::munmap(segment.second.data, segment.second.length) == -1) {
        INTRINSIC_SHARED_MEMORY_LOG(WARNING, logger_,
                                    "Failed to unmap memory for '{:s}' with "
                                    "error: {:s}. Continuing anyways.",
                                    segment_name,
                                    intrinsic::StrError(errno).data());
      }
    }
  }
}

const SegmentHeader* SharedMemoryManager::GetSegmentHeader(
    std::string_view name) {
  uint8_t* header = GetRawSegment(name);
  return reinterpret_cast<SegmentHeader*>(header);
}

Status SharedMemoryManager::InitSegment(std::string_view name,
                                        bool must_be_used, size_t payload_size,
                                        const std::string& type_id) {
  if (memory_segments_.size() >= kMaxNumberOfSegments) {
    return FormatStatus(
        StatusCode::kResourceExhausted,
        "Unable to add '{}'. Max number of segments ({}) exceeded.", name,
        kMaxNumberOfSegments);
  }
  if (type_id.size() > SegmentHeader::TypeInfo::kMaxSize) {
    return FormatStatus(
        StatusCode::kInvalidArgument,
        "Type id '{}' for segment {} exceeds max size of {} bytes", type_id,
        name, SegmentHeader::TypeInfo::kMaxSize);
  }
  if (memory_segments_.contains(std::string(name))) {
    return FormatStatus(StatusCode::kAlreadyExists,
                        "Shared memory segment '{}' already exists", name);
  }
  INTR_RETURN_STATUS_IF_ERROR(VerifyName(name));

  // Creates an anonymous memory segment and stores the fd
  // https://man7.org/linux/man-pages/man2/memfd_create.2.html
  // Default flags are O_RDWR | O_LARGEFILE.
  int shm_fd = memfd_create(name.data(), 0);
  if (shm_fd == -1) {
    return FormatStatus(
        StatusCode::kInternal,
        "Failed to create shared memory segment '{}' with error: {}", name,
        intrinsic::StrError(errno).data());
  }

  auto close_fd_on_error = Cleanup([&]() noexcept {
    if (::close(shm_fd) == -1) {
      INTRINSIC_SHARED_MEMORY_LOG(
          WARNING, logger_,
          "Failed to clean up shm_fd for '{:s}' with "
          "error: {:s} (in addition to an error while setting up the segment)",
          name, intrinsic::StrError(errno).data());
    }
  });
  const auto segment_size = sizeof(SegmentHeader) + payload_size;
  if (::ftruncate(shm_fd, segment_size) == -1) {
    // Resizes new shm segments.
    return FormatStatus(
        StatusCode::kInternal,
        "Unable to resize shared memory segment '{}' with error: {}", name,
        intrinsic::StrError(errno).data());
  }

  struct stat shared_memory_stats;
  if (fstat(shm_fd, &shared_memory_stats) != 0) {
    // Return an error and forward errno
    return FormatStatus(
        StatusCode::kInternal,
        "Failed to read size of segment '{}'. fstat() failed with: {}", name,
        intrinsic::StrError(errno).data());
  }
  // The opening logic depends on the size of the segment.
  if (shared_memory_stats.st_size != segment_size) {
    return FormatStatus(StatusCode::kInternal,
                        "The size of the shared memory segment '{}' "
                        "({} bytes) is not the expected size ({} bytes)",
                        name, shared_memory_stats.st_size, segment_size);
  }

  auto* data = static_cast<uint8_t*>(
      ::mmap(nullptr, segment_size, PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_LOCKED, shm_fd, 0));
  if (data == nullptr || data == MAP_FAILED) {
    return FormatStatus(
        StatusCode::kInternal,
        "Unable to map shared memory segment '{}' with error: {}", name,
        intrinsic::StrError(errno).data());
  }

  // Additionally locking the pages as recommended by
  // https://man7.org/linux/man-pages/man2/mmap.2.html, because major faults are
  // not acceptable after the initialization of the mapping.
  if (mlock(/*__addr=*/data, /*__len=*/segment_size) != 0) {
    return FormatStatus(
        StatusCode::kInternal,
        "Unable to mlock shared memory segment '{}' with error: ", name,
        intrinsic::StrError(errno).data());
  }

  const std::string name_str(name);
  segment_name_to_file_descriptor_map_.insert({name_str, shm_fd});
  // We use a placement new operator here to initialize the "raw" segment
  // data correctly.
  new (data) SegmentHeader(type_id);
  memory_segments_.insert({
      name_str,
      {
          .data = data,
          .length = segment_size,
          .must_be_used = must_be_used,
          .fd = shm_fd,
      },
  });
  std::move(close_fd_on_error).Cancel();
  return OkStatus();
}

uint8_t* SharedMemoryManager::GetRawValue(std::string_view name) {
  auto* data = GetRawSegment(name);
  if (data == nullptr) {
    return data;
  }
  return data + sizeof(SegmentHeader);
}

uint8_t* SharedMemoryManager::GetRawSegment(std::string_view name) {
  auto result = memory_segments_.find(std::string(name));
  if (result == memory_segments_.end()) {
    return nullptr;
  }
  return result->second.data;
}

std::vector<std::string> SharedMemoryManager::GetRegisteredMemoryNames() const {
  std::vector<std::string> result;
  result.reserve(memory_segments_.size());
  for (const auto& [name, unused] : memory_segments_) {
    result.push_back(name);
  }
  return result;
}

std::string SharedMemoryManager::ModuleName() const { return module_name_; }

std::string SharedMemoryManager::SharedMemoryNamespace() const {
  return shared_memory_namespace_;
}

intrinsic_fbs::SegmentInfo SharedMemoryManager::GetSegmentInfo() const {
  return SegmentInfoFromHashMap(memory_segments_);
}

}  // namespace intrinsic::icon
