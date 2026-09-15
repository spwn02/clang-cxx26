// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std {
#if _LIBCPP_STD_VER >= 23
  // [mdspan.extents], class template extents
  using std::extents;

  // [mdspan.extents.dextents], alias template dextents
  using std::dextents;

#  if _LIBCPP_STD_VER >= 26
  // [mdspan.extents.dims]
  using std::dims;
#  endif // _LIBCPP_STD_VER >= 26

  // [mdspan.layout], layout mapping
  using std::layout_left;
  using std::layout_right;
  using std::layout_stride;

  // [mdspan.accessor.default], class template default_accessor
  using std::default_accessor;

#  if _LIBCPP_STD_VER >= 26
  // [mdspan.accessor.aligned], class template aligned_accessor
  using std::aligned_accessor;
#  endif // _LIBCPP_STD_VER >= 26

  // [mdspan.mdspan], class template mdspan
  using std::mdspan;

#  if _LIBCPP_STD_VER >= 26
  // [mdspan.layout.left.padded], [mdspan.layout.right.padded], padded layout mappings
  using std::layout_left_padded;
  using std::layout_right_padded;

  // [mdspan.sub], submdspan
  using std::full_extent;
  using std::full_extent_t;
  using std::extent_slice;
  using std::range_slice;
  using std::canonical_slices;
  using std::subextents;
  using std::submdspan_mapping_result;
  using std::submdspan_mapping;
  using std::submdspan;
#  endif // _LIBCPP_STD_VER >= 26
#endif // _LIBCPP_STD_VER >= 23
} // namespace std
