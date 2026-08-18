//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <hive>

// std::pmr::hive<T> is an alias for std::hive<T, std::pmr::polymorphic_allocator<T>>
// ([hive.syn]): namespace pmr { template<class T> using hive = std::hive<T, polymorphic_allocator<T>>; }

#include <hive>
#include <memory_resource>
#include <type_traits>
#include <cassert>
#include <string>
#include <vector>
#include <array>
#include <cstddef>
#include <algorithm>

int main(int, char**) {
  static_assert(std::is_same_v<std::pmr::hive<int>, std::hive<int, std::pmr::polymorphic_allocator<int>>>);
  static_assert(std::is_same_v<std::pmr::hive<int>::allocator_type, std::pmr::polymorphic_allocator<int>>);

  {
    // Default-constructed pmr::hive uses the default (get_default_resource()) resource.
    std::pmr::hive<int> h;
    h.insert(1);
    h.insert(2);
    h.insert(3);
    assert(h.size() == 3);
    std::vector<int> v(h.begin(), h.end());
    std::sort(v.begin(), v.end());
    assert((v == std::vector<int>{1, 2, 3}));
  }

  {
    // Allocation is actually routed through the supplied memory_resource.
    std::array<std::byte, 4096> buffer;
    std::pmr::monotonic_buffer_resource mbr(buffer.data(), buffer.size());
    std::pmr::hive<std::string> h(&mbr);
    for (int i = 0; i < 50; ++i)
      h.insert(std::to_string(i));
    assert(h.size() == 50);
    assert(h.get_allocator().resource() == &mbr);
  }

  {
    // Copy/move preserve element content; allocator-extended construction
    // lets a pmr container be built with an explicit resource.
    std::pmr::hive<int> a{1, 2, 3, 4, 5};
    std::pmr::hive<int> b(a, std::pmr::get_default_resource());
    assert(b.size() == 5);
    std::pmr::hive<int> c(std::move(b));
    assert(c.size() == 5);
  }

  return 0;
}
