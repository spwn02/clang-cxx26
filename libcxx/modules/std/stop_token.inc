// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std {
#if _LIBCPP_HAS_THREADS
  // [stoptoken], class stop_­token
  using std::stop_token;

  // [stopsource], class stop_­source
  using std::stop_source;

  // no-shared-stop-state indicator
  using std::nostopstate;
  using std::nostopstate_t;

  // [stopcallback], class template stop_­callback
  using std::stop_callback;

#  if _LIBCPP_STD_VER >= 26
  // [stoptoken.concepts]
  using std::stoppable_token;
  using std::unstoppable_token;
  using std::stop_callback_for_t;

  // [stoptoken.never], class never_stop_token
  using std::never_stop_token;

  // [stopsource.inplace], class inplace_stop_source
  using std::inplace_stop_source;

  // [stoptoken.inplace], class inplace_stop_token
  using std::inplace_stop_token;

  // [stopcallback.inplace], class template inplace_stop_callback
  using std::inplace_stop_callback;
#  endif // _LIBCPP_STD_VER >= 26
#endif // _LIBCPP_HAS_THREADS
} // namespace std
