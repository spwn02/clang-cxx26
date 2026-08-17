//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// basic_mask's own member surface: [simd.mask.ctor], [simd.mask.subscr], [simd.mask.unary],
// [simd.mask.conv], [simd.mask.binary], [simd.mask.cassign], [simd.mask.comparison]. Coverage
// elsewhere in this test suite only exercises basic_mask incidentally (as the return type of
// comparisons, or the argument to select/reduce/permute); this file targets the class's own
// constructors and operators directly, including two explicitness distinctions the clause calls
// out by name: the bool constructor is explicit while the bitset constructor is implicit
// ([simd.mask.ctor]/1,7), and the unary +/-/~ operators are conditionally *deleted*, not merely
// unconstrained, when Bytes has no corresponding vectorizable signed-integer type
// ([simd.mask.unary]/3).

#include <bitset>
#include <cassert>
#include <complex>
#include <concepts>
#include <simd>

// __has_integer_from<Bytes> is exposition-only, so this concept spells the check as an ordinary
// requires-expression instead of naming it -- a bare requires{} at non-template scope hard-errors
// in this toolchain instead of evaluating to false (see [[simd-n5050-port]] memory), so this is
// routed through a named concept template, matching select_adl.pass.cpp/permute_overloads.pass.cpp.
template <class Mask>
concept has_unary_plus = requires(const Mask& k) { +k; };

int main(int, char**) {
  using mask4 = std::simd::vec<int, 4>::mask_type;

  // [simd.mask.ctor]/1: explicit, bool broadcast.
  {
    mask4 k(true);
    assert(k[0] && k[1] && k[2] && k[3]);
    static_assert(!std::convertible_to<bool, mask4>); // explicit -- no implicit bool -> mask.
  }

  // [simd.mask.ctor]/4-6: generator.
  {
    mask4 k([](auto i) { return static_cast<int>(i) % 2 == 0; });
    assert(k[0] && !k[1] && k[2] && !k[3]);
  }

  // [simd.mask.ctor]/2: explicit conversion between same-size masks of differing Bytes.
  {
    mask4 k([](auto i) { return static_cast<int>(i) < 2; }); // T,T,F,F
    using mask_same_size = std::simd::vec<char, 4>::mask_type; // Bytes == 1, size == 4, matches k's size
    mask_same_size converted(k);
    assert(converted[0] == true && converted[1] == true && converted[2] == false && converted[3] == false);
    static_assert(!std::convertible_to<mask4, mask_same_size>); // explicit, per [simd.mask.ctor]/2.
  }

  // [simd.mask.ctor]/7: bitset constructor is *implicit*, unlike every other basic_mask ctor.
  {
    std::bitset<4> b("1010");
    mask4 k = b; // implicit conversion must be well-formed
    assert(k[0] == false && k[1] == true && k[2] == false && k[3] == true);
    static_assert(std::convertible_to<std::bitset<4>, mask4>);
  }

  // [simd.mask.ctor]/8: unsigned_integral bit-pattern constructor -- explicit, low bits map to low
  // lanes, bits beyond size() are ignored, and value bits beyond numeric_limits<T>::digits are
  // implicitly false (the loop's second pass in basic_mask.h:131-132).
  {
    mask4 k(0b0101u); // bit0=1(lane0 true), bit1=0, bit2=1(lane2 true), bit3=0
    assert(k[0] == true && k[1] == false && k[2] == true && k[3] == false);
    static_assert(!std::convertible_to<unsigned, mask4>);
  }

  // [simd.mask.subscr]: index-vector subscript delegates to permute -- already covered in depth by
  // permute_overloads.pass.cpp; this just confirms the member operator[] itself dispatches
  // correctly, not the permute machinery underneath it.
  {
    mask4 k([](auto i) { return static_cast<int>(i) % 2 == 0; }); // T,F,T,F
    std::simd::vec<int, 2> indices([](auto i) { return i == 0 ? 0 : 2; });
    auto sub = k[indices];
    static_assert(std::same_as<decltype(sub), std::simd::resize_t<2, mask4>>);
    assert(sub[0] == true && sub[1] == true);
  }

  // [simd.mask.unary]: operator! is unconditional.
  {
    mask4 k([](auto i) { return static_cast<int>(i) % 2 == 0; }); // T,F,T,F
    auto n = !k;
    assert(n[0] == false && n[1] == true && n[2] == false && n[3] == true);
  }

  // [simd.mask.unary]/3: +,-,~ promote to a signed-int vec of matching Bytes when one exists (int
  // here, Bytes == 4) -- checked as a type fact, not just a value fact, since the interesting part
  // is that this operator's return type differs from basic_mask itself.
  {
    mask4 k([](auto i) { return static_cast<int>(i) % 2 == 0; }); // T,F,T,F -> as int: 1,0,1,0
    auto p = +k;
    static_assert(std::same_as<decltype(p), std::simd::vec<int, 4>>);
    assert(p[0] == 1 && p[1] == 0 && p[2] == 1 && p[3] == 0);

    auto m = -k;
    static_assert(std::same_as<decltype(m), std::simd::vec<int, 4>>);
    assert(m[0] == -1 && m[1] == 0);

    auto b = ~k;
    static_assert(std::same_as<decltype(b), std::simd::vec<int, 4>>);
    assert(b[0] == ~1 && b[1] == ~0);
  }

  // Bytes == 16 has no vectorizable *signed-integer* type of that width (int/short/char/long long
  // cover 1/2/4/8; nothing covers 16) -- [simd.mask.unary]/3 deletes +,-,~ there rather than leaving
  // them merely absent. Reached through the public API via a complex<double> vec's mask_type
  // (sizeof(complex<double>) == 16), not by naming any exposition-only ABI type directly.
  {
    using mask16 = std::simd::vec<std::complex<double>, 2>::mask_type;
    static_assert(!has_unary_plus<mask16>);
    // operator! stays available regardless -- it isn't gated on __has_integer_from at all.
    mask16 k16(true);
    assert((!k16)[0] == false);   // !k16 is all-false...
    assert((!(!k16))[0] == true); // ...and !(!k16) is all-true again.
  }

  // [simd.mask.conv]: explicit(sizeof(Up) != Bytes) conversion to basic_vec<Up, Ap> of matching
  // size -- exercised both where the explicit() condition is false (same-size element, implicit
  // conversion must compile) and true (different-size element, must stay explicit-only).
  {
    mask4 k([](auto i) { return static_cast<int>(i) % 2 == 0; }); // T,F,T,F
    std::simd::vec<int, 4> as_int = k; // sizeof(int) == Bytes (4) -- implicit is well-formed
    assert(as_int[0] == 1 && as_int[1] == 0);

    auto as_short = static_cast<std::simd::vec<short, 4>>(k); // sizeof(short) != Bytes -- explicit only
    assert(as_short[0] == 1 && as_short[2] == 1);
    static_assert(!std::convertible_to<mask4, std::simd::vec<short, 4>>);
  }

  // to_bitset / to_ullong.
  {
    mask4 k([](auto i) { return static_cast<int>(i) == 0 || static_cast<int>(i) == 3; }); // T,F,F,T
    auto bs = k.to_bitset();
    static_assert(std::same_as<decltype(bs), std::bitset<4>>);
    assert(bs.to_string() == "1001");
    assert(k.to_ullong() == 0b1001ull);
  }

  // [simd.mask.binary] and [simd.mask.comparison]: all return basic_mask, not bool -- checked as a
  // type fact for one representative (&&), then values checked for the rest.
  {
    mask4 a([](auto i) { return static_cast<int>(i) < 2; });      // T,T,F,F
    mask4 b([](auto i) { return static_cast<int>(i) % 2 == 0; }); // T,F,T,F

    auto both = a && b;
    static_assert(std::same_as<decltype(both), mask4>);
    assert(both[0] == true && both[1] == false && both[2] == false && both[3] == false);

    assert((a & b)[0] == true && (a & b)[1] == false);
    assert((a || b)[1] == true && (a | b)[2] == true);
    assert((a ^ b)[0] == false && (a ^ b)[1] == true && (a ^ b)[2] == true && (a ^ b)[3] == false);

    assert((a == b)[0] == true && (a == b)[1] == false);
    assert((a != b)[1] == true);
    assert((a >= b)[1] == true);  // true(1) >= false(0)
    assert((a <= b)[3] == true);  // false(0) <= false(0)
    assert((a > b)[1] == true);   // true(1) > false(0)
    assert((a < b)[2] == true);   // false(0) < true(1)
  }

  // [simd.mask.cassign]: &=, |=, ^= mutate in place and return a reference to the (mutated) lhs.
  {
    mask4 a([](auto i) { return static_cast<int>(i) < 2; });      // T,T,F,F
    mask4 b([](auto i) { return static_cast<int>(i) % 2 == 0; }); // T,F,T,F

    mask4 c = a;
    auto& ref = (c &= b);
    assert(&ref == &c);
    assert(c[0] == true && c[1] == false && c[2] == false && c[3] == false);

    mask4 d = a;
    d |= b;
    assert(d[1] == true && d[2] == true);

    mask4 e = a;
    e ^= b;
    assert(e[0] == false && e[1] == true);
  }

  return 0;
}
