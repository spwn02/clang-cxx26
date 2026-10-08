// RUN: %clang_cc1 %s -std=c++20 -fsyntax-only -verify
// RUN: %clang_cc1 %s -std=c++2c -fsyntax-only -verify
#include "Inputs/std-coroutine.h"

// P3950R1: a promise type may declare both return_void and return_value. If overload resolution for p.return_void()
// succeeds, flowing off the end of the coroutine is equivalent to co_return;, otherwise it has undefined behavior (no error).

template <class P>
struct task {
  using promise_type = P;
};

#define PROMISE_BODY(Self)                                                                                              \
  task<Self> get_return_object();                                                                                       \
  std::suspend_never initial_suspend() noexcept;                                                                        \
  std::suspend_never final_suspend() noexcept;                                                                          \
  void unhandled_exception();

struct both {
  PROMISE_BODY(both)
  void return_void();
  void return_value(int);
};
task<both> flows_off() { co_await std::suspend_never{}; }
task<both> returns_void() { co_return; }
task<both> returns_value() { co_return 1; }

// return_void exists but cannot be called without arguments: not an error (only the usual -Wreturn-type warning), flowing off
// the end is undefined
struct void_needs_argument {
  PROMISE_BODY(void_needs_argument)
  void return_void(int);
  void return_value(int);
};
task<void_needs_argument> flows_off_without_viable_return_void() { co_await std::suspend_never{}; }
// expected-warning@-1 {{non-void coroutine does not return a value}}
task<void_needs_argument> returns_value_only() { co_return 1; }

// every overload is a candidate: an ambiguous set is not "succeeds" either
struct ambiguous_void {
  PROMISE_BODY(ambiguous_void)
  void return_void(int = 0);
  void return_void(long = 0);
  void return_value(int);
};
task<ambiguous_void> flows_off_with_ambiguous_return_void() { co_await std::suspend_never{}; }
// expected-warning@-1 {{non-void coroutine does not return a value}}

// a deleted return_void is the selected function: the call is ill-formed
struct deleted_void {
  PROMISE_BODY(deleted_void)
  void return_void() = delete; // expected-note {{'return_void' has been explicitly marked deleted here}}
  void return_value(int);
};
task<deleted_void> flows_off_with_deleted_return_void() { // expected-error {{attempt to use a deleted function}}
  co_await std::suspend_never{};
}

// an inaccessible return_void is selected as well
struct private_void {
  PROMISE_BODY(private_void)
  void return_value(int);

private:
  void return_void(); // expected-note {{declared private here}}
};
task<private_void> flows_off_with_private_return_void() { // expected-error {{'return_void' is a private member of 'private_void'}}
  co_await std::suspend_never{};
}

// templates: both are looked up when the promise is known
template <class T>
task<T> flows_off_template() {
  co_await std::suspend_never{};
}
template task<both> flows_off_template<both>();
template task<void_needs_argument> flows_off_template<void_needs_argument>();

// the macro that advertises the feature
#if __cplusplus > 202302L
static_assert(__cpp_impl_coroutine == 202606L);
#else
static_assert(__cpp_impl_coroutine == 201902L);
#endif
