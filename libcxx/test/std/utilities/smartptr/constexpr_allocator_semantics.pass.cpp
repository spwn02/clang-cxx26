//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS(has-fconstexpr-steps): -fconstexpr-steps=5000000

// [util.smartptr.shared.const], [util.smartptr.shared.create]: in constant evaluation the allocator is used for the
// control block and the elements, and the for_overwrite factories default-initialize without the allocator's
// construct.

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

struct Del { constexpr void operator()(int* p) const { delete p; } };
template <class T> struct CAlloc {
  using value_type = T;
  int* allocs; int* deallocs; int* constructs; int* destroys;
  constexpr CAlloc(int* a, int* d, int* c, int* y) : allocs(a), deallocs(d), constructs(c), destroys(y) {}
  template <class U> constexpr CAlloc(const CAlloc<U>& o) : allocs(o.allocs), deallocs(o.deallocs), constructs(o.constructs), destroys(o.destroys) {}
  constexpr T* allocate(std::size_t n) { ++*allocs; return std::allocator<T>{}.allocate(n); }
  constexpr void deallocate(T* p, std::size_t n) { ++*deallocs; std::allocator<T>{}.deallocate(p, n); }
  template <class U, class... A> constexpr void construct(U* p, A&&... a) { ++*constructs; std::construct_at(p, std::forward<A>(a)...); }
  template <class U> constexpr void destroy(U* p) { ++*destroys; std::destroy_at(p); }
  template <class U> constexpr bool operator==(const CAlloc<U>&) const { return true; }
};

#define CASE(name, ...)                                                                                                \
  constexpr bool name() { return __VA_ARGS__; }                                                                        \
  static_assert(name(), #name);

CASE(ctor_alloc_used, []{ int a=0,d=0,c=0,y=0; { std::shared_ptr<int> p(new int(1), Del{}, CAlloc<int>(&a,&d,&c,&y)); if (a != 1) return false; } return a == 1 && d == 1; }())
CASE(ctor_nullptr_alloc_used, []{ int a=0,d=0,c=0,y=0; { std::shared_ptr<int> p(nullptr, Del{}, CAlloc<int>(&a,&d,&c,&y)); } return a == 1 && d == 1; }())
CASE(alloc_shared_used, []{ int a=0,d=0,c=0,y=0; { auto p = std::allocate_shared<int>(CAlloc<int>(&a,&d,&c,&y), 5); if (*p != 5) return false; } return a >= 1 && a == d && c == 1 && y == 1; }())
CASE(alloc_shared_arr_used, []{ int a=0,d=0,c=0,y=0; { auto p = std::allocate_shared<int[]>(CAlloc<int>(&a,&d,&c,&y), 3); } return a >= 1 && a == d && c == 3 && y == 3; }())
CASE(alloc_shared_md_used, []{ int a=0,d=0,c=0,y=0; { auto p = std::allocate_shared<int[2][3]>(CAlloc<int>(&a,&d,&c,&y)); if (p[1][2] != 0) return false; } return a >= 1 && a == d && c == 6 && y == 6; }())
CASE(overwrite_no_construct, []{ int a=0,d=0,c=0,y=0; { auto p = std::allocate_shared_for_overwrite<int>(CAlloc<int>(&a,&d,&c,&y)); *p = 3; } return a >= 1 && a == d && c == 0 && y == 0; }())
CASE(overwrite_arr_no_construct, []{ int a=0,d=0,c=0,y=0; { auto p = std::allocate_shared_for_overwrite<int[]>(CAlloc<int>(&a,&d,&c,&y), 3); p[0] = 1; } return a >= 1 && a == d && c == 0 && y == 0; }())
CASE(overwrite_bounded_no_construct, []{ int a=0,d=0,c=0,y=0; { auto p = std::allocate_shared_for_overwrite<int[3]>(CAlloc<int>(&a,&d,&c,&y)); p[0] = 1; } return a >= 1 && a == d && c == 0 && y == 0; }())
CASE(overwrite_md_no_construct, []{ int a=0,d=0,c=0,y=0; { auto p = std::allocate_shared_for_overwrite<int[2][3]>(CAlloc<int>(&a,&d,&c,&y)); p[1][2] = 1; } return a >= 1 && a == d && c == 0 && y == 0; }())

int main(int, char**) {
  assert(ctor_alloc_used());
  assert(ctor_nullptr_alloc_used());
  assert(alloc_shared_used());
  assert(alloc_shared_arr_used());
  assert(alloc_shared_md_used());
  assert(overwrite_no_construct());
  assert(overwrite_arr_no_construct());
  assert(overwrite_bounded_no_construct());
  assert(overwrite_md_no_construct());
  return 0;
}
