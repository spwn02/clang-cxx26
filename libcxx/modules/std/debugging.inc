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
  using std::breakpoint;
  using std::breakpoint_if_debugging;
  using std::is_debugger_present;
#endif
#if _LIBCPP_STD_VER >= 29 && __has_builtin(__builtin_constexpr_diag)
  using std::constexpr_error_str;
  using std::constexpr_print_str;
  using std::constexpr_warning_str;
#endif
} // namespace std
