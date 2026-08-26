# Copyright 2026 Intrinsic Innovation LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Automatically generated from BUILD files in /usr/local/google/home/tobit/workspaces/insrc/incode/icon/no_absl

add_library(icon_shared_memory_icon_testing_realtime_annotations INTERFACE)
target_sources(icon_shared_memory_icon_testing_realtime_annotations PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/testing/realtime_annotations.h"
)
target_include_directories(icon_shared_memory_icon_testing_realtime_annotations INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
install(TARGETS icon_shared_memory_icon_testing_realtime_annotations
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/testing/realtime_annotations.h"
        DESTINATION "include/icon/testing"
)

add_library(icon_shared_memory_icon_utils_attributes INTERFACE)
target_sources(icon_shared_memory_icon_utils_attributes PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/attributes.h"
)
target_include_directories(icon_shared_memory_icon_utils_attributes INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
install(TARGETS icon_shared_memory_icon_utils_attributes
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/attributes.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_icon_utils_realtime_guard INTERFACE)
target_sources(icon_shared_memory_icon_utils_realtime_guard PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/realtime_guard.h"
)
target_include_directories(icon_shared_memory_icon_utils_realtime_guard INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
install(TARGETS icon_shared_memory_icon_utils_realtime_guard
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/realtime_guard.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_icon_utils_status INTERFACE)
target_sources(icon_shared_memory_icon_utils_status PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status.h"
)
target_include_directories(icon_shared_memory_icon_utils_status INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_utils_status INTERFACE
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_realtime_guard
  icon_shared_memory_icon_testing_realtime_annotations
)
install(TARGETS icon_shared_memory_icon_utils_status
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_icon_utils_status_and_expected_macros INTERFACE)
target_sources(icon_shared_memory_icon_utils_status_and_expected_macros PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status_and_expected_macros.h"
)
target_include_directories(icon_shared_memory_icon_utils_status_and_expected_macros INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_utils_status_and_expected_macros INTERFACE
  icon_shared_memory_icon_utils_status
  tl::expected
)
install(TARGETS icon_shared_memory_icon_utils_status_and_expected_macros
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status_and_expected_macros.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_icon_utils_status_and_expected_test_macros INTERFACE)
target_sources(icon_shared_memory_icon_utils_status_and_expected_test_macros PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status_and_expected_test_macros.h"
)
target_include_directories(icon_shared_memory_icon_utils_status_and_expected_test_macros INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_utils_status_and_expected_test_macros INTERFACE
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_status_and_expected_macros
  tl::expected
)
install(TARGETS icon_shared_memory_icon_utils_status_and_expected_test_macros
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status_and_expected_test_macros.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_icon_hal_hardware_interface_traits INTERFACE)
target_sources(icon_shared_memory_icon_hal_hardware_interface_traits PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_interface_traits.h"
)
target_include_directories(icon_shared_memory_icon_hal_hardware_interface_traits INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
install(TARGETS icon_shared_memory_icon_hal_hardware_interface_traits
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_interface_traits.h"
        DESTINATION "include/icon/hal"
)

add_library(icon_shared_memory_icon_hal_interfaces_icon_state_fbs_utils STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/icon_state_utils.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/icon_state_utils.h"
)
target_include_directories(icon_shared_memory_icon_hal_interfaces_icon_state_fbs_utils PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_interfaces_icon_state_fbs_utils PUBLIC
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_utils_attributes
  flatbuffers::flatbuffers
)
install(TARGETS icon_shared_memory_icon_hal_interfaces_icon_state_fbs_utils
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/icon_state_utils.h"
        DESTINATION "include/icon/hal/interfaces"
)

add_library(icon_shared_memory_icon_hal_icon_state_register INTERFACE)
target_sources(icon_shared_memory_icon_hal_icon_state_register PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/icon_state_register.h"
)
target_include_directories(icon_shared_memory_icon_hal_icon_state_register INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_icon_state_register INTERFACE
  icon_shared_memory_icon_hal_hardware_interface_traits
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_hal_interfaces_icon_state_fbs_utils
)
install(TARGETS icon_shared_memory_icon_hal_icon_state_register
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/icon_state_register.h"
        DESTINATION "include/icon/hal"
)

add_library(icon_shared_memory_icon_flatbuffers_flatbuffer_utils INTERFACE)
target_sources(icon_shared_memory_icon_flatbuffers_flatbuffer_utils PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/flatbuffers/flatbuffer_utils.h"
)
target_include_directories(icon_shared_memory_icon_flatbuffers_flatbuffer_utils INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_flatbuffers_flatbuffer_utils INTERFACE
  icon_shared_memory_icon_utils_status
  flatbuffers::flatbuffers
)
install(TARGETS icon_shared_memory_icon_flatbuffers_flatbuffer_utils
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/flatbuffers/flatbuffer_utils.h"
        DESTINATION "include/icon/flatbuffers"
)

add_library(icon_shared_memory_icon_flatbuffers_fixed_string INTERFACE)
target_sources(icon_shared_memory_icon_flatbuffers_fixed_string PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/flatbuffers/fixed_string.h"
)
target_include_directories(icon_shared_memory_icon_flatbuffers_fixed_string INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_flatbuffers_fixed_string INTERFACE
  icon_shared_memory_icon_utils_status
  flatbuffers::flatbuffers
  tl::expected
)
install(TARGETS icon_shared_memory_icon_flatbuffers_fixed_string
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/flatbuffers/fixed_string.h"
        DESTINATION "include/icon/flatbuffers"
)

add_library(icon_shared_memory_icon_interprocess_shared_memory_manager_segment_info_utils STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/segment_info_utils.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/segment_info_utils.h"
)
target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_manager_segment_info_utils PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_manager_segment_info_utils PUBLIC
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_flatbuffers_fixed_string
  icon_shared_memory_icon_utils_status
  tl::expected
)
install(TARGETS icon_shared_memory_icon_interprocess_shared_memory_manager_segment_info_utils
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/segment_info_utils.h"
        DESTINATION "include/icon/interprocess/shared_memory_manager"
)

add_library(icon_shared_memory_icon_utils_cleanup INTERFACE)
target_sources(icon_shared_memory_icon_utils_cleanup PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/cleanup.h"
)
target_include_directories(icon_shared_memory_icon_utils_cleanup INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_utils_cleanup INTERFACE
  icon_shared_memory_icon_utils_attributes
)
install(TARGETS icon_shared_memory_icon_utils_cleanup
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/cleanup.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_icon_utils_log INTERFACE)
target_sources(icon_shared_memory_icon_utils_log PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/log.h"
)
target_include_directories(icon_shared_memory_icon_utils_log INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
install(TARGETS icon_shared_memory_icon_utils_log
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/log.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_icon_utils_strerror INTERFACE)
target_sources(icon_shared_memory_icon_utils_strerror PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/strerror.h"
)
target_include_directories(icon_shared_memory_icon_utils_strerror INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
install(TARGETS icon_shared_memory_icon_utils_strerror
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/strerror.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_icon_utils_time STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/time.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/time.h"
)
target_include_directories(icon_shared_memory_icon_utils_time PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
install(TARGETS icon_shared_memory_icon_utils_time
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/time.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/domain_socket_utils.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/domain_socket_utils.h"
)
target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils PUBLIC
  icon_shared_memory_icon_interprocess_shared_memory_manager_segment_info_utils
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_flatbuffers_flatbuffer_utils
  icon_shared_memory_icon_utils_cleanup
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_strerror
  icon_shared_memory_icon_utils_time
)
install(TARGETS icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/domain_socket_utils.h"
        DESTINATION "include/icon/interprocess/shared_memory_manager"
)

add_library(icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/segment_header.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/segment_header.h"
)
target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header PUBLIC
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_time
)
install(TARGETS icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/segment_header.h"
        DESTINATION "include/icon/interprocess/shared_memory_manager"
)

add_library(icon_shared_memory_icon_utils_format INTERFACE)
target_sources(icon_shared_memory_icon_utils_format PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/format.h"
)
target_include_directories(icon_shared_memory_icon_utils_format INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
install(TARGETS icon_shared_memory_icon_utils_format
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/format.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/memory_segment.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/memory_segment.h"
)
target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment PUBLIC
  icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils
  icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header
  icon_shared_memory_icon_utils_format
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_strerror
  icon_shared_memory_icon_utils_time
)
install(TARGETS icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/memory_segment.h"
        DESTINATION "include/icon/interprocess/shared_memory_manager"
)

add_library(icon_shared_memory_icon_utils_current_cycle STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/current_cycle.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/current_cycle.h"
)
target_include_directories(icon_shared_memory_icon_utils_current_cycle PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
install(TARGETS icon_shared_memory_icon_utils_current_cycle
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/current_cycle.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_icon_hal_hardware_interface_handle INTERFACE)
target_sources(icon_shared_memory_icon_hal_hardware_interface_handle PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_interface_handle.h"
)
target_include_directories(icon_shared_memory_icon_hal_hardware_interface_handle INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_hardware_interface_handle INTERFACE
  icon_shared_memory_icon_hal_icon_state_register
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment
  icon_shared_memory_icon_utils_current_cycle
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_time
  flatbuffers::flatbuffers
)
install(TARGETS icon_shared_memory_icon_hal_hardware_interface_handle
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_interface_handle.h"
        DESTINATION "include/icon/hal"
)

add_library(icon_shared_memory_icon_hal_interfaces_control_period_fbs_utils STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/control_period_utils.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/control_period_utils.h"
)
target_include_directories(icon_shared_memory_icon_hal_interfaces_control_period_fbs_utils PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_interfaces_control_period_fbs_utils PUBLIC
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_hal_hardware_interface_handle
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_format
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_time
  flatbuffers::flatbuffers
)
install(TARGETS icon_shared_memory_icon_hal_interfaces_control_period_fbs_utils
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/control_period_utils.h"
        DESTINATION "include/icon/hal/interfaces"
)

add_library(icon_shared_memory_icon_utils_check INTERFACE)
target_sources(icon_shared_memory_icon_utils_check PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/check.h"
)
target_include_directories(icon_shared_memory_icon_utils_check INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
install(TARGETS icon_shared_memory_icon_utils_check
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/check.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_icon_utils_mutex INTERFACE)
target_sources(icon_shared_memory_icon_utils_mutex PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/mutex.h"
)
target_include_directories(icon_shared_memory_icon_utils_mutex INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_utils_mutex INTERFACE
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_check
)
install(TARGETS icon_shared_memory_icon_utils_mutex
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/mutex.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_platform_common_buffers_rt_queue_buffer INTERFACE)
target_sources(icon_shared_memory_platform_common_buffers_rt_queue_buffer PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/platform/common/buffers/rt_queue_buffer.h"
)
target_include_directories(icon_shared_memory_platform_common_buffers_rt_queue_buffer INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_platform_common_buffers_rt_queue_buffer INTERFACE
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_check
)
install(TARGETS icon_shared_memory_platform_common_buffers_rt_queue_buffer
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/platform/common/buffers/rt_queue_buffer.h"
        DESTINATION "include/platform/common/buffers"
)

add_library(icon_shared_memory_platform_common_buffers_rt_queue INTERFACE)
target_sources(icon_shared_memory_platform_common_buffers_rt_queue PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/platform/common/buffers/rt_queue.h"
)
target_include_directories(icon_shared_memory_platform_common_buffers_rt_queue INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_platform_common_buffers_rt_queue INTERFACE
  icon_shared_memory_platform_common_buffers_rt_queue_buffer
  icon_shared_memory_icon_testing_realtime_annotations
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_realtime_guard
)
install(TARGETS icon_shared_memory_platform_common_buffers_rt_queue
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/platform/common/buffers/rt_queue.h"
        DESTINATION "include/platform/common/buffers"
)

add_library(icon_shared_memory_platform_common_buffers_rt_queue_multi_writer INTERFACE)
target_sources(icon_shared_memory_platform_common_buffers_rt_queue_multi_writer PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/platform/common/buffers/rt_queue_multi_writer.h"
)
target_include_directories(icon_shared_memory_platform_common_buffers_rt_queue_multi_writer INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_platform_common_buffers_rt_queue_multi_writer INTERFACE
  icon_shared_memory_platform_common_buffers_rt_queue
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_mutex
  icon_shared_memory_icon_utils_realtime_guard
  icon_shared_memory_icon_utils_status
)
install(TARGETS icon_shared_memory_platform_common_buffers_rt_queue_multi_writer
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/platform/common/buffers/rt_queue_multi_writer.h"
        DESTINATION "include/platform/common/buffers"
)

add_library(icon_shared_memory_icon_hal_hardware_module_util STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_module_util.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_module_util.h"
)
target_include_directories(icon_shared_memory_icon_hal_hardware_module_util PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_hardware_module_util PUBLIC
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_mutex
  icon_shared_memory_icon_utils_status
)
install(TARGETS icon_shared_memory_icon_hal_hardware_module_util
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_module_util.h"
        DESTINATION "include/icon/hal"
)

add_library(icon_shared_memory_icon_utils_status_matchers INTERFACE)
target_sources(icon_shared_memory_icon_utils_status_matchers PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status_matchers.h"
)
target_include_directories(icon_shared_memory_icon_utils_status_matchers INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_utils_status_matchers INTERFACE
  icon_shared_memory_icon_utils_status
  GTest::gmock
  tl::expected
)
install(TARGETS icon_shared_memory_icon_utils_status_matchers
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status_matchers.h"
        DESTINATION "include/icon/utils"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_hal_hardware_module_util_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_module_util_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_hal_hardware_module_util_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_hal_hardware_module_util_test PRIVATE
    icon_shared_memory_icon_hal_hardware_module_util
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    icon_shared_memory_icon_utils_status_matchers
    GTest::gmock
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_hal_hardware_module_util_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_cleanup_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/cleanup_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_cleanup_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_cleanup_test PRIVATE
    icon_shared_memory_icon_utils_cleanup
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_cleanup_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_format_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/format_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_format_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_format_test PRIVATE
    icon_shared_memory_icon_utils_format
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_format_test)
endif()

add_library(icon_shared_memory_icon_interprocess_binary_futex STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/binary_futex.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/binary_futex.h"
)
target_include_directories(icon_shared_memory_icon_interprocess_binary_futex PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_interprocess_binary_futex PUBLIC
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_strerror
  icon_shared_memory_icon_utils_time
)
install(TARGETS icon_shared_memory_icon_interprocess_binary_futex
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/binary_futex.h"
        DESTINATION "include/icon/interprocess"
)

add_library(icon_shared_memory_icon_interprocess_lockable_binary_futex INTERFACE)
target_sources(icon_shared_memory_icon_interprocess_lockable_binary_futex PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/lockable_binary_futex.h"
)
target_include_directories(icon_shared_memory_icon_interprocess_lockable_binary_futex INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_interprocess_lockable_binary_futex INTERFACE
  icon_shared_memory_icon_interprocess_binary_futex
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_check
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_strerror
  icon_shared_memory_icon_utils_time
)
install(TARGETS icon_shared_memory_icon_interprocess_lockable_binary_futex
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/lockable_binary_futex.h"
        DESTINATION "include/icon/interprocess"
)

add_library(icon_shared_memory_icon_utils_status_helpers INTERFACE)
target_sources(icon_shared_memory_icon_utils_status_helpers PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status_helpers.h"
)
target_include_directories(icon_shared_memory_icon_utils_status_helpers INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_utils_status_helpers INTERFACE
  icon_shared_memory_icon_utils_status
)
install(TARGETS icon_shared_memory_icon_utils_status_helpers
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status_helpers.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_platform_common_buffers_rt_promise INTERFACE)
target_sources(icon_shared_memory_platform_common_buffers_rt_promise PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/platform/common/buffers/rt_promise.h"
)
target_include_directories(icon_shared_memory_platform_common_buffers_rt_promise INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_platform_common_buffers_rt_promise INTERFACE
  icon_shared_memory_platform_common_buffers_rt_queue
  icon_shared_memory_icon_interprocess_binary_futex
  icon_shared_memory_icon_interprocess_lockable_binary_futex
  icon_shared_memory_icon_testing_realtime_annotations
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_status_and_expected_macros
  icon_shared_memory_icon_utils_status_helpers
  tl::expected
)
install(TARGETS icon_shared_memory_platform_common_buffers_rt_promise
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/platform/common/buffers/rt_promise.h"
        DESTINATION "include/platform/common/buffers"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_status_and_expected_macros_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status_and_expected_macros_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_status_and_expected_macros_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_status_and_expected_macros_test PRIVATE
    icon_shared_memory_icon_utils_status_and_expected_macros
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_status_and_expected_macros_test)
endif()

add_library(icon_shared_memory_icon_interprocess_shared_memory_manager STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/shared_memory_manager.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/shared_memory_manager.h"
)
target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_manager PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_manager PUBLIC
  icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils
  icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment
  icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_flatbuffers_flatbuffer_utils
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_status_and_expected_macros
  rt
)
install(TARGETS icon_shared_memory_icon_interprocess_shared_memory_manager
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/shared_memory_manager.h"
        DESTINATION "include/icon/interprocess/shared_memory_manager"
)

add_library(icon_shared_memory_icon_hal_hardware_interface_registry STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/get_hardware_interface.h"
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_interface_registry.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_interface_registry.h"
)
target_include_directories(icon_shared_memory_icon_hal_hardware_interface_registry PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_hardware_interface_registry PUBLIC
  icon_shared_memory_icon_hal_hardware_interface_handle
  icon_shared_memory_icon_hal_hardware_interface_traits
  icon_shared_memory_icon_hal_icon_state_register
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_interprocess_shared_memory_manager
  icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils
  icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment
  icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header
  icon_shared_memory_icon_interprocess_shared_memory_manager_segment_info_utils
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_realtime_guard
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_status_and_expected_macros
  flatbuffers::flatbuffers
)
install(TARGETS icon_shared_memory_icon_hal_hardware_interface_registry
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/get_hardware_interface.h"
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_interface_registry.h"
        DESTINATION "include/icon/hal"
)

add_library(icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_server STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/domain_socket_server.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/domain_socket_server.h"
)
target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_server PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_server PUBLIC
  icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils
  icon_shared_memory_icon_interprocess_shared_memory_manager
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_flatbuffers_fixed_string
  icon_shared_memory_icon_hal_hardware_interface_registry
  icon_shared_memory_icon_utils_cleanup
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_status_and_expected_macros
  icon_shared_memory_icon_utils_time
)
install(TARGETS icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_server
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/domain_socket_server.h"
        DESTINATION "include/icon/interprocess/shared_memory_manager"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_platform_common_buffers_rt_queue_multi_writer_test
    "${CMAKE_CURRENT_LIST_DIR}/platform/common/buffers/rt_queue_multi_writer_test.cc"
  )
  target_include_directories(icon_shared_memory_platform_common_buffers_rt_queue_multi_writer_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_platform_common_buffers_rt_queue_multi_writer_test PRIVATE
    icon_shared_memory_platform_common_buffers_rt_queue
    icon_shared_memory_platform_common_buffers_rt_queue_multi_writer
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    GTest::gmock
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_platform_common_buffers_rt_queue_multi_writer_test)
endif()

add_library(icon_shared_memory_eigenmath INTERFACE)
target_sources(icon_shared_memory_eigenmath PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/eigenmath/types.h"
)
target_include_directories(icon_shared_memory_eigenmath INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_eigenmath INTERFACE
  Eigen3::Eigen
)
install(TARGETS icon_shared_memory_eigenmath
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/eigenmath/types.h"
        DESTINATION "include/eigenmath"
)

add_library(icon_shared_memory_icon_interprocess_shared_memory_manager_testing_unique_segment_name STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/testing/unique_segment_name.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/testing/unique_segment_name.h"
)
target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_manager_testing_unique_segment_name PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_manager_testing_unique_segment_name PUBLIC
  icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment
)
install(TARGETS icon_shared_memory_icon_interprocess_shared_memory_manager_testing_unique_segment_name
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/testing/unique_segment_name.h"
        DESTINATION "include/icon/interprocess/shared_memory_manager/testing"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/memory_segment_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment_test PRIVATE
    icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment
    icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header
    icon_shared_memory_icon_interprocess_shared_memory_manager
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_icon_interprocess_shared_memory_manager_testing_unique_segment_name
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    icon_shared_memory_icon_utils_time
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment_test)
endif()

add_library(icon_shared_memory_icon_utils_async_buffer INTERFACE)
target_sources(icon_shared_memory_icon_utils_async_buffer PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/async_buffer.h"
)
target_include_directories(icon_shared_memory_icon_utils_async_buffer INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_utils_async_buffer INTERFACE
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_check
)
install(TARGETS icon_shared_memory_icon_utils_async_buffer
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/async_buffer.h"
        DESTINATION "include/icon/utils"
)

install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/flatbuffers/fixed_string_test.fbs"
        DESTINATION "include/icon/flatbuffers"
)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/icon/flatbuffers/fixed_string_test.fbs.h"
  COMMAND "${FLATC_EXECUTABLE}" --cpp --filename-suffix .fbs --keep-prefix --reflect-names --scoped-enums --gen-mutable --filename-ext h
          -o "${CMAKE_CURRENT_BINARY_DIR}/icon/flatbuffers"
          -I "${INSRC_ROOT}"
          "${CMAKE_CURRENT_LIST_DIR}/icon/flatbuffers/fixed_string_test.fbs"
  DEPENDS "${CMAKE_CURRENT_LIST_DIR}/icon/flatbuffers/fixed_string_test.fbs"
  COMMENT "Generating C++ Flatbuffers headers for icon/flatbuffers/fixed_string_test.fbs"
)
add_library(icon_shared_memory_icon_flatbuffers_fixed_string_test_fbs_cc INTERFACE)
target_include_directories(icon_shared_memory_icon_flatbuffers_fixed_string_test_fbs_cc INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
  "$<INSTALL_INTERFACE:include>"
)
target_sources(icon_shared_memory_icon_flatbuffers_fixed_string_test_fbs_cc PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/icon/flatbuffers/fixed_string_test.fbs.h"
)
target_link_libraries(icon_shared_memory_icon_flatbuffers_fixed_string_test_fbs_cc INTERFACE
  flatbuffers::flatbuffers
)
install(TARGETS icon_shared_memory_icon_flatbuffers_fixed_string_test_fbs_cc
        EXPORT icon_shared_memoryTargets
)
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/icon/flatbuffers/fixed_string_test.fbs.h"
        DESTINATION "include/icon/flatbuffers"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_check_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/check_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_check_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_check_test PRIVATE
    icon_shared_memory_icon_utils_check
    GTest::gmock
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_check_test)
endif()

add_library(icon_shared_memory_icon_utils_mock_log_sink INTERFACE)
target_sources(icon_shared_memory_icon_utils_mock_log_sink PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/mock_log_sink.h"
)
target_include_directories(icon_shared_memory_icon_utils_mock_log_sink INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_utils_mock_log_sink INTERFACE
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_mutex
  icon_shared_memory_icon_testing_realtime_annotations
)
install(TARGETS icon_shared_memory_icon_utils_mock_log_sink
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/mock_log_sink.h"
        DESTINATION "include/icon/utils"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_log_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/log_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_log_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_log_test PRIVATE
    icon_shared_memory_icon_utils_log
    icon_shared_memory_icon_utils_mock_log_sink
    icon_shared_memory_icon_utils_time
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_log_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_async_buffer_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/async_buffer_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_async_buffer_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_async_buffer_test PRIVATE
    icon_shared_memory_icon_utils_async_buffer
    GTest::gmock
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_async_buffer_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/segment_header_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header_test PRIVATE
    icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header
    icon_shared_memory_icon_utils_mock_log_sink
    icon_shared_memory_icon_utils_time
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_flatbuffers_fixed_string_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/flatbuffers/fixed_string_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_flatbuffers_fixed_string_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_flatbuffers_fixed_string_test PRIVATE
    icon_shared_memory_icon_flatbuffers_fixed_string
    icon_shared_memory_icon_flatbuffers_fixed_string_test_fbs_cc
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    flatbuffers::flatbuffers
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_flatbuffers_fixed_string_test)
endif()

add_library(icon_shared_memory_icon_utils_async_request INTERFACE)
target_sources(icon_shared_memory_icon_utils_async_request PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/utils/async_request.h"
)
target_include_directories(icon_shared_memory_icon_utils_async_request INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_utils_async_request INTERFACE
  icon_shared_memory_icon_testing_realtime_annotations
  icon_shared_memory_icon_utils_status
  icon_shared_memory_platform_common_buffers_rt_promise
)
install(TARGETS icon_shared_memory_icon_utils_async_request
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/utils/async_request.h"
        DESTINATION "include/icon/utils"
)

add_library(icon_shared_memory_util_thread_lockstep STATIC
  "${CMAKE_CURRENT_LIST_DIR}/util/thread/lockstep.cc"
  "${CMAKE_CURRENT_LIST_DIR}/util/thread/lockstep.h"
)
target_include_directories(icon_shared_memory_util_thread_lockstep PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_util_thread_lockstep PUBLIC
  icon_shared_memory_icon_interprocess_binary_futex
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_time
)
install(TARGETS icon_shared_memory_util_thread_lockstep
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/util/thread/lockstep.h"
        DESTINATION "include/util/thread"
)

add_library(icon_shared_memory_icon_interprocess_shared_memory_lockstep STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_lockstep/shared_memory_lockstep.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_lockstep/shared_memory_lockstep.h"
)
target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_lockstep PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_lockstep PUBLIC
  icon_shared_memory_icon_interprocess_shared_memory_manager
  icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils
  icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_status_and_expected_macros
  icon_shared_memory_util_thread_lockstep
)
install(TARGETS icon_shared_memory_icon_interprocess_shared_memory_lockstep
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_lockstep/shared_memory_lockstep.h"
        DESTINATION "include/icon/interprocess/shared_memory_lockstep"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_interprocess_shared_memory_lockstep_shared_memory_lockstep_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_lockstep/shared_memory_lockstep_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_lockstep_shared_memory_lockstep_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_lockstep_shared_memory_lockstep_test PRIVATE
    icon_shared_memory_icon_interprocess_shared_memory_lockstep
    icon_shared_memory_icon_interprocess_shared_memory_manager
    icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment
    icon_shared_memory_icon_interprocess_shared_memory_manager_testing_unique_segment_name
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    icon_shared_memory_icon_utils_time
    icon_shared_memory_util_thread_lockstep
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_interprocess_shared_memory_lockstep_shared_memory_lockstep_test)
endif()

add_library(icon_shared_memory_kinematics_types_joint_limits STATIC
  "${CMAKE_CURRENT_LIST_DIR}/kinematics/types/joint_limits.cc"
  "${CMAKE_CURRENT_LIST_DIR}/kinematics/types/joint_limits.h"
)
target_include_directories(icon_shared_memory_kinematics_types_joint_limits PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_kinematics_types_joint_limits PUBLIC
  icon_shared_memory_eigenmath
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_status
  tl::expected
)
install(TARGETS icon_shared_memory_kinematics_types_joint_limits
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/kinematics/types/joint_limits.h"
        DESTINATION "include/kinematics/types"
)

add_library(icon_shared_memory_icon_hal_interfaces_joint_limits_fbs_utils STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/joint_limits_utils.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/joint_limits_utils.h"
)
target_include_directories(icon_shared_memory_icon_hal_interfaces_joint_limits_fbs_utils PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_interfaces_joint_limits_fbs_utils PUBLIC
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_status_and_expected_macros
  icon_shared_memory_kinematics_types_joint_limits
  flatbuffers::flatbuffers
)
install(TARGETS icon_shared_memory_icon_hal_interfaces_joint_limits_fbs_utils
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/joint_limits_utils.h"
        DESTINATION "include/icon/hal/interfaces"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_hal_hardware_interface_registry_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_interface_registry_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_hal_hardware_interface_registry_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_hal_hardware_interface_registry_test PRIVATE
    icon_shared_memory_icon_hal_hardware_interface_handle
    icon_shared_memory_icon_hal_hardware_interface_registry
    icon_shared_memory_icon_hal_hardware_interface_traits
    icon_shared_memory_icon_hal_icon_state_register
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_icon_hal_interfaces_joint_limits_fbs_utils
    icon_shared_memory_icon_interprocess_shared_memory_manager
    icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header
    icon_shared_memory_icon_interprocess_shared_memory_manager_testing_unique_segment_name
    icon_shared_memory_icon_utils_current_cycle
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    icon_shared_memory_icon_utils_time
    flatbuffers::flatbuffers
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_hal_hardware_interface_registry_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_hal_interfaces_joint_limits_utils_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/joint_limits_utils_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_hal_interfaces_joint_limits_utils_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_hal_interfaces_joint_limits_utils_test PRIVATE
    icon_shared_memory_icon_hal_interfaces_joint_limits_fbs_utils
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_icon_hal_hardware_interface_handle
    icon_shared_memory_icon_hal_hardware_interface_registry
    icon_shared_memory_icon_hal_hardware_interface_traits
    icon_shared_memory_icon_interprocess_shared_memory_manager
    icon_shared_memory_icon_interprocess_shared_memory_manager_testing_unique_segment_name
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    icon_shared_memory_kinematics_types_joint_limits
    flatbuffers::flatbuffers
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_hal_interfaces_joint_limits_utils_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_status_helpers_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status_helpers_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_status_helpers_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_status_helpers_test PRIVATE
    icon_shared_memory_icon_utils_status_helpers
    GTest::gmock
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_status_helpers_test)
endif()

add_library(icon_shared_memory_util_thread_thread_options INTERFACE)
target_sources(icon_shared_memory_util_thread_thread_options PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/util/thread/thread_options.h"
)
target_include_directories(icon_shared_memory_util_thread_thread_options INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
install(TARGETS icon_shared_memory_util_thread_thread_options
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/util/thread/thread_options.h"
        DESTINATION "include/util/thread"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_flatbuffers_flatbuffer_utils_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/flatbuffers/flatbuffer_utils_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_flatbuffers_flatbuffer_utils_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_flatbuffers_flatbuffer_utils_test PRIVATE
    icon_shared_memory_icon_flatbuffers_flatbuffer_utils
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_icon_utils_status
    flatbuffers::flatbuffers
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_flatbuffers_flatbuffer_utils_test)
endif()

add_library(icon_shared_memory_icon_hal_interfaces_hardware_module_state_fbs_utils STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/hardware_module_state_utils.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/hardware_module_state_utils.h"
)
target_include_directories(icon_shared_memory_icon_hal_interfaces_hardware_module_state_fbs_utils PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_interfaces_hardware_module_state_fbs_utils PUBLIC
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_utils_attributes
  flatbuffers::flatbuffers
)
install(TARGETS icon_shared_memory_icon_hal_interfaces_hardware_module_state_fbs_utils
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/hardware_module_state_utils.h"
        DESTINATION "include/icon/hal/interfaces"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_hal_get_hardware_interface_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/hal/get_hardware_interface_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_hal_get_hardware_interface_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_hal_get_hardware_interface_test PRIVATE
    icon_shared_memory_icon_hal_hardware_interface_registry
    icon_shared_memory_icon_hal_icon_state_register
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_icon_flatbuffers_flatbuffer_utils
    icon_shared_memory_icon_hal_interfaces_icon_state_fbs_utils
    icon_shared_memory_icon_hal_interfaces_joint_limits_fbs_utils
    icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    flatbuffers::flatbuffers
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_hal_get_hardware_interface_test)
endif()

add_library(icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_server STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/remote_trigger/remote_trigger_constants.h"
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/remote_trigger/remote_trigger_server.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/remote_trigger/remote_trigger_server.h"
)
target_include_directories(icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_server PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_server PUBLIC
  icon_shared_memory_icon_interprocess_binary_futex
  icon_shared_memory_icon_interprocess_shared_memory_manager
  icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_status_and_expected_macros
  icon_shared_memory_icon_utils_time
  tl::expected
)
install(TARGETS icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_server
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/remote_trigger/remote_trigger_constants.h"
        "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/remote_trigger/remote_trigger_server.h"
        DESTINATION "include/icon/interprocess/remote_trigger"
)

add_library(icon_shared_memory_icon_testing_malloc_test INTERFACE)
target_sources(icon_shared_memory_icon_testing_malloc_test PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/testing/malloc_test.h"
)
target_include_directories(icon_shared_memory_icon_testing_malloc_test INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_testing_malloc_test INTERFACE
  GTest::gmock
)
install(TARGETS icon_shared_memory_icon_testing_malloc_test
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/testing/malloc_test.h"
        DESTINATION "include/icon/testing"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_util_thread_lockstep_test
    "${CMAKE_CURRENT_LIST_DIR}/util/thread/lockstep_test.cc"
  )
  target_include_directories(icon_shared_memory_util_thread_lockstep_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_util_thread_lockstep_test PRIVATE
    icon_shared_memory_util_thread_lockstep
    icon_shared_memory_icon_utils_log
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    icon_shared_memory_icon_utils_time
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_util_thread_lockstep_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_interprocess_shared_memory_manager_segment_info_utils_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/segment_info_utils_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_manager_segment_info_utils_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_manager_segment_info_utils_test PRIVATE
    icon_shared_memory_icon_interprocess_shared_memory_manager_segment_info_utils
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_icon_flatbuffers_flatbuffer_utils
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_interprocess_shared_memory_manager_segment_info_utils_test)
endif()

add_library(icon_shared_memory_icon_hal_hardware_module_init_context INTERFACE)
target_sources(icon_shared_memory_icon_hal_hardware_module_init_context PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_module_init_context.h"
)
target_include_directories(icon_shared_memory_icon_hal_hardware_module_init_context INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_hardware_module_init_context INTERFACE
  icon_shared_memory_icon_hal_hardware_interface_registry
)
install(TARGETS icon_shared_memory_icon_hal_hardware_module_init_context
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_module_init_context.h"
        DESTINATION "include/icon/hal"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_mutex_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/mutex_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_mutex_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_mutex_test PRIVATE
    icon_shared_memory_icon_utils_attributes
    icon_shared_memory_icon_utils_mutex
    GTest::gmock
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_mutex_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_platform_common_buffers_rt_queue_buffer_test
    "${CMAKE_CURRENT_LIST_DIR}/platform/common/buffers/rt_queue_buffer_test.cc"
  )
  target_include_directories(icon_shared_memory_platform_common_buffers_rt_queue_buffer_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_platform_common_buffers_rt_queue_buffer_test PRIVATE
    icon_shared_memory_platform_common_buffers_rt_queue_buffer
    icon_shared_memory_icon_utils_time
    GTest::gmock
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_platform_common_buffers_rt_queue_buffer_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_strerror_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/strerror_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_strerror_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_strerror_test PRIVATE
    icon_shared_memory_icon_utils_strerror
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_strerror_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/domain_socket_utils_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils_test PRIVATE
    icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils
    icon_shared_memory_icon_utils_mock_log_sink
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_time
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_status_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_status_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_status_test PRIVATE
    icon_shared_memory_icon_utils_status
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_status_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_interprocess_lockable_binary_futex_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/lockable_binary_futex_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_interprocess_lockable_binary_futex_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_interprocess_lockable_binary_futex_test PRIVATE
    icon_shared_memory_icon_interprocess_lockable_binary_futex
    icon_shared_memory_icon_utils_attributes
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    icon_shared_memory_icon_utils_status_matchers
    icon_shared_memory_icon_utils_time
    GTest::gmock
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_interprocess_lockable_binary_futex_test)
endif()

add_library(icon_shared_memory_icon_hal_interfaces_joint_command_fbs_utils STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/joint_command_utils.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/joint_command_utils.h"
)
target_include_directories(icon_shared_memory_icon_hal_interfaces_joint_command_fbs_utils PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_interfaces_joint_command_fbs_utils PUBLIC
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_flatbuffers_flatbuffer_utils
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_status_and_expected_macros
  flatbuffers::flatbuffers
)
install(TARGETS icon_shared_memory_icon_hal_interfaces_joint_command_fbs_utils
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/joint_command_utils.h"
        DESTINATION "include/icon/hal/interfaces"
)

add_library(icon_shared_memory_icon_control_realtime_clock_interface STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/control/realtime_clock_interface.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/control/realtime_clock_interface.h"
)
target_include_directories(icon_shared_memory_icon_control_realtime_clock_interface PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_control_realtime_clock_interface PUBLIC
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_time
)
install(TARGETS icon_shared_memory_icon_control_realtime_clock_interface
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/control/realtime_clock_interface.h"
        DESTINATION "include/icon/control"
)

add_library(icon_shared_memory_icon_hal_realtime_clock STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/realtime_clock.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/realtime_clock.h"
)
target_include_directories(icon_shared_memory_icon_hal_realtime_clock PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_realtime_clock PUBLIC
  icon_shared_memory_icon_control_realtime_clock_interface
  icon_shared_memory_icon_interprocess_shared_memory_lockstep
  icon_shared_memory_icon_interprocess_shared_memory_manager
  icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_time
  icon_shared_memory_util_thread_lockstep
)
install(TARGETS icon_shared_memory_icon_hal_realtime_clock
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/realtime_clock.h"
        DESTINATION "include/icon/hal"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_status_matchers_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/status_matchers_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_status_matchers_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_status_matchers_test PRIVATE
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_matchers
    GTest::gmock
    GTest::gmock_main
    tl::expected
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_status_matchers_test)
endif()

add_library(icon_shared_memory_icon_hal_interfaces_joint_state_fbs_utils STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/joint_state_utils.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/joint_state_utils.h"
)
target_include_directories(icon_shared_memory_icon_hal_interfaces_joint_state_fbs_utils PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_interfaces_joint_state_fbs_utils PUBLIC
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_utils_attributes
  flatbuffers::flatbuffers
)
install(TARGETS icon_shared_memory_icon_hal_interfaces_joint_state_fbs_utils
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/joint_state_utils.h"
        DESTINATION "include/icon/hal/interfaces"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_hal_interfaces_joint_state_utils_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/joint_state_utils_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_hal_interfaces_joint_state_utils_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_hal_interfaces_joint_state_utils_test PRIVATE
    icon_shared_memory_icon_hal_interfaces_joint_state_fbs_utils
    icon_shared_memory_external_fbs_cc
    flatbuffers::flatbuffers
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_hal_interfaces_joint_state_utils_test)
endif()

add_library(icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_test_common INTERFACE)
target_sources(icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_test_common PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/remote_trigger/remote_trigger_test_common.h"
)
target_include_directories(icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_test_common INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_test_common INTERFACE
  icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_server
)
install(TARGETS icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_test_common
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/remote_trigger/remote_trigger_test_common.h"
        DESTINATION "include/icon/interprocess/remote_trigger"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_server_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/remote_trigger/remote_trigger_server_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_server_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_server_test PRIVATE
    icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_server
    icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_test_common
    icon_shared_memory_icon_interprocess_shared_memory_manager
    icon_shared_memory_icon_interprocess_shared_memory_manager_testing_unique_segment_name
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    GTest::gmock_main
    tl::expected
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_server_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_interprocess_binary_futex_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/binary_futex_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_interprocess_binary_futex_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_interprocess_binary_futex_test PRIVATE
    icon_shared_memory_icon_interprocess_binary_futex
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    icon_shared_memory_icon_utils_time
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_interprocess_binary_futex_test)
endif()

add_library(icon_shared_memory_icon_hal_control_period_register INTERFACE)
target_sources(icon_shared_memory_icon_hal_control_period_register PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/control_period_register.h"
)
target_include_directories(icon_shared_memory_icon_hal_control_period_register INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_control_period_register INTERFACE
  icon_shared_memory_icon_hal_hardware_interface_traits
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_hal_interfaces_control_period_fbs_utils
)
install(TARGETS icon_shared_memory_icon_hal_control_period_register
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/control_period_register.h"
        DESTINATION "include/icon/hal"
)

add_library(icon_shared_memory_icon_hal_hardware_module_interface INTERFACE)
target_sources(icon_shared_memory_icon_hal_hardware_module_interface PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_module_interface.h"
)
target_include_directories(icon_shared_memory_icon_hal_hardware_module_interface INTERFACE
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_hardware_module_interface INTERFACE
  icon_shared_memory_icon_hal_hardware_module_init_context
  icon_shared_memory_icon_control_realtime_clock_interface
  icon_shared_memory_icon_utils_status
)
install(TARGETS icon_shared_memory_icon_hal_hardware_module_interface
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_module_interface.h"
        DESTINATION "include/icon/hal"
)

add_library(icon_shared_memory_icon_hal_hardware_module_runtime STATIC
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_module_runtime.cc"
  "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_module_runtime.h"
)
target_include_directories(icon_shared_memory_icon_hal_hardware_module_runtime PUBLIC
  "$<BUILD_INTERFACE:${INSRC_ROOT}>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(icon_shared_memory_icon_hal_hardware_module_runtime PUBLIC
  icon_shared_memory_icon_hal_control_period_register
  icon_shared_memory_icon_hal_hardware_interface_handle
  icon_shared_memory_icon_hal_hardware_interface_registry
  icon_shared_memory_icon_hal_hardware_interface_traits
  icon_shared_memory_icon_hal_hardware_module_init_context
  icon_shared_memory_icon_hal_hardware_module_interface
  icon_shared_memory_icon_hal_hardware_module_util
  icon_shared_memory_icon_hal_icon_state_register
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_external_fbs_cc
  icon_shared_memory_icon_hal_interfaces_control_period_fbs_utils
  icon_shared_memory_icon_hal_interfaces_hardware_module_state_fbs_utils
  icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_server
  icon_shared_memory_icon_interprocess_shared_memory_manager
  icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_server
  icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils
  icon_shared_memory_icon_testing_realtime_annotations
  icon_shared_memory_icon_utils_async_buffer
  icon_shared_memory_icon_utils_async_request
  icon_shared_memory_icon_utils_attributes
  icon_shared_memory_icon_utils_cleanup
  icon_shared_memory_icon_utils_log
  icon_shared_memory_icon_utils_mutex
  icon_shared_memory_icon_utils_realtime_guard
  icon_shared_memory_icon_utils_status
  icon_shared_memory_icon_utils_status_and_expected_macros
  icon_shared_memory_icon_utils_time
  icon_shared_memory_platform_common_buffers_rt_promise
  icon_shared_memory_platform_common_buffers_rt_queue
  icon_shared_memory_platform_common_buffers_rt_queue_multi_writer
  icon_shared_memory_util_thread_thread_options
  flatbuffers::flatbuffers
  tl::expected
)
install(TARGETS icon_shared_memory_icon_hal_hardware_module_runtime
        EXPORT icon_shared_memoryTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(FILES
        "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_module_runtime.h"
        DESTINATION "include/icon/hal"
)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_hal_hardware_module_runtime_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/hal/hardware_module_runtime_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_hal_hardware_module_runtime_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_hal_hardware_module_runtime_test PRIVATE
    icon_shared_memory_icon_hal_control_period_register
    icon_shared_memory_icon_hal_hardware_interface_handle
    icon_shared_memory_icon_hal_hardware_interface_registry
    icon_shared_memory_icon_hal_hardware_interface_traits
    icon_shared_memory_icon_hal_hardware_module_init_context
    icon_shared_memory_icon_hal_hardware_module_interface
    icon_shared_memory_icon_hal_hardware_module_runtime
    icon_shared_memory_icon_hal_hardware_module_util
    icon_shared_memory_icon_hal_icon_state_register
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_icon_hal_interfaces_control_period_fbs_utils
    icon_shared_memory_icon_hal_interfaces_hardware_module_state_fbs_utils
    icon_shared_memory_icon_hal_interfaces_joint_command_fbs_utils
    icon_shared_memory_icon_hal_interfaces_joint_limits_fbs_utils
    icon_shared_memory_icon_hal_interfaces_joint_state_fbs_utils
    icon_shared_memory_icon_interprocess_binary_futex
    icon_shared_memory_icon_interprocess_remote_trigger_remote_trigger_server
    icon_shared_memory_icon_interprocess_shared_memory_manager
    icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils
    icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment
    icon_shared_memory_icon_interprocess_shared_memory_manager_testing_unique_segment_name
    icon_shared_memory_icon_utils_log
    icon_shared_memory_icon_utils_mock_log_sink
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_macros
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    icon_shared_memory_icon_utils_status_matchers
    flatbuffers::flatbuffers
    GTest::gmock
    GTest::gmock_main
    tl::expected
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_hal_hardware_module_runtime_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_server_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/domain_socket_server_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_server_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_server_test PRIVATE
    icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_server
    icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils
    icon_shared_memory_icon_interprocess_shared_memory_manager
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_icon_flatbuffers_flatbuffer_utils
    icon_shared_memory_icon_hal_hardware_interface_registry
    icon_shared_memory_icon_interprocess_shared_memory_manager_testing_unique_segment_name
    icon_shared_memory_icon_utils_cleanup
    icon_shared_memory_icon_utils_log
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    icon_shared_memory_icon_utils_time
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_server_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_time_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/time_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_time_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_time_test PRIVATE
    icon_shared_memory_icon_utils_time
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_time_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_interprocess_shared_memory_manager_shared_memory_manager_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/interprocess/shared_memory_manager/shared_memory_manager_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_interprocess_shared_memory_manager_shared_memory_manager_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_interprocess_shared_memory_manager_shared_memory_manager_test PRIVATE
    icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_server
    icon_shared_memory_icon_interprocess_shared_memory_manager_domain_socket_utils
    icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment
    icon_shared_memory_icon_interprocess_shared_memory_manager_segment_header
    icon_shared_memory_icon_interprocess_shared_memory_manager_segment_info_utils
    icon_shared_memory_icon_interprocess_shared_memory_manager
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_icon_flatbuffers_flatbuffer_utils
    icon_shared_memory_icon_interprocess_shared_memory_manager_testing_unique_segment_name
    icon_shared_memory_icon_utils_mock_log_sink
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_interprocess_shared_memory_manager_shared_memory_manager_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_current_cycle_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/current_cycle_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_current_cycle_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_current_cycle_test PRIVATE
    icon_shared_memory_icon_utils_current_cycle
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_current_cycle_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_hal_interfaces_hardware_module_state_utils_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/hardware_module_state_utils_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_hal_interfaces_hardware_module_state_utils_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_hal_interfaces_hardware_module_state_utils_test PRIVATE
    icon_shared_memory_icon_hal_interfaces_hardware_module_state_fbs_utils
    icon_shared_memory_external_fbs_cc
    flatbuffers::flatbuffers
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_hal_interfaces_hardware_module_state_utils_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_hal_interfaces_icon_state_utils_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/icon_state_utils_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_hal_interfaces_icon_state_utils_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_hal_interfaces_icon_state_utils_test PRIVATE
    icon_shared_memory_icon_hal_interfaces_icon_state_fbs_utils
    icon_shared_memory_external_fbs_cc
    flatbuffers::flatbuffers
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_hal_interfaces_icon_state_utils_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_utils_async_request_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/utils/async_request_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_utils_async_request_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_utils_async_request_test PRIVATE
    icon_shared_memory_icon_utils_async_request
    icon_shared_memory_icon_utils_log
    icon_shared_memory_icon_utils_mock_log_sink
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    icon_shared_memory_icon_testing_malloc_test
    icon_shared_memory_platform_common_buffers_rt_promise
    GTest::gmock
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_utils_async_request_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_icon_hal_interfaces_control_period_utils_test
    "${CMAKE_CURRENT_LIST_DIR}/icon/hal/interfaces/control_period_utils_test.cc"
  )
  target_include_directories(icon_shared_memory_icon_hal_interfaces_control_period_utils_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_icon_hal_interfaces_control_period_utils_test PRIVATE
    icon_shared_memory_icon_hal_interfaces_control_period_fbs_utils
    icon_shared_memory_external_fbs_cc
    icon_shared_memory_icon_hal_hardware_interface_handle
    icon_shared_memory_icon_interprocess_shared_memory_manager
    icon_shared_memory_icon_interprocess_shared_memory_manager_memory_segment
    icon_shared_memory_icon_interprocess_shared_memory_manager_testing_unique_segment_name
    icon_shared_memory_icon_utils_mock_log_sink
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    icon_shared_memory_icon_utils_status_matchers
    icon_shared_memory_icon_utils_time
    flatbuffers::flatbuffers
    GTest::gmock
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_icon_hal_interfaces_control_period_utils_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_kinematics_types_joint_limits_test
    "${CMAKE_CURRENT_LIST_DIR}/kinematics/types/joint_limits_test.cc"
  )
  target_include_directories(icon_shared_memory_kinematics_types_joint_limits_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_kinematics_types_joint_limits_test PRIVATE
    icon_shared_memory_kinematics_types_joint_limits
    icon_shared_memory_eigenmath
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_kinematics_types_joint_limits_test)
endif()

if(BUILD_TESTING)
  add_executable(icon_shared_memory_platform_common_buffers_rt_promise_test
    "${CMAKE_CURRENT_LIST_DIR}/platform/common/buffers/rt_promise_test.cc"
  )
  target_include_directories(icon_shared_memory_platform_common_buffers_rt_promise_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_platform_common_buffers_rt_promise_test PRIVATE
    icon_shared_memory_platform_common_buffers_rt_promise
    icon_shared_memory_icon_testing_malloc_test
    icon_shared_memory_icon_utils_log
    icon_shared_memory_icon_utils_mock_log_sink
    icon_shared_memory_icon_utils_status
    icon_shared_memory_icon_utils_status_and_expected_test_macros
    icon_shared_memory_icon_utils_status_matchers
    icon_shared_memory_icon_utils_time
    GTest::gmock
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_platform_common_buffers_rt_promise_test)
endif()

add_executable(icon_shared_memory_icon_testing_malloc_test_main
  "${CMAKE_CURRENT_LIST_DIR}/icon/testing/malloc_test_main.cc"
)
target_include_directories(icon_shared_memory_icon_testing_malloc_test_main PRIVATE "${INSRC_ROOT}")
target_link_libraries(icon_shared_memory_icon_testing_malloc_test_main PRIVATE
  GTest::gmock
)
install(TARGETS icon_shared_memory_icon_testing_malloc_test_main RUNTIME DESTINATION bin)

if(BUILD_TESTING)
  add_executable(icon_shared_memory_platform_common_buffers_rt_queue_test
    "${CMAKE_CURRENT_LIST_DIR}/platform/common/buffers/rt_queue_test.cc"
  )
  target_include_directories(icon_shared_memory_platform_common_buffers_rt_queue_test PRIVATE "${INSRC_ROOT}")
  target_link_libraries(icon_shared_memory_platform_common_buffers_rt_queue_test PRIVATE
    icon_shared_memory_platform_common_buffers_rt_queue
    icon_shared_memory_icon_testing_malloc_test
    icon_shared_memory_icon_utils_time
    GTest::gmock
    GTest::gmock_main
  )
  gtest_add_tests(TARGET icon_shared_memory_platform_common_buffers_rt_queue_test)
endif()
