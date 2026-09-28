//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ferror-limit=0

// <simd>

#include <limits>
#include <simd>

using vec  = std::simd::vec<int, 1>;
using mask = vec::mask_type;
using four_vec = std::simd::vec<int, 4>;

// Failed constexpr maps trigger one initialization error, one non-constant static assertion, and
// two non-constant if-constexpr conditions per map call. Keep notes from those errors unannotated.
// expected-error@*:* 4 {{constexpr variable '__src_ix' must be initialized by a constant expression}}
// expected-error@*:* 4 {{static assertion expression is not an integral constant expression}}
// expected-error@*:* 8 {{constexpr if condition is not a constant expression}}

int next_index() { return 0; }

struct member_map {
  int value;
  constexpr int operator()(std::ptrdiff_t) const { return value; }
};

void test(vec v, mask m, int runtime) {
  // Non-constant maps are rejected for both generator arities and overload lengths.
  (void)std::simd::permute<1>(v, [](auto) { return next_index(); });
  (void)std::simd::permute(v, [](auto, auto) { return next_index(); });

  // A constexpr call operator still fails if its answer reads runtime object state.
  member_map by_member{runtime};
  (void)std::simd::permute<1>(v, by_member);

  (void)std::simd::permute(v, [runtime](auto) { return runtime; });

  // Out-of-range indices are rejected for vectors and masks, including both length forms.
  (void)std::simd::permute<1>(v, [](auto i) { return i == 0 ? 4 : 0; }); // expected-error@*:* 1 {{simd::permute: index must be zero_element, uninit_element, or in range}}
  (void)std::simd::permute(v, [](auto i, auto) { return i == 0 ? -3 : 0; }); // expected-error@*:* 1 {{simd::permute: index must be zero_element, uninit_element, or in range}}
  (void)std::simd::permute<1>(m, [](auto i) { return i == 0 ? 4 : 0; }); // expected-error@*:* 1 {{simd::permute: index must be zero_element, uninit_element, or in range}}
  (void)std::simd::permute(m, [](auto i, auto) { return i == 0 ? -3 : 0; }); // expected-error@*:* 1 {{simd::permute: index must be zero_element, uninit_element, or in range}}

  // Unsigned maximum values must not compare equal to the negative sentinels.
  (void)std::simd::permute(v, [](auto) { return std::numeric_limits<unsigned long long>::max(); }); // expected-error@*:* 1 {{simd::permute: index must be zero_element, uninit_element, or in range}}

  // Diagnose an invalid index on a later lane without producing errors for earlier lanes.
  four_vec four([](auto i) { return static_cast<int>(i); });
  (void)std::simd::permute(four, [](auto i) { return i == 3 ? 4 : 0; }); // expected-error@*:* 1 {{simd::permute: index must be zero_element, uninit_element, or in range}}

#if defined(__SIZEOF_INT128__)
  // Preserve the full signed result type when checking the range.
  (void)std::simd::permute<1>(v, [](auto) { return static_cast<__int128>(1) << 100; }); // expected-error@*:* 1 {{simd::permute: index must be zero_element, uninit_element, or in range}}
#endif
}
