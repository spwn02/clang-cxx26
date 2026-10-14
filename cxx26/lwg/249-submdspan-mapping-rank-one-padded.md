# LWG issue draft (tracker spwn02/clang-cxx26#249)

**Title:** `submdspan_mapping` of rank-one padded layouts ignores the stride of an `extent_slice`, and the sliceable postcondition does not mention strides
**Section:** [mdspan.sub.map.leftpad], [mdspan.sub.map.rightpad], [mdspan.sub.map.sliceable]
**Status:** New (to be submitted)
**Wording is relative to:** N5050 (draft of 2026-06-01, commit 8ab1a9b of cplusplus/draft)
**Priority:** 3

## Discussion

[mdspan.sub.map.leftpad] (and its mirror [mdspan.sub.map.rightpad]) specify the result of `layout_left_padded::mapping<Extents>::submdspan-mapping-impl(slices...)`. The second bullet of *Returns* reads:

> otherwise, `submdspan_mapping_result{layout_left::mapping(sub_ext), offset}`, if `rank_ == 1` is `true` or `SubExtents::rank() == 0` is `true`;

The first part of the condition applies to every slice of a rank-one mapping, including a strided one. For `rank_ == 1` and the slice `extent_slice{0, cw<4>, cw<2>}` the result has `SubExtents::rank() == 1` with extent 2, and `layout_left::mapping(sub_ext)` has stride 1. The returned mapping therefore addresses the first two elements `0, 1` (plus `offset`) instead of elements `0, 2`.

The general rule of [mdspan.sub.map.common] gives the intended meaning: for a slice `s` that is an `extent_slice` with `s.extent > 1`, the stride of the corresponding sub-dimension is `stride(k) * s.stride` (the list that defines `sub_strides`), so the strided slice yields a `layout_stride` mapping with stride 2. The `layout_left`, `layout_right` and `layout_stride` specializations ([mdspan.sub.map.left], [mdspan.sub.map.right], [mdspan.sub.map.stride]) are unaffected; only the padded specializations contain the `rank_ == 1` shortcut. Unit-stride slices are unaffected.

The generic requirement that is meant to catch this does not do so: [mdspan.sub.map.sliceable] *Returns* requires

> `smr.mapping(i...) + smr.offset == m(j)`, where `j...[ρ]` is equal to the sum of the lower bound of the `submdspan` slice range ... and zero if the type of `valid_slices...[ρ]` is a collapsing slice type, `i...[MAP_RANK(valid_slices,ρ)]` otherwise.

The index `i` is added to the lower bound without being multiplied by the slice stride. Read literally, the (stride-1) `layout_left::mapping` above satisfies this postcondition, while a correct `layout_stride` result for the same slice (the result that [mdspan.sub.map.common] itself prescribes for the default mappings) does not. The two defects hide each other: the postcondition can neither detect the wrong rank-one result nor accept the right one.

Reproducer: `layout_left_padded<dynamic_extent>::mapping<extents<int, 8>>` with `submdspan_mapping(m, extent_slice{0, cw<4>, cw<2>})`: `smr.mapping(1) + smr.offset` is `1`, `m(2)` is the element the slice selects as its second element.

## Proposed resolution

Wording is relative to N5050.

1. In [mdspan.sub.map.leftpad] *Returns*, change the second bullet:

   > otherwise, `submdspan_mapping_result{layout_left::mapping(sub_ext), offset}`, if ~~`rank_ == 1` is `true` or~~ `SubExtents::rank() == 0` is `true`;

   The case `rank_ == 1` with a unit-stride slice and `SubExtents::rank() == 1` is covered by the third bullet (`SubExtents::rank() == 1` and `SliceSpecifiers...[0]` is a unit-stride slice type, same result); a strided slice falls to the last bullet and yields the `layout_stride` mapping.

2. Make the same change in [mdspan.sub.map.rightpad] with `layout_right::mapping`.

3. In [mdspan.sub.map.sliceable] *Returns*, replace "`i...[MAP_RANK(valid_slices,ρ)]` otherwise" by:

   > otherwise, the product of `i...[MAP_RANK(valid_slices,ρ)]` and `s.stride` if the type of `valid_slices...[ρ]` is a specialization of `extent_slice` and `s.extent > 1` is `true`, where `s` is `valid_slices...[ρ]`, and `i...[MAP_RANK(valid_slices,ρ)]` otherwise.

   (the same condition as the one that defines `sub_strides` in [mdspan.sub.map.common]).

## Implementation

Implemented as written in N5050 in the clang-cxx26 fork (the fork follows the draft wording); the rank-one case is pinned by `libcxx/test/std/containers/views/mdspan/submdspan/submdspan.conformance.pass.cpp` ("rank-one padded mappings"). Not changed upstream.
