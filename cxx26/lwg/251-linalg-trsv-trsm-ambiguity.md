# LWG issue draft (tracker spwn02/clang-cxx26#251)

**Title:** Out-of-place `triangular_matrix_vector_solve` / `triangular_matrix_matrix_{left,right}_solve` without a divide operation are ambiguous with the in-place overloads that take one
**Section:** [linalg.syn], [linalg.algs.blas2.trsv], [linalg.algs.blas3.trsm]
**Status:** New (to be submitted)
**Wording is relative to:** N5050 (draft of 2026-06-01, commit 8ab1a9b of cplusplus/draft)
**Priority:** 2

## Discussion

[linalg.syn] declares for `triangular_matrix_vector_solve` both

```cpp
// not in place, without divide operation
template<in-matrix InMat, class Triangle, class DiagonalStorage,
         in-vector InVec, out-vector OutVec>
  void triangular_matrix_vector_solve(InMat A, Triangle t, DiagonalStorage d, InVec b, OutVec x);

// in place, with divide operation
template<in-matrix InMat, class Triangle, class DiagonalStorage,
         inout-vector InOutVec, class BinaryDivideOp>
  void triangular_matrix_vector_solve(InMat A, Triangle t, DiagonalStorage d,
                                      InOutVec b, BinaryDivideOp divide);
```

(and the corresponding pairs with a leading `ExecutionPolicy&& exec`). `BinaryDivideOp` is an unconstrained template parameter, and the concepts `in-vector`, `out-vector` and `inout-vector` ([linalg.helpers.concepts]) do not subsume one another (`in-vector` requires `is-mdspan<T> && T::rank() == 1`; `out-vector` and `inout-vector` repeat that and add the same assignability and `is_always_unique()` requirements, but each concept states its constraint expressions separately, so their atomic constraints are not identical and no concept subsumes another). For a call `triangular_matrix_vector_solve(A, t, d, b, x)` where `b` and `x` are both writable rank-1 `mdspan`s with unique layouts (the usual case of the out-of-place overload: `b` an input that happens to be writable, `x` the output), both templates are viable with the same parameter types, neither is more specialized, and the call is ambiguous. Neither [linalg.algs.blas2.trsv] nor [linalg.algs.blas3.trsm] adds a *Constraints* element to the in-place overloads. The same holds for `triangular_matrix_matrix_left_solve` and `triangular_matrix_matrix_right_solve` ([linalg.syn], [linalg.algs.blas3.trsm]), for which the five-argument not-in-place overload `(A, t, d, B, X)` is ambiguous with `(A, t, d, InOutMat B, divide)`.

The ambiguity is not a corner case: the overloads without a divide operation exist so that callers need not spell `divides<void>{}`, but for any `b` that is not a `const`/read-only `mdspan` the shorter call cannot be written. Calling code has to pass `divides<void>{}` explicitly, which selects the six-argument overload and defeats the purpose of the five-argument one. Passing a read-only `b` (for example `mdspan<const double, ...>`) works because `inout-vector` is then not satisfied.

Reproducer (against the wording as written, any conforming implementation of `in-vector`/`out-vector`/`inout-vector`): `std::array<double,4> bb, xx; mdspan b(bb.data(), 2), x(xx.data(), 2); triangular_matrix_vector_solve(A, lower_triangle, explicit_diagonal, b, x);` is ill-formed (ambiguous).

## Proposed resolution

Wording is relative to N5050.

In [linalg.algs.blas2.trsv], add to the paragraph that applies to all functions (or to the in-place overloads' item descriptions), and in [linalg.algs.blas3.trsm] for the in-place overloads of `triangular_matrix_matrix_left_solve` and `triangular_matrix_matrix_right_solve`:

> *Constraints:* `is-mdspan<BinaryDivideOp>` is `false` for the in-place overloads that take a `BinaryDivideOp` parameter.

(`is-mdspan` is the exposition-only trait of [linalg.helpers.concepts].) An alternative with the same effect is to constrain the in-place overloads with `!is-mdspan<BinaryDivideOp>` directly in the declarations of [linalg.syn].

An `mdspan` is never a divide operation (a callable invoked on two element values), so no valid call is excluded.

## Implementation

The clang-cxx26 fork implements the wording as written, so the five-argument call with two writable `mdspan`s is rejected; pinned by `static_assert(!can_solve_*<...>)` in `libcxx/test/std/numerics/linalg/all_overloads.compile.pass.cpp`, and the fork's own tests pass `std::divides<void>{}` explicitly. With the proposed constraint the fork could drop the negative assertions.
