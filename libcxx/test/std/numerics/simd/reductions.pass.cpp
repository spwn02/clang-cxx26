//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// [simd.mask.reductions] and [simd.reductions].
//
// Two structural things this file specifically targets, beyond plain value checks:
//   - reduce's masked overload has two Constraints-disjoint shapes, distinguished by
//     __is_known_reduction_op<BinaryOperation>: plus/multiplies/bit_and/bit_or/bit_xor get a
//     *defaulted* identity_element (the clause's own five special-cased operations,
//     [simd.reductions]/9), any other BinaryOperation requires the caller to supply one explicitly.
//     Both shapes are exercised here, not just the defaulted one.
//   - reduce_min/reduce_max's masked overloads Return numeric_limits<T>::max()/lowest() when
//     none_of(mask) holds (reductions.h:214-215, 238-239) -- an all-false mask is the one input
//     shape whose result isn't derivable from "some in-range element", so it gets its own check.

#include <cassert>
#include <concepts>
#include <functional>
#include <limits>
#include <simd>

int main(int, char**) {
  using vec4 = std::simd::vec<int, 4>;

  // ======================= [simd.mask.reductions] =======================
  {
    vec4::mask_type all_true(true);
    vec4::mask_type all_false(false);
    vec4::mask_type mixed([](auto i) { return static_cast<int>(i) < 2; }); // T,T,F,F

    assert(std::simd::all_of(all_true) && !std::simd::all_of(mixed) && !std::simd::all_of(all_false));
    assert(std::simd::any_of(mixed) && !std::simd::any_of(all_false) && std::simd::any_of(all_true));
    assert(std::simd::none_of(all_false) && !std::simd::none_of(mixed));
    assert(std::simd::reduce_count(mixed) == 2 && std::simd::reduce_count(all_true) == 4 &&
           std::simd::reduce_count(all_false) == 0);
    assert(std::simd::reduce_min_index(mixed) == 0);
    assert(std::simd::reduce_max_index(mixed) == 1);
  }

  // Scalar-bool overloads -- [simd.mask.reductions]/9-12, a different overload set entirely (plain
  // bool argument, not a basic_mask), sharing the same names.
  {
    assert(std::simd::all_of(true) && !std::simd::all_of(false));
    assert(std::simd::any_of(true) && !std::simd::any_of(false));
    assert(!std::simd::none_of(true) && std::simd::none_of(false));
    assert(std::simd::reduce_count(true) == 1 && std::simd::reduce_count(false) == 0);
    assert(std::simd::reduce_min_index(true) == 0);
    assert(std::simd::reduce_max_index(true) == 0);
  }

  // ======================= [simd.reductions] =======================
  vec4 x([](auto i) { return static_cast<int>(i) + 1; }); // {1,2,3,4}

  // reduce, unmasked, defaulted BinaryOperation (plus<>).
  {
    static_assert(std::same_as<decltype(std::simd::reduce(x)), int>);
    assert(std::simd::reduce(x) == 10);
  }
  // reduce, unmasked, explicit BinaryOperation.
  {
    assert(std::simd::reduce(x, std::multiplies<>{}) == 24);
  }

  // reduce, masked, known op (multiplies<>) -- identity_element defaults to 1
  // (__reduction_identity<multiplies<>, int>()), not 0.
  {
    vec4::mask_type k([](auto i) { return static_cast<int>(i) % 2 == 0; }); // T,F,T,F -> {1,3}
    assert(std::simd::reduce(x, k, std::multiplies<>{}) == 3);
  }
  // reduce, masked, known op, all-false mask -- Returns identity_element without touching x at all.
  {
    vec4::mask_type k(false);
    assert(std::simd::reduce(x, k, std::multiplies<>{}) == 1);
    assert(std::simd::reduce(x, k) == 0); // plus<>'s identity
  }
  // reduce, masked, *unknown* op -- Constraints require an explicit identity_element argument (no
  // default exists for this shape); exercised with a lambda-wrapped op so it's provably not one of
  // the five special-cased std::functional types.
  //
  // __reduction_binary_operation ([simd.expos.defn]/7-8) checks the op against vec<T,1>, not T
  // itself -- binary_op(vec<T,1>, vec<T,1>) -> vec<T,1> -- even though reduce's own loop invokes it
  // with plain scalar T values. The lambda must therefore work generically over both: select(a>b,
  // a, b) does, since select(bool,T,U) (the scalar shape) and select(mask,vec,vec) (the vec shape,
  // found via ADL) are both spelled the same way at the call site.
  {
    auto max_op = [](auto a, auto b) { return std::simd::select(a > b, a, b); };
    vec4::mask_type k([](auto i) { return static_cast<int>(i) == 1 || static_cast<int>(i) == 3; }); // F,T,F,T -> {2,4}
    assert(std::simd::reduce(x, k, max_op, std::numeric_limits<int>::min()) == 4);
  }

  // reduce, scalar overloads -- [simd.reductions]/10-14, a plain T argument (not basic_vec) always
  // just returns that value; the masked scalar form still exercises the same
  // known-op/identity-defaulting split as the vec form above.
  {
    assert(std::simd::reduce(7) == 7);
    assert(std::simd::reduce(7, true, std::multiplies<>{}) == 7);
    assert(std::simd::reduce(7, false, std::multiplies<>{}) == 1);
    auto min_op = [](auto a, auto b) { return std::simd::select(a < b, a, b); };
    assert(std::simd::reduce(7, false, min_op, 999) == 999);
  }

  // reduce_min/reduce_max, unmasked.
  {
    assert(std::simd::reduce_min(x) == 1);
    assert(std::simd::reduce_max(x) == 4);
  }
  // reduce_min/reduce_max, masked -- an in-range subset picks the right extreme among only the
  // masked-in lanes, not the whole vec.
  {
    vec4::mask_type k([](auto i) { return static_cast<int>(i) == 1 || static_cast<int>(i) == 2; }); // F,T,T,F -> {2,3}
    assert(std::simd::reduce_min(x, k) == 2);
    assert(std::simd::reduce_max(x, k) == 3);
  }
  // reduce_min/reduce_max, masked, all-false -- Returns numeric_limits<T>::max()/lowest(), the one
  // shape whose result can't come from "some in-range element" since there isn't one.
  {
    vec4::mask_type k(false);
    assert(std::simd::reduce_min(x, k) == std::numeric_limits<int>::max());
    assert(std::simd::reduce_max(x, k) == std::numeric_limits<int>::lowest());
  }

  // reduce_min/reduce_max, scalar overloads.
  {
    assert(std::simd::reduce_min(5) == 5);
    assert(std::simd::reduce_max(5) == 5);
    assert(std::simd::reduce_min(5, false) == std::numeric_limits<int>::max());
    assert(std::simd::reduce_max(5, false) == std::numeric_limits<int>::lowest());
    assert(std::simd::reduce_min(5, true) == 5);
  }

  // Plain-std:: re-exports (algorithms.h's min/max/minmax/clamp is a different set than
  // reduce/reduce_min/reduce_max -- reduce* is explicitly *excluded* from the re-export list per
  // [[simd-n5050-port]]'s memory, since it has no non-simd std:: counterpart). Confirm reduce_min
  // stays simd::-qualified-only by checking it's still reachable that way, not asserting an absence
  // (there's no meaningful negative check for "a name isn't re-exported" without redeclaring it, as
  // the no_exposition_only_names.verify.cpp file already does for a different name set).
  {
    assert(std::simd::reduce_min(x) == 1);
  }

  return 0;
}
