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

// namespace execution {
//   struct stopped_as_optional_t { ... };
//   inline constexpr stopped_as_optional_t stopped_as_optional{};
// }

#include <cassert>
#include <execution>
#include <optional>
#include <type_traits>
#include <utility>

using namespace std::execution;

// A sender whose single set_value completion carries `int` but that, depending on `stop`,
// actually completes with either that value or set_stopped() at runtime -- needed to exercise
// both of stopped_as_optional's branches while still giving single-sender-value-type
// (<__execution/get_completion_signatures.h>'s __single_sender_value_type) something concrete
// (`int`) to pin, unlike just_stopped() alone (which advertises no set_value completion at all,
// so its own single-sender-value-type is void -- see the exclusion check below).
struct value_or_stopped_sndr {
  using sender_concept = sender_tag;
  bool stop;

  template <class _Rcvr>
  struct __opstate {
    using operation_state_concept = operation_state_tag;
    bool stop;
    _Rcvr rcvr;
    void start() & noexcept {
      if (stop) {
        std::execution::set_stopped(std::move(rcvr));
      } else {
        std::execution::set_value(std::move(rcvr), 42);
      }
    }
  };

  template <class _Rcvr>
  auto connect(_Rcvr&& __rcvr) && -> __opstate<std::remove_cvref_t<_Rcvr>> {
    return {stop, std::forward<_Rcvr>(__rcvr)};
  }

  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return completion_signatures<set_value_t(int), set_stopped_t()>{};
  }
};
static_assert(sender<value_or_stopped_sndr>);

int main(int, char**) {
  // Value path: the child's value completion becomes an engaged optional.
  {
    auto r = std::this_thread::sync_wait(stopped_as_optional(value_or_stopped_sndr{false}));
    assert(r.has_value());
    assert(std::get<0>(*r).has_value());
    assert(*std::get<0>(*r) == 42);
  }
  // Stopped path: the child's stopped completion becomes a disengaged optional -- the overall
  // sender never itself completes with stopped ([exec.stopped.opt]p1).
  {
    auto r = std::this_thread::sync_wait(stopped_as_optional(value_or_stopped_sndr{true}));
    assert(r.has_value());
    assert(!std::get<0>(*r).has_value());
  }
  // Call-syntax and pipe-syntax forms are equivalent.
  {
    auto r = std::this_thread::sync_wait(value_or_stopped_sndr{false} | stopped_as_optional);
    assert(r.has_value());
    assert(std::get<0>(*r).has_value());
    assert(*std::get<0>(*r) == 42);
  }
  // Completion signatures: a single set_value_t(optional<int>), no error signature (both the
  // then-lambda's optional<int> construction and the just-lambda are statically nothrow here).
  {
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(stopped_as_optional(value_or_stopped_sndr{false})), env<>>,
                                  completion_signatures<set_value_t(std::optional<int>)>>);
    static_assert(!sends_stopped<decltype(stopped_as_optional(value_or_stopped_sndr{false})), env<>>);
  }
  // [exec.stopped.opt]p3's Mandates (single-sender-value-type<child, Env> is not void) is
  // enforced via SFINAE non-participation (docs/CXX26_GAPS.md's P3068 deviation, applied here
  // the same way <__execution/then.h>/<__execution/let.h> apply it) rather than a diagnostic:
  // just_stopped() advertises no set_value completion at all, so its single-sender-value-type
  // is void, and the resulting stopped_as_optional sender simply isn't a sender_in.
  static_assert(!sender_in<decltype(stopped_as_optional(just_stopped())), env<>>);
  // Same conclusion for a sender whose sole set_value completion carries zero datums.
  static_assert(!sender_in<decltype(stopped_as_optional(just())), env<>>);
  return 0;
}
