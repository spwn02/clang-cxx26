//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// P1899R3 stride_view -- construction, base(), stride(), the views::stride CPO
// (including pipeability), and enable_borrowed_range.

#include <algorithm>
#include <ranges>
#include <cassert>
#include <concepts>
#include <forward_list>
#include <list>
#include <vector>

template <class T>
concept HasStrideView = requires { typename std::ranges::stride_view<T>; };

struct NotAView {};
static_assert(!HasStrideView<NotAView>);

bool test() {
  // Default construction.
  {
    std::ranges::stride_view<std::ranges::empty_view<int>> sv;
    static_assert(std::default_initializable<decltype(sv)>);
    assert(sv.stride() == 1);
  }

  // Explicit (base, stride) construction, base()/stride() accessors.
  {
    int a[]  = {1, 2, 3, 4, 5};
    auto sv  = std::ranges::stride_view(a, 2);
    assert(sv.stride() == 2);
    assert(std::ranges::equal(sv.base(), a));
  }

  // CTAD.
  {
    std::vector<int> v = {1, 2, 3};
    std::ranges::stride_view sv(v, 1);
    static_assert(std::same_as<decltype(sv), std::ranges::stride_view<std::ranges::ref_view<std::vector<int>>>>);
  }

  // views::stride CPO, both call forms.
  {
    std::vector<int> v = {0, 1, 2, 3, 4, 5, 6};
    auto sv1            = std::views::stride(v, 2);
    auto sv2            = v | std::views::stride(2);
    std::vector<int> want = {0, 2, 4, 6};
    assert(std::ranges::equal(sv1, want));
    assert(std::ranges::equal(sv2, want));
  }

  // Pipeability composes with other adaptors.
  {
    std::vector<int> v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto r              = v | std::views::stride(2) | std::views::take(3);
    std::vector<int> want = {0, 2, 4};
    assert(std::ranges::equal(r, want));
  }

  // enable_borrowed_range propagates from the underlying view.
  {
    static_assert(std::ranges::borrowed_range<std::ranges::stride_view<std::ranges::ref_view<std::vector<int>>>>);
    static_assert(!std::ranges::borrowed_range<std::ranges::stride_view<std::ranges::owning_view<std::vector<int>>>>);
  }

  // stride(1) yields every element, identical to the base range.
  {
    std::list<int> l   = {5, 4, 3, 2, 1};
    auto sv             = std::ranges::stride_view(l, 1);
    assert(std::ranges::equal(sv, l));
  }

  // A stride larger than the range yields exactly one element.
  {
    std::vector<int> v  = {10, 20, 30};
    auto sv              = std::ranges::stride_view(v, 100);
    std::vector<int> got(sv.begin(), sv.end());
    assert((got == std::vector<int>{10}));
  }

  // An input-only underlying range is still usable (forward_iterator not required).
  {
    std::forward_list<int> fl = {0, 1, 2, 3, 4};
    auto sv                    = std::ranges::stride_view(fl, 2);
    std::vector<int> got(sv.begin(), sv.end());
    assert((got == std::vector<int>{0, 2, 4}));
  }

  return true;
}

int main(int, char**) {
  test();
  return 0;
}
