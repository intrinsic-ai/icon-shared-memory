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

#include "icon/flatbuffers/flatbuffer_utils.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

#include "flatbuffers/buffer.h"
#include "flatbuffers/flatbuffer_builder.h"
#include "flatbuffers/vector.h"
#include "gmock/gmock.h"
#include "flatbuffer_definitions/icon/flatbuffers/transform_types.fbs.h"
#include "flatbuffer_definitions/icon/interprocess/shared_memory_manager/segment_info.fbs.h"
#include "gtest/gtest.h"
#include "icon/utils/status.h"

namespace intrinsic_fbs {
namespace {

template <typename T>
using decay_ptr_t =
    std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<T>>>;

// Compare fields of `Point` directly because FlatBuffers struct types generated
// externally do not have `operator==` defined.
MATCHER_P(PointEq, expected, "") {
  // The internal `flatbuffers::Vector::Get` function returns a pointer while
  // the external implementation returns a value. We handle both cases with the
  // same matcher.
  ::testing::StaticAssertTypeEq<intrinsic_fbs::Point,
                                decay_ptr_t<expected_type>>();
  ::testing::StaticAssertTypeEq<intrinsic_fbs::Point, decay_ptr_t<arg_type>>();

  auto to_pointer = [](auto& item) {
    if constexpr (std::is_pointer_v<std::decay_t<decltype(item)>>) {
      return item;
    } else {
      return std::addressof(item);
    }
  };

  const auto* arg_ptr = to_pointer(arg);
  const auto* exp_ptr = to_pointer(expected);

  if (arg_ptr == nullptr) {
    *result_listener << "argument is a null pointer";
    return false;
  }
  if (exp_ptr == nullptr) {
    *result_listener << "expected parameter is a null pointer";
    return false;
  }

  return arg_ptr->x() == exp_ptr->x() && arg_ptr->y() == exp_ptr->y() &&
         arg_ptr->z() == exp_ptr->z();
}

TEST(FlatbufferArrayNumElementsTest, ReturnsCorrectNumElements) {
  EXPECT_EQ(FlatbufferArrayNumElements(&SegmentInfo::names),
            SegmentInfo{}.names()->size());
}

TEST(FlatbufferUtilsTest, CopiesFlatbufferDoubleVector) {
  auto create_vector = [](flatbuffers::FlatBufferBuilder& builder,
                          const std::vector<double>& data) {
    auto vectorOffset = builder.CreateVector(data);
    builder.Finish(vectorOffset);

    uint8_t* buffer = builder.GetBufferPointer();
    flatbuffers::Vector<double>* retrievedVector =
        flatbuffers::GetMutableRoot<flatbuffers::Vector<double>>(buffer);
    return retrievedVector;
  };

  const size_t kNDof = 6;
  std::vector<double> ones(kNDof, 1.0);
  std::vector<double> zeros(kNDof, 0.0);
  flatbuffers::FlatBufferBuilder builder;
  flatbuffers::FlatBufferBuilder builder2;
  flatbuffers::Vector<double>* vector = create_vector(builder, ones);
  flatbuffers::Vector<double>* vector2 = create_vector(builder2, zeros);
  for (int i = 0; i < kNDof; ++i) {
    EXPECT_EQ(zeros.at(i), vector2->Get(i));
  }
  auto result = CopyFbsVector(*vector, *vector2);
  EXPECT_EQ(result.code, intrinsic::StatusCode::kOk) << ToString(result);
  for (int i = 0; i < kNDof; ++i) {
    EXPECT_EQ(ones.at(i), vector2->Get(i));
  }
}

TEST(FlatbufferUtilsTest, CopiesFlatbufferPointVector) {
  intrinsic_fbs::Point zero;
  zero.mutate_x(0.0);
  zero.mutate_y(0.0);
  zero.mutate_z(0.0);
  intrinsic_fbs::Point one;
  one.mutate_x(1.0);
  one.mutate_y(1.0);
  one.mutate_z(1.0);

  auto create_vector = [](flatbuffers::FlatBufferBuilder& builder,
                          const std::vector<intrinsic_fbs::Point>& data) {
    auto vectorOffset = builder.CreateVectorOfStructs(data);
    builder.Finish(vectorOffset);

    uint8_t* buffer = builder.GetBufferPointer();
    flatbuffers::Vector<intrinsic_fbs::Point>* retrievedVector =
        flatbuffers::GetMutableRoot<flatbuffers::Vector<intrinsic_fbs::Point>>(
            buffer);
    return retrievedVector;
  };

  const size_t kNDof = 6;
  std::vector<intrinsic_fbs::Point> ones;
  for (int i = 0; i < kNDof; ++i) {
    ones.push_back(one);
  }
  std::vector<intrinsic_fbs::Point> zeros;
  for (int i = 0; i < kNDof; ++i) {
    zeros.push_back(zero);
  }
  flatbuffers::FlatBufferBuilder builder;
  flatbuffers::FlatBufferBuilder builder2;
  flatbuffers::Vector<intrinsic_fbs::Point>* vector =
      create_vector(builder, ones);
  flatbuffers::Vector<intrinsic_fbs::Point>* vector2 =
      create_vector(builder2, zeros);
  // Compare fields directly because FlatBuffers struct types generated
  // externally do not have operator== defined.
  for (int i = 0; i < kNDof; ++i) {
    EXPECT_THAT(vector2->Get(i), PointEq(zero));
  }
  auto result = CopyFbsVector(*vector, *vector2);
  EXPECT_EQ(result.code, intrinsic::StatusCode::kOk) << ToString(result);
  for (int i = 0; i < kNDof; ++i) {
    EXPECT_THAT(vector2->Get(i), PointEq(one));
  }
}

TEST(FlatbufferUtilsTest, CopiesFlatbufferDoubleVectorWithWrongSize) {
  auto create_vector = [](flatbuffers::FlatBufferBuilder& builder,
                          const std::vector<double>& data) {
    auto vectorOffset = builder.CreateVector(data);
    builder.Finish(vectorOffset);

    uint8_t* buffer = builder.GetBufferPointer();
    flatbuffers::Vector<double>* retrievedVector =
        flatbuffers::GetMutableRoot<flatbuffers::Vector<double>>(buffer);
    return retrievedVector;
  };

  const size_t kNDof = 6;
  std::vector<double> ones(kNDof, 1.0);
  std::vector<double> zeros(kNDof + 1, 0.0);
  flatbuffers::FlatBufferBuilder builder;
  flatbuffers::FlatBufferBuilder builder2;
  flatbuffers::Vector<double>* vector = create_vector(builder, ones);
  flatbuffers::Vector<double>* vector2 = create_vector(builder2, zeros);

  auto result = CopyFbsVector(*vector, *vector2);

  EXPECT_EQ(result.code, intrinsic::StatusCode::kOutOfRange)
      << ToString(result);
}

}  // namespace
}  // namespace intrinsic_fbs
