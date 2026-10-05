//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <memory>

// template<class T>
//   constexpr void start_lifetime(T& r) noexcept;     // P3726R2, [obj.lifetime]

#include <cassert>
#include <memory>
#include <new>
#include <type_traits>

#if !defined(__cpp_lib_start_lifetime) || __cpp_lib_start_lifetime != 202603L
#  error "__cpp_lib_start_lifetime has the wrong value"
#endif

struct Agg {
  int a;
  int b;
};

struct WithInit {
  int a = 5;
  int b = 6;
};

union U {
  int i;
  Agg agg;
};

template <class T>
concept can_start = requires(T& t) { std::start_lifetime(t); };

static_assert(can_start<Agg>);
static_assert(noexcept(std::start_lifetime(std::declval<Agg&>())));
static_assert(std::is_same_v<decltype(std::start_lifetime(std::declval<Agg&>())), void>);

constexpr bool test() {
  // already within its lifetime: no effect
  {
    Agg agg{1, 2};
    std::start_lifetime(agg);
    if (agg.a != 1 || agg.b != 2)
      return false;
  }
  // storage that is not an object yet, no initialization (not even of default member initializers) takes place
  {
    std::allocator<WithInit> alloc;
    WithInit* p = alloc.allocate(1);
    std::start_lifetime(*p);
    std::construct_at(&p->a, 1);
    std::construct_at(&p->b, 2);
    bool ok = p->a == 1 && p->b == 2;
    alloc.deallocate(p, 1);
    if (!ok)
      return false;
  }
  // arrays
  {
    std::allocator<int[4]> alloc;
    int(*p)[4] = alloc.allocate(1);
    std::start_lifetime(*p);
    std::construct_at(&(*p)[2], 9);
    bool ok = (*p)[2] == 9;
    alloc.deallocate(p, 1);
    if (!ok)
      return false;
  }
  // a member of a union becomes the active member
  {
    U u{.i = 1};
    std::start_lifetime(u.agg);
    std::construct_at(&u.agg.a, 3);
    std::construct_at(&u.agg.b, 4);
    if (u.agg.a != 3 || u.agg.b != 4)
      return false;
  }
  return true;
}

int main(int, char**) {
  assert(test());
  static_assert(test());

  // at run time: nothing happens
  alignas(Agg) unsigned char storage[sizeof(Agg)] = {};
  auto* agg = std::launder(reinterpret_cast<Agg*>(storage));
  std::start_lifetime(*agg);
  agg->a = 1;
  assert(agg->a == 1);
  return 0;
}
