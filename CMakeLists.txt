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

cmake_minimum_required(VERSION 3.19)
project(icon_shared_memory CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# -fPIC is required if these libraries are going to be linked into a plugin
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 1. Setup packages and tools
find_package(Eigen3 REQUIRED)
find_package(FlatBuffers REQUIRED)
find_package(GTest REQUIRED)
find_package(tl-expected REQUIRED)
find_program(FLATC_EXECUTABLE flatc REQUIRED)
include(
  "${CMAKE_CURRENT_SOURCE_DIR}/flatbuffer_definitions/targets.cmake")

enable_testing()
include(GoogleTest)

# Define repository root
get_filename_component(INSRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# 2. Include subdirectories in topological dependency order
include("${CMAKE_CURRENT_LIST_DIR}/targets.cmake")

# 3. Generate & Install CMake Package Files
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/icon_shared_memoryConfigVersion.cmake"
  VERSION 1.0.0
  COMPATIBILITY AnyNewerVersion
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/icon_shared_memoryConfig.cmake"
  "include(CMakeFindDependencyMacro)\n"
  "find_dependency(Eigen3)\n"
  "find_dependency(FlatBuffers)\n"
  "find_dependency(GTest)\n"
  "find_dependency(tl-expected)\n"
  "include(\"\$\{CMAKE_CURRENT_LIST_DIR\}/icon_shared_memoryTargets.cmake\")\n"
)

install(EXPORT icon_shared_memoryTargets
        FILE icon_shared_memoryTargets.cmake
        DESTINATION lib/cmake/icon_shared_memory
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/icon_shared_memoryConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/icon_shared_memoryConfigVersion.cmake"
  DESTINATION lib/cmake/icon_shared_memory
)