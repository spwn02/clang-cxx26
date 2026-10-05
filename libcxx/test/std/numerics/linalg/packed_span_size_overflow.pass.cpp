//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

// [linalg.layout.packed.overview]: for static extents, N * (N + 1) (not only its half, the required span size) must be
// representable as index_type, so a narrow index_type bounds N below sqrt(max).

#include <linalg>

#include <cstdint>
#include <mdspan>

using upper_col = std::linalg::layout_blas_packed<std::linalg::upper_triangle_t, std::linalg::column_major_t>;

// With index_type = uint8_t and N = 15: N*(N+1) == 240 is representable and required_span_size() == 120.
using narrow_extents = std::extents<std::uint8_t, 15, 15>;
using narrow_mapping = upper_col::mapping<narrow_extents>;

static_assert(narrow_mapping{}.required_span_size() == 120);

constexpr bool test() {
  narrow_mapping m{narrow_extents{}};
  if (m.required_span_size() != 120)
    return false;
  for (std::uint8_t j = 0; j != 15; ++j)
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
