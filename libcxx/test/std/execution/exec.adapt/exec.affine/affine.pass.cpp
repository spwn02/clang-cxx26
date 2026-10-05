//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// <execution>

// [exec.affine]: affine adapts a sender into one that completes on the start scheduler of the receiver's environment.
//   affine(sndr) is make-sender(affine, env<>(), sndr); its transformation is child.affine() if that is well-formed and
//   continues_on(child, UNSTOPPABLE-SCHEDULER(get_start_scheduler(env))) otherwise. If the start scheduler is missing or
//   is not an infallible-scheduler<Env> the completion signatures are an exception.

#include <cassert>
#include <concepts>
#include <execution>
#include <exception>
#include <stdexcept>
#include <stop_token>
#include <utility>

namespace ex = std::execution;

template <class Sch>
using unstoppable_sch = ex::__unstoppable_scheduler<Sch>;

// ---------------------------------------------------------------------------------------------------------------
// a scheduler that counts how many schedule operations were started on it
struct Counted {
  using scheduler_concept = ex::scheduler_tag;
  struct Sender {
    using sender_concept = ex::sender_tag;
    int* counter;
    ex::env<> get_env() const noexcept { return {}; }
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t()>{};
    }
    template <class Rcvr>
    struct Op {
      using operation_state_concept = ex::operation_state_tag;
      int* counter;
      Rcvr rcvr;
      void start() & noexcept {
        ++*counter;
        ex::set_value(std::move(rcvr));
      }
    };
    template <class Rcvr>
    Op<std::remove_cvref_t<Rcvr>> connect(Rcvr&& r) const noexcept {
      return {counter, std::forward<Rcvr>(r)};
    }
  };
  int* counter;
  Sender schedule() const noexcept { return {counter}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(Counted, Counted) = default;
};
static_assert(ex::scheduler<Counted>);

struct Env {
  Counted sch;
  Counted query(ex::get_start_scheduler_t) const noexcept { return sch; }
};

template <class Env2>
struct ValueRcvr {
  using receiver_concept = ex::receiver_tag;
  int* out;
  Env2 env;
  void set_value(int v) && noexcept { *out = v; }
  void set_error(std::exception_ptr) && noexcept { assert(false); }
  void set_stopped() && noexcept { assert(false); }
  Env2 get_env() const noexcept { return env; }
};

// a child sender without an affine member: it completes wherever
struct Child {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }
  template <class Rcvr>
  struct Op {
    using operation_state_concept = ex::operation_state_tag;
    Rcvr rcvr;
    void start() & noexcept { ex::set_value(std::move(rcvr), 7); }
  };
  template <class Rcvr>
  Op<std::remove_cvref_t<Rcvr>> connect(Rcvr&& r) const noexcept {
    return {std::forward<Rcvr>(r)};
  }
};

// a child sender with an affine member returns it from the transformation
struct AffineChild : Child {
  struct Result : Child {
    int marker = 99;
  };
  Result affine() const { return {}; }
};

void test_affine_on_the_start_scheduler() {
  int count = 0;
  int out   = 0;
  // the schedule operation of the start scheduler is what the sender completes through
  auto op = ex::connect(ex::affine(Child{}), ValueRcvr<Env>{&out, Env{Counted{&count}}});
  ex::start(op);
  assert(out == 7);
  assert(count == 1);
}

void test_member() {
  // child.affine() is used when it exists
  using Transformed = decltype(ex::transform_sender(ex::affine(AffineChild{}), Env{}));
  static_assert(std::same_as<Transformed, AffineChild::Result>);
  // the sender of just/just_error/just_stopped and read_env are their own affine
  using J = decltype(ex::just(1));
  static_assert(std::same_as<decltype(ex::transform_sender(ex::affine(ex::just(1)), Env{})), J>);
  int count = 0;
  int out   = 0;
  auto op   = ex::connect(ex::affine(ex::just(5)), ValueRcvr<Env>{&out, Env{Counted{&count}}});
  ex::start(op);
  assert(out == 5);
  assert(count == 0); // nothing was scheduled
}

void test_unstoppable_scheduler() {
  unstoppable_sch<Counted> sch{Counted{nullptr}};
  assert(ex::get_forward_progress_guarantee(sch) == ex::forward_progress_guarantee::weakly_parallel);
  assert(sch == unstoppable_sch<Counted>{Counted{nullptr}});
  int other = 0;
  assert(!(sch == unstoppable_sch<Counted>{Counted{&other}}));
}

void test_pipe() {
  int count = 0;
  int out   = 0;
  auto op   = ex::connect(Child{} | ex::affine, ValueRcvr<Env>{&out, Env{Counted{&count}}});
  ex::start(op);
  assert(out == 7 && count == 1);
}

// ---------------------------------------------------------------------------------------------------------------
// the shape of the sender
static_assert(std::is_class_v<ex::affine_t>);
static_assert(std::same_as<ex::tag_of_t<decltype(ex::affine(Child{}))>, ex::affine_t>);
static_assert(ex::sender<decltype(ex::affine(Child{}))>);
template <class T>
concept can_affine = requires(T&& t) { ex::affine(std::forward<T>(t)); };
static_assert(can_affine<Child>);
static_assert(!can_affine<int>);
static_assert(std::derived_from<ex::affine_t, ex::sender_adaptor_closure<ex::affine_t>>);

// ---------------------------------------------------------------------------------------------------------------
// the start scheduler must be infallible
struct FallibleSch {
  using scheduler_concept = ex::scheduler_tag;
  struct Sender {
    using sender_concept = ex::sender_tag;
    ex::env<> get_env() const noexcept { return {}; }
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr)>{};
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
  Sender schedule() const noexcept { return {}; }
  ex::forward_progress_guarantee query(ex::get_forward_progress_guarantee_t) const noexcept {
    return ex::forward_progress_guarantee::weakly_parallel;
  }
  friend bool operator==(FallibleSch, FallibleSch) = default;
};
struct FallibleEnv {
  FallibleSch query(ex::get_start_scheduler_t) const noexcept { return {}; }
};
using AffineChildSender = decltype(ex::affine(Child{}));
static_assert(ex::sender_in<AffineChildSender, Env>);
static_assert(!ex::sender_in<AffineChildSender, FallibleEnv>);
static_assert(!ex::sender_in<AffineChildSender, ex::env<>>);

consteval bool throws(auto probe) {
  try {
    probe();
    return false;
  } catch (std::exception&) {
    return true;
  }
}
static_assert(throws([] { (void)ex::get_completion_signatures<AffineChildSender, FallibleEnv>(); }));
static_assert(throws([] { (void)ex::get_completion_signatures<AffineChildSender, ex::env<>>(); }));
static_assert(!throws([] { (void)ex::get_completion_signatures<AffineChildSender, Env>(); }));

// with a stop token that cannot be requested the schedule operation of a scheduler that may be stopped is fine as well
struct StoppableSch : FallibleSch {
  struct Sender2 : FallibleSch::Sender {
    template <class Self, class... Env>
    static consteval auto get_completion_signatures() {
      return ex::completion_signatures<ex::set_value_t(), ex::set_stopped_t()>{};
    }
  };
  Sender2 schedule() const noexcept { return {}; }
};
struct UnstoppableEnv {
  StoppableSch query(ex::get_start_scheduler_t) const noexcept { return {}; }
  std::never_stop_token query(std::get_stop_token_t) const noexcept { return {}; }
};
struct StoppableEnv {
  StoppableSch query(ex::get_start_scheduler_t) const noexcept { return {}; }
  std::inplace_stop_token query(std::get_stop_token_t) const noexcept { return {}; }
};
// stop can be requested: a schedule operation that completes with a value or stopped is infallible
static_assert(ex::__infallible_scheduler<StoppableSch, StoppableEnv>);
// stop cannot be requested: it would only be infallible if it completed with a value
static_assert(!ex::__infallible_scheduler<StoppableSch, UnstoppableEnv>);
static_assert(!ex::__infallible_scheduler<FallibleSch, StoppableEnv>);
static_assert(ex::__infallible_scheduler<Counted, UnstoppableEnv>);
static_assert(ex::__infallible_scheduler<Counted, StoppableEnv>);
static_assert(ex::__infallible_scheduler<Counted, ex::env<>>);
static_assert(!ex::__infallible_scheduler<int, ex::env<>>);

// ---------------------------------------------------------------------------------------------------------------
// UNSTOPPABLE-SCHEDULER
static_assert(ex::scheduler<unstoppable_sch<Counted>>);
static_assert(unstoppable_sch<Counted>(Counted{nullptr}) == unstoppable_sch<Counted>(Counted{nullptr}));
static_assert(std::same_as<ex::schedule_result_t<unstoppable_sch<Counted>>,
                           decltype(ex::unstoppable(ex::schedule(std::declval<Counted>())))>);

int main(int, char**) {
  test_affine_on_the_start_scheduler();
  test_member();
  test_pipe();
  test_unstoppable_scheduler();
  return 0;
}
