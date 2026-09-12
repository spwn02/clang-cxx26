//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <generator>
//
// Test co_yield ranges::elements_of for non-generator ranges.

#include <generator>

#include <array>
#include <cassert>
#include <cstddef>
#include <memory_resource>
#include <ranges>
#include <vector>

class tracking_resource : public std::pmr::memory_resource {
public:
  std::size_t allocations   = 0;
  std::size_t deallocations = 0;

private:
  void* do_allocate(std::size_t size, std::size_t alignment) override {
    ++allocations;
    return std::pmr::new_delete_resource()->allocate(size, alignment);
  }

  void do_deallocate(void* pointer, std::size_t size, std::size_t alignment) override {
    ++deallocations;
    std::pmr::new_delete_resource()->deallocate(pointer, size, alignment);
  }

  bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }
};

static std::generator<int> vector_elements() {
  std::vector<int> values = {1, 2, 3};
  co_yield std::ranges::elements_of(values);
}

static std::generator<int> array_elements() {
  std::array<int, 4> values = {4, 5, 6, 7};
  co_yield std::ranges::elements_of(values);
}

static std::generator<int> mixed_elements() {
  co_yield 0;

  std::vector<int> middle = {1, 2, 3};
  co_yield std::ranges::elements_of(middle);

  co_yield 4;

  std::array<int, 2> tail = {5, 6};
  co_yield std::ranges::elements_of(tail);

  co_yield 7;
}

static std::generator<int> nested_range_elements() {
  co_yield 10;

  std::vector<int> middle = {11, 12};
  co_yield std::ranges::elements_of(middle);

  std::array<int, 2> tail = {13, 14};
  co_yield std::ranges::elements_of(tail);

  co_yield 15;
}

static std::generator<int> recursive_range_elements() {
  co_yield 9;
  co_yield std::ranges::elements_of(nested_range_elements());
  co_yield 16;
}

static std::generator<int> allocator_range_elements(tracking_resource& resource) {
  std::array<int, 3> values = {20, 21, 22};
  co_yield std::ranges::elements_of(values, std::pmr::polymorphic_allocator<>(&resource));
}

template <class Range>
static std::vector<int> collect(Range&& range) {
  std::vector<int> result;
  for (int value : range)
    result.push_back(value);
  return result;
}

int main(int, char**) {
  assert((collect(vector_elements()) == std::vector<int>{1, 2, 3}));
  assert((collect(array_elements()) == std::vector<int>{4, 5, 6, 7}));
  assert((collect(mixed_elements()) == std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7}));
  assert((collect(recursive_range_elements()) == std::vector<int>{9, 10, 11, 12, 13, 14, 15, 16}));

  tracking_resource resource;
  assert((collect(allocator_range_elements(resource)) == std::vector<int>{20, 21, 22}));
  assert(resource.allocations == 1);
  assert(resource.deallocations == 1);

  return 0;
}
