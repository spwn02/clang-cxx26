//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// <execution>

// [exec.adapt.obj]: the closure of a partial application (adaptor(args...)) is a perfect forwarding call wrapper: calling
// an lvalue closure passes the bound arguments as lvalues, so the same closure can be used any number of times; and
// associate(token) is such a closure ([exec.associate]).

#include <cassert>
#include <concepts>
#include <execution>
#include <memory>
#include <thread>
#include <tuple>

namespace ex = std::execution;

// a function object that cannot work after it has been moved from
struct Adder {
  std::shared_ptr<int> p = std::make_shared<int>(4);
  void operator()(int, int& x) const noexcept {
    assert(p);
    x += *p;
  }
  void operator()(int, int, int& x) const noexcept {
    assert(p);
    x += *p;
  }
};

template <class Closure>
void use_twice(Closure& closure) {
  auto a = std::this_thread::sync_wait(ex::just(1) | closure);
  auto b = std::this_thread::sync_wait(ex::just(1) | closure);
  assert(a && b);
  assert(std::get<0>(*a) == 5 && std::get<0>(*b) == 5);
}

// Only the adaptors whose single argument is a sender are closures themselves: affine, into_variant,
// stopped_as_optional. A closure composed with bulk, bulk_chunked, bulk_unchunked or associate (they take more arguments)
// is not a closure.
template <class Closure, class Adaptor>
concept can_pipe_closure_into = requires(Closure c, Adaptor a) { c | a; };
inline constexpr auto then_closure = ex::then([](int x) noexcept { return x; });
static_assert(can_pipe_closure_into<decltype(then_closure), ex::affine_t>);
static_assert(can_pipe_closure_into<decltype(then_closure), ex::into_variant_t>);
static_assert(!can_pipe_closure_into<decltype(then_closure), ex::bulk_t>);
static_assert(!can_pipe_closure_into<decltype(then_closure), ex::bulk_chunked_t>);
static_assert(!can_pipe_closure_into<decltype(then_closure), ex::bulk_unchunked_t>);
static_assert(!can_pipe_closure_into<decltype(then_closure), ex::associate_t>);

int main(int, char**) {
  // an lvalue closure can be reused: nothing it holds is consumed by a call
  {
    auto unchunked = ex::bulk_unchunked(ex::seq, 1, Adder{});
    use_twice(unchunked);
    auto bulk = ex::bulk(ex::seq, 1, Adder{});
    use_twice(bulk);
    auto chunked = ex::bulk_chunked(ex::seq, 1, [p = std::make_shared<int>(4)](int, int, int& x) noexcept {
      assert(p);
      x += *p;
    });
    use_twice(chunked);
  }
  // an rvalue closure may move what it holds
  {
    auto r = std::this_thread::sync_wait(ex::just(1) | ex::bulk_unchunked(ex::seq, 1, Adder{}));
    assert(r && std::get<0>(*r) == 5);
  }
  // the closure of the other adaptors stays reusable as well
  {
    auto then = ex::then([p = std::make_shared<int>(2)](int x) noexcept {
      assert(p);
      return x + *p;
    });
    auto a = std::this_thread::sync_wait(ex::just(1) | then);
    auto b = std::this_thread::sync_wait(ex::just(1) | then);
    assert(a && b && std::get<0>(*a) == 3 && std::get<0>(*b) == 3);
  }
  // associate(token) is a pipeable closure
  {
    ex::counting_scope scope;
    auto closure = ex::associate(scope.get_token());
    auto a = std::this_thread::sync_wait(ex::just(1) | closure);
    auto b = std::this_thread::sync_wait(ex::just(2) | ex::associate(scope.get_token()));
    auto c = std::this_thread::sync_wait(ex::associate(ex::just(3), scope.get_token()));
    assert(a && b && c);
    assert(std::get<0>(*a) == 1 && std::get<0>(*b) == 2 && std::get<0>(*c) == 3);
    std::this_thread::sync_wait(scope.join());
  }
  return 0;
}
