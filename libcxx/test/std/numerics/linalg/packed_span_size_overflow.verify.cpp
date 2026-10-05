//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

// [linalg.layout.packed.overview]: N * (N + 1) must be representable as index_type for static extents.

#include <linalg>

#include <cstdint>
#include <mdspan>

// expected-note@*:* 0+ {{in instantiation of}}

using upper_col = std::linalg::layout_blas_packed<std::linalg::upper_triangle_t, std::linalg::column_major_t>;

// 15 * 16 == 240 fits uint8_t.
[[maybe_unused]] upper_col::mapping<std::extents<std::uint8_t, 15, 15>> fits;

// 22 * 23 == 506 overflows uint8_t even though the required span size 253 fits.
// expected-error@*:* {{layout_blas_packed::mapping required span size must be representable as index_type}}
[[maybe_unused]] upper_col::mapping<std::extents<std::uint8_t, 22, 22>> overflows_but_half_fits;

// 16 * 17 == 272 overflows uint8_t.
// expected-error@*:* {{layout_blas_packed::mapping required span size must be representable as index_type}}
[[maybe_unused]] upper_col::mapping<std::extents<std::uint8_t, 16, 16>> overflows;

// A signed char index_type: 15 * 16 == 240 does not fit although its half (120) does.
// expected-error@*:* {{layout_blas_packed::mapping required span size must be representable as index_type}}
[[maybe_unused]] upper_col::mapping<std::extents<signed char, 15, 15>> signed_overflows_but_half_fits;
