# icon-shared-memory

`icon-shared-memory` provides the shared-memory inter-process communication (IPC) layer, real-time data structures, and utilities used by [Intrinsic](https://www.intrinsic.ai/)'s Real-Time Control Framework (**ICON**).

This library provides lock-free and low-latency inter-process communication primitives (such as shared memory lockstep synchronization, binary futexes, and real-time buffers) designed for high-frequency robotic control loops and hardware abstraction layers.

---

## Repository Contents

- `eigenmath/`: Math and geometry utilities interoperable with Eigen.
- `flatbuffer_definitions/`: [FlatBuffers](https://flatbuffers.dev/) schema definitions and CMake generation targets.
- `icon/`: Core ICON shared-memory communication, HAL interfaces, hardware module runtime, and real-time utilities.
  - `icon/control/`: Real-time clock interfaces.
  - `icon/flatbuffers/`: [FlatBuffers](https://flatbuffers.dev/) serialization utilities and schema headers.
  - `icon/hal/`: Hardware interface registries, traits, hardware module lifecycle utilities, and clock interfaces.
  - `icon/interprocess/`: Shared memory manager, binary futexes, and remote trigger server/client.
  - `icon/testing/`: Test fixtures, real-time annotations, and allocation assertion helpers.
  - `icon/utils/`: Real-time guards, mutexes, status helpers, and timing utilities.
- `kinematics/`: Joint limits and kinematic type definitions.
- `platform/`: Real-time queue buffers, promises, and synchronization primitives.
- `util/`: Low-level system and threading utilities (such as lockstep synchronization).

---

## Build Instructions

### Bazel Build (Default / Source of Truth)

**Bazel** is the default build system and serves as the single source of truth for all target definitions, dependencies, and code structure in this repository.

#### Consuming as a Bazel Dependency (Bzlmod / Workspace)

This repository is structured for consumption within a Bazel workspace using **Bzlmod**.

To add `icon-shared-memory` to your project's `MODULE.bazel`:

```bzl
# Example: Adding icon_shared_memory via local_path_override or git_override
bazel_dep(name = "icon_shared_memory")
local_path_override(
    module_name = "icon_shared_memory",
    path = "/path/to/icon-shared-memory",
)
```

Downstream targets can then depend on the exported C++ libraries:

```bzl
cc_library(
    name = "my_hardware_module",
    srcs = ["my_hardware_module.cc"],
    deps = [
        "@icon_shared_memory//icon/interprocess/shared_memory_manager",
        "@icon_shared_memory//icon/hal:hardware_module_runtime",
        "@icon_shared_memory//icon/utils:realtime_guard",
    ],
)
```

> **Note:** Building with Bazel requires a C++20 compliant toolchain (Clang or GCC) and rules for external dependencies (such as `@rules_cc`).

---

### CMake Build (Derivative / For Downstream Projects)

CMake build support is provided as a derivative for downstream projects and workflows that require CMake integration.

The CMake build files (`CMakeLists.txt` and the various `targets.cmake` files across the repository) are **generated from the Bazel `BUILD` files** using the [`bazel_to_cmake.py`](bazel_to_cmake.py) script.

#### Prerequisites

Ensure the following dependencies and tools are installed on your system:
- **C++20 Compiler** ([Clang](https://clang.llvm.org/get_started.html) >= 13 or [GCC](https://gcc.gnu.org/install/download.html) >= 11)
- [**CMake**](https://cmake.org/download/) (>= 3.19)
- [**Eigen3**](https://libeigen.gitlab.io/releases/)
- [**FlatBuffers**](https://github.com/google/flatbuffers#quick-start) (including the `flatc` compiler executable and development headers)
- [**GoogleTest**](https://github.com/google/googletest) (`GTest` / `GMock`)
- [**tl-expected**](https://github.com/TartanLlama/expected): C++17 implementation of the C++23 feature [`std::expected`](https://en.cppreference.com/cpp/utility/expected).

On Debian/Ubuntu systems, prerequisites can typically be installed via:
```bash
sudo apt-get update && sudo apt-get install \
    build-essential \
    clang \
    cmake \
    libeigen3-dev \
    libflatbuffers-dev \
    flatbuffers-compiler \
    libgtest-dev \
    libgmock-dev \
    libexpected-dev
```

#### Building with CMake

To configure, build, and test in a dedicated `build/` directory:

```bash
# 1. Configure the build directory (with tests enabled)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON

# 2. Compile the libraries and tests
cmake --build build -j$(nproc)

# 3. Run the test suite
ctest --test-dir build --output-on-failure
```

#### Installation

To install the CMake targets and package configuration files:

```bash
# Install to default prefix or specify --prefix <path>
cmake --install build
```

Downstream CMake projects can then consume this library via:
```cmake
find_package(icon_shared_memory REQUIRED)
target_link_libraries(your_target PRIVATE icon_shared_memory::icon_shared_memory_icon_interprocess)
```

---

## Build File Synchronization (`bazel_to_cmake.py`)

Because Bazel is the primary source of truth, **CMake build files should never be modified manually**. Regenerating the CMake files via `bazel_to_cmake.py` is a mandatory prerequisite when contributing changes and proposing Pull Requests upstream, ensuring CMake and Bazel build targets remain fully synchronized:

1. Any changes to targets, sources, compiler flags, or dependencies **must be made in the Bazel `BUILD` files first**.
2. After updating the Bazel configuration, regenerate the CMake files:

```bash
python3 bazel_to_cmake.py
```

If a change in Bazel targets does not produce the expected CMake configuration (such as newly introduced external dependencies or custom build rules), update [`dependencies.json`](dependencies.json) or [`bazel_to_cmake.py`](bazel_to_cmake.py) accordingly to support the new translation.

---

## Related Repositories

- **[icon-hwm-controller](https://github.com/intrinsic-ai/icon-hwm-controller)**: A Hardware Module (HWM) controller implementation that leverages `icon-shared-memory` to interface real-time hardware control loops with the `ros2_control` framework.



---

## Documentation and related repositories

* [**Intrinsic Developer Community**](https://developer.intrinsic.ai): Complete guides, interactive tutorials, and API references.
* [**icon-hwm-controller**](https://github.com/intrinsic-ai/icon-hwm-controller): An Intrinsic Hardware Module (HWM) for [`ros2_control`](https://control.ros.org/) hardware interfaces built on top of `icon-shared-memory`.

---

## Contributing and community

Contributions are welcome! Please review:

* [CONTRIBUTING.md](CONTRIBUTING.md): Details on signing the Google Contributor License Agreement (CLA), community guidelines, C++20 coding standards, and pull request workflows.  
* [SECURITY.md](SECURITY.md): Instructions for reporting security vulnerabilities.

---

## License

This project is licensed under the [Apache 2.0 License](LICENSE).

---

> **Disclaimer**: This is not an officially supported Google product.

---

### Trademark notice

"Intrinsic" and "Intrinsic Core" are trademarks of Intrinsic Innovation LLC. See [TRADEMARK.md](TRADEMARK.md) for usage guidelines.
