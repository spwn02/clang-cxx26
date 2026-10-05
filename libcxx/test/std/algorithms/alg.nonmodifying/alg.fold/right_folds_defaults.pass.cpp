//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS(clang): -Wno-braced-scalar-init

// <algorithm>

// [alg.fold]: ranges::fold_right defaults T to iter_value_t<I> / range_value_t<R>, so a braced initializer deduces.

#include <algorithm>
#include <cassert>
#include <functional>
#include <vector>

int main(int, char**) {
  std::vector<int> v{1, 2};
  assert((std::ranges::fold_right(v, {0}, std::plus<>{}) == 3));
  assert((std::ranges::fold_right(v.begin(), v.end(), {0}, std::plus<>{}) == 3));
  return 0;
}
