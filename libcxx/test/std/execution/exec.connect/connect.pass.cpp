//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// struct connect_t { ... };
// inline constexpr connect_t connect{};
// template<class Sndr, class Rcvr>
//   using connect_result_t = decltype(connect(declval<Sndr>(), declval<Rcvr>()));

#include <cassert>
#include <execution>
#include <type_traits>

using namespace std::execution;

using CS = completion_signatures<set_value_t(int)>;

struct MyReceiver {
  using receiver_concept = receiver_tag;
  int* value;
  void set_value(int v) && noexcept { *value = v; }
  void set_error(int) && noexcept {}
  void set_stopped() && noexcept {}
  auto get_env() const noexcept { return env<>{}; }
};

struct MyOp {
  using operation_state_concept = operation_state_tag;
  MyReceiver rcvr;
  void start() & noexcept { std::execution::set_value(std::move(rcvr), 42); }
};

struct MySender {
  using sender_concept = sender_tag;
  auto get_env() const noexcept { return env<>{}; }
  template <class Self>
  static consteval auto get_completion_signatures() {
    return CS{};
  }
  MyOp connect(MyReceiver rcvr) && noexcept { return MyOp{std::move(rcvr)}; }
};

static_assert(std::is_same_v<connect_result_t<MySender, MyReceiver>, MyOp>);

int main(int, char**) {
  int value = 0;
  auto op = connect(MySender{}, MyReceiver{&value});
  start(op);
  assert(value == 42);
  return 0;
}
