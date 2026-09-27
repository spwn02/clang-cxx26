// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [iterator.synopsis]: every entity except the four stream iterators is freestanding.

#include <iterator>
#include <version>

#ifndef _LIBCPP_FREESTANDING
#  error "-ffreestanding must select libc++ freestanding mode"
#endif
#if !defined(__cpp_lib_freestanding_iterator) || __cpp_lib_freestanding_iterator != 202306L
#  error "missing or wrong __cpp_lib_freestanding_iterator"
#endif

struct Container {
  using value_type = int;
  int* first;
  int* last;
  int* begin() { return first; }
  int* end() { return last; }
  int* data() { return first; }
  constexpr unsigned size() const { return static_cast<unsigned>(last - first); }
  constexpr bool empty() const { return first == last; }
  void push_back(int) {}
  void push_front(int) {}
  int* insert(int* position, int) { return position; }
};

struct Sentinel {
  int* end;
  friend bool operator==(int* position, Sentinel sentinel) { return position == sentinel.end; }
  friend bool operator==(Sentinel sentinel, int* position) { return position == sentinel.end; }
};

void test_iterator() {
  static_assert(std::is_base_of_v<std::input_iterator_tag, std::forward_iterator_tag>);
  static_assert(std::contiguous_iterator<int*>);
  static_assert(std::same_as<std::iterator_traits<int*>::value_type, int>);
  int values[] = {1, 2, 3};
  std::reverse_iterator reverse(values + 3);
  std::move_iterator moved(values);
  using Common = std::common_iterator<int*, Sentinel>;
  using Counted = std::counted_iterator<int*>;
  Common common(values);
  Counted counted(values, 3);
  Container container{values, values + 3};
  auto back = std::back_inserter(container);
  auto front = std::front_inserter(container);
  auto insert = std::inserter(container, values);
  *back = 4;
  *front = 5;
  *insert = 6;
  int* current = values;
  std::advance(current, 1);
  auto length = std::distance(values, values + 3);
  auto following = std::next(current);
  auto preceding = std::prev(following);
  auto first = std::begin(container);
  auto last = std::end(container);
  auto count = std::size(container);
  auto is_empty = std::empty(container);
  auto pointer = std::data(container);
  std::default_sentinel_t default_sentinel;
  std::unreachable_sentinel_t unreachable_sentinel;
  (void)reverse; (void)moved; (void)common; (void)counted; (void)length;
  (void)preceding; (void)first; (void)last; (void)count; (void)is_empty;
  (void)pointer; (void)default_sentinel; (void)unreachable_sentinel;
}
