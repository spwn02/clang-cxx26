//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -Wno-constant-logical-operand

#include <cstddef>
#include <string.h>
#include <type_traits>
#include <utility>

enum class E { value = 2 };

struct Assignable {
  int value;
  constexpr Assignable(int v) : value(v) {}
  constexpr Assignable(const Assignable&) = default;
  constexpr bool operator==(const Assignable&) const = default;
  constexpr Assignable operator=(Assignable other) const { return other; }
};

struct Logical {
  int value;
  constexpr bool operator==(const Logical&) const = default;
  friend constexpr Logical operator&&(Logical, Logical) { return {7}; }
  friend constexpr Logical operator||(Logical, Logical) { return {8}; }
};

struct Mutator {
  int value;
  constexpr bool operator==(const Mutator&) const = default;
  constexpr Mutator operator++() const { return {value + 1}; }
  constexpr Mutator operator++(int) const { return {value + 1}; }
  constexpr Mutator operator--() const { return {value - 1}; }
  constexpr Mutator operator--(int) const { return {value - 1}; }
};

constexpr int add(int x, int y) { return x + y; }
constexpr int values[] = {4, 8, 15};
struct S { int member; };
constexpr S object{9};

template <class T>
struct is_cw : std::false_type {};
template <auto X, class T>
struct is_cw<std::constant_wrapper<X, T>> : std::true_type {};
template <class L, class R>
concept has_comma = requires(L l, R r) { l, r; };
template <class L, class R>
concept has_assign = requires(L l, R r) { l = r; };
#define DEFINE_CW_COMPOUND_CONCEPT(__name, __op) \
  template <class L, class R> \
  concept __name = requires(L l, R r) { l __op r; }
DEFINE_CW_COMPOUND_CONCEPT(has_plus_assign, +=);
DEFINE_CW_COMPOUND_CONCEPT(has_minus_assign, -=);
DEFINE_CW_COMPOUND_CONCEPT(has_mult_assign, *=);
DEFINE_CW_COMPOUND_CONCEPT(has_div_assign, /=);
DEFINE_CW_COMPOUND_CONCEPT(has_mod_assign, %=);
DEFINE_CW_COMPOUND_CONCEPT(has_and_assign, &=);
DEFINE_CW_COMPOUND_CONCEPT(has_or_assign, |=);
DEFINE_CW_COMPOUND_CONCEPT(has_xor_assign, ^=);
DEFINE_CW_COMPOUND_CONCEPT(has_lshift_assign, <<=);
DEFINE_CW_COMPOUND_CONCEPT(has_rshift_assign, >>=);
#undef DEFINE_CW_COMPOUND_CONCEPT

static_assert(std::constant_wrapper<42>::value == 42);
static_assert(std::is_same_v<std::constant_wrapper<42>::type, std::constant_wrapper<42>>);
static_assert(std::is_same_v<std::constant_wrapper<42>::value_type, int>);
static_assert(std::constant_wrapper<true>::value);
static_assert(std::constant_wrapper<E::value>::value == E::value);
static_assert(std::constant_wrapper<&values[0]>::value == values);
static_assert(static_cast<int>(std::constant_wrapper<42>{}) == 42);

using Assigned = decltype(std::constant_wrapper<Assignable{1}>{} = std::constant_wrapper<Assignable{2}>{});
static_assert(is_cw<Assigned>::value && Assigned::value.value == 2);
static_assert(!has_assign<std::constant_wrapper<1>, std::constant_wrapper<2>>);

static_assert((+std::constant_wrapper<2>{}).value == 2);
static_assert((-std::constant_wrapper<2>{}).value == -2);
static_assert((~std::constant_wrapper<2>{}).value == ~2);
static_assert((!std::constant_wrapper<0>{}).value);
static_assert(is_cw<decltype(operator&(std::constant_wrapper<2>{}))>::value);
static_assert((*std::constant_wrapper<&values[0]>{}).value == 4);

static_assert((std::constant_wrapper<7>{} + std::constant_wrapper<2>{}).value == 9);
static_assert((std::constant_wrapper<7>{} - std::constant_wrapper<2>{}).value == 5);
static_assert((std::constant_wrapper<7>{} * std::constant_wrapper<2>{}).value == 14);
static_assert((std::constant_wrapper<7>{} / std::constant_wrapper<2>{}).value == 3);
static_assert((std::constant_wrapper<7>{} % std::constant_wrapper<2>{}).value == 1);
static_assert((std::constant_wrapper<7>{} << std::constant_wrapper<2>{}).value == 28);
static_assert((std::constant_wrapper<7>{} >> std::constant_wrapper<2>{}).value == 1);
static_assert((std::constant_wrapper<7>{} & std::constant_wrapper<2>{}).value == 2);
static_assert((std::constant_wrapper<7>{} | std::constant_wrapper<2>{}).value == 7);
static_assert((std::constant_wrapper<7>{} ^ std::constant_wrapper<2>{}).value == 5);
static_assert((std::constant_wrapper<1>{} < std::constant_wrapper<2>{}).value);
static_assert((std::constant_wrapper<1>{} <= std::constant_wrapper<1>{}).value);
static_assert((std::constant_wrapper<1>{} == std::constant_wrapper<1>{}).value);
static_assert((std::constant_wrapper<1>{} != std::constant_wrapper<2>{}).value);
static_assert((std::constant_wrapper<2>{} > std::constant_wrapper<1>{}).value);
static_assert((std::constant_wrapper<2>{} >= std::constant_wrapper<2>{}).value);
static_assert(is_cw<decltype(std::constant_wrapper<&object>{} ->* std::constant_wrapper<&S::member>{})>::value);

static_assert((std::constant_wrapper<1>{} && std::constant_wrapper<2>{}) == true);
static_assert((std::constant_wrapper<0>{} || std::constant_wrapper<2>{}) == true);
using L = decltype(std::constant_wrapper<Logical{1}>{} && std::constant_wrapper<Logical{2}>{});
static_assert(is_cw<L>::value && L::value.value == 7);

static_assert(!has_comma<std::constant_wrapper<1>, std::constant_wrapper<2>>);

using Callable = std::constant_wrapper<&add>;
using Called = decltype(Callable{}(std::constant_wrapper<1>{}, std::constant_wrapper<2>{}));
static_assert(is_cw<Called>::value && Called::value == 3);
static_assert(Callable{}(1, 2) == 3);
using Indexed = std::constant_wrapper<values>;
using IndexedResult = decltype(Indexed{}[std::constant_wrapper<1>{}]);
static_assert(is_cw<IndexedResult>::value && IndexedResult::value == 8);
static_assert(Indexed{}[1] == 8);

using M = std::constant_wrapper<Mutator{4}>;
static_assert((++M{}).value.value == 5);
static_assert((M{}++).value.value == 5);
static_assert((--M{}).value.value == 3);
static_assert((M{}--).value.value == 3);

static_assert(!has_plus_assign<std::constant_wrapper<1>, std::constant_wrapper<2>>);
static_assert(!has_minus_assign<std::constant_wrapper<1>, std::constant_wrapper<2>>);
static_assert(!has_mult_assign<std::constant_wrapper<1>, std::constant_wrapper<2>>);
static_assert(!has_div_assign<std::constant_wrapper<1>, std::constant_wrapper<2>>);
static_assert(!has_mod_assign<std::constant_wrapper<1>, std::constant_wrapper<2>>);
static_assert(!has_and_assign<std::constant_wrapper<1>, std::constant_wrapper<2>>);
static_assert(!has_or_assign<std::constant_wrapper<1>, std::constant_wrapper<2>>);
static_assert(!has_xor_assign<std::constant_wrapper<1>, std::constant_wrapper<2>>);
static_assert(!has_lshift_assign<std::constant_wrapper<1>, std::constant_wrapper<2>>);
static_assert(!has_rshift_assign<std::constant_wrapper<1>, std::constant_wrapper<2>>);

static_assert(__cpp_lib_constant_wrapper == 202606L);

int main() {}
