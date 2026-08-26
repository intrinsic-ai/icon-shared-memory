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

#ifndef KINEMATICS_TYPES_JOINT_LIMITS_H_
#define KINEMATICS_TYPES_JOINT_LIMITS_H_

#include <cstddef>

#include "eigenmath/types.h"
#include "icon/utils/status.h"
#include "tl/expected.hpp"

namespace intrinsic {

// Holds joint-space limits for position, velocity, acceleration, jerk and
// torque.
struct JointLimits {
  // Maximum size allowed.
  static constexpr eigenmath::VectorXd::Index kMaxSize =
      eigenmath::MAX_EIGEN_VECTOR_SIZE;

  // Makes JointLimits with each limit range set to (-infinity, infinity).
  //
  // `size` is the number of elements each limit vector should have,
  // corresponding to the number of joints.
  static tl::expected<JointLimits, RealtimeStatus> Unlimited(size_t size);

  // Returns the number of elements the `min_position` vector has, corresponding
  // to the number of joints. When `IsSizeConsistent()` is true, this equals the
  // size of each limit vector.
  eigenmath::VectorXd::Index size() const;

  // Returns true if all limit vectors have the same size.
  bool IsSizeConsistent() const;

  // Sets the size of all limit vectors to `size`. Clears all limit values to 0.
  RealtimeStatus SetSize(eigenmath::VectorXd::Index size);

  // Sets each limit range to (-infinity, infinity).
  void SetUnlimited();

  bool IsValid() const;

  bool operator==(const JointLimits& other) const;

  // Limit vectors.
  eigenmath::VectorNd min_position;
  eigenmath::VectorNd max_position;
  eigenmath::VectorNd max_velocity;
  eigenmath::VectorNd max_acceleration;
  eigenmath::VectorNd max_jerk;
  eigenmath::VectorNd max_torque;
};

JointLimits CreateSimpleJointLimits(int ndof, double max_position,
                                    double max_velocity,
                                    double max_acceleration, double max_jerk);

JointLimits CreateSimpleJointLimits(int ndof, double max_position,
                                    double max_velocity,
                                    double max_acceleration, double max_jerk,
                                    double max_effort);

}  // namespace intrinsic
#endif  // KINEMATICS_TYPES_JOINT_LIMITS_H_
