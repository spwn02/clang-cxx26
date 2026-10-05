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

// A library adaptor whose advertised completions contain no error completion must never complete with one: a receiver
// that only handles the advertised completions has to work (the exception handling of an adaptor is only compiled when
// the adaptor advertises set_error_t(exception_ptr)).

#include <cassert>
#include <execution>

namespace ex = std::execution;

struct only_value_and_stopped_rcvr {
  using receiver_concept = ex::receiver_tag;
  int* count;
  template <class... Ts>
  void set_value(Ts&&...) && noexcept {
    ++*count;
  }
  void set_stopped() && noexcept {}
  auto get_env() const noexcept { return ex::prop(ex::get_start_scheduler, ex::inline_scheduler{}); }
};

template <class Sndr>
void run(Sndr&& sndr) {
  int count = 0;
  auto op   = ex::connect(std::forward<Sndr>(sndr), only_value_and_stopped_rcvr{&count});
  ex::start(op);
  assert(count == 1);
}

int main(int, char**) {
  auto f = [](int x) noexcept { return x; };
  run(ex::then(ex::just(1), f));
  run(ex::upon_error(ex::just(1), f));
  run(ex::upon_stopped(ex::just(1), []() noexcept { return 0; }));
  run(ex::continues_on(ex::just(1), ex::inline_scheduler{}));
  run(ex::schedule_from(ex::just(1)));
  run(ex::starts_on(ex::inline_scheduler{}, ex::just(1)));
  run(ex::write_env(ex::just(1), ex::env<>{}));
  run(ex::read_env(std::get_stop_token));
  run(ex::into_variant(ex::just(1)));
  run(ex::when_all(ex::just(1), ex::just(2)));
  run(ex::when_all_with_variant(ex::just(1)));
  run(ex::bulk(ex::just(1), ex::seq, 2, [](int, int&) noexcept {}));
  run(ex::bulk_chunked(ex::just(1), ex::seq, 2, [](int, int, int&) noexcept {}));
  run(ex::stopped_as_optional(ex::just(1)));
  run(ex::stopped_as_error(ex::just(1), 5));
  run(ex::let_value(ex::just(1), [](int x) noexcept { return ex::just(x); }));
  run(ex::let_error(ex::just_error(1), [](int x) noexcept { return ex::just(x); }));
  run(ex::let_stopped(ex::just_stopped(), []() noexcept { return ex::just(1); }));
  run(ex::on(ex::inline_scheduler{}, ex::just(1)));
  return 0;
}
