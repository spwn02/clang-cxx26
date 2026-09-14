# mdspan submdspan design note

Date: 2026-09-14

Scope: Wave 4 issue #14, implemented against the current working draft
`[mdspan.sub]` at <https://eel.is/c++draft/mdspan.sub>, not the original
P2630R4/P2642R6/P3355R2 wording. The draft was fetched on 2026-09-14 and is
one page containing the complete submdspan clause tree, including padded
layouts.

## Current public vocabulary

The draft's public types are:

* `std::extent_slice<OffsetType, ExtentType, StrideType>`, with the three
  no-unique-address data members `offset`, `extent`, and `stride`.
* `std::range_slice<FirstType, LastType, StrideType =
  std::constant_wrapper<1zu>>`, with `first`, `last`, and `stride` members.
* `std::submdspan_mapping_result<LayoutMapping>`, containing `mapping` and a
  `size_t offset`.

The old paper names `strided_slice` and `submdspan_extents` are not aliases in
the current wording: the implementation must use `extent_slice` and
`subextents`, and must provide `canonical_slices` as the separate
canonicalization operation.

## Canonicalization and extents

The exposition-only pipeline is:

1. `canonical-index<IndexType>(s)` returns `cw<IndexType(S::value)>` for an
   integral-constant-like `S`, otherwise converts `s` to `IndexType`.
2. `canonical-range-slice` computes a regularly spaced `extent_slice`; an
   empty span has stride one, and a constant span and constant stride preserve
   a constant-wrapper extent.
3. `canonical-slice` preserves `full_extent_t`, canonicalizes an index or an
   `extent_slice`, converts a `range_slice` from `[first,last)` to offset/span,
   and converts a two-element tuple-like slice similarly.
4. `canonical_slices(extents, raw...)` returns a tuple of canonical slices.
5. `subextents(extents, raw...)` canonicalizes first, removes collapsing
   slices, and gives a surviving `extent_slice` its constant extent when its
   extent type is a `constant_wrapper`; otherwise the surviving extent is
   dynamic. A full extent preserves the source static extent.

`std::constant_wrapper` is already implemented locally in
`__utility/constant_wrapper.h`; the submdspan implementation will include the
public utility header through the narrow internal dependency needed for its
traits and `cw` alias. Static-extent calculations must not fall back to the
old paper's runtime-only behavior.

## Mapping strategy

The shared mapping algorithm computes `SubExtents`, one stride per surviving
dimension, and the source offset from the lower bounds. `layout_stride` always
returns a `layout_stride::mapping<SubExtents>`.

`layout_left` and `layout_right` return a same-layout mapping when the
surviving unit-stride/full-extent pattern permits it. The current draft adds a
padded-layout result for the contiguous interior pattern, with static padding
computed from the source static extents and padding stride; all other cases
fall back to `layout_stride`. Padded source mappings have dedicated
specializations and reduce to ordinary left/right mappings in rank-one or
rank-zero cases. P3222 transposed special cases must be tested against these
padded branches rather than treated as an independent mapping algorithm.

The customization point is intentionally ADL-based. Built-in layout mapping
classes expose the internal implementation hook, while `submdspan_mapping`
dispatches to it; P3663's future-proofing additions must preserve user-defined
ADL customization and avoid assuming only the standard layout policies exist.

## `submdspan` and `mdspan.at`

`submdspan` canonicalizes raw slices, obtains the mapping result, offsets the
source accessor's data handle, and constructs the result mdspan with the
accessor's `offset_policy`. It is constrained by the mapping's
`sliceable-mapping` property.

`mdspan::at()` will use the existing `_LIBCPP_ASSERT` bounds-checking mechanism
used by the hardened indexing path, but provide the specified throwing
`std::out_of_range` behavior. Existing `operator[]` checks must remain
unchanged except where shared range validation can be factored safely.

## Testing and delivery

Each implementation phase gets focused standalone-compile/run tests because
the sandbox's lit multiprocessing setup cannot bind its forkserver socket.
The design/API phase is committed before implementation. Feature-test macro
and paper-status updates are made with the phase that completes each paper;
the linalg audit and its FTM update remain a separate final phase.
