//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// LWG4444: the default template argument of the new-value parameter of
// ranges::replace is iter_value_t<I> / range_value_t<R>, and that of
// ranges::replace_if is iter_value_t<I> / range_value_t<R>, so a braced
// initializer list can be passed for the new value.

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

int main(int, char**) {
  std::vector<std::string> v{"a", "b", "c"};
  std::ranges::replace(v, "a", {});
  std::ranges::replace(v.begin(), v.end(), "b", {});
  std::ranges::replace_if(v, [](const std::string& s) { return s == "c"; }, {});
  assert(v[0].empty() && v[1].empty() && v[2].empty());
  return 0;
}
