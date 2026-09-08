//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest
// ADDITIONAL_COMPILE_FLAGS: -fentity-proxy-reflection

#include <meta>

#include <cassert>

constexpr auto unchecked = std::meta::access_context::unchecked();

struct B {
  int f() const { return 1; }
  static int g() { return 2; }
  static int s;
  int field;
  using T = int;
  struct N {};
};

struct D : private B {
  using B::f;
  using B::g;
  using B::s;
  using B::field;
  using B::T;
  using B::N;
};

consteval bool check_member_queries() {
  int proxies = 0;
  for (auto m : members_of(^^D, unchecked)) {
    if (!is_entity_proxy(m))
      continue;
    ++proxies;
    if (is_constructor(m) || is_destructor(m) ||
        is_special_member_function(m) || is_static_member(m))
      return false;
    if (is_enumerable_type(m))
      return false;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    if (has_complete_definition(m))
      return false;
#pragma clang diagnostic pop
  }
  return proxies == 6;
}
static_assert(check_member_queries());

template <class T> struct OB {
  int value() & { return 1; }
  int value() const & { return 2; }
  int value() && { return 3; }
  int value() const && { return 4; }
  int operator*() const { return 5; }
};

template <class T> struct S : private OB<T> {
  using OB<T>::value;
  using OB<T>::operator*;
};

template <std::meta::info M> int probe() { return 7; }

void check_proxy_as_template_arg() {
  int n = 0;
  int total = 0;
  const void *proxy_addrs[5] = {};
  template for (constexpr auto m : std::define_static_array(
          members_of(^^S<int>, unchecked))) {
    if constexpr (is_entity_proxy(m)) {
      total += probe<m>();
      proxy_addrs[n] = reinterpret_cast<const void *>(&probe<m>);
      assert(proxy_addrs[n] !=
             reinterpret_cast<const void *>(&probe<underlying_entity_of(m)>));
      ++n;
    }
  }
  assert(n == 5);
  assert(total == 35);
  for (int i = 0; i < n; ++i)
    for (int j = i + 1; j < n; ++j)
      assert(proxy_addrs[i] != proxy_addrs[j]);
}

int main() {
  check_proxy_as_template_arg();
  return 0;
}
