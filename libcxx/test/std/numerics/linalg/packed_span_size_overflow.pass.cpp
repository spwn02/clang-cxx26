//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

// layout_blas_packed::mapping::required_span_size() is N*(N+1)/2. With a narrow
// index_type the halved result can be representable even though the
// intermediate N*(N+1) is not, so the representability check must not reject
// those extents.

#include <linalg>

#include <cstdint>
#include <mdspan>

using upper_col = std::linalg::layout_blas_packed<std::linalg::upper_triangle_t, std::linalg::column_major_t>;

// With index_type = uint8_t and N = 22: N*(N+1) == 506 overflows uint8_t, but
// required_span_size() == 253 fits comfortably.
using narrow_extents = std::extents<std::uint8_t, 22, 22>;
using narrow_mapping = upper_col::mapping<narrow_extents>;

static_assert(narrow_mapping{}.required_span_size() == 253);

// N = 21 is the odd counterpart: 21*22 == 462 also overflows uint8_t, while
// required_span_size() == 231 fits.
using odd_extents = std::extents<std::uint8_t, 21, 21>;
using odd_mapping = upper_col::mapping<odd_extents>;

static_assert(odd_mapping{}.required_span_size() == 231);

constexpr bool test() {
  // The mapping still addresses every stored element within the span for the
  // narrow case.
  narrow_mapping m{narrow_extents{}};
  if (m.required_span_size() != 253)
    return false;
  for (std::uint8_t j = 0; j != 22; ++j)
    for (std::uint8_t i = 0; i <= j; ++i)
      if (m(i, j) >= m.required_span_size())
        return false;

  // A wider index_type is unaffected.
  using wide_mapping = upper_col::mapping<std::extents<size_t, 5, 5>>;
  wide_mapping w{};
  if (w.required_span_size() != 15)
    return false;

  // Mirrored access maps to the stored element.
  if (w(3, 1) != w(1, 3))
    return false;

  return true;
}

int main(int, char**) {
  static_assert(test());
  return test() ? 0 : 1;
}
