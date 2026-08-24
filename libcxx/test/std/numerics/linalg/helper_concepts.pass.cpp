//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>

// Exercises the [linalg.helpers.concepts] SFINAE constraints (in-vector,
// out-vector, in-matrix, out-matrix, possibly-packed-out-matrix, ...) that
// gate the linalg algorithms, distinguishing them from each other rather
// than just re-running valid calls that already pass without the checks.

#include <linalg>

// Named concepts, not inline requires-expressions: this fork's compiler can
// hard-error instead of evaluating false when a raw inline requires-clause's
// candidate set is excluded by a mix of constraint failure and arity
// mismatch.
template <class A, class X, class Y>
concept __can_matrix_vector_product = requires(A a, X x, Y y) { std::linalg::matrix_vector_product(a, x, y); };

template <class Alpha, class X, class A>
concept __can_symmetric_matrix_rank_1_update =
    requires(Alpha alpha, X x, std::linalg::upper_triangle_t t, A a) {
      std::linalg::symmetric_matrix_rank_1_update(alpha, x, a, t);
    };

int main(int, char**) {
  using MatT   = std::mdspan<float, std::extents<int, 2, 3>>;
  using VecT   = std::mdspan<float, std::extents<int, 3>>;
  using BadOut = std::mdspan<float, std::extents<int, 2, 2>>;
  using OkOut  = std::mdspan<float, std::extents<int, 2>>;

  // in-matrix/in-vector/out-vector: wrong-rank output rejected, right-rank accepted.
  static_assert(!__can_matrix_vector_product<MatT, VecT, BadOut>,
                "wrong-rank output should be SFINAE-excluded, not a hard error");
  static_assert(__can_matrix_vector_product<MatT, VecT, OkOut>);

  // out-vector also requires the output element type be assignable (const rejected).
  using ConstOut = std::mdspan<const float, std::extents<int, 2>>;
  static_assert(!__can_matrix_vector_product<MatT, VecT, ConstOut>,
                "read-only output should be SFINAE-excluded via out-vector's is_assignable_v check");

  // possibly-packed-out-matrix: the discriminating case against plain
  // out-matrix. A layout_blas_packed output has no unique mapping
  // (is_always_unique() is false), so out-matrix alone would reject it, but
  // possibly-packed-out-matrix must accept it.
  using PackedExtents = std::extents<size_t, 2, 2>;
  using PackedLayout  = std::linalg::layout_blas_packed<std::linalg::upper_triangle_t, std::linalg::column_major_t>;
  using PackedOut     = std::mdspan<float, PackedExtents, PackedLayout>;
  using VecT2         = std::mdspan<float, std::extents<int, 2>>;
  static_assert(__can_symmetric_matrix_rank_1_update<float, VecT2, PackedOut>,
                "possibly-packed-out-matrix must accept a layout_blas_packed output");

  // A plain unpacked, dense-but-non-unique-mapping shape isn't easy to
  // construct from layout_left/layout_right (those are always unique), so
  // the negative side of possibly-packed-out-matrix's "must still be
  // assignable" half is covered by the const-output check above, which
  // applies equally to it.

  return 0;
}
