//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// [simd.alg]/10: simd::select(k, a, b) is a plain function template in namespace simd whose return
// type and body are expressed purely in terms of __simd_select_impl(k, a, b) -- an unqualified call
// found *only* via ADL on hidden friends declared inside basic_vec and basic_mask themselves
// ([simd.cond], [simd.mask.cond]). There is no primary __simd_select_impl declaration anywhere at
// namespace scope for ordinary lookup to find.
//
// This means select's four argument shapes are not one overload doing type dispatch internally --
// they are four independent hidden friends, each found by ADL only because at least one of k, a, b
// is a class type declared in that friend's enclosing class. If a shape's hidden friend goes
// missing, or its constraints are wrong, the call doesn't become a different overload: it fails to
// find any __simd_select_impl at all and select() itself becomes ill-formed via its trailing
// decltype. This file exercises all four shapes plus the two ordinary (non-ADL) select overloads,
// so a regression that drops or misconstrains any hidden friend shows up as a hard compile error
// here rather than a silently narrower overload set.
//
// One shape here is not incidental coverage: mask/scalar/scalar (basic_mask.h's third
// __simd_select_impl, constrained on same_as<T0,T1> && vectorizable<T0> && sizeof(T0) == Bytes) was
// this port's headline defect -- select(mask, scalar, scalar) did not compile at all before it was
// added. See the audit at
// https://claude.ai/code/artifact/ec199a8d-5630-4a21-a780-1b5aef8322ad#s9.

#include <cassert>
#include <concepts>
#include <simd>

// A bare `requires { ... }` at non-template (function-body) scope hard-errors in this toolchain
// instead of evaluating to false when every select() overload candidate fails via constraint or
// substitution failure -- routing the check through a named concept template forces it through
// normal template-argument-deduction machinery, which is SFINAE-friendly as expected.
template <class Mask, class T0, class T1>
concept select_shape4_viable = requires(const Mask& k, T0 a, T1 b) { std::simd::select(k, a, b); };

int main(int, char**) {
  using vec  = std::simd::vec<int, 4>;
  using mask = vec::mask_type;

  mask k([](auto i) { return static_cast<int>(i) % 2 == 0; }); // true, false, true, false

  // Shape 1: __simd_select_impl(mask, vec, vec) -- the hidden friend in basic_vec.h. ADL finds it
  // through the vec arguments; no free function declares it.
  {
    vec a(1), b(2);
    vec r = std::simd::select(k, a, b);
    static_assert(std::same_as<decltype(r), vec>);
    assert(r[0] == 1 && r[1] == 2 && r[2] == 1 && r[3] == 2);
  }

  // Shape 2: __simd_select_impl(mask, mask, mask) -- the hidden friend in basic_mask.h found
  // through the *first* argument's type, since all three arguments here are the same mask type.
  {
    mask a([](auto) { return true; });
    mask b([](auto) { return false; });
    mask r = std::simd::select(k, a, b);
    static_assert(std::same_as<decltype(r), mask>);
    assert(r[0] == true && r[1] == false && r[2] == true && r[3] == false);
  }

  // Shape 3: __simd_select_impl(mask, bool, bool) -> mask -- distinct from shape 2 (the a/b
  // arguments are plain bool, not basic_mask), and distinct from shape 4 below (a/b here must
  // deduce to the mask's own bool value_type, not an arbitrary vectorizable scalar).
  {
    mask r = std::simd::select(k, true, false);
    static_assert(std::same_as<decltype(r), mask>);
    assert(r[0] == true && r[1] == false && r[2] == true && r[3] == false);
  }

  // Shape 4: __simd_select_impl(mask, T, T) -> vec<T, size> for vectorizable, non-bool T -- the
  // fix for the headline defect. ADL finds this purely through k's type; a and b are scalar ints
  // with no simd type anywhere in their own type, so ordinary lookup contributes nothing here.
  {
    auto r = std::simd::select(k, 7, 9);
    static_assert(std::same_as<decltype(r), std::simd::vec<int, 4>>);
    assert(r[0] == 7 && r[1] == 9 && r[2] == 7 && r[3] == 9);

    // Shape 4's constraints (same_as<T0,T1> && vectorizable<T0> && sizeof(T0) == Bytes) are what
    // keep it from over-matching. Check both hold as checked facts, not just as "this one call
    // happens to compile": mismatched operand types must stay rejected (same_as<T0,T1>), and an
    // operand whose size doesn't match the mask's element size must stay rejected (sizeof(T0) ==
    // Bytes -- k's mask_type here is 4 bytes wide, matching int, not short). Always qualified as
    // std::simd:: so this can't accidentally probe an unrelated ::select from a C header.
    static_assert(!select_shape4_viable<mask, int, double>);
    static_assert(!select_shape4_viable<mask, short, short>);
  }

  // The two ordinary overloads in algorithms.h are declared at namespace scope and resolve by
  // normal overload resolution, not ADL -- included here so the boundary between "ADL-dispatched"
  // and "ordinarily-looked-up" select overloads is exercised in the same file.
  {
    // select(bool, T, U): [simd.alg]/9's plain ternary-replacement overload.
    int r = std::simd::select(true, 3, 4);
    assert(r == 3);
  }
  {
    // select(mask, T, U) itself, i.e. the dispatcher whose trailing decltype is what makes all four
    // shapes above type-check in the first place.
    vec a(5), b(6);
    auto r = std::simd::select(k, a, b);
    assert(r[0] == 5 && r[1] == 6);
  }

  return 0;
}
