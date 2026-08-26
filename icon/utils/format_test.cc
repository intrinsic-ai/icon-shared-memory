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

#include "icon/utils/format.h"

#include <string>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace intrinsic {
namespace {

using ::testing::HasSubstr;

struct CustomStreamableType {
  std::string name;
  std::string title;
  int age;
};

template <class Ostream>
inline Ostream&& operator<<(Ostream&& str, const CustomStreamableType& t) {
  if (!t.title.empty()) {
    str << t.title << " ";
  }
  str << t.name << " (" << t.age << ")";
  return std::forward<Ostream>(str);
}
}  // namespace

TEST(FormatMap, WorksWithBuiltinTypes) {
  // someone who is good at the economy please help me budget this. my family is
  // dying
  auto accounts = std::unordered_map<std::string, int>{
      {"Food", 200},     {"Data", 150},    {"Rent", 800},
      {"Candles", 3600}, {"Utility", 150},
  };
  std::string formatted =
      FormatMap(accounts, /*element_separator=*/"\n", /*pair_separator=*/" $",
                /*key_quotes=*/"|", /*value_quotes=*/"*");
  // The map is unordered, so we expect all of these substrings, but in an
  // unknown order.
  EXPECT_THAT(formatted, HasSubstr("|Food| $*200*"));
  EXPECT_THAT(formatted, HasSubstr("|Data| $*150*"));
  EXPECT_THAT(formatted, HasSubstr("|Rent| $*800*"));
  EXPECT_THAT(formatted, HasSubstr("|Candles| $*3600*"));
  EXPECT_THAT(formatted, HasSubstr("|Utility| $*150*"));
}

TEST(FormatMap, WorksWithCustomType) {
  auto accounts = std::unordered_map<std::string, CustomStreamableType>{
      {"Captain", {.name = "Jane Doe", .title = "Dame", .age = 53}},
      {"Bosun", {.name = "Frank", .title = "", .age = 19}},
  };

  std::string formatted =
      FormatMap(accounts, /*element_separator=*/"\n", /*pair_separator=*/": ",
                /*key_quotes=*/"'",
                /*value_quotes=*/"");
  // The map is unordered, so we expect all of these substrings, but in an
  // unknown order.
  EXPECT_THAT(formatted, HasSubstr("'Captain': Dame Jane Doe (53)"));
  EXPECT_THAT(formatted, HasSubstr("'Bosun': Frank (19)"));
}

}  // namespace intrinsic
