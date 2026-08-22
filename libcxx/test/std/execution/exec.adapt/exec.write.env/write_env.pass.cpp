//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// inline constexpr unspecified write_env{};

#include <cassert>
#include <execution>
#include <type_traits>

using namespace std::execution;

// A forwarding query, so FWD-ENV (the fallback write_env's receiver joins its own env with)
// actually passes it through instead of stripping it -- [exec.fwd.env]/[exec.snd.expos]p4.
// Unconstrained `const auto&` parameter and fixed `int` return type, matching
// exec.factories/exec.read.env/read_env.pass.cpp's own get_value_t precedent.
struct get_value_t : std::forwarding_query_t {
  constexpr int operator()(const auto& __env) const noexcept { return __env.query(get_value_t{}); }
};
inline constexpr get_value_t get_value{};

// A separate, non-self-referential *caller* for get_value, used only by the joined-env
// completion-signature checks below. get_value_t itself can't discriminate "does this env
// answer get_value": its `const auto&` parameter plus fixed (non-deduced) `int` return type
// make the call `get_value_t()(env)` well-formed for *any* env, since forming the call never
// needs the body (`env.query(...)`) to compile -- only actually invoking it would fail, and
// read_env's own get_completion_signatures never does that (everything there is decltype/
// noexcept, an unevaluated context). read_value_t's own constrained operator() makes
// `read_env(read_value)`'s sender_in genuinely track whether the connecting env answers
// get_value. References the already-complete `get_value` object (not a freshly-constructed
// `get_value_t{}`) so the constraint doesn't need get_value_t to be complete at a point inside
// its own definition.
struct read_value_t {
  template <class _Env>
    requires requires(const _Env& __env) { __env.query(get_value); }
  constexpr int operator()(const _Env& __env) const noexcept { return __env.query(get_value); }
};
inline constexpr read_value_t read_value{};

struct OuterEnv {
  int value;
  constexpr int query(get_value_t) const noexcept { return value; }
};

struct OuterRcvr {
  using receiver_concept = receiver_tag;
  int* out;
  OuterEnv env_;
  void set_value(int v) && noexcept { *out = v; }
  void set_error(int) && noexcept { assert(false); }
  void set_stopped() && noexcept { assert(false); }
  const OuterEnv& get_env() const noexcept { return env_; }
};

// [exec.adapt.general]p3.2: write_env's own (sender-level) attributes are FWD-ENV(get_env(child))
// -- unaffected by write_env's own env argument, which only customizes the environment its
// *child* is connected through. child_sndr's own attribute answers get_value with 99; the
// write_env wrapping it below is given a different written value (1), so if get_env(the
// write_env sender) ever returned 1 instead of 99, that would mean write_env customized its own
// sender-level attributes instead of only the receiver-env its child connects through.
struct child_sndr {
  using sender_concept = sender_tag;
  constexpr auto get_env() const noexcept { return prop(get_value, 99); }
};
static_assert(sender<child_sndr>);

// [exec.write.env]p2 never says "write_env denotes a pipeable sender adaptor object" (the phrase
// [exec.then]p2/[exec.on]p2/[exec.let]p2 use), so unlike then/on/let_value, write_env has no
// `write_env(env)` partial-application form.
static_assert(!std::is_invocable_v<decltype(write_env), decltype(prop(get_value, 1))>);

// [exec.write.env]p5 (check-types): the child's own signatures, computed against the *joined*
// environment type (join-env(state, FWD-ENV(Env))), not against state or Env alone. Uses
// read_value (not get_value) as the child's query -- see read_value_t's own comment for why
// get_value_t can't discriminate these cases.

// state answers (prop(get_value, 10)); Env doesn't (env<>{}) -- the joined env still answers,
// via state.
static_assert(sender_in<decltype(write_env(read_env(read_value), prop(get_value, 10))), env<>>);
static_assert(std::is_same_v<completion_signatures_of_t<decltype(write_env(read_env(read_value), prop(get_value, 10))), env<>>,
                              completion_signatures<set_value_t(int)>>);

// state answers nothing (env<>{}), and neither does Env: the joined env can't answer either, so
// sender_in is false -- not a hard error, matching read_env's own "dependent-sender-as-soft-
// failure" deviation (docs/CXX26_GAPS.md, M2 deviation 2), which write_env inherits by
// delegating straight to the child's own get_completion_signatures over the joined type.
static_assert(!sender_in<decltype(write_env(read_env(read_value), env<>{})), env<>>);

// ...but if the *outer* Env answers it (even though state doesn't), join-env's FWD-ENV fallback
// makes it visible to the child -- the discriminating case: this fails if __write_env_join ever
// dropped its second (FWD-ENV) argument entirely and only ever consulted state.
struct env_with_value {
  constexpr int query(get_value_t) const noexcept { return 42; }
};
static_assert(sender_in<decltype(write_env(read_env(read_value), env<>{})), env_with_value>);
static_assert(std::is_same_v<completion_signatures_of_t<decltype(write_env(read_env(read_value), env<>{})), env_with_value>,
                              completion_signatures<set_value_t(int)>>);

int main(int, char**) {
  // write_env's own env takes priority over the outer receiver's env for a query both answer.
  {
    int value = 0;
    auto op = connect(write_env(read_env(get_value), prop(get_value, 10)), OuterRcvr{&value, OuterEnv{20}});
    start(op);
    assert(value == 10);
  }
  // A query write_env's own env doesn't answer falls through to the outer receiver's env
  // (join-env's [exec.write.env]p4 "otherwise" branch) -- env<>{} answers nothing.
  {
    int value = 0;
    auto op = connect(write_env(read_env(get_value), env<>{}), OuterRcvr{&value, OuterEnv{20}});
    start(op);
    assert(value == 20);
  }
  // write_env's own sender-level attributes come from its child, not its own env argument.
  {
    auto e = get_env(write_env(child_sndr{}, prop(get_value, 1)));
    assert(e.query(get_value_t{}) == 99);
  }
  return 0;
}
