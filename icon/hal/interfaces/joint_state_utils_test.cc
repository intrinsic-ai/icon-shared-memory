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

#include "icon/hal/interfaces/joint_state_utils.h"

#include <cstdint>

#include "flatbuffers/buffer.h"
#include "flatbuffers/detached_buffer.h"
#include "flatbuffers/verifier.h"
#include "flatbuffer_definitions/icon/hal/interfaces/joint_state.fbs.h"
#include "gtest/gtest.h"

using ::intrinsic_fbs::BuildJointAccelerationState;
using ::intrinsic_fbs::BuildJointPositionState;
using ::intrinsic_fbs::BuildJointTorqueState;
using ::intrinsic_fbs::BuildJointVelocityState;
using ::intrinsic_fbs::JointAccelerationState;
using ::intrinsic_fbs::JointPositionState;
using ::intrinsic_fbs::JointTorqueState;
using ::intrinsic_fbs::JointVelocityState;

namespace intrinsic::hardware {
namespace {

TEST(JointStateTest, CreateJointPositionStateWithSize) {
  uint32_t num_dof = 6;
  flatbuffers::DetachedBuffer buffer = BuildJointPositionState(num_dof);
  flatbuffers::Verifier verifier(buffer.data(), buffer.size());
  ASSERT_EQ(verifier.VerifyBuffer<JointPositionState>(), true);

  auto state = flatbuffers::GetMutableRoot<JointPositionState>(buffer.data());
  ASSERT_NE(state, nullptr);

  ASSERT_EQ(state->position()->size(), num_dof);
  for (int i = 0; i < num_dof; ++i) {
    EXPECT_EQ(state->position()->Get(i), 0.0);
  }
}

TEST(JointStateTest, CreateJointVelocityStateWithSize) {
  uint32_t num_dof = 6;
  flatbuffers::DetachedBuffer buffer = BuildJointVelocityState(num_dof);
  flatbuffers::Verifier verifier(buffer.data(), buffer.size());
  ASSERT_EQ(verifier.VerifyBuffer<JointVelocityState>(), true);

  auto state = flatbuffers::GetMutableRoot<JointVelocityState>(buffer.data());
  ASSERT_NE(state, nullptr);

  ASSERT_EQ(state->velocity()->size(), num_dof);
  for (int i = 0; i < num_dof; ++i) {
    EXPECT_EQ(state->velocity()->Get(i), 0.0);
  }
}

TEST(JointStateTest, CreateJointAccelerationStateWithSize) {
  uint32_t num_dof = 6;
  flatbuffers::DetachedBuffer buffer = BuildJointAccelerationState(num_dof);
  flatbuffers::Verifier verifier(buffer.data(), buffer.size());
  ASSERT_EQ(verifier.VerifyBuffer<JointAccelerationState>(), true);

  auto state =
      flatbuffers::GetMutableRoot<JointAccelerationState>(buffer.data());
  ASSERT_NE(state, nullptr);

  ASSERT_EQ(state->acceleration()->size(), num_dof);
  for (int i = 0; i < num_dof; ++i) {
    EXPECT_EQ(state->acceleration()->Get(i), 0.0);
  }
}

TEST(JointStateTest, CreateJointTorqueStateWithSize) {
  uint32_t num_dof = 6;
  flatbuffers::DetachedBuffer buffer = BuildJointTorqueState(num_dof);
  flatbuffers::Verifier verifier(buffer.data(), buffer.size());
  ASSERT_EQ(verifier.VerifyBuffer<JointTorqueState>(), true);

  auto state = flatbuffers::GetMutableRoot<JointTorqueState>(buffer.data());
  ASSERT_NE(state, nullptr);

  ASSERT_EQ(state->torque()->size(), num_dof);
  for (int i = 0; i < num_dof; ++i) {
    EXPECT_EQ(state->torque()->Get(i), 0.0);
  }
}

}  // namespace
}  // namespace intrinsic::hardware
