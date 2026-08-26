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

#include "icon/interprocess/shared_memory_manager/domain_socket_utils.h"

#include <fcntl.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#include "flatbuffer_definitions/icon/interprocess/shared_memory_manager/segment_info.fbs.h"
#include "icon/interprocess/shared_memory_manager/segment_info_utils.h"
#include "icon/utils/cleanup.h"
#include "icon/utils/log.h"
#include "icon/utils/status.h"
#include "icon/utils/strerror.h"
#include "icon/utils/time.h"
#include "tl/expected.hpp"

namespace intrinsic::icon {

namespace {

constexpr std::string_view kDomainSocketDirectory = "/tmp/intrinsic_icon";

// Validates that the socket path fits into `sockaddr_un`.
Status PathLengthIsValidForSockaddrUn(std::filesystem::path socket_path) {
  // sockaddr_un::sun_path requires a null terminator.
  const size_t kMaxSocketPathLength = sizeof(sockaddr_un::sun_path) - 1;
  if (socket_path.native().length() > kMaxSocketPathLength) {
    return FormatStatus(StatusCode::kInvalidArgument,
                        "Socket path is too long. Got: '{}' with length {}, "
                        "but max length is {}",
                        socket_path.native(), socket_path.native().length(),
                        kMaxSocketPathLength);
  }
  return OkStatus();
}

Status ConnectToServer(int to_server_sock,
                       std::filesystem::path absolute_socket_path,
                       Time deadline, const log::Logger* logger) {
  auto addr =
      domain_socket_internal::AddressFromAbsolutePath(absolute_socket_path);
  if (!addr) {
    return addr.error();
  }

  bool logged_connection_retry = false;
  do {
    if (::connect(to_server_sock, (sockaddr*)&(addr.value()),
                  sizeof(sockaddr_un)) == 0) {
      INTRINSIC_SHARED_MEMORY_LOG(INFO, logger, "Connected to server.");
      return OkStatus();
    }

    if (!logged_connection_retry) {
      INTRINSIC_SHARED_MEMORY_LOG(WARNING, logger,
                                  "Failed to connect to socket '{:s}' with "
                                  "error: '{:s}'. Retrying until {:s}",
                                  absolute_socket_path.native(),
                                  intrinsic::StrError(errno).data(),
                                  FormatTime(deadline).data());
      logged_connection_retry = true;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  } while (Now() < deadline);

  INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger,
                              "Failed to connect to socket before {:s}.",
                              FormatTime(deadline).data());
  return FormatStatus(StatusCode::kDeadlineExceeded,
                      "Failed to connect to socket '{}' before {}.",
                      absolute_socket_path.native(),
                      std::string_view(FormatTime(deadline).data()));
}

}  // namespace

namespace domain_socket_internal {

tl::expected<std::filesystem::path, Status> AbsoluteSocketPath(
    std::filesystem::path absolute_path, std::string_view module_name) {
  if (!absolute_path.is_absolute()) {
    return tl::unexpected(
        FormatStatus(StatusCode::kInvalidArgument,
                     "The path `socket_directory` must be absolute. Got: {}",
                     absolute_path.native()));
  }

  std::filesystem::path full_socket_path =
      (absolute_path / module_name)
          .concat(domain_socket_internal::kSocketSuffix);

  if (auto status = PathLengthIsValidForSockaddrUn(full_socket_path);
      !status.ok()) {
    return tl::unexpected(status);
  }
  return full_socket_path;
}

Status CreateSocketDirectory(std::filesystem::path absolute_path) {
  if (!absolute_path.is_absolute()) {
    return FormatStatus(StatusCode::kInvalidArgument,
                        "The path `socket_directory` must be absolute. Got: {}",
                        absolute_path.native());
  }

  {
    std::error_code ec;
    std::filesystem::create_directories(absolute_path, ec);
    if (ec) {
      return FormatStatus(
          StatusCode::kInternal,
          "Failed to create socket directory '{}'. Error code {}: {} ({})",
          absolute_path.native(), ec.category().name(), ec.value(),
          ec.message());
    }
  }

  return OkStatus();
}

tl::expected<sockaddr_un, Status> AddressFromAbsolutePath(
    std::filesystem::path absolute_socket_path) {
  sockaddr_un addr;
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;

  if (auto status = PathLengthIsValidForSockaddrUn(absolute_socket_path);
      !status.ok()) {
    return tl::unexpected(status);
  }
  // snprintf always includes the null terminator as required by
  // sockaddr_un::sun_path.
  if (std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s",
                    absolute_socket_path.c_str()) < 0) {
    return tl::unexpected(FormatStatus(
        StatusCode::kInternal,
        "Failed to copy socket path '{}' to sockaddr_un struct with error: {}",
        absolute_socket_path.native(), intrinsic::StrError(errno).data()));
  }
  return addr;
}

}  // namespace domain_socket_internal

// Receives a single message from the open socket `to_server_sock`.
// Returns InternalError if a received file descriptor is not valid.
// Returns InternalError on parsing errors.
// Returns InternalError when the size of the received message is wrong.
// Returns FailedPreconditionError when the socket protocol version of the
// message doesn't match domain_socket_internal::kDomainSocketProtocolVersion.
tl::expected<domain_socket_internal::ShmDescriptors, Status> GetSingleMessage(
    int to_server_sock, const log::Logger* logger) {
  domain_socket_internal::ShmDescriptors descriptors;
  descriptors.file_descriptors_in_order.reserve(
      domain_socket_internal::kMaxFdsPerMessage);

  constexpr size_t kExpectedBytes = sizeof(descriptors.transfer_data);

  // Allocates size for the max number of fds.
  std::array<char, CMSG_SPACE(domain_socket_internal::kMaxFdsPerMessage *
                              sizeof(int))>
      cmsg_buf{0};

  iovec iov{.iov_base = (char*)(&descriptors.transfer_data),
            .iov_len = kExpectedBytes};

  msghdr msgh{
      .msg_name = nullptr,
      .msg_namelen = 0,
      .msg_iov = &iov,
      .msg_iovlen = 1,
      // ancillary data = file descriptors.
      .msg_control = cmsg_buf.data(),
      .msg_controllen = cmsg_buf.size(),
      .msg_flags = 0,
  };

  // Copy of the first message header, with the pointer to the ancillary data
  // that contains the file descriptors.
  msghdr first_msghdr = msgh;
  size_t received_bytes_sum = 0;

  // Receiving a single message can require multiple calls to recvmsg
  // https://gist.github.com/kentonv/bc7592af98c68ba2738f4436920868dc
  while (received_bytes_sum < kExpectedBytes) {
    if (received_bytes_sum > 0) {
      // recvmsg() transmits the control message (which contains the file
      // descriptors) on the first call. For subsequent calls, we set the
      // msg_control and msg_controllen fields of msgh to zero so recvmsg() does
      // not overwrite the control message.
      //
      // Because we made a copy of msgh before the loop, we can still access the
      // control message later.
      msgh.msg_control = nullptr;
      msgh.msg_controllen = 0;
    }

    // Returns the number of bytes received.
    ssize_t received_bytes = recvmsg(to_server_sock, &msgh, 0);
    if (received_bytes == -1) {
      return tl::unexpected(FormatStatus(
          StatusCode::kInternal, "Failed to receive data with error: {}",
          intrinsic::StrError(errno).data()));
    }

    if (received_bytes == 0) {
      return tl::unexpected(Status{
          .code = StatusCode::kInternal,
          .message = {"No data received. Likely due to shutdown."},
      });
    }

    received_bytes_sum += received_bytes;
    if ((received_bytes_sum) > kExpectedBytes) {
      return tl::unexpected(
          FormatStatus(StatusCode::kOutOfRange,
                       "Received {} bytes but only expected {} bytes",
                       received_bytes_sum, kExpectedBytes));
    }

    msgh.msg_iov->iov_base =
        (char*)(&descriptors.transfer_data) + received_bytes_sum;
    msgh.msg_iov->iov_len = kExpectedBytes - received_bytes_sum;
  }

  // // Returns error instead of retrying because the data is transmitted as a
  // // single message.
  if (received_bytes_sum != kExpectedBytes) {
    return tl::unexpected(FormatStatus(
        StatusCode::kInternal, "Received {} bytes but only expected {} bytes",
        received_bytes_sum, kExpectedBytes));
  }

  if (descriptors.transfer_data.domain_socket_protocol_version !=
      domain_socket_internal::kDomainSocketProtocolVersion) {
    return tl::unexpected(FormatStatus(
        StatusCode::kFailedPrecondition,
        "Incompatible domain socket protocol version. Got: {}, expected {}.",
        descriptors.transfer_data.domain_socket_protocol_version,
        domain_socket_internal::kDomainSocketProtocolVersion));
  }

  const int kNumNames = descriptors.transfer_data.file_descriptor_names.size();
  const auto segment_names = GetNamesFromFileDescriptorNames(
      descriptors.transfer_data.file_descriptor_names);
  if (!segment_names) {
    return tl::unexpected(segment_names.error());
  }

  // Parses fd data.
  struct cmsghdr* cmsg = CMSG_FIRSTHDR(&first_msghdr);
  if (!cmsg) {
    return tl::unexpected(Status{
        .code = StatusCode::kInternal,
        .message = {"No control message received"},
    });
  }

  if (cmsg->cmsg_len != CMSG_LEN(kNumNames * sizeof(int))) {
    return tl::unexpected(
        FormatStatus(StatusCode::kInternal,
                     "Unexpected size of control message ({}/{}). Expected {} "
                     "bytes, got {} bytes.",
                     descriptors.transfer_data.message_index,
                     descriptors.transfer_data.num_messages,
                     CMSG_LEN(kNumNames * sizeof(int)), cmsg->cmsg_len));
  }

  if (cmsg->cmsg_level != SOL_SOCKET) {
    return tl::unexpected(
        FormatStatus(StatusCode::kInternal,
                     "Unexpected level of control message. Expected {}, got {}",
                     SOL_SOCKET, cmsg->cmsg_level));
  }
  if (cmsg->cmsg_type != SCM_RIGHTS) {
    return tl::unexpected(FormatStatus(
        StatusCode::kInternal,
        "Unexpected type of control message. Expected '{}', got '{}'",
        static_cast<std::underlying_type_t<decltype(SCM_RIGHTS)>>(SCM_RIGHTS),
        cmsg->cmsg_type));
  }
  descriptors.file_descriptors_in_order.resize(kNumNames);
  // https://man7.org/linux/man-pages/man7/unix.7.html
  // If the number of file descriptors received in the ancillary data cause
  // the process to exceed its RLIMIT_NOFILE resource limit, the excess file
  // descriptors are automatically closed in the receiving process. One cannot
  // split the list over multiple recvmsg calls.
  for (int i = 0; i < kNumNames; ++i) {
    int fd = reinterpret_cast<int*>(CMSG_DATA(cmsg))[i];
    if (fcntl(fd, F_GETFD) == -1) {
      return tl::unexpected(
          FormatStatus(StatusCode::kInternal,
                       "File descriptor for segment '{}' is not valid. Either "
                       "the receiving process can't open any more files, or "
                       "the server sent a closed file descriptor. Error: {}",
                       (*segment_names)[i], intrinsic::StrError(errno).data()));
    }

    descriptors.file_descriptors_in_order[i] = fd;
  }
  INTRINSIC_SHARED_MEMORY_LOG(
      INFO, logger,
      "Received {:d} valid file descriptors in message {:d} of {:d}.",
      kNumNames, descriptors.transfer_data.message_index,
      descriptors.transfer_data.num_messages);

  return descriptors;
}

tl::expected<SegmentNameToFileDescriptorMap, Status>
GetSegmentNameToFileDescriptorMap(std::filesystem::path socket_directory,
                                  std::string_view module_name,
                                  std::chrono::seconds connection_timeout,
                                  const log::Logger* logger) {
  Time deadline = Now() + connection_timeout;
  int to_server_sock = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (to_server_sock == -1) {
    return tl::unexpected(FormatStatus(
        StatusCode::kInternal,
        "Failed to create GetShmDescriptors client socket with error: {}",
        intrinsic::StrError(errno).data()));
  }
  // Closes the socket on exit.
  Cleanup close_socket([to_server_sock, logger]() noexcept {
    if (::close(to_server_sock) == -1) {
      INTRINSIC_SHARED_MEMORY_LOG(
          ERROR, logger,
          "Failed to close GetShmDescriptors client socket with error: {:s}.",
          intrinsic::StrError(errno).data());
    }
  });

  if (auto status =
          domain_socket_internal::CreateSocketDirectory(socket_directory);
      !status.ok()) {
    return tl::unexpected(status);
  }

  auto absolute_socket_path =
      domain_socket_internal::AbsoluteSocketPath(socket_directory, module_name);
  if (!absolute_socket_path) {
    return tl::unexpected(absolute_socket_path.error());
  }

  if (auto status = ConnectToServer(to_server_sock, *absolute_socket_path,
                                    deadline, logger);
      !status.ok()) {
    return tl::unexpected(status);
  }

  // The socket blocks at most for one Second if no data is received.
  // This stops a misbehaving server from doing damage.
  struct timeval socket_receive_timeout{
      .tv_sec = 1,
      .tv_usec = 0,
  };
  if (::setsockopt(to_server_sock, SOL_SOCKET, SO_RCVTIMEO,
                   (const char*)&socket_receive_timeout,
                   sizeof socket_receive_timeout) == -1) {
    return tl::unexpected(FormatStatus(
        StatusCode::kInternal, "Failed to set socket timeout with error: {}",
        intrinsic::StrError(errno).data()));
  }

  SegmentNameToFileDescriptorMap segment_name_to_file_descriptor_map;
  // Reserves enough data for at least one message.
  // Reserves more data below once we know how many messages are expected.
  segment_name_to_file_descriptor_map.reserve(
      domain_socket_internal::kMaxFdsPerMessage);

  size_t remaining_messages = 0;
  size_t expected_message_index = 1;
  size_t expected_num_messages = 0;
  do {
    const auto message = GetSingleMessage(to_server_sock, logger);
    if (!message) {
      return tl::unexpected(message.error());
    }

    if (expected_message_index != message->transfer_data.message_index) {
      return tl::unexpected(FormatStatus(
          StatusCode::kInternal, "Expected message with index {} but got {}",
          expected_message_index, message->transfer_data.message_index));
    }
    // Reserve the correct space after receiving the first message.
    if (expected_message_index == 1) {
      expected_num_messages = message->transfer_data.num_messages;

      segment_name_to_file_descriptor_map.reserve(
          domain_socket_internal::kMaxFdsPerMessage * expected_num_messages);
    }

    if (expected_num_messages != message->transfer_data.num_messages) {
      return tl::unexpected(FormatStatus(
          StatusCode::kFailedPrecondition,
          "Expected {} messages, but message {} claims that there are {}.",
          expected_num_messages, message->transfer_data.message_index,
          message->transfer_data.num_messages));
    }
    remaining_messages = expected_num_messages - expected_message_index;
    expected_message_index++;

    auto names = GetNamesFromFileDescriptorNames(
        message->transfer_data.file_descriptor_names);
    if (!names) {
      return tl::unexpected(names.error());
    }

    if (names->size() != message->file_descriptors_in_order.size()) {
      return tl::unexpected(FormatStatus(
          StatusCode::kFailedPrecondition,
          "Names and file descriptors have different sizes. Got: {} names "
          "and "
          "{} file descriptors",
          names->size(), message->file_descriptors_in_order.size()));
    }

    for (size_t i = 0; i < names->size(); ++i) {
      segment_name_to_file_descriptor_map[names->at(i)] =
          message->file_descriptors_in_order[i];
    }
  } while (remaining_messages > 0);

  return segment_name_to_file_descriptor_map;
}

std::filesystem::path SocketDirectoryFromNamespace(
    std::string_view shared_memory_namespace) {
  auto socket_base_path = std::filesystem::path(kDomainSocketDirectory);
  if (shared_memory_namespace.empty()) {
    return socket_base_path;
  }
  // Ensure that we **don't** replace the base path if
  // `shared_memory_namespace` happens to be an absolute path (i.e. start with
  // a directory separator).
  //
  // But at the same time, we do want a separator if `shared_memory_namespace`
  // *doesn't* start with one. Appending the empty string to
  // `socket_base_path` ensures that there's a separator before
  // `shared_memory_namespace`.
  return (socket_base_path / "").concat(shared_memory_namespace);
}

}  // namespace intrinsic::icon
