// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std {
  using std::initializer_list;

#if _LIBCPP_STD_VER < 26
  // [support.initlist.range], initializer list range access; moved to <iterator> by P3016R6
  using std::begin;
  using std::end;
#endif
} // namespace std
