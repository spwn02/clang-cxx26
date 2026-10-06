//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// <thread>

// [thread.attributes]: thread::name_hint<char> and thread::stack_size_hint, and the constructors thread(attrs..., f, args...)
// and jthread(attrs..., f, args...).

#include <cassert>
#include <concepts>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>

#include "test_macros.h"

#if defined(__linux__)
#  include <pthread.h>
#endif

#ifndef __cpp_lib_thread_attributes
#  error __cpp_lib_thread_attributes should be defined
#endif

using NameHint = std::thread::name_hint<char>;

// the attribute types
static_assert(!std::is_copy_constructible_v<NameHint>);
static_assert(!std::is_move_constructible_v<NameHint>);
static_assert(std::is_nothrow_constructible_v<NameHint, std::string_view>);
static_assert(!std::is_convertible_v<std::string_view, NameHint>); // explicit
static_assert(std::is_nothrow_constructible_v<std::thread::stack_size_hint, std::size_t>);
static_assert(std::is_copy_constructible_v<std::thread::stack_size_hint>);
static_assert(std::is_same_v<std::jthread::name_hint<char>, NameHint>);
static_assert(std::is_same_v<std::jthread::stack_size_hint, std::thread::stack_size_hint>);

// the deduction guides
static_assert(std::is_same_v<decltype(std::thread::name_hint("name")), NameHint>);
static_assert(std::is_same_v<decltype(std::thread::name_hint(std::string("name"))), NameHint>);
constexpr std::thread::stack_size_hint constexpr_hint(4096); // constexpr constructor
static_assert(sizeof(constexpr_hint) > 0);

// only char is accepted
template <class T>
concept has_name_hint = requires { typename std::thread::name_hint<T>; };
static_assert(has_name_hint<char>);
static_assert(!has_name_hint<wchar_t>);
static_assert(!has_name_hint<int>);

#if defined(__linux__)
std::string current_name() {
  char buffer[32] = {};
  pthread_getname_np(pthread_self(), buffer, sizeof(buffer));
  return buffer;
}

std::size_t current_stack_size() {
  pthread_attr_t attr;
  pthread_getattr_np(pthread_self(), &attr);
  std::size_t size;
  void* address;
  pthread_attr_getstack(&attr, &address, &size);
  pthread_attr_destroy(&attr);
  return size;
}
#endif

int main(int, char**) {
  // no attributes: as before
  {
    int x = 0;
    std::thread t([&x] { x = 1; });
    t.join();
    assert(x == 1);
  }

  // attributes first, then the function and its arguments
  {
    int x = 0;
    std::thread t(std::thread::stack_size_hint(1 << 20), NameHint("attributes"), [&x](int a, int b) { x = a + b; }, 40, 2);
    t.join();
    assert(x == 42);
  }

  // an lvalue attribute, a name from a std::string and a zero size (ignored)
  {
    std::thread::stack_size_hint zero(0);
    std::string name = "from-string";
    int x            = 0;
    std::thread t(zero, std::thread::name_hint(name), [&x] { x = 7; });
    t.join();
    assert(x == 7);
  }

  // the stack size is a hint that may be adjusted: tiny, odd and absurdly large sizes still start the thread
  for (std::size_t size : {std::size_t(1), std::size_t(100), (std::size_t(1) << 20) + 1, ~std::size_t(0)}) {
    int x = 0;
    std::thread t(std::thread::stack_size_hint(size), [&x] { x = 11; });
    t.join();
    assert(x == 11);
  }

  // the function can be a member function pointer, arguments are decay-copied as without attributes
  {
    struct S {
      int v = 0;
      void set(int i) { v = i; }
    } s;
    std::thread t(std::thread::name_hint("member"), &S::set, &s, 5);
    t.join();
    assert(s.v == 5);
  }

#if defined(__linux__)
  // the name (truncated to the 15 characters Linux accepts) and the stack size are applied to the new thread
  {
    std::string name;
    std::size_t stack = 0;
    std::thread t(std::thread::name_hint("a-rather-long-thread-name"), std::thread::stack_size_hint(4 << 20), [&] {
      name  = current_name();
      stack = current_stack_size();
    });
    t.join();
    assert(name == "a-rather-long-t");
    assert(stack >= (4u << 20));
  }
  {
    std::string name;
    std::thread t(std::thread::name_hint(std::string_view("short")), [&] { name = current_name(); });
    t.join();
    assert(name == "short");
  }
#endif

  // jthread: the stop token follows the function as without attributes
  {
    bool stop_possible = false;
    std::jthread j(NameHint("jt"), std::thread::stack_size_hint(1 << 20), [&](std::stop_token st, int v) {
      stop_possible = st.stop_possible() && v == 3;
    }, 3);
    j.join();
    assert(stop_possible);
  }
  {
    int x = 0;
    std::jthread j(std::thread::stack_size_hint(1 << 20), [&x] { x = 9; });
    j.join();
    assert(x == 9);
  }
#if defined(__linux__)
  {
    std::string name;
    std::jthread j(std::thread::name_hint("jthread-name"), [&](std::stop_token) { name = current_name(); });
    j.join();
    assert(name == "jthread-name");
  }
#endif

  return 0;
}
