//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

#include <cassert>
#include <execution>
#include <exception>
#include <memory_resource>

struct allocator_aware {
  using allocator_type = std::pmr::polymorphic_allocator<>;

  allocator_aware(int value) : value(value), resource(nullptr) {}
  allocator_aware(std::allocator_arg_t, allocator_type alloc, const allocator_aware& other)
      : value(other.value), resource(alloc.resource()) {}

  int value;
  std::pmr::memory_resource* resource;
};

struct receiver {
  using receiver_concept = std::execution::receiver_tag;

  std::pmr::memory_resource* resource;
  allocator_aware* result;

  auto get_env() const noexcept {
    return std::execution::prop(std::get_allocator,
                                std::pmr::polymorphic_allocator<>(resource));
  }

  void set_value(allocator_aware value) && noexcept { *result = value; }
  void set_error(std::exception_ptr) && noexcept { assert(false); }
  void set_stopped() && noexcept { assert(false); }
};

int main(int, char**) {
  std::byte storage[1024];
  std::pmr::monotonic_buffer_resource resource(storage, sizeof(storage));

  // P3433R1's product-type branch allocator-constructs each element of just's tuple.
  allocator_aware result{0};
  auto op = std::execution::connect(std::execution::just(allocator_aware{42}), receiver{&resource, &result});
  std::execution::start(op);
  assert(result.value == 42);
  assert(result.resource == &resource);

  // The non-product branch allocator-constructs then's stored callable.
  struct fn {
    using allocator_type = std::pmr::polymorphic_allocator<>;
    fn() : resource(nullptr) {}
    fn(std::allocator_arg_t, allocator_type alloc, const fn&) : resource(alloc.resource()) {}
    allocator_aware operator()(int value) const {
      allocator_aware result{value};
      result.resource = resource;
      return result;
    }
    std::pmr::memory_resource* resource;
  } callable;

  auto sender = std::execution::just(7) | std::execution::then(callable);
  auto op2 = std::execution::connect(std::move(sender), receiver{&resource, &result});
  std::execution::start(op2);
  assert(result.value == 7);
  assert(result.resource == &resource);

  return 0;
}
