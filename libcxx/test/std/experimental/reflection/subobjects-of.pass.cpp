//===----------------------------------------------------------------------===//
//
// Copyright 2026 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

// <experimental/reflection>
//
// [meta.reflection.member.queries]

#include <meta>

#include <vector>

using std::meta::access_context;
using std::meta::bases_of;
using std::meta::info;
using std::meta::nonstatic_data_members_of;
using std::meta::subobjects_of;

struct Base1 {};
struct Base2 {};
struct Derived : Base1, Base2 {
  int first;
  double second;
};

consteval auto concatenate_subobjects() -> std::vector<info> {
  constexpr auto ctx = access_context::unchecked();
  std::vector<info> result = bases_of(^^Derived, ctx);
  for (info member : nonstatic_data_members_of(^^Derived, ctx))
    result.push_back(member);
  return result;
}

constexpr auto ctx = access_context::unchecked();

static_assert(subobjects_of(^^Derived, ctx).size() ==
              bases_of(^^Derived, ctx).size() +
                  nonstatic_data_members_of(^^Derived, ctx).size());
static_assert(subobjects_of(^^Derived, ctx) == concatenate_subobjects());
static_assert(subobjects_of(^^Derived, ctx)[0] == bases_of(^^Derived, ctx)[0]);
static_assert(subobjects_of(^^Derived, ctx)[1] == bases_of(^^Derived, ctx)[1]);
static_assert(subobjects_of(^^Derived, ctx)[2] ==
              nonstatic_data_members_of(^^Derived, ctx)[0]);
static_assert(subobjects_of(^^Derived, ctx)[3] ==
              nonstatic_data_members_of(^^Derived, ctx)[1]);

int main() {}
