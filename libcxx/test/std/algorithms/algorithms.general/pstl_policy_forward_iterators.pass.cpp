//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// <algorithm>

// The ExecutionPolicy overloads of find_end, find_first_of, adjacent_find, mismatch, search,
// for_each_n and generate_n take forward iterators ([alg.find], [alg.search], [alg.mismatch],
// [alg.foreach], [alg.generate]), with and without the predicate, and for_each_n and generate_n
// return the end of the range. The random access iterators take the parallel backends, the
// forward iterators the serial algorithm.

#include <algorithm>
#include <cassert>
#include <execution>
#include <functional>
#include <iterator>
#include <utility>

#include "test_iterators.h"

template <class It>
void test() {
  using P = std::pair<It, It>;
  const auto& seq = std::execution::seq;
  auto eq        = [](int a, int b) { return a == b; };

  int a[]{1, 2, 3, 1, 2, 3, 4, 4};
  int p[]{2, 3};
  int q[]{9, 3};
  auto first = It(a);
  auto last  = It(a + 8);

  // find_end: the last 2, 3
  assert(std::find_end(seq, first, last, It(p), It(p + 2)) == It(a + 4));
  assert(std::find_end(seq, first, last, It(p), It(p + 2), eq) == It(a + 4));
  assert(std::find_end(seq, first, last, It(q), It(q + 2)) == last);

  // find_first_of: the first 3
  assert(std::find_first_of(seq, first, last, It(q), It(q + 2)) == It(a + 2));
  assert(std::find_first_of(seq, first, last, It(q), It(q + 2), eq) == It(a + 2));
  int none[]{7, 8};
  assert(std::find_first_of(seq, first, last, It(none), It(none + 2)) == last);

  // adjacent_find: the 4, 4
  assert(std::adjacent_find(seq, first, last) == It(a + 6));
  assert(std::adjacent_find(seq, first, last, eq) == It(a + 6));
  assert(std::adjacent_find(seq, first, first) == first);

  // mismatch with the three and the four iterators
  int b[]{1, 2, 3, 1, 9, 3, 4, 4};
  {
    P r = std::mismatch(seq, first, last, It(b));
    assert(r.first == It(a + 4) && r.second == It(b + 4));
  }
  {
    P r = std::mismatch(seq, first, last, It(b), eq);
    assert(r.first == It(a + 4) && r.second == It(b + 4));
  }
  {
    P r = std::mismatch(seq, first, last, It(b), It(b + 8));
    assert(r.first == It(a + 4) && r.second == It(b + 4));
  }
  {
    P r = std::mismatch(seq, first, last, It(b), It(b + 8), eq);
    assert(r.first == It(a + 4) && r.second == It(b + 4));
  }
  {
    P r = std::mismatch(seq, first, last, It(b), It(b + 2));
    assert(r.first == It(a + 2) && r.second == It(b + 2));
  }

  // search
  int s[]{3, 4, 4};
  assert(std::search(seq, first, last, It(s), It(s + 3)) == It(a + 5));
  assert(std::search(seq, first, last, It(s), It(s + 3), eq) == It(a + 5));
  assert(std::search(seq, first, last, It(none), It(none + 2)) == last);

  // for_each_n and generate_n return the end of the range, also for n == 0
  {
    int c[4]{};
    int sum = 0;
    It r    = std::for_each_n(seq, It(c), 4, [&](int& x) { x = 5; sum += x; });
    assert(r == It(c + 4));
    assert(sum == 20 && c[0] == 5 && c[3] == 5);
    assert(std::for_each_n(seq, It(c), 0, [](int& x) { x = 0; }) == It(c));
    assert(c[0] == 5);
  }
  {
    int c[4]{};
    int v = 0;
    It r  = std::generate_n(seq, It(c), 4, [&] { return ++v; });
    assert(r == It(c + 4));
    assert(c[0] == 1 && c[3] == 4);
    assert(std::generate_n(seq, It(c), 0, [] { return 9; }) == It(c));
    assert(c[0] == 1);
  }
}

int main(int, char**) {
  test<forward_iterator<int*>>();
  test<bidirectional_iterator<int*>>();
  test<random_access_iterator<int*>>();
  test<int*>();
  return 0;
}
