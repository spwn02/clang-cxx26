//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <inplace_vector>

// insert/emplace in the middle for a type whose move leaves the source modified: the elements after the insertion point have to
// be shifted without losing the last one.

#include <cassert>
#include <inplace_vector>
#include <string>
#include <vector>

int main(int, char**) {
  for (std::size_t pos = 0; pos <= 3; ++pos) {
    std::inplace_vector<std::string, 8> v = {"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa1", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb2",
                                             "cccccccccccccccccccccccccccccccccccccccc3"};
    auto it = v.insert(v.begin() + pos, std::string("new new new new new new new new new new new"));
    assert(it == v.begin() + pos);
    assert(v.size() == 4);
    std::vector<std::string> expected = {"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa1", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb2",
                                         "cccccccccccccccccccccccccccccccccccccccc3"};
    expected.insert(expected.begin() + pos, "new new new new new new new new new new new");
    for (std::size_t i = 0; i != 4; ++i)
      assert(v[i] == expected[i]);

    auto e = v.emplace(v.begin() + pos, 5, 'z');
    assert(*e == "zzzzz");
    assert(v.size() == 5);
    expected.insert(expected.begin() + pos, "zzzzz");
    for (std::size_t i = 0; i != 5; ++i)
      assert(v[i] == expected[i]);
  }
  return 0;
}
