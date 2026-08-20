//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// template<class Rcvr>
//   concept receiver = ...;
// template<class Rcvr, class Completions>
//   concept receiver_of = ...;
// struct set_value_t { ... };
// struct set_error_t { ... };
// struct set_stopped_t { ... };

#include <cassert>
#include <execution>
#include <type_traits>
#include <utility>

using namespace std::execution;

using CS = completion_signatures<set_value_t(), set_value_t(int, float), set_error_t(int), set_stopped_t()>;

struct MyReceiver {
  using receiver_concept = receiver_tag;
  int* value_count;
  int* error_count;
  int* stopped_count;

  void set_value(int, float) && noexcept { ++*value_count; }
  void set_value() && noexcept { ++*value_count; }
  void set_error(int) && noexcept { ++*error_count; }
  void set_stopped() && noexcept { ++*stopped_count; }
  auto get_env() const noexcept { return env<>{}; }
};
static_assert(receiver<MyReceiver>);
static_assert(receiver_of<MyReceiver, CS>);

struct NotAReceiver {};
static_assert(!receiver<NotAReceiver>);

struct MissingCompletion {
  using receiver_concept = receiver_tag;
  void set_value() && noexcept {}
  auto get_env() const noexcept { return env<>{}; }
};
static_assert(receiver<MissingCompletion>);
static_assert(!receiver_of<MissingCompletion, CS>); // CS also needs set_value(int,float)/set_error(int)/set_stopped()

// set_value/set_error/set_stopped are ill-formed on lvalues and const rvalues.
template <class Rcvr>
concept CanSetValue = requires(Rcvr r) { set_value(r); };
static_assert(!CanSetValue<MyReceiver&>);
static_assert(!CanSetValue<const MyReceiver>);

int main(int, char**) {
  int values = 0, errors = 0, stops = 0;

  set_value(MyReceiver{&values, &errors, &stops}, 1, 2.0f);
  assert(values == 1);
  static_assert(noexcept(set_value(std::declval<MyReceiver>(), 1, 2.0f)));

  set_error(MyReceiver{&values, &errors, &stops}, 3);
  assert(errors == 1);
  static_assert(noexcept(set_error(std::declval<MyReceiver>(), 3)));

  set_stopped(MyReceiver{&values, &errors, &stops});
  assert(stops == 1);
  static_assert(noexcept(set_stopped(std::declval<MyReceiver>())));

  return 0;
}
