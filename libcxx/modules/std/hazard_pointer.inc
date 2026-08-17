// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std {
#if _LIBCPP_STD_VER >= 26
  // 4.1.3, class template hazard_pointer_obj_base
  using std::hazard_pointer_obj_base;
  // 4.1.4, class hazard_pointer
  using std::hazard_pointer;
  // 4.1.5, Construct non-empty hazard_pointer
  using std::make_hazard_pointer;
  // 4.1.6, Hazard pointer swap
  using std::swap;
#endif // _LIBCPP_STD_VER >= 26
} // namespace std
