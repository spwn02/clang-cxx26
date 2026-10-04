//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

#include <execution>
#include <coroutine>
#include <cassert>
#include <memory>
#include <type_traits>
namespace ex = std::execution;

// [execution.syn]: "using type = remove_cvref_t<E>; type error;"
static_assert(std::same_as<ex::with_error<const int&>::type, int>);
static_assert(std::same_as<decltype(ex::with_error<const int&>::error), int>);
// [task.class]: "template<class T = void, class Environment = env<>>"
// "start_scheduler_type is Environment::start_scheduler_type ... task_scheduler otherwise."
static_assert(std::same_as<ex::task<int>, ex::task<int, ex::env<>>>);
static_assert(std::same_as<ex::task<>::start_scheduler_type, ex::task_scheduler>);

struct counters { int allocations = 0; int deallocations = 0; int size = 0; int alignment = 0; };
template <class T> struct allocator {
  using value_type = T;
  counters* counts = nullptr;
  allocator() = default;
  explicit allocator(counters* c) : counts(c) {}
  template <class U> allocator(const allocator<U>& other) : counts(other.counts) {}
  T* allocate(std::size_t n) {
    ++counts->allocations; counts->size = sizeof(T); counts->alignment = alignof(T);
    return std::allocator<T>{}.allocate(n);
  }
  void deallocate(T* p, std::size_t n) { ++counts->deallocations; std::allocator<T>{}.deallocate(p, n); }
  template <class U> bool operator==(const allocator<U>& other) const noexcept { return counts == other.counts; }
};
int own_count = 0, env_count = 0;
struct own_env {
  template <class E> explicit own_env(const E&) { ++own_count; }
};
struct environment {
  using start_scheduler_type = ex::inline_scheduler;
  using allocator_type = allocator<std::byte>;
  template <class> using env_type = own_env;
  explicit environment(own_env&) { ++env_count; }
};
struct errors_environment : environment {
  using error_types = ex::completion_signatures<ex::set_error_t(int)>;
  using environment::environment;
};
static_assert(std::same_as<ex::task<int, environment>::start_scheduler_type, ex::inline_scheduler>);
// [task.class]: "error_types is Environment::error_types ... completion_signatures<set_error_t(exception_ptr)> otherwise."
static_assert(std::same_as<ex::task<int, errors_environment>::error_types, errors_environment::error_types>);
// [task.members]: "template<class Self, class... Env> static consteval auto get_completion_signatures();"
using sigs = ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(int), ex::set_stopped_t()>;
static_assert(std::same_as<decltype(ex::task<int, errors_environment>::get_completion_signatures<ex::task<int, errors_environment>>()), sigs>);
static_assert(std::same_as<decltype(ex::task<int, errors_environment>::get_completion_signatures<ex::task<int, errors_environment>, ex::env<>, ex::env<>>()), sigs>);

// Each coroutine waits for the run_loop: completion cannot occur during start().
ex::task<int, environment> value(std::allocator_arg_t, allocator<std::byte>, ex::run_loop& loop) {
  co_await ex::schedule(loop.get_scheduler()); co_return 42;
}
ex::task<int, environment> error(std::allocator_arg_t, allocator<std::byte>, ex::run_loop& loop) {
  co_await ex::schedule(loop.get_scheduler()); throw 42; co_return 0;
}
ex::task<int, environment> stopped(std::allocator_arg_t, allocator<std::byte>, ex::run_loop& loop) {
  co_await ex::schedule(loop.get_scheduler()); co_await ex::just_stopped(); co_return 0;
}
ex::task<int, errors_environment> custom_error(std::allocator_arg_t, allocator<std::byte>, ex::run_loop& loop) {
  co_await ex::schedule(loop.get_scheduler()); co_yield ex::with_error{42}; co_return 0;
}
ex::task<int&, environment> reference(std::allocator_arg_t, allocator<std::byte>, ex::run_loop& loop, int& v) {
  co_await ex::schedule(loop.get_scheduler()); co_return v;
}
ex::task<int, environment> stop_token(std::allocator_arg_t, allocator<std::byte>, ex::run_loop&) {
  auto token = co_await ex::read_env(std::get_stop_token);
  co_return token.stop_requested();
}
struct receiver {
  using receiver_concept = ex::receiver_tag;
  counters* counts;
  int* result;
  std::inplace_stop_token token;
  int** ref = nullptr;
  void set_value(int& v) && noexcept {
    assert(counts->deallocations == 1); *result = v; if (ref) *ref = &v;
  }
  void set_value(int&& v) && noexcept { assert(counts->deallocations == 1); *result = v; }
  void set_error(std::exception_ptr) && noexcept { assert(counts->deallocations == 1); *result = -1; }
  void set_error(int v) && noexcept { assert(counts->deallocations == 1); *result = -v; }
  void set_stopped() && noexcept { assert(counts->deallocations == 1); *result = -2; }
  auto get_env() const noexcept {
    return ex::env{ex::prop{std::get_stop_token, token}, ex::prop{std::get_allocator, allocator<std::byte>(counts)},
                   ex::prop{ex::get_start_scheduler, ex::inline_scheduler{}}};
  }
};
int main(int, char**) {
  for (int kind = 0; kind != 6; ++kind) {
    ex::run_loop loop;
    counters c;
    int result = 0, v = 77; int* ref = nullptr;
    // A requested stop is also observed by the run_loop operation a task schedules on, so only the
    // stop-token case runs with a stop request and without scheduling.
    std::inplace_stop_source source; if (kind == 5) source.request_stop();
    receiver r{&c, &result, source.get_token(), &ref};
    auto run = [&](auto t) {
      // [task.state]: "own-env with own-env-t(get_env(rcvr)) ... environment with Environment(own-env)"
      int before_own = own_count, before_env = env_count;
      auto op = ex::connect(std::move(t), r);
      assert(own_count == before_own + 1 && env_count == before_env + 1);
      // [task.promise]: "rebind_alloc<U> ... whose size and alignment are both STDCPP_DEFAULT_NEW_ALIGNMENT"
      assert(c.size == __STDCPP_DEFAULT_NEW_ALIGNMENT__ && c.alignment == __STDCPP_DEFAULT_NEW_ALIGNMENT__);
      ex::start(op); assert(result == 0 && c.deallocations == 0);
      loop.finish(); loop.run();
      // [task.promise]: "The asynchronous completion first destroys the coroutine frame ... and then invokes"
      // [task.promise], yield_value: "first destroying the coroutine frame ... and then invoking set_error"
      // [task.promise], unhandled_stopped: "first destroying the coroutine frame ... and then invoking set_stopped"
      assert(c.deallocations == 1);
    };
    if (kind == 0) { run(value(std::allocator_arg, allocator<std::byte>(&c), loop)); assert(result == 42); }
    if (kind == 1) { run(error(std::allocator_arg, allocator<std::byte>(&c), loop)); assert(result == -1); }
    if (kind == 2) { run(stopped(std::allocator_arg, allocator<std::byte>(&c), loop)); assert(result == -2); }
    // [task.promise]: "std::move(err.error) is convertible to exactly one of the set_error_t argument types of error_types."
    if (kind == 3) { run(custom_error(std::allocator_arg, allocator<std::byte>(&c), loop)); assert(result == -42); }
    // [task.class]: "T is void, a reference type, or a cv-unqualified non-array object type"
    if (kind == 4) { run(reference(std::allocator_arg, allocator<std::byte>(&c), loop, v)); assert(ref == &v); }
    // [task.state]: "If same_as<...> is true, returns get_stop_token(get_env(rcvr))."
    if (kind == 5) {
      auto op = ex::connect(stop_token(std::allocator_arg, allocator<std::byte>(&c), loop), r);
      ex::start(op);
      assert(result == 1 && c.deallocations == 1);
    }
    assert(c.deallocations == 1);
  }
  return 0;
}
