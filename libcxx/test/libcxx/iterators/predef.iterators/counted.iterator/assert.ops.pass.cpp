//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: has-unix-headers
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: libcpp-hardening-mode=none
// XFAIL: libcpp-hardening-mode=debug && availability-verbose_abort-missing

#include <cassert>
#include <iterator>

#include "check_assertion.h"
#include "test_iterators.h"

int main(int, char**) {
  int data[] = {1, 2, 3};

  {
    std::counted_iterator i(data, 1);
    assert(*i == 1);
    assert(i[0] == 1);
    assert(std::ranges::iter_move(i) == 1);
    std::ranges::iter_swap(i, i);

    TEST_LIBCPP_ASSERT_FAILURE(++std::counted_iterator(data, 0), "Iterator already at or past end.");
    TEST_LIBCPP_ASSERT_FAILURE(std::counted_iterator(data, 0)++, "Iterator already at or past end.");
    TEST_LIBCPP_ASSERT_FAILURE(std::counted_iterator(data, 0)[0], "Subscript argument must be less than size.");
    TEST_LIBCPP_ASSERT_FAILURE(
        std::ranges::iter_move(std::counted_iterator(data, 0)), "Iterator must not be past end of range.");
    TEST_LIBCPP_ASSERT_FAILURE(
        std::ranges::iter_swap(std::counted_iterator(data, 0), i),
        "Iterators must not be past end of range.");
    TEST_LIBCPP_ASSERT_FAILURE(
        std::ranges::iter_swap(i, std::counted_iterator(data, 0)),
        "Iterators must not be past end of range.");
  }

  {
    using Input = cpp17_input_iterator<int*>;
    Input input(data);
    std::counted_iterator<Input> i(input, 1);
    i++;
    assert(i.count() == 0);

    Input empty_input(data);
    std::counted_iterator<Input> empty(empty_input, 0);
    TEST_LIBCPP_ASSERT_FAILURE(empty++, "Iterator already at or past end.");
  }

  {
    using Forward = forward_iterator<int*>;
    Forward forward(data);
    std::counted_iterator<Forward> i(forward, 1);
    assert(i++ == std::counted_iterator<Forward>(forward, 1));

    std::counted_iterator<Forward> empty(forward, 0);
    TEST_LIBCPP_ASSERT_FAILURE(empty++, "Iterator already at or past end.");
  }

  {
    TEST_LIBCPP_ASSERT_FAILURE(
        std::counted_iterator(data, -1), "__n must not be negative.");

    std::counted_iterator i(data, 1);
    i += 1;
    assert(i.count() == 0);
    TEST_LIBCPP_ASSERT_FAILURE(i += 1, "Cannot advance iterator past end.");

    std::counted_iterator j(data + 1, 1);
    j -= 1;
    assert(j.count() == 2);
    TEST_LIBCPP_ASSERT_FAILURE(j -= -3,
                               "Attempt to subtract too large of a size: counted_iterator would be decremented before the first element of its range.");
  }

  return 0;
}
