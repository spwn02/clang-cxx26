// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding -fno-exceptions

// P1642R11 and P2976R1 [memory.syn]: selected facilities are freestanding.

#include <memory>
#include <atomic>
#include <functional>
#include <version>

#ifndef _LIBCPP_FREESTANDING
#  error "-ffreestanding must select libc++ freestanding mode"
#endif

#if !defined(__cpp_lib_freestanding_memory) || __cpp_lib_freestanding_memory != 202502L
#  error "missing or wrong __cpp_lib_freestanding_memory"
#endif

struct Object {
  constexpr Object() : value(0) {}
  constexpr explicit Object(int input) : value(input) {}
  int value;
};

template <class Type>
struct TestAllocator {
  using value_type = Type;
};

void test_memory() {
  Object object;
  Object* address = std::addressof(object);
  std::allocator_arg_t allocator_tag = std::allocator_arg;
  (void)allocator_tag;

  int storage[2] = {};
  void* aligned = storage;
  std::size_t space = sizeof(storage);
  void* result = std::align(alignof(int), sizeof(int), aligned, space);

  int values[2] = {1, 2};
  int destination[2];
  std::uninitialized_default_construct(destination, destination + 2);
  std::destroy(destination, destination + 2);
  std::uninitialized_value_construct(destination, destination + 2);
  std::destroy_n(destination, 2);
  std::uninitialized_copy(values, values + 2, destination);
  std::uninitialized_copy_n(values, 2, destination);
  std::destroy_at(destination);
  std::uninitialized_move(values, values + 2, destination);
  std::uninitialized_fill(destination, destination + 2, 3);
  std::uninitialized_fill_n(destination, 2, 4);
  std::destroy(destination, destination + 2);

  std::unique_ptr<int> pointer(new int(4));
  auto allocator_args = std::uses_allocator_construction_args<Object>(TestAllocator<int>{});
  (void)allocator_args;
  (void)address;
  (void)result;
  (void)pointer;
}

static_assert(std::is_same_v<std::pointer_traits<int*>::element_type, int>);
static_assert(std::is_same_v<decltype(std::to_address(static_cast<int*>(nullptr))), int*>);
static_assert(std::is_same_v<decltype(std::allocator_arg), const std::allocator_arg_t>);
static_assert(std::uses_allocator_v<Object, TestAllocator<Object>> == false);
static_assert(std::is_same_v<std::allocator_traits<TestAllocator<int>>::value_type, int>);
static_assert(std::is_same_v<decltype(std::allocator_traits<TestAllocator<int>>::allocate_at_least(
                                 std::declval<TestAllocator<int>&>(), 1)),
                             std::allocation_result<int*, std::size_t>>);
static_assert(std::is_same_v<decltype(std::make_obj_using_allocator<Object>(TestAllocator<int>{})),
                             Object>);
static_assert(std::is_same_v<decltype(std::uninitialized_construct_using_allocator<Object>(
                                 static_cast<Object*>(nullptr), TestAllocator<int>{})),
                             Object*>);
static_assert(std::is_same_v<decltype(std::assume_aligned<alignof(int)>(static_cast<int*>(nullptr))), int*>);
static_assert(std::is_same_v<decltype(std::construct_at(static_cast<Object*>(nullptr))), Object*>);
static_assert(std::is_same_v<decltype(std::destroy_at(static_cast<Object*>(nullptr))), void>);
static_assert(std::is_same_v<decltype(std::destroy_n(static_cast<Object*>(nullptr), 0)), Object*>);
static_assert(std::is_same_v<decltype(std::uninitialized_default_construct_n(static_cast<Object*>(nullptr), 0)), Object*>);
static_assert(std::is_same_v<decltype(std::uninitialized_value_construct_n(static_cast<Object*>(nullptr), 0)), Object*>);
static_assert(std::is_same_v<decltype(std::uninitialized_copy_n(static_cast<Object*>(nullptr), 0,
                                                               static_cast<Object*>(nullptr))), Object*>);
static_assert(std::is_same_v<decltype(std::uninitialized_move_n(static_cast<Object*>(nullptr), 0,
                                                                static_cast<Object*>(nullptr))),
                             std::pair<Object*, Object*>>);
static_assert(std::is_same_v<decltype(std::uninitialized_fill_n(static_cast<Object*>(nullptr), 0, Object{})), Object*>);
static_assert(std::is_same_v<decltype(std::default_delete<int>{}(static_cast<int*>(nullptr))), void>);
static_assert(std::is_same_v<decltype(std::unique_ptr<int>{}), std::unique_ptr<int>>);
static_assert(std::is_same_v<decltype(std::atomic<int>{}), std::atomic<int>>);
static_assert(std::is_same_v<decltype(std::hash<std::unique_ptr<int>>{}(std::declval<std::unique_ptr<int>>())),
                             std::size_t>);
