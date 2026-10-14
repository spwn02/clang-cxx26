# LWG issue draft (tracker spwn02/clang-cxx26#256)

**Title:** `ranges::set_symmetric_difference`: *Returns* uses the undefined quantity K ("N is equal to M + K")
**Section:** [set.symmetric.difference]
**Status:** New (to be submitted)
**Wording is relative to:** N5050 (draft of 2026-06-01, commit 8ab1a9b of cplusplus/draft)
**Priority:** 3 (editorial if the intent below is confirmed)

## Discussion

[set.symmetric.difference] defines, in paragraph *Let*, the quantities `M` (number of elements of the sorted symmetric difference), `result_last` and `N` (`min(M, result_last - result)`). The *Returns* element then says:

> `{last1, last2, result + N}` for the overloads in namespace `ranges`, if N is equal to M + K.
> Otherwise, `{first1 + A, first2 + B, result_last}` for the overloads in namespace `ranges`, where A and B are the numbers of copied or skipped elements in `[first1, last1)` and `[first2, last2)`, respectively.

`K` is not defined anywhere in the paragraph or the surrounding subclause (the paragraph defines M, N, A and B); it looks like an editing leftover. For the sequential `ranges` overloads N equals M always, and the first bullet is meant to describe the case of fully processed ranges.

## Proposed resolution

Wording is relative to N5050. In [set.symmetric.difference] *Returns*:

> `{last1, last2, result + N}` for the overloads in namespace `ranges`, if N is equal to M ~~+ K~~.

The analogous *Returns* of `ranges::set_union` ([set.union]) has exactly the same three bullets with the condition "if N is equal to M", which supports reading `M + K` as an editing leftover of `M`.

## Implementation

The clang-cxx26 fork implements K = 0 (`{last1, last2, result + N}` when N == M, otherwise `{first1 + A, first2 + B, result_last}` with A and B counted under the paragraph's own *skipped* rule); pinned by the `policy_overloads` algorithm tests.
