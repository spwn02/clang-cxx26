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
// Test allocator-aware coroutine frame allocation and deallocation.

#include <generator>

#include <cassert>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <utility>
#include <vector>

class tracking_resource : public std::pmr::memory_resource {
public:
  std::size_t allocations   = 0;
  std::size_t deallocations = 0;
  std::size_t bytes         = 0;
  std::size_t alignment     = 0;

private:
  void* do_allocate(std::size_t size, std::size_t align) override {
    ++allocations;
    bytes     = size;
    alignment = align;
    return std::pmr::new_delete_resource()->allocate(size, align);
  }

  void do_deallocate(void* pointer, std::size_t size, std::size_t align) override {
    ++deallocations;
    assert(size == bytes);
    assert(align == alignment);
    std::pmr::new_delete_resource()->deallocate(pointer, size, align);
  }

  bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }
};

struct allocator_statistics {
  std::size_t allocations   = 0;
  std::size_t deallocations = 0;
};

template <class T>
class tracking_allocator {
public:
  using value_type = T;

  explicit tracking_allocator(allocator_statistics& statistics) : statistics_(&statistics) {}

  template <class U>
  tracking_allocator(const tracking_allocator<U>& other) : statistics_(other.statistics_) {}

  T* allocate(std::size_t count) {
    ++statistics_->allocations;
    return std::allocator<T>().allocate(count);
  }

  void deallocate(T* pointer, std::size_t count) {
    ++statistics_->deallocations;
    std::allocator<T>().deallocate(pointer, count);
  }

  template <class U>
  bool operator==(const tracking_allocator<U>& other) const {
    return statistics_ == other.statistics_;
  }

private:
  template <class>
  friend class tracking_allocator;

  allocator_statistics* statistics_;
};

static std::pmr::generator<int>
allocator_arg_generator(std::allocator_arg_t, std::pmr::polymorphic_allocator<>, int count) {
  for (int i = 0; i != count; ++i)
    co_yield i;
}

static std::generator<int>
type_erased_allocator_generator(std::allocator_arg_t, tracking_allocator<std::byte>, int value) {
  co_yield value;
}

struct generator_factory {
  std::pmr::generator<int>
  allocator_arg_generator(std::allocator_arg_t, std::pmr::polymorphic_allocator<>, int value) const {
    co_yield value;
  }
};

int main(int, char**) {
  tracking_resource resource;
  std::pmr::polymorphic_allocator<> allocator(&resource);

  {
    auto generator = allocator_arg_generator(std::allocator_arg, allocator, 3);
    assert(resource.allocations == 1);
    assert(resource.deallocations == 0);

    std::vector<int> result;
    for (int value : generator)
      result.push_back(value);
    assert((result == std::vector<int>{0, 1, 2}));
  }
  assert(resource.allocations == 1);
  assert(resource.deallocations == 1);

  {
    const generator_factory factory;
    auto generator = factory.allocator_arg_generator(std::allocator_arg, allocator, 42);
    assert(resource.allocations == 2);
    assert(resource.deallocations == 1);

    auto iterator = generator.begin();
    assert(*iterator == 42);
    ++iterator;
    assert(iterator == generator.end());
  }
  assert(resource.allocations == 2);
  assert(resource.deallocations == 2);

  allocator_statistics statistics;
  {
    auto generator =
        type_erased_allocator_generator(std::allocator_arg, tracking_allocator<std::byte>(statistics), 17);
    assert(statistics.allocations == 1);
    assert(statistics.deallocations == 0);

    auto iterator = generator.begin();
    assert(*iterator == 17);
    ++iterator;
    assert(iterator == generator.end());
  }
  assert(statistics.allocations == 1);
  assert(statistics.deallocations == 1);

  return 0;
}
