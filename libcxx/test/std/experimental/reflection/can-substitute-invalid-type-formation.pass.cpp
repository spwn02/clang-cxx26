//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// <experimental/reflection>
//
// [reflection]
//
// Regression test: can_substitute must report substitution failure -- not
// crash -- when substituting the arguments into the declaration forms an
// invalid type inside a template-id (e.g. a reference to void).

#include <meta>
#include <vector>

namespace meta = std::meta;
constexpr auto ctx = meta::access_context::unchecked();

template <class T> struct trait { using type = void; };
template <class OT> typename trait<OT&>::type f() {}

template <class T> struct Exp {
  template <class OT = T>
  typename trait<OT&>::type swap(Exp&) {}
};

template <class T> using Ref = typename trait<T&>::type;
template <class T> typename trait<T&>::type* vt = nullptr;

consteval int probe_member_templates(meta::info cls) {
  int substitutable = 0, probed = 0;
  for (auto mem : meta::members_of(cls, ctx))
    if (meta::is_function_template(mem)) {
      ++probed;
      if (meta::can_substitute(mem, std::vector<meta::info>{}))
        ++substitutable;
    }
  return substitutable * 100 + probed;
}

static_assert(meta::can_substitute(^^f, {^^int}));
static_assert(meta::substitute(^^f, {^^int}) != meta::info{});
static_assert(!meta::can_substitute(^^f, {^^void}));

static_assert(probe_member_templates(^^Exp<void>) == 1);
static_assert(probe_member_templates(^^Exp<int>) == 101);

static_assert(!meta::can_substitute(^^Ref, {^^void}));
static_assert(meta::can_substitute(^^Ref, {^^int}));
static_assert(!meta::can_substitute(^^vt, {^^void}));
static_assert(meta::can_substitute(^^vt, {^^int}));

int main() { return 0; }
