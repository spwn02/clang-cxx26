//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// Upstream bloomberg/clang-p2996#208: reflection-derived constexpr values in
// the reported class-template setup became non-constant.

#include <array>
#include <meta>
#include <string>

template <class T>
consteval std::size_t member_count() {
  constexpr auto context = std::meta::access_context::current();
  constexpr auto members =
      std::define_static_array(std::meta::nonstatic_data_members_of(^^T, context));
  std::size_t count = 0;
  template for (constexpr std::meta::info member : members)
    ++count;
  return count;
}

template <class T>
struct structure {
  static constexpr std::size_t size = member_count<T>();
  static constexpr std::array<std::size_t, size> values{};
};

struct dummy {
  std::string value;
};

static_assert(structure<dummy>::size == 1);
static_assert(structure<dummy>::values.size() == 1);

int main(int, char**) { return 0; }
