//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// [simd.permute.static] and [simd.permute.dynamic] both declare permute(v, arg) under the same
// name: the static form takes an index-generator callable (IdxMap, invoked per-lane at each index),
// the dynamic form takes an index *vector* (a simd-integral V). Before this port's
// __idx_map_1arg/__idx_map_2arg/__idx_map_like concepts existed, the static overload's IdxMap
// template parameter was unconstrained, so it also matched a dynamic call's index-vector argument
// -- an index vector is itself invocable-shaped enough in some generic-lambda contexts to trip
// deduction, and the two overloads became ambiguous rather than one cleanly winning. This file
// exercises both forms, plus the fixed-size non-defaulted-length static overload, so a
// regression that widens IdxMap's constraint back toward "unconstrained" shows up as an ambiguous
// call here instead of a silently-broadened overload set.

#include <cassert>
#include <concepts>
#include <cstddef>
#include <simd>

// A bare `requires { ... }` at non-template (function-body) scope hard-errors in this toolchain
// instead of evaluating to false when every permute() overload candidate fails via constraint or
// substitution failure -- routing the check through a named concept template forces it through
// normal template-argument-deduction machinery, which is SFINAE-friendly as expected.
template <class Vec, class Arg>
concept permute2_viable = requires(const Vec& v, Arg a) { std::simd::permute(v, a); };

// simd_size_type itself is exposition-only ([simd.expos]), so this concept spells the same
// underlying type (ptrdiff_t) directly rather than naming it.
template <std::ptrdiff_t Np, class Vec, class Arg>
concept permuteN_viable = requires(const Vec& v, Arg a) { std::simd::permute<Np>(v, a); };

int main(int, char**) {
  using vec = std::simd::vec<int, 4>;
  using idx = std::simd::vec<int, 4>;

  vec v([](auto i) { return static_cast<int>(i) + 10; }); // {10, 11, 12, 13}

  // [simd.permute.static]: IdxMap is a callable, found via __idx_map_1arg (gen(i)) or
  // __idx_map_2arg (gen(i, n)). Neither idx (a simd-integral vec, no operator()) nor a plain int
  // satisfies either concept, so this cannot be confused with the dynamic overload below.
  {
    auto reversed = std::simd::permute<4>(v, [](auto i) { return 3 - static_cast<int>(i); });
    static_assert(std::same_as<decltype(reversed), vec>);
    assert(reversed[0] == 13 && reversed[1] == 12 && reversed[2] == 11 && reversed[3] == 10);
  }
  {
    // The two-argument gen(i, n) form, exercised separately from gen(i) so both __idx_map_1arg and
    // __idx_map_2arg are covered, not just whichever one the concept disjunction picks first.
    auto rotated = std::simd::permute<4>(v, [](auto i, auto n) { return (static_cast<int>(i) + 1) % n; });
    static_assert(std::same_as<decltype(rotated), vec>);
    assert(rotated[0] == 11 && rotated[3] == 10);
  }
  {
    // Same-size overload with the length deduced from v rather than passed explicitly.
    auto same_size = std::simd::permute(v, [](auto i) { return static_cast<int>(i); });
    static_assert(std::same_as<decltype(same_size), vec>);
    assert(same_size[0] == 10 && same_size[3] == 13);
  }

  // [simd.permute.dynamic]: the second argument is a simd-integral *vector*, matched by
  // __simd_integral, not __idx_map_like -- this is what the static overload must NOT also accept.
  {
    idx indices([](auto i) { return 3 - static_cast<int>(i); });

    // The historical bug: an unconstrained static-permute IdxMap parameter also matched this call,
    // making it ambiguous between the static and dynamic overloads. Check unambiguous resolution as
    // a named fact, not just as a side effect of the call below compiling.
    static_assert(requires { std::simd::permute(v, indices); });

    auto reversed = std::simd::permute(v, indices);
    static_assert(std::same_as<decltype(reversed), vec>);
    assert(reversed[0] == 13 && reversed[1] == 12 && reversed[2] == 11 && reversed[3] == 10);
  }
  {
    // Neither overload should accept an argument that is nothing like an index-generator or an
    // index-vector -- this bounds how broad __idx_map_like/__simd_integral are allowed to drift.
    static_assert(!permute2_viable<vec, int>);
    static_assert(!permuteN_viable<4, vec, idx>);
  }

  // simd::mask has the identical static/dynamic permute split via a parallel pair of overloads
  // (resize_t<Np, mask> instead of resize_t<Np, vec>) -- exercise both against a mask too, since
  // the disambiguation lives in the same __idx_map_like/__simd_integral concepts either way.
  {
    using mask = vec::mask_type;
    mask k([](auto i) { return static_cast<int>(i) % 2 == 0; }); // true, false, true, false

    auto static_perm = std::simd::permute<4>(k, [](auto i) { return 3 - static_cast<int>(i); });
    static_assert(std::same_as<decltype(static_perm), mask>);
    assert(static_perm[0] == false && static_perm[3] == true);

    idx indices([](auto i) { return 3 - static_cast<int>(i); });
    auto dynamic_perm = std::simd::permute(k, indices);
    static_assert(std::same_as<decltype(dynamic_perm), mask>);
    assert(dynamic_perm[0] == false && dynamic_perm[3] == true);
  }

  return 0;
}
