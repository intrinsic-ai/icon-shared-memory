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

#!/usr/bin/env python3

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

"""
Helper script to translate the bazel build files for //incode/icon/no_absl to
CMake.

Running this script is the first step to export the absl-free shared-memory
comms layer from insrc to a Github repo.

# Inputs

## BUILD file hierarchy
The script reads the BUILD files in //incode/icon/no_absl and its
subdirectories. It does not require any special syntax.

## dependencies.json
The script requires a `dependencies.json` file to manage "external"
dependencies, i.e. those outside of the //incode/icon/no_absl directory.

This file must be a JSON object that maps each of the external dependencies to
an object that contains

* package_name: The name of the dependency to use with `find_package()`
* target_name: The name of the dependency to use with `target_link_libraries()`

Example `dependencies.json`:

```json
"@com_gitlab_libeigen_eigen//:eigen": {
  "package_name": "Eigen3",
  "target_name": "Eigen3::Eigen"
},
"@com_google_googletest//:gtest": {
  "package_name": "GTest",
  "target_name": "GTest::gtest"
},
```

# Outputs

The script creates a number of files:

* //CMakeLists.txt

  The main CMake config file, which

  * finds dependencies
  * sets global compiler options (C++ standard version, `-fPIE`)
  * creates Version.cmake and Config.cmake files to let other packages find the
    exported `icon_shared_memory` package
  * sets up package installation

* //targets.cmake

  Contains all targets from the no_absl subtree. They're all in a single file,
  rather than structured by bazel package, because CMake requires us to define a
  target's dependencies before the target itself, and some bazel packages
  contain targets that have dependency relations in both directions.

  That is, if we had one CMake file per package, there would be no correct order
  to load them in. Thus, all targets are in a single CMake file, in reverse
  dependency order (i.e. dependencies first, dependees last).

* //flatbuffer_definitions/targets.cmake

  A CMake config file that contains targets for the few flatbuffer definitions
  **outside** the //incode/icon/no_absl tree that //incode/icon/no_absl depends
  on.

  These files are not duplicated in the //incode/icon/no_absl to prevent version
  skew.

# Example use:

```bash
cd /path/to/insrc/incode/icon/no_absl
# Make sure to have a clean git workspace before this, so you can remove the
# generated CMake files after exporting.
python3 bazel_to_cmake.py

# Now run Copybara
cd /path/to/insrc
copybara \
  copy.bara.sky \
  no_absl_with_fbs \
  . \
  --folder-dir=/tmp/icon_no_absl_export

# Build the CMake package
# This requires:
# * Eigen3
# * GTest
# * flatbuffers
# * tl::expected
cd /tmp/icon_no_absl_export
mkdir build
cd build
cmake ..
make -j4  # or however many cores you want to use
```
"""

from dataclasses import dataclass
import json
import os
import subprocess
import sys
import xml.etree.ElementTree as ET


@dataclass
class BazelTarget:
  target_type: str
  label: str
  srcs: list[str]
  hdrs: list[str]
  linkopts: list[str]
  deps: list[str]


@dataclass
class BazelTargetData:
  all_targets: dict[str, BazelTarget]
  targets_by_package_path: dict[str, list[str]]


@dataclass
class DependencyMapping:
  # This is the CMake package name for the dependency.
  # Use with `find_package()`.
  package_name: str
  # This is the actual CMake target name for the dependency.
  # Use with `target_link_libraries`.
  target_name: str


EXTERNAL_FBS_TARGET_NAME = "icon_shared_memory_external_fbs_cc"


def main():
  script_dir = os.path.dirname(os.path.abspath(__file__))
  no_absl_root = script_dir
  if os.path.exists(os.path.join(no_absl_root, "MODULE.bazel")):
    bazel_workspace_root = no_absl_root
    is_standalone = True
  else:
    bazel_workspace_root = os.path.abspath(no_absl_root + "/../../..")
    is_standalone = False

  package_name_prefix = get_package_name_prefix(
      no_absl_root, bazel_workspace_root
  )

  if not is_standalone:
    handle_google3_flatbuffers(no_absl_root, package_name_prefix)

  deps_map_path = os.path.join(no_absl_root, "dependencies.json")
  external_dependency_map = load_dependency_map(deps_map_path)

  sorted_targets = find_bazel_targets(
      package_name_prefix, bazel_workspace_root, no_absl_root, is_standalone
  )
  sorted_targets = topological_sort_targets(
      sorted_targets, package_name_prefix, external_dependency_map
  )

  # Generate one big CMake file with ALL THE TARGETS
  generate_cmake_targets_file_for_workspace(
      package_name_prefix=package_name_prefix,
      targets=sorted_targets,
      bazel_start_dir=no_absl_root,
      dependency_map=external_dependency_map,
  )
  generate_root_cmake_file(no_absl_root, external_dependency_map)


def handle_google3_flatbuffers(bazel_start_dir, package_name_prefix):
  """Creates flatbuffer_files.bara.sky for flatbuffer deps from outside of `bazel_start_dir`.

  Args:
    bazel_start_dir: The root directory that we want to export from
    package_name_prefix: All bazel targets in `bazel_start_dir` share this prefix.
  """
  bazel_cmd = [
      "bazel",
      "cquery",
      "--output",
      "files",
      (
          'filter(".*\\.fbs", filter("//google3/...",'
          f' deps("{package_name_prefix}/...")))'
      ),
  ]

  # Sort the paths alphabetically and filter out any empty output lines
  fbs_file_paths = [
      p
      for p in sorted(
          subprocess.run(
              bazel_cmd,
              capture_output=True,
              text=True,
              check=True,
          ).stdout.split("\n")
      )
      if p
  ]

  # Ensure flatbuffers.bzl is always exported to provide Starlark macros for Bazel
  if "flatbuffer_definitions/platform/flatbuffers.bzl" not in fbs_file_paths:
    fbs_file_paths.append("flatbuffer_definitions/platform/flatbuffers.bzl")

  # Drop the flatbuffer paths into flatbuffer_files.bara.sky
  flatbuffer_files_bara_sky_contents = ["FLATBUFFER_FILES = ["]
  flatbuffer_files_bara_sky_contents.extend(
      [f'    "{path}",' for path in sorted(fbs_file_paths)]
  )
  flatbuffer_files_bara_sky_contents.append("]")
  bara_sky_path = os.path.join(bazel_start_dir, "flatbuffer_files.bara.sky")
  print(f"Writing {bara_sky_path}...")
  with open(bara_sky_path, "w", encoding="utf-8") as out:
    out.write("\n".join(flatbuffer_files_bara_sky_contents) + "\n")


def load_dependency_map(dependency_map_path) -> dict[str, DependencyMapping]:
  """Loads a map from (canonical) bazel target name to its CMake replacement

  See docstrings on DependencyMapping for details about the replacements.
  """
  if not os.path.exists(dependency_map_path):
    print(
        f"Error: Mapping file {dependency_map_path} does not exist.",
        file=sys.stderr,
    )
    sys.exit(1)

  with open(dependency_map_path, encoding="utf-8") as f:
    return {
        k: DependencyMapping(
            package_name=v.get("package_name", ""),
            target_name=v.get("target_name", ""),
        )
        for (k, v) in json.load(f).items()
    }


def get_package_name_prefix(bazel_start_dir, bazel_workspace_root_dir):
  """
  Returns the common Bazel package name prefix for targets in `bazel_start_dir`, for a workspace rooted in `bazel_workspace_root_dir`.

  The return value does NOT have a final '/' character.
  """
  if os.path.abspath(bazel_start_dir) == os.path.abspath(bazel_workspace_root_dir):
    return ""

  relative_start_dir = os.path.relpath(
      path=bazel_start_dir, start=bazel_workspace_root_dir
  )
  if relative_start_dir.startswith(".."):
    raise ValueError(
        f"bazel_start_dir ({bazel_start_dir}) must be below bazel_root_dir"
        f" ({bazel_workspace_root_dir}), but the path relative to"
        f" bazel_root_dir is {relative_start_dir}"
    )
  return ("//" + relative_start_dir).rstrip("/")


def find_bazel_targets(
    package_name_prefix,
    bazel_workspace_root,
    bazel_start_dir,
    is_standalone: bool = False,
) -> list[BazelTarget]:
  """Uses bazel query to find and convert all targets we want to convert, in reverse topological order.

  That is, the most depended-on targets come first in the output list, so that
  we can declare them before their dependees in the generated CMake.

  Supported target types:
    * cc_library
    * cc_binary
    * cc_test
    * flatbuffers_library
    * cc_flatbuffers_library
  """
  # This command writes (to stdout) the canonical target paths of all targets
  # of the above kinds.
  if is_standalone:
    query_scope = "//..."
  else:
    query_scope = (
        f"deps({package_name_prefix}/...) intersect"
        f" ({package_name_prefix}/... union //flatbuffer_definitions/...)"
    )

  bazel_cmd = [
      "bazel",
      "query",
      "--output",
      "xml",
      "--order_output",
      "deps",
      (
          f"kind('(cc_library|cc_binary|cc_test|flatbuffers_library|cc_flatbuffers_library) rule', {query_scope})"
      ),
  ]
  try:
    target_xml = subprocess.run(
        bazel_cmd,
        capture_output=True,
        text=True,
        check=True,
    ).stdout

  except subprocess.CalledProcessError as e:
    print(f"Output: {e.output}")
    print(f"stderr: {e.stderr}")
    raise

  xml_root = ET.fromstring(target_xml)
  # Very simple example of bazel query XML output:
  # <?xml version="1.1" encoding="UTF-8" standalone="no"?>
  # <query version="2">
  #   <rule class="cc_library" location="/workspaces/insrc/icon/utils/BUILD:264:11" name="//icon/utils:time">
  #     <string name="name" value="time"/>
  #     <string name="generator_name" value="time"/>
  #     <string name="generator_function" value="cc_library"/>
  #     <string name="generator_location" value="icon/utils/BUILD:264:11"/>
  #     <list name="srcs">
  #         <label value="//icon/utils:time.cc"/>
  #     </list>
  #     <list name="hdrs">
  #         <label value="//icon/utils:time.h"/>
  #     </list>
  #     <rule-input name="//icon/utils:time.cc"/>
  #     <rule-input name="//icon/utils:time.h"/>
  #     <rule-input name="@bazel_tools//tools/cpp:current_cc_toolchain"/>
  #     <rule-input name="@bazel_tools//tools/def_parser:def_parser"/>
  #   </rule>
  # </query>
  #
  # We care about only some of the attibutes and sub-elements:
  # * rule.class to determine the kind of CMake rule to generate
  # * rule.name for the name. this is the canonical workspace name, so no need
  #   to combine it with rule.location to get a unique name!
  # * <list name="srcs">... for the source files. Need to translate label names
  #   to file names, but that's an easy transformation (unless an input file is
  #   generated, which thankfully none of our inputs are)
  # * <list name="hdrs">... for headers. Similar to srcs
  # * <list name="deps">... for dependencies
  # * <list name="linkopts">...
  if xml_root.tag != "query":
    raise ValueError(
        "Expected root tag of bazel query XML output to be 'query', but got"
        f" '{xml_root.tag}'"
    )

  def label_to_filename(label):
    # * Strip the leading '//' or '@//'
    # * Replace the ':' at the end with a slash
    # * Join the result with the workspace root
    clean_label = label.removeprefix("@//").removeprefix("//")
    if clean_label.startswith("flatbuffer_definitions/"):
      rel = clean_label.removeprefix("flatbuffer_definitions/").replace(":", "/")
      return f"flatbuffer_definitions/{rel}"
    return os.path.relpath(
        os.path.join(bazel_workspace_root, clean_label.replace(":", "/")),
        bazel_start_dir,
    )

  targets = []
  for rule in xml_root:
    if rule.tag != "rule":
      print(
          f"Got unexpected child of type '{rule.tag}' - we expect all children"
          " to be 'rule'"
      )
      continue
    # Get rule class
    target_type = rule.get("class") or ""

    # Get name
    label = rule.get("name") or ""

    # Special behavior for cc_flatbuffers_library (this is a macro that
    # generates two targets, but we only want to create one CMake rule):
    # * Ignore the one that's not *_internal (the other target only depends on
    #   the internal one)
    # * Rename the internal target to match the original macro name
    #
    # Get generator, if any
    generator_function = rule.find("./string[@name='generator_function']")
    if (
        generator_function is not None
        and generator_function.get("value") == "cc_flatbuffers_library"
    ):
      if not label.endswith("internal"):
        continue
      target_type = "cc_flatbuffers_library"
      # Get generator name, and use that to replace the final part of the label
      generator_name = rule.find("./string[@name='generator_name']")
      if generator_name is None:
        raise ValueError(f"generator_name is missing for target {label}")
      label = label.split(":")[0] + ":" + (generator_name.get("value") or "")
    # Get sources
    sources = []
    source_elems = rule.find("./list[@name='srcs']")
    if source_elems is not None:
      sources = [
          label_to_filename(name)
          for source in source_elems
          if (name := source.get("value")) is not None
      ]

    # Get headers
    headers = []
    header_elems = rule.find("./list[@name='hdrs']")
    if header_elems is not None:
      headers = [
          label_to_filename(name)
          for header in header_elems
          if (name := header.get("value")) is not None
      ]

    # Check that all source and header files actually exist
    for source in sources:
      if source.startswith("flatbuffer_definitions/") and not is_standalone:
        rel = source.removeprefix("flatbuffer_definitions/")
        exists = os.path.exists(
            os.path.join(bazel_workspace_root, "google3/intrinsic", rel)
        )
      else:
        exists = os.path.exists(os.path.join(bazel_start_dir, source))
      if not exists:
        raise ValueError(
            f"{target_type}({label}): Source file {source} does not exist"
        )
    for header in headers:
      if header.startswith("flatbuffer_definitions/") and not is_standalone:
        rel = header.removeprefix("flatbuffer_definitions/")
        exists = os.path.exists(
            os.path.join(bazel_workspace_root, "google3/intrinsic", rel)
        )
      else:
        exists = os.path.exists(os.path.join(bazel_start_dir, header))
      if not exists:
        raise ValueError(
            f"{target_type}({label}): Header file {header} does not exist"
        )

    # Get linkopts
    linkopts = []
    linkopt_elems = rule.find("./list[@name='linkopts']")
    if linkopt_elems is not None:
      linkopts = [
          name
          for linkopt in linkopt_elems
          if (name := linkopt.get("value")) is not None
      ]

    # Get deps
    deps = []
    dep_elems = rule.find("./list[@name='deps']")
    if dep_elems is not None:
      deps = [
          name for dep in dep_elems if (name := dep.get("value")) is not None
      ]

    targets.append(
        BazelTarget(
            target_type=target_type,
            label=label,
            srcs=sources,
            hdrs=headers,
            linkopts=linkopts,
            deps=deps,
        )
    )
  return list(reversed(targets))


def canonicalize_bazel_label(package_path, label, package_name_prefix):
  """
  Converts a Bazel label to its fully-qualified canonical form relative to the
  workspace root.

  Examples:
    - ':status' under 'icon/utils' -> '//icon/utils:status'
    - 'status' under 'icon/utils' -> '//icon/utils:status'
    - '//icon/utils' -> '//icon/utils:utils'
    - '//google3/foo:bar' -> '//google3/foo:bar'
  """
  clean_label = label.removeprefix("@//")
  if clean_label.startswith(package_name_prefix) if package_name_prefix else clean_label.startswith("//"):
    # Label is qualified and inside the directory we're exporting, but may omit
    # the target name if it matches the package name
    parts = clean_label.split(":")
    if len(parts) == 2:
      return clean_label
    target_name = clean_label.split("/")[-1]
    return f"{clean_label}:{target_name}"
  if clean_label.startswith("//") or clean_label.startswith("@"):
    # Label is fully qualified but outside the directory we're exporting. Leave
    # unchanged so that `dependencies.json` substitutions work.
    return clean_label
  if clean_label.startswith(":"):
    # Local label reference, add package_name_prefix and package path
    if package_path:
      prefix = f"{package_name_prefix}/" if package_name_prefix else "//"
      return f"{prefix}{package_path}{clean_label}"
    prefix = package_name_prefix if package_name_prefix else "//"
    return f"{prefix}{clean_label}"

  # Local label name, add package_name_prefix and package path, **and** add
  # colon before the label.
  if package_path:
    prefix = f"{package_name_prefix}/" if package_name_prefix else "//"
    return f"{prefix}{package_path}:{clean_label}"
  prefix = package_name_prefix if package_name_prefix else "//"
  return f"{prefix}:{clean_label}"


def label_to_package_name(label: str, package_name_prefix: str):
  clean_label = label.removeprefix("@//")
  if clean_label.startswith("//flatbuffer_definitions/"):
    rel = clean_label.removeprefix("//flatbuffer_definitions/")
    pkg = rel.split(":")[0]
    return f"flatbuffer_definitions/{pkg}"
  if clean_label.startswith("//flatbuffer_definitions/"):
    rel = clean_label.removeprefix("//flatbuffer_definitions/")
    pkg = rel.split(":")[0]
    return f"flatbuffer_definitions/{pkg}"
  parts = clean_label.removeprefix(package_name_prefix).lstrip("/").split(":")
  return parts[0]


def label_to_cmake_target(
    canonical_label: str,
    package_name_prefix: str,
    external_dependency_map: dict[str, DependencyMapping],
) -> str:
  """
  Translates a canonical Bazel label to the corresponding CMake target name.

  Rules:
    - If the label is mapped in dependencies.json, return the mapped target name.
    - If the label is a local target (in the subtree), map it to the form
      'icon_shared_memory_<pkg_path_underscores>_<target_name>'.
      If target_name is identical to the last package path component, simplify to
      'icon_shared_memory_<pkg_path_underscores>'.
  """
  clean_label = canonical_label.removeprefix("@//")
  if clean_label in external_dependency_map:
    cmake_target = external_dependency_map[clean_label].target_name
    return cmake_target
  if canonical_label in external_dependency_map:
    cmake_target = external_dependency_map[canonical_label].target_name
    return cmake_target
  if clean_label.startswith("//flatbuffer_definitions/"):
    rel = clean_label.removeprefix("//flatbuffer_definitions/")
    package, target = rel.split(":")
    pkg_underscores = package.replace("/", "_")
    last_comp = package.split("/")[-1]
    if target == last_comp:
      return f"icon_shared_memory_{pkg_underscores}"
    return f"icon_shared_memory_{pkg_underscores}_{target}"
  if clean_label.startswith("//flatbuffer_definitions/"):
    rel = clean_label.removeprefix("//flatbuffer_definitions/")
    package, target = rel.split(":")
    pkg_underscores = package.replace("/", "_")
    last_comp = package.split("/")[-1]
    if target == last_comp:
      return f"icon_shared_memory_{pkg_underscores}"
    return f"icon_shared_memory_{pkg_underscores}_{target}"
  if clean_label.startswith(package_name_prefix):
    parts = (
        clean_label.removeprefix(package_name_prefix).lstrip("/").split(":")
    )
    package = parts[0]
    target = parts[1]
    package_underscores = package.replace("/", "_")
    last_package_component = package.split("/")[-1]
    if target == last_package_component:
      return f"icon_shared_memory_{package_underscores}"
    else:
      return f"icon_shared_memory_{package_underscores}_{target}"
  else:
    raise ValueError(
        f"External target {clean_label} is not mapped in dependencies.json!"
    )


def topological_sort_targets(
    targets: list[BazelTarget],
    package_name_prefix: str,
    dependency_map: dict[str, DependencyMapping],
) -> list[BazelTarget]:
  """Sorts targets in topological dependency order with deterministic alphabetical tie-breaking."""
  target_map = {t.label: t for t in targets}
  target_name_map = {}
  for t in targets:
    name = label_to_cmake_target(t.label, package_name_prefix, dependency_map)
    if name:
      target_name_map[t.label] = name

  deps_map = {t.label: set() for t in targets}
  dependents_map = {t.label: set() for t in targets}

  for t in targets:
    for d in t.deps:
      if d in target_map and d != t.label:
        deps_map[t.label].add(d)
        dependents_map[d].add(t.label)

  in_degree = {label: len(deps) for label, deps in deps_map.items()}
  ready = sorted(
      [label for label, deg in in_degree.items() if deg == 0],
      key=lambda l: target_name_map.get(l, l),
  )

  result = []
  while ready:
    curr_label = ready.pop(0)
    result.append(target_map[curr_label])
    for dependent in sorted(
        dependents_map[curr_label], key=lambda l: target_name_map.get(l, l)
    ):
      in_degree[dependent] -= 1
      if in_degree[dependent] == 0:
        dep_key = target_name_map.get(dependent, dependent)
        bisect_idx = 0
        while bisect_idx < len(ready) and target_name_map.get(
            ready[bisect_idx], ready[bisect_idx]
        ) < dep_key:
          bisect_idx += 1
        ready.insert(bisect_idx, dependent)

  if len(result) != len(targets):
    for t in targets:
      if t not in result:
        result.append(t)

  return result


def generate_cmake_targets_file_for_workspace(
    package_name_prefix: str,
    targets: list[BazelTarget],
    bazel_start_dir: str,
    dependency_map: dict[str, DependencyMapping],
):
  """Generates `targets.cmake` for the entire workspace.

  The generated file contains CMake targets for each supported bazel target, in
  the same order as `target_data` (which should be reverse dependency order).

  This function maps any bazel dependencies onto corresponding (internal or
  external) CMake dependencies using `dependency_map`.
  """
  cmake_lines = []
  cmake_lines.append(
      "# Automatically generated from BUILD files by bazel_to_cmake.py"
  )
  cmake_lines.append("")

  for target in targets:
    t_type = target.target_type
    srcs = target.srcs
    hdrs = target.hdrs
    deps = target.deps
    linkopts = target.linkopts

    target_name = label_to_cmake_target(
        target.label, package_name_prefix, dependency_map
    )
    if not target_name:
      continue

    # Map deps
    cmake_deps = []
    for dep in deps:
      dep_name = label_to_cmake_target(dep, package_name_prefix, dependency_map)
      if not dep_name:
        continue
      cmake_deps.append(dep_name)
    for opt in linkopts:
      if opt.startswith("-l"):
        cmake_deps.append(opt[2:])
      else:
        cmake_deps.append(opt)

    package_name = label_to_package_name(target.label, package_name_prefix)
    # Destination directory path for header installation
    if package_name.startswith("flatbuffer_definitions/"):
      install_dest = f"include/{package_name}".rstrip("/")
    elif package_name_prefix:
      install_dest = f"include/{package_name}".rstrip("/")
    else:
      install_dest = f"include/{package_name}".rstrip("/")

    cpp_extensions = (".cc", ".cpp", ".c", ".cxx", ".C")

    if t_type == "cc_library":
      # Check header-only
      cpp_srcs = [s for s in srcs if s.endswith(cpp_extensions)]
      is_header_only = not cpp_srcs

      if is_header_only:
        cmake_lines.append(f"add_library({target_name} INTERFACE)")
        if hdrs:
          cmake_lines.append(f"target_sources({target_name} PRIVATE")
          for h in sorted(hdrs):
            cmake_lines.append(f'  "${{CMAKE_CURRENT_LIST_DIR}}/{h}"')
          cmake_lines.append(")")
        cmake_lines.append(
            f"target_include_directories({target_name} INTERFACE"
        )
        cmake_lines.append('  "$<BUILD_INTERFACE:${INSRC_ROOT}>"')
        cmake_lines.append('  "$<INSTALL_INTERFACE:include>"')
        cmake_lines.append(")")
        if cmake_deps:
          cmake_lines.append(f"target_link_libraries({target_name} INTERFACE")
          for d in cmake_deps:
            cmake_lines.append(f"  {d}")
          cmake_lines.append(")")
      else:
        all_files = sorted(srcs + hdrs)
        cmake_lines.append(f"add_library({target_name} STATIC")
        for f in all_files:
          cmake_lines.append(f'  "${{CMAKE_CURRENT_LIST_DIR}}/{f}"')
        cmake_lines.append(")")
        cmake_lines.append(f"target_include_directories({target_name} PUBLIC")
        cmake_lines.append('  "$<BUILD_INTERFACE:${INSRC_ROOT}>"')
        cmake_lines.append('  "$<INSTALL_INTERFACE:include>"')
        cmake_lines.append(")")
        if cmake_deps:
          cmake_lines.append(f"target_link_libraries({target_name} PUBLIC")
          for d in cmake_deps:
            cmake_lines.append(f"  {d}")
          cmake_lines.append(")")

      # Install target
      cmake_lines.append(f"install(TARGETS {target_name}")
      cmake_lines.append("        EXPORT icon_shared_memoryTargets")
      cmake_lines.append("        LIBRARY DESTINATION lib")
      cmake_lines.append("        ARCHIVE DESTINATION lib")
      cmake_lines.append("        RUNTIME DESTINATION bin")
      cmake_lines.append("        INCLUDES DESTINATION include")
      cmake_lines.append(")")

      # Install associated headers
      if hdrs:
        cmake_lines.append("install(FILES")
        for h in sorted(hdrs):
          cmake_lines.append(f'        "${{CMAKE_CURRENT_LIST_DIR}}/{h}"')
        cmake_lines.append(f'        DESTINATION "{install_dest}"')
        cmake_lines.append(")")

      cmake_lines.append("")

    elif t_type == "cc_binary":
      all_files = sorted(srcs + hdrs)
      cmake_lines.append(f"add_executable({target_name}")
      for f in all_files:
        cmake_lines.append(f'  "${{CMAKE_CURRENT_LIST_DIR}}/{f}"')
      cmake_lines.append(")")
      cmake_lines.append(
          f'target_include_directories({target_name} PRIVATE "${{INSRC_ROOT}}")'
      )
      if cmake_deps:
        cmake_lines.append(f"target_link_libraries({target_name} PRIVATE")
        for d in cmake_deps:
          cmake_lines.append(f"  {d}")
        cmake_lines.append(")")
      cmake_lines.append(
          f"install(TARGETS {target_name} RUNTIME DESTINATION bin)"
      )
      cmake_lines.append("")

    elif t_type == "cc_test":
      all_files = sorted(srcs + hdrs)
      cmake_lines.append("if(BUILD_TESTING)")
      cmake_lines.append(f"  add_executable({target_name}")
      for f in all_files:
        cmake_lines.append(f'    "${{CMAKE_CURRENT_LIST_DIR}}/{f}"')
      cmake_lines.append("  )")
      cmake_lines.append(
          f"  target_include_directories({target_name} PRIVATE"
          ' "${INSRC_ROOT}")'
      )
      if cmake_deps:
        cmake_lines.append(f"  target_link_libraries({target_name} PRIVATE")
        for d in cmake_deps:
          cmake_lines.append(f"    {d}")
        cmake_lines.append("  )")
      cmake_lines.append(f"  gtest_add_tests(TARGET {target_name})")
      cmake_lines.append("endif()")
      cmake_lines.append("")

    elif t_type == "flatbuffers_library":
      # Just install original schemas
      if srcs:
        cmake_lines.append("install(FILES")
        for s in sorted(srcs):
          cmake_lines.append(f'        "${{CMAKE_CURRENT_LIST_DIR}}/{s}"')
        cmake_lines.append(f'        DESTINATION "{install_dest}"')
        cmake_lines.append(")")
        cmake_lines.append("")

    elif t_type == "cc_flatbuffers_library":
      # Compile schemas of dependencies
      fbs_srcs = []
      for dep in deps:
        dep_target = next((t for t in targets if t.label == dep), None)
        if dep_target and dep_target.target_type == "flatbuffers_library":
          fbs_srcs.extend(dep_target.srcs)

      # Define custom commands and output targets for each .fbs schema file
      outputs = []
      for fbs in sorted(fbs_srcs):
        fbs_base = os.path.splitext(fbs)[0]
        pkg_path = os.path.dirname(fbs)
        out_header = f"${{CMAKE_CURRENT_BINARY_DIR}}/{fbs_base}.fbs.h"
        outputs.append(out_header)

        cmake_lines.append("add_custom_command(")
        cmake_lines.append(f'  OUTPUT "{out_header}"')
        cmake_lines.append(
            '  COMMAND "${FLATC_EXECUTABLE}" --cpp --filename-suffix .fbs'
            " --keep-prefix"
            " --reflect-names"
            " --scoped-enums"
            " --gen-mutable"
            " --filename-ext h"
        )
        cmake_lines.append(
            "          -o"
            f' "${{CMAKE_CURRENT_BINARY_DIR}}/{pkg_path}"'
        )
        cmake_lines.append('          -I "${INSRC_ROOT}"')
        cmake_lines.append(f'          "${{CMAKE_CURRENT_LIST_DIR}}/{fbs}"')
        cmake_lines.append(f'  DEPENDS "${{CMAKE_CURRENT_LIST_DIR}}/{fbs}"')
        cmake_lines.append(
            f'  COMMENT "Generating C++ Flatbuffers headers for {fbs}"'
        )
        cmake_lines.append(")")

      # Interface Library Target
      cmake_lines.append(f"add_library({target_name} INTERFACE)")
      cmake_lines.append(f"target_include_directories({target_name} INTERFACE")
      cmake_lines.append('  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"')
      cmake_lines.append('  "$<INSTALL_INTERFACE:include>"')
      cmake_lines.append(")")
      if outputs:
        cmake_lines.append(f"target_sources({target_name} PRIVATE")
        for o in outputs:
          cmake_lines.append(f'  "{o}"')
        cmake_lines.append(")")

      cmake_lines.append(f"target_link_libraries({target_name} INTERFACE")
      cmake_lines.append("  flatbuffers::flatbuffers")
      cmake_lines.append(")")
      cmake_lines.append(f"install(TARGETS {target_name}")
      cmake_lines.append("        EXPORT icon_shared_memoryTargets")
      cmake_lines.append(")")

      # Install generated flatbuffers headers
      if outputs:
        cmake_lines.append("install(FILES")
        for o in outputs:
          cmake_lines.append(f'        "{o}"')
        cmake_lines.append(f'        DESTINATION "{install_dest}"')
        cmake_lines.append(")")

      cmake_lines.append("")

  # Write targets.cmake in the subdirectory
  os.makedirs(bazel_start_dir, exist_ok=True)
  cmake_file_path = os.path.join(bazel_start_dir, "targets.cmake")
  print(f"Writing {cmake_file_path}...")
  with open(cmake_file_path, "w", encoding="utf-8") as out:
    out.write("\n".join(cmake_lines))


def generate_root_cmake_file(bazel_start_dir, external_dependency_map):
  """Generates the root CMakeLists.txt file.

  This file includes the `targets.cmake` files for each subdirectory ("package")
  as well as for the "external" flatbuffer definitions (see
  `handle_google3_flatbuffers()`).
  """
  root_cmake_lines = [
      "cmake_minimum_required(VERSION 3.19)",
      "project(icon_shared_memory CXX)",
      "",
      "set(CMAKE_CXX_STANDARD 20)",
      "set(CMAKE_CXX_STANDARD_REQUIRED ON)",
      "",
      "include(CMakePackageConfigHelpers)",
      "include(GNUInstallDirs)",
      "",
      (
          "# -fPIC is required if these libraries are going to be linked into a"
          " plugin"
      ),
      "set(CMAKE_POSITION_INDEPENDENT_CODE ON)",
      "",
      "# 1. Setup packages and tools",
  ]

  packages_to_find = {
      dep.package_name
      for dep in external_dependency_map.values()
      if dep.package_name
  }
  packages_to_find.add("FlatBuffers")
  for package_name in sorted(packages_to_find):
    root_cmake_lines.append(f"find_package({package_name} REQUIRED)")

  root_cmake_lines.append("find_program(FLATC_EXECUTABLE flatc REQUIRED)")

  root_cmake_lines.extend([
      "",
      "enable_testing()",
      "include(GoogleTest)",
      "",
      "# Define repository root",
      (
          'get_filename_component(INSRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}"'
          " ABSOLUTE)"
      ),
      "",
      "# 2. Include subdirectories in topological dependency order",
  ])

  root_cmake_lines.append('include("${CMAKE_CURRENT_LIST_DIR}/targets.cmake")')

  root_cmake_lines.extend([
      "",
      "# 3. Generate & Install CMake Package Files",
      "write_basic_package_version_file(",
      '  "${CMAKE_CURRENT_BINARY_DIR}/icon_shared_memoryConfigVersion.cmake"',
      "  VERSION 1.0.0",
      "  COMPATIBILITY AnyNewerVersion",
      ")",
      "",
  ])

  # Generate config file. This lets CMake ensure all of our external
  # dependencies are present.
  config_file_lines = ["include(CMakeFindDependencyMacro)"]
  for package_name in sorted(packages_to_find):
    config_file_lines.append(f"find_dependency({package_name})")

  config_file_lines.append(
      # We need to escape the quotes because this line is passed to CMake's
      # `file()` function.
      # We also need to escape the variable expansion, because we want to
      # evaluate CMAKE_CURRENT_LIST_DIR when `icon_shared_memoryConfig.cmake`
      # is evaluated, not when CMakeLists.txt is.
      r"""include(\"\$\{CMAKE_CURRENT_LIST_DIR\}/icon_shared_memoryTargets.cmake\")"""
  )

  root_cmake_lines.append(
      'file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/icon_shared_memoryConfig.cmake"'
  )
  for cl in config_file_lines:
    root_cmake_lines.append(f'  "{cl}\\n"')
  root_cmake_lines.append(")")

  root_cmake_lines.extend([
      "",
      "install(EXPORT icon_shared_memoryTargets",
      "        FILE icon_shared_memoryTargets.cmake",
      "        DESTINATION lib/cmake/icon_shared_memory",
      ")",
      "",
      "install(FILES",
      '  "${CMAKE_CURRENT_BINARY_DIR}/icon_shared_memoryConfig.cmake"',
      '  "${CMAKE_CURRENT_BINARY_DIR}/icon_shared_memoryConfigVersion.cmake"',
      "  DESTINATION lib/cmake/icon_shared_memory",
      ")",
  ])

  root_cmake_path = os.path.join(bazel_start_dir, "CMakeLists.txt")
  print(f"Writing {root_cmake_path}...")
  with open(root_cmake_path, "w", encoding="utf-8") as out:
    out.write("\n".join(root_cmake_lines))


if __name__ == "__main__":
  main()
