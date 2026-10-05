//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [memory.syn], [specialized.algorithms]: the overloads of uninitialized_default_construct(_n), uninitialized_value_construct(_n),
// uninitialized_copy(_n), uninitialized_move(_n), uninitialized_fill(_n), destroy(_n) taking an execution policy, in namespace std
// (C++17) and, for random access iterators and ranges, in namespace std::ranges (P3179R9, C++26).

#include "test_macros.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <execution>
#include <list>
#include <memory>
#include <new>
#include <utility>
#include <vector>

#if TEST_STD_VER >= 20
#  include <span>
#endif
#if TEST_STD_VER >= 26
#  include <ranges>
#endif

struct Tracked {
  static inline int live = 0;
  int value;
  Tracked() : value(7) { ++live; }
  Tracked(int v) : value(v) { ++live; }
  Tracked(const Tracked& other) : value(other.value) { ++live; }
  Tracked(Tracked&& other) noexcept : value(other.value) {
    other.value = -1;
    ++live;
  }
  ~Tracked() { --live; }
};

struct Plain {
  int value;
};

template <class T, std::size_t N>
struct Storage {
  alignas(T) unsigned char bytes[sizeof(T) * N];
  Storage() { std::memset(bytes, 0xAB, sizeof(bytes)); }
  T* begin() { return std::launder(reinterpret_cast<T*>(bytes)); }
  T* end() { return begin() + N; }
};

template <class Policy>
void test_std(Policy&& policy) {
  // default and value construction
  {
    Storage<Tracked, 4> s;
    std::uninitialized_default_construct(policy, s.begin(), s.end());
    assert(Tracked::live == 4);
    for (auto* p = s.begin(); p != s.end(); ++p)
      assert(p->value == 7);
    std::destroy(policy, s.begin(), s.end());
    assert(Tracked::live == 0);
  }
  {
    Storage<Plain, 4> s;
    auto* r = std::uninitialized_default_construct_n(policy, s.begin(), 4);
    assert(r == s.end());
    unsigned char marker[sizeof(Plain)];
    std::memset(marker, 0xAB, sizeof(marker));
    assert(std::memcmp(s.begin(), marker, sizeof(Plain)) == 0); // default-initialization leaves trivial types alone
    std::destroy_n(policy, s.begin(), 4);
  }
  {
    Storage<Plain, 4> s;
    std::uninitialized_value_construct(policy, s.begin(), s.end());
    for (auto* p = s.begin(); p != s.end(); ++p)
      assert(p->value == 0);
    Storage<Plain, 4> t;
    auto* r = std::uninitialized_value_construct_n(policy, t.begin(), 4);
    assert(r == t.end());
    for (auto* p = t.begin(); p != t.end(); ++p)
      assert(p->value == 0);
  }
  // copy and move; the source may be a forward range
  {
    std::list<int> src{1, 2, 3, 4};
    Storage<Tracked, 4> s;
    auto* r = std::uninitialized_copy(policy, src.begin(), src.end(), s.begin());
    assert(r == s.end() && Tracked::live == 4);
    int expected = 1;
    for (auto* p = s.begin(); p != s.end(); ++p)
      assert(p->value == expected++);
    std::destroy(policy, s.begin(), s.end());
    assert(Tracked::live == 0);

    Storage<Tracked, 4> t;
    auto* r2 = std::uninitialized_copy_n(policy, src.begin(), 3, t.begin());
    assert(r2 == t.begin() + 3 && Tracked::live == 3);
    assert(t.begin()[2].value == 3);
    std::destroy_n(policy, t.begin(), 3);
    assert(Tracked::live == 0);
  }
  {
    std::vector<Tracked> src;
    src.reserve(4);
    for (int i = 0; i != 4; ++i)
      src.emplace_back(i + 10);
    Storage<Tracked, 4> s;
    auto* r = std::uninitialized_move(policy, src.begin(), src.end(), s.begin());
    assert(r == s.end());
    for (int i = 0; i != 4; ++i) {
      assert(s.begin()[i].value == i + 10);
      assert(src[i].value == -1); // moved from
    }
    std::destroy(policy, s.begin(), s.end());

    Storage<Tracked, 4> t;
    auto result = std::uninitialized_move_n(policy, src.begin(), 2, t.begin());
    static_assert(std::is_same_v<decltype(result), std::pair<std::vector<Tracked>::iterator, Tracked*>>);
    assert(result.first == src.begin() + 2 && result.second == t.begin() + 2);
    std::destroy_n(policy, t.begin(), 2);
  }
  // fill
  {
    Storage<Tracked, 5> s;
    std::uninitialized_fill(policy, s.begin(), s.end(), Tracked(9));
    for (auto* p = s.begin(); p != s.end(); ++p)
      assert(p->value == 9);
    std::destroy(policy, s.begin(), s.end());
    Storage<Tracked, 5> t;
    auto* r = std::uninitialized_fill_n(policy, t.begin(), 3, Tracked(4));
    assert(r == t.begin() + 3 && t.begin()[2].value == 4);
    std::destroy_n(policy, t.begin(), 3);
  }
  assert(Tracked::live == 0);
}

#if TEST_STD_VER >= 26
template <class Policy>
void test_ranges(Policy&& policy) {
  namespace rg = std::ranges;
  // default and value construction (iterator/sentinel, range, counted)
  {
    Storage<Tracked, 4> s;
    assert(rg::uninitialized_default_construct(policy, s.begin(), s.end()) == s.end());
    assert(Tracked::live == 4);
    assert(rg::destroy(policy, s.begin(), s.end()) == s.end());
    assert(Tracked::live == 0);
    std::span<Tracked> span(s.begin(), 4);
    assert(rg::uninitialized_default_construct(policy, span) == span.end());
    assert(rg::destroy(policy, span) == span.end());
    assert(rg::uninitialized_default_construct_n(policy, s.begin(), 3) == s.begin() + 3);
    assert(rg::destroy_n(policy, s.begin(), 3) == s.begin() + 3);
    assert(Tracked::live == 0);
  }
  {
    Storage<Plain, 4> s;
    std::span<Plain> span(s.begin(), 4);
    assert(rg::uninitialized_value_construct(policy, s.begin(), s.end()) == s.end());
    for (auto& e : span)
      assert(e.value == 0);
    Storage<Plain, 4> t;
    std::span<Plain> tspan(t.begin(), 4);
    assert(rg::uninitialized_value_construct(policy, tspan) == tspan.end());
    Storage<Plain, 4> u;
    assert(rg::uninitialized_value_construct_n(policy, u.begin(), 4) == u.end());
    for (auto* p = u.begin(); p != u.end(); ++p)
      assert(p->value == 0);
  }
  // copy: the output range bounds the number of elements
  {
    std::vector<int> in{1, 2, 3, 4, 5};
    Storage<Tracked, 3> s;
    auto r = rg::uninitialized_copy(policy, in.begin(), in.end(), s.begin(), s.end());
    static_assert(std::is_same_v<decltype(r), rg::uninitialized_copy_result<std::vector<int>::iterator, Tracked*>>);
    assert(r.in == in.begin() + 3 && r.out == s.end() && Tracked::live == 3);
    assert(s.begin()[2].value == 3);
    std::destroy(policy, s.begin(), s.end());

    Storage<Tracked, 8> t; // the input range bounds it
    std::span<Tracked> tspan(t.begin(), 8);
    auto r2 = rg::uninitialized_copy(policy, in, tspan);
    assert(r2.in == in.end() && r2.out == tspan.begin() + 5 && Tracked::live == 5);
    std::destroy_n(policy, t.begin(), 5);

    Storage<Tracked, 4> u;
    auto r3 = rg::uninitialized_copy_n(policy, in.begin(), 4, u.begin(), u.begin() + 2);
    static_assert(std::is_same_v<decltype(r3), rg::uninitialized_copy_n_result<std::vector<int>::iterator, Tracked*>>);
    assert(r3.in == in.begin() + 2 && r3.out == u.begin() + 2 && Tracked::live == 2);
    std::destroy_n(policy, u.begin(), 2);
    assert(Tracked::live == 0);
  }
  // move
  {
    std::vector<Tracked> in;
    in.reserve(5);
    for (int i = 0; i != 5; ++i)
      in.emplace_back(i + 20);
    Storage<Tracked, 3> s;
    auto r = rg::uninitialized_move(policy, in.begin(), in.end(), s.begin(), s.end());
    assert(r.in == in.begin() + 3 && r.out == s.end());
    assert(s.begin()[0].value == 20 && in[0].value == -1 && in[3].value == 23);
    std::destroy(policy, s.begin(), s.end());

    Storage<Tracked, 8> t;
    std::span<Tracked> tspan(t.begin(), 8);
    auto r2 = rg::uninitialized_move(policy, in, tspan);
    assert(r2.in == in.end() && r2.out == tspan.begin() + 5);
    std::destroy_n(policy, t.begin(), 5);

    for (int i = 0; i != 5; ++i)
      in[i].value = i + 30;
    Storage<Tracked, 4> u;
    auto r3 = rg::uninitialized_move_n(policy, in.begin(), 4, u.begin(), u.begin() + 3);
    assert(r3.in == in.begin() + 3 && r3.out == u.begin() + 3);
    assert(u.begin()[2].value == 32);
    std::destroy_n(policy, u.begin(), 3);
    assert(Tracked::live == 5); // only the vector's elements remain
  }
  // fill
  {
    Storage<Tracked, 5> s;
    assert(rg::uninitialized_fill(policy, s.begin(), s.end(), Tracked(6)) == s.end());
    std::span<Tracked> span(s.begin(), 5);
    for (auto& e : span)
      assert(e.value == 6);
    assert(rg::destroy(policy, span) == span.end());
    assert(rg::uninitialized_fill(policy, span, Tracked(8)) == span.end());
    assert(span[4].value == 8);
    rg::destroy(policy, span);
    assert(rg::uninitialized_fill_n(policy, s.begin(), 2, Tracked(5)) == s.begin() + 2);
    assert(s.begin()[1].value == 5);
    rg::destroy_n(policy, s.begin(), 2);
    // P2248R8: the value type is the default of the value argument's type
    Storage<Plain, 3> p;
    rg::uninitialized_fill(policy, p.begin(), p.end(), {});
    for (auto* e = p.begin(); e != p.end(); ++e)
      assert(e->value == 0);
  }
}
#endif

int main(int, char**) {
  test_std(std::execution::seq);
  test_std(std::execution::par);
  test_std(std::execution::par_unseq);
#if TEST_STD_VER >= 20
  test_std(std::execution::unseq);
#endif
#if TEST_STD_VER >= 26
  test_ranges(std::execution::seq);
  test_ranges(std::execution::par);
  test_ranges(std::execution::par_unseq);
  test_ranges(std::execution::unseq);
#endif
  return 0;
}
