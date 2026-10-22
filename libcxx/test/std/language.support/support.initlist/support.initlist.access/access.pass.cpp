//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03

// template<class E> class initializer_list;

// const E* begin() const;
// const E* end() const;
// size_t size() const;
// constexpr const E* data() const noexcept;  // C++26
// constexpr bool empty() const noexcept;     // C++26

#include <initializer_list>
#include <cassert>
#include <cstddef>

#include "test_macros.h"

struct A
{
    A(std::initializer_list<int> il)
    {
        const int* b = il.begin();
        const int* e = il.end();
        assert(il.size() == 3);
        assert(static_cast<std::size_t>(e - b) == il.size());
        assert(*b++ == 3);
        assert(*b++ == 2);
        assert(*b++ == 1);
    }
};

#if TEST_STD_VER > 11
struct B
{
    constexpr B(std::initializer_list<int> il)
    {
        const int* b = il.begin();
        const int* e = il.end();
        assert(il.size() == 3);
        assert(static_cast<std::size_t>(e - b) == il.size());
        assert(*b++ == 3);
        assert(*b++ == 2);
        assert(*b++ == 1);
    }
};

#endif // TEST_STD_VER > 11

#if TEST_STD_VER >= 26
constexpr bool test_data_empty() {
  std::initializer_list<int> il = {1, 2, 3};
  ASSERT_NOEXCEPT(il.data());
  ASSERT_NOEXCEPT(il.empty());
  ASSERT_SAME_TYPE(decltype(il.data()), const int*);
  ASSERT_SAME_TYPE(decltype(il.empty()), bool);
  assert(il.data() == il.begin());
  assert(!il.empty());
  std::initializer_list<int> none;
  assert(none.empty() && none.size() == 0);
  return true;
}
static_assert(test_data_empty());
#endif

int main(int, char**)
{
#if TEST_STD_VER >= 26
    assert(test_data_empty());
#endif
    A test1 = {3, 2, 1}; (void)test1;
#if TEST_STD_VER > 11
    constexpr B test2 = {3, 2, 1};
    (void)test2;
#endif // TEST_STD_VER > 11

  return 0;
}
