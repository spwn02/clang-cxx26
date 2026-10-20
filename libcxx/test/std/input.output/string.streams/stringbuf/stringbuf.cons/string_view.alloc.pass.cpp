//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <sstream>

// template<class T>
//   basic_stringbuf(const T& t, ios_base::openmode which, const Allocator& a);
// template<class T>
//   basic_stringbuf(const T& t, const Allocator& a);

// [stringbuf.cons]: "direct-non-list-initializes buf with sv, a": the allocator is the one passed
// (an allocator that has no default constructor and is not propagated by assignment is usable).

#include <cassert>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string_view>

template <class T>
struct A {
  using value_type = T;
  int id;
  A() = delete;
  explicit A(int n) : id(n) {}
  template <class U>
  A(const A<U>& x) : id(x.id) {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  friend bool operator==(const A&, const A&) = default;
};

int main(int, char**) {
  using B = std::basic_stringbuf<char, std::char_traits<char>, A<char>>;
  {
    B b(std::string_view("abc"), std::ios::in | std::ios::out, A<char>(7));
    assert(b.get_allocator().id == 7);
    assert(b.view() == "abc");
  }
  {
    B b(std::string_view("abc"), A<char>(8));
    assert(b.get_allocator().id == 8);
    assert(b.view() == "abc");
  }
  return 0;
}
