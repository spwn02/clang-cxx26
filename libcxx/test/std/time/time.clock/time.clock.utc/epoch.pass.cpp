//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17
// UNSUPPORTED: no-filesystem, no-localization, no-tzdb
// XFAIL: libcpp-has-no-experimental-tzdb
// XFAIL: availability-tzdb-missing

#include <chrono>
#include <cassert>

int main(int, char**) {
  using namespace std::chrono;
  constexpr sys_days epoch = January / 1 / 1970;
  constexpr utc_seconds utc_epoch{};

  assert(utc_clock::to_sys(utc_epoch) == sys_seconds{});
  assert(utc_clock::from_sys(sys_seconds{}) == utc_epoch);
  assert(utc_clock::to_sys(utc_seconds{seconds{1}}) == sys_seconds{seconds{1}});
  assert(utc_clock::from_sys(sys_seconds{seconds{1}}) == utc_seconds{seconds{1}});
  assert(sys_days{floor<days>(utc_clock::to_sys(utc_epoch))} == epoch);
  return 0;
}
