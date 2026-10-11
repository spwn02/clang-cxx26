//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// A non-type template parameter of type std::meta::info whose argument is a
// null reflection, a base specifier, a data member description or an
// annotation is substituted as a reflection expression; a generic lambda in
// the class template transforms the substituted body again, and that
// transformation must keep those reflection values as they are (it hit an
// unreachable in TreeTransform::TransformCXXReflectExpr).

#include <meta>

using namespace std::meta;

struct Base {};
struct Derived : Base {
  [[= 7]] int member;
};

constexpr info null_refl{};
constexpr info base_refl = bases_of(^^Derived, access_context::unchecked())[0];
constexpr info spec_refl = data_member_spec(^^int, {.name = "x"});
constexpr info annotation_refl = annotations_of(^^Derived::member)[0];

template <info R>
struct Holder {
  static constexpr bool is_null() {
    return [](auto) { return R == null_refl; }(0);
  }

  static constexpr bool is_same_as(info other) {
    return [](auto o) { return R == o; }(other);
  }

  template <class U>
  static constexpr bool member_template() {
    return [](auto) { return R == null_refl; }(U{});
  }
};

static_assert(Holder<null_refl>::is_null());
static_assert(!Holder<base_refl>::is_null());
static_assert(!Holder<spec_refl>::is_null());
static_assert(!Holder<annotation_refl>::is_null());

static_assert(Holder<null_refl>::is_same_as(null_refl));
static_assert(Holder<base_refl>::is_same_as(base_refl));
static_assert(Holder<spec_refl>::is_same_as(spec_refl));
static_assert(Holder<annotation_refl>::is_same_as(annotation_refl));
static_assert(!Holder<base_refl>::is_same_as(null_refl));

static_assert(Holder<null_refl>::member_template<int>());
static_assert(!Holder<base_refl>::member_template<int>());
static_assert(!Holder<spec_refl>::member_template<int>());
static_assert(!Holder<annotation_refl>::member_template<int>());

int main(int, char**) { return 0; }
