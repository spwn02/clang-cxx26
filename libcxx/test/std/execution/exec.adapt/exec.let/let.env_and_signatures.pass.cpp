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

// [exec.let]: the sender the function returns is connected through let-env joined in front of FWD-ENV(env), where
// let-env is the SCHED-ENV of the completion scheduler of the child for the intercepted completion if it has one, a
// MAKE-ENV of its completion domain if not, and env<>{} otherwise. The completion signatures have an exception completion
// unless decay-copying the datums, calling the function and connecting the sender it returns are all noexcept, and the
// function must be invocable with the datums and return a sender (check-types).

#include <cassert>
#include <concepts>
#include <exception>
#include <execution>
#include <type_traits>

namespace ex = std::execution;

struct Sch {
  using scheduler_concept = ex::scheduler_tag;
  struct Sender {
    using sender_concept = ex::sender_tag;
    ex::env<> get_env() const noexcept { return {}; }
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t()>{};
    }
    template <class Rcvr>
    struct Op {
      using operation_state_concept = ex::operation_state_tag;
      Rcvr rcvr;
      void start() & noexcept { ex::set_value(std::move(rcvr)); }
    };
    template <class Rcvr>
    Op<std::remove_cvref_t<Rcvr>> connect(Rcvr&& r) const noexcept {
      return {std::forward<Rcvr>(r)};
    }
  };
  int id;
  Sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(Sch, Sch) = default;
};
struct Domain {
  int tag = 0;
};

// a child that completes on Sch{7}, in Domain
struct SchedAttrs {
  Sch query(ex::get_completion_scheduler_t<ex::set_value_t>) const noexcept { return {7}; }
};
struct DomainAttrs {
  Domain query(ex::get_completion_domain_t<ex::set_value_t>) const noexcept { return {}; }
};
template <class Attrs>
struct Child {
  using sender_concept = ex::sender_tag;
  Attrs get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }
  template <class Rcvr>
  struct Op {
    using operation_state_concept = ex::operation_state_tag;
    Rcvr rcvr;
    void start() & noexcept { ex::set_value(std::move(rcvr), 1); }
  };
  template <class Rcvr>
  Op<std::remove_cvref_t<Rcvr>> connect(Rcvr&& r) const noexcept {
    return {std::forward<Rcvr>(r)};
  }
};

template <class Value>
struct ValueRcvr {
  using receiver_concept = ex::receiver_tag;
  Value* out;
  void set_value(Value v) && noexcept { *out = v; }
  void set_error(std::exception_ptr) && noexcept { assert(false); }
  void set_stopped() && noexcept { assert(false); }
  ex::env<> get_env() const noexcept { return {}; }
};

template <class Sndr, class Value>
void run(Sndr&& sndr, Value& out) {
  auto op = ex::connect(std::forward<Sndr>(sndr), ValueRcvr<Value>{&out});
  ex::start(op);
}

void test_let_env() {
  // the continuation starts where the child completed: its start scheduler is the completion scheduler of the child
  {
    Sch got{0};
    run(ex::let_value(Child<SchedAttrs>{}, [](int) noexcept { return ex::read_env(ex::get_start_scheduler); }), got);
    assert(got.id == 7);
  }
  // only a completion domain: the continuation has it as its domain
  {
    Domain got{1};
    run(ex::let_value(Child<DomainAttrs>{}, [](int) noexcept { return ex::read_env(ex::get_domain); }), got);
    assert(got.tag == 0); // a default constructed Domain: the one of the child, not default_domain
    static_assert(std::same_as<decltype(ex::get_domain(ex::__let_env<ex::set_value_t>(Child<DomainAttrs>{}, ex::env<>{}))), Domain>);
  }
  // nothing known about the child: the environment of the continuation is the one of the receiver
  static_assert(std::same_as<ex::__let_env_t<ex::set_value_t, decltype(ex::just(1)), ex::env<>>, ex::env<>>);
  static_assert(std::same_as<decltype(ex::get_start_scheduler(ex::__let_env<ex::set_value_t>(Child<SchedAttrs>{}, ex::env<>{}))), Sch>);
}

// the sender the function returns, with a connect that may throw
struct ThrowingConnect {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>{};
  }
  template <class Rcvr>
  struct Op {
    using operation_state_concept = ex::operation_state_tag;
    Rcvr rcvr;
    void start() & noexcept { ex::set_value(std::move(rcvr)); }
  };
  template <class Rcvr>
  Op<std::remove_cvref_t<Rcvr>> connect(Rcvr&& r) const {
    return {std::forward<Rcvr>(r)};
  }
};

void test_signatures() {
  using V = ex::set_value_t;
  using E = ex::set_error_t;
  // connecting the sender returned by the function can throw: an exception completion
  auto throwing = ex::let_value(ex::just(1), [](int) noexcept { return ThrowingConnect{}; });
  static_assert(std::same_as<ex::completion_signatures_of_t<decltype(throwing)>,
                             ex::completion_signatures<V(), E(std::exception_ptr)>>);
  // everything noexcept: none
  auto nothrow = ex::let_value(ex::just(1), [](int x) noexcept { return ex::just(x); });
  static_assert(std::same_as<ex::completion_signatures_of_t<decltype(nothrow)>, ex::completion_signatures<V(int)>>);
  // a function that may throw
  auto throwing_fn = ex::let_value(ex::just(1), [](int x) { return ex::just(x); });
  static_assert(std::same_as<ex::completion_signatures_of_t<decltype(throwing_fn)>,
                             ex::completion_signatures<V(int), E(std::exception_ptr)>>);
}

// check-types: not invocable with the datums, not returning a sender
inline constexpr auto takes_string = [](const char*) noexcept { return ex::just(); };
inline constexpr auto returns_int = [](int x) noexcept { return x; };
void test_check_types() {
  static_assert(ex::sender_in<decltype(ex::let_value(ex::just(1), [](int) noexcept { return ex::just(); }))>);
  static_assert(!ex::sender_in<decltype(ex::let_value(ex::just(1), takes_string))>);
  static_assert(!ex::sender_in<decltype(ex::let_value(ex::just(1), returns_int))>);
  static_assert(ex::sender_in<decltype(ex::let_value(ex::just(1), [](int) noexcept { return ex::just(); })), ex::env<>>);
  static_assert(!ex::sender_in<decltype(ex::let_value(ex::just(1), takes_string)), ex::env<>>);
}

int main(int, char**) {
  test_let_env();
  test_signatures();
  test_check_types();
  return 0;
}
