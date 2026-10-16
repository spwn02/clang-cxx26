//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// struct just_t { ... };
// struct just_error_t { ... };
// struct just_stopped_t { ... };
// inline constexpr just_t just{};
// inline constexpr just_error_t just_error{};
// inline constexpr just_stopped_t just_stopped{};

#include <cassert>
#include <execution>
#include <tuple>
#include <type_traits>
#include <utility>

using namespace std::execution;

template <class... Vs>
struct ValueReceiver {
  using receiver_concept = receiver_tag;
  std::tuple<Vs...>* out;
  void set_value(Vs... vs) && noexcept { *out = std::tuple<Vs...>(vs...); }
  void set_error(int) && noexcept { assert(false); }
  void set_stopped() && noexcept { assert(false); }
  auto get_env() const noexcept { return env<>{}; }
};

struct ErrorReceiver {
  using receiver_concept = receiver_tag;
  int* out;
  void set_value() && noexcept { assert(false); }
  void set_error(int e) && noexcept { *out = e; }
  void set_stopped() && noexcept { assert(false); }
  auto get_env() const noexcept { return env<>{}; }
};

struct StoppedReceiver {
  using receiver_concept = receiver_tag;
  bool* out;
  void set_value() && noexcept { assert(false); }
  void set_error(int) && noexcept { assert(false); }
  void set_stopped() && noexcept { *out = true; }
  auto get_env() const noexcept { return env<>{}; }
};

// just()
static_assert(sender<decltype(just())>);
static_assert(sender_in<decltype(just())>);
static_assert(std::is_same_v<completion_signatures_of_t<decltype(just())>, completion_signatures<set_value_t()>>);

// just(42, 'a')
static_assert(std::is_same_v<completion_signatures_of_t<decltype(just(42, 'a'))>,
                              completion_signatures<set_value_t(int, char)>>);

// just_error(42)
static_assert(
    std::is_same_v<completion_signatures_of_t<decltype(just_error(42))>, completion_signatures<set_error_t(int)>>);

// just_stopped()
static_assert(std::is_same_v<completion_signatures_of_t<decltype(just_stopped())>,
                              completion_signatures<set_stopped_t()>>);

int main(int, char**) {
  {
    std::tuple<int, char> result;
    auto op = connect(just(42, 'a'), ValueReceiver<int, char>{&result});
    start(op);
    assert(result == std::tuple<int, char>(42, 'a'));
  }
  {
    int err = 0;
    auto op = connect(just_error(7), ErrorReceiver{&err});
    start(op);
    assert(err == 7);
  }
  {
    bool stopped = false;
    auto op = connect(just_stopped(), StoppedReceiver{&stopped});
    start(op);
    assert(stopped);
  }
  return 0;
}
