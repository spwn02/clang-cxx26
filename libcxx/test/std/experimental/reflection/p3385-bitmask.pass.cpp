//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fattribute-reflection

#ifdef TEST_IMPORT_STD
import std;
#else
#include <meta>
#include <type_traits>
#endif

// P3385R8 [meta.reflection.queries]: attribute_comparison is an
// implementation-defined bitmask type ([bitmask.types]).
using C = std::meta::attribute_comparison;
constexpr C ns = C::ignore_namespace;
constexpr C arg = C::ignore_argument;
constexpr C zero{};
static_assert(ns != zero && arg != zero && ns != arg);
static_assert((ns & arg) == zero);
static_assert((ns | arg) == (ns ^ arg));
static_assert((ns & ~ns) == zero);
static_assert((ns | zero) == ns);
static_assert((ns & (ns | arg)) == ns);
static_assert((ns ^ ns) == zero);
static_assert(~(~ns) == ns);
static_assert(std::is_same_v<decltype(ns & arg), C>);
static_assert(std::is_same_v<decltype(ns ^ arg), C>);
static_assert(std::is_same_v<decltype(~ns), C>);
static_assert(noexcept(ns | arg) && noexcept(ns & arg) && noexcept(ns ^ arg) && noexcept(~ns));
constexpr bool compound() {
  C c{};
  static_assert(std::is_same_v<decltype(c |= ns), C&>);
  static_assert(std::is_same_v<decltype(c &= ns), C&>);
  static_assert(std::is_same_v<decltype(c ^= ns), C&>);
  static_assert(noexcept(c |= ns) && noexcept(c &= ns) && noexcept(c ^= ns));
  if (&(c |= ns) != &c || c != ns) return false;
  if (&(c |= arg) != &c || c != (ns | arg)) return false;
  if (&(c &= ns) != &c || c != ns) return false;
  if (&(c ^= ns) != &c || c != zero) return false;
  return true;
}
static_assert(compound());

// P3385R8 has_attribute Returns: flags determine significant components.
struct [[nodiscard("message")]] S {};
static_assert(std::meta::has_attribute(^^S, ^^[[nodiscard("message")]]));
static_assert(!std::meta::has_attribute(^^S, ^^[[nodiscard("other")]]));
static_assert(std::meta::has_attribute(^^S, ^^[[nodiscard("other")]], (ns | arg) & ~ns));
static_assert(std::meta::attributes_of(^^S).size() == 1);
static_assert(std::meta::is_attribute(std::meta::attributes_of(^^S)[0]));

int main(int, char**) {}
