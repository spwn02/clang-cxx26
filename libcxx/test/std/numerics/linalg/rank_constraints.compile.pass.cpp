//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>
// [linalg.algs.blas1.swap], [linalg.algs.blas1.copy], [linalg.algs.blas1.add]

#include <execution>
#include <linalg>
#include <mdspan>

using Vector = std::mdspan<int, std::extents<std::size_t, 2>>;
using Matrix = std::mdspan<int, std::extents<std::size_t, 2, 2>>;

template <class A, class B>
concept CanSwap = requires(A a, B b) { std::linalg::swap_elements(a, b); };

template <class A, class B>
concept CanCopy = requires(A a, B b) { std::linalg::copy(a, b); };

template <class A, class B, class C>
concept CanAdd = requires(A a, B b, C c) { std::linalg::add(a, b, c); };

template <class A, class B>
concept CanSwapWithPolicy = requires(A a, B b) { std::linalg::swap_elements(std::execution::seq, a, b); };

template <class A, class B>
concept CanCopyWithPolicy = requires(A a, B b) { std::linalg::copy(std::execution::seq, a, b); };

template <class A, class B, class C>
concept CanAddWithPolicy = requires(A a, B b, C c) { std::linalg::add(std::execution::seq, a, b, c); };

static_assert(CanSwap<Vector, Vector>);
static_assert(CanCopy<Vector, Vector>);
static_assert(CanAdd<Vector, Vector, Vector>);
static_assert(CanSwapWithPolicy<Vector, Vector>);
static_assert(CanCopyWithPolicy<Vector, Vector>);
static_assert(CanAddWithPolicy<Vector, Vector, Vector>);

static_assert(!CanSwap<Vector, Matrix>);
static_assert(!CanCopy<Vector, Matrix>);
static_assert(!CanAdd<Vector, Vector, Matrix>);
static_assert(!CanAdd<Vector, Matrix, Vector>);
static_assert(!CanSwapWithPolicy<Vector, Matrix>);
static_assert(!CanCopyWithPolicy<Vector, Matrix>);
static_assert(!CanAddWithPolicy<Vector, Vector, Matrix>);
static_assert(!CanAddWithPolicy<Vector, Matrix, Vector>);
