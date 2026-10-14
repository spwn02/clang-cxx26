# LWG issue draft (tracker spwn02/clang-cxx26#252)

**Title:** `ranges::set_difference` (parallel, bounded output): the returned `first1 + A` can skip an element of the difference that was not copied
**Section:** [set.difference]
**Status:** New (to be submitted)
**Wording is relative to:** N5050 (draft of 2026-06-01, commit 8ab1a9b of cplusplus/draft)
**Priority:** 3

## Discussion

For the parallel overloads of `ranges::set_difference` ([set.difference], the ones returning `set_difference_truncated_result`), the *Effects* define when elements are *skipped*:

> Of those equivalent elements, the first min(m, n) elements in both ranges are considered *skipped*. An element from the second range is also considered skipped if it compares less than the min(N + 1, M)th element of the sorted difference.

and the *Returns* are, when the output is full before the difference is complete (N < M):

> Otherwise, `{first1 + A, first2 + B, result_last}`, where A and B are the numbers of copied or skipped elements in `[first1, last1)` and `[first2, last2)`, respectively.

The "first min(m, n) elements" rule is global: every element of the first range that is paired with an equivalent element of the second range is *skipped*, wherever it is, including after the element where copying stops. A therefore counts the copied elements *plus all paired elements of the whole range*, and `first1 + A` is generally not a position at which the algorithm could be resumed.

Example. `left = {1, 2, 2, 4, 6}`, `right = {2, 3, 6}`, output capacity 1. The sorted difference is `{1, 2, 4}` (M = 3): of the two equivalent `2` in `left` only the last is in the difference, because `right` has one `2`; `6` is paired and not in the difference. N = 1, so only `1` is copied. The skipped elements of `left` are the first `2` and the `6` (paired), so A = 1 (copied) + 2 (skipped) = 3 and the result is `first1 + 3`, which points at `4`. The second `2`, an element of the difference that was **not** copied, lies before that position. A caller who resumes from `{first1 + A, first2 + B}` silently loses it.

A position-based reading is what the return value is for: the (N + 1)th element x of the sorted difference (here the second `2`) is the element at which copying stopped, so `first1 + A` should point to x, and `first2 + B` should point past every element of the second range that has already been consumed. Those are the elements of the second range that compare less than **or equivalent to** x: x is in the difference only because the first range has more elements equivalent to it than the second range, so every element of the second range that is equivalent to x was already paired with an earlier equivalent element of the first range. (If B counted only the elements that compare less than x, resuming at `{first1 + A, first2 + B}` would pair the second range's `2` with x again and lose x.) N5050 wording does neither.

For comparison, the *skipped* rule of `ranges::set_symmetric_difference` ([set.symmetric.difference]) is stated relative to the (N + 1)th element ("compares less than or equivalent to the (N + 1)th element ..., unless it is from the same range as that element and does not precede it"), i.e. it does not count paired elements that lie beyond the stopping point.

## Proposed resolution

Wording is relative to N5050. In [set.difference] *Returns*, replace the second sub-bullet of the third bullet:

> Otherwise, `{first1 + A, first2 + B, result_last}`, where ~~A and B are the numbers of copied or skipped elements in `[first1, last1)` and `[first2, last2)`, respectively.~~ A is the number of elements of `[first1, last1)` that precede the (N + 1)th element of the sorted difference, and B is the number of elements of `[first2, last2)` that compare less than or equivalent to that element.

For the example above (`left = {1, 2, 2, 4, 6}`, `right = {2, 3, 6}`, capacity 1) this gives A = 2 and B = 1, i.e. `{first1 + 2, first2 + 1, result_last}`: resuming there copies the second `2` and `4` and skips the paired `6`.

The first sub-bullet (N equal to M) is unaffected.

## Implementation

The clang-cxx26 fork implements the wording literally; the behavior (including the example above) is pinned by `libcxx/test/std/algorithms/alg.sorting/alg.set.operations/set.difference/ranges.pstl.bounded_set_difference.pass.cpp`.
