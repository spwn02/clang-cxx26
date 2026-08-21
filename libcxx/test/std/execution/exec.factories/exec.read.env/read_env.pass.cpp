//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// inline constexpr unspecified read_env{};

#include <cassert>
#include <execution>
#include <type_traits>

using namespace std::execution;

struct get_value_t {
  constexpr int operator()(const auto& env) const noexcept { return env.query(get_value_t{}); }
};
inline constexpr get_value_t get_value{};

struct MyEnv {
  int value;
  constexpr int query(get_value_t) const noexcept { return value; }
};

struct MyReceiver {
  using receiver_concept = receiver_tag;
  int* out;
  MyEnv env_;
  void set_value(int v) && noexcept { *out = v; }
  void set_error(int) && noexcept { assert(false); }
  void set_stopped() && noexcept { assert(false); }
  const MyEnv& get_env() const noexcept { return env_; }
};

using ReadValueSndr = decltype(read_env(get_value));

// sender is true unconditionally; sender_in (which requires computing completion signatures with no Env)
// is false, since read_env's signatures genuinely depend on the receiver's environment -- this fork's
// documented "dependent-sender-as-soft-failure" deviation (docs/CXX26_GAPS.md, M2's deviation 2 and M3's
// read_env note) rather than dependent_sender<Sndr> reporting true.
static_assert(sender<ReadValueSndr>);
static_assert(!sender_in<ReadValueSndr>);

static_assert(sender_in<ReadValueSndr, env_of_t<MyReceiver>>);
static_assert(std::is_same_v<completion_signatures_of_t<ReadValueSndr, env_of_t<MyReceiver>>,
                              completion_signatures<set_value_t(int)>>);

// A query whose call on the environment is ill-formed also makes sender_in false, not a hard error.
struct get_nothing_t {
  constexpr int operator()(int) const noexcept { return 0; }
};
using ReadNothingSndr = decltype(read_env(get_nothing_t{}));
static_assert(!sender_in<ReadNothingSndr, env_of_t<MyReceiver>>);

int main(int, char**) {
  int value = 0;
  auto op = connect(read_env(get_value), MyReceiver{&value, MyEnv{99}});
  start(op);
  assert(value == 99);
  return 0;
}
