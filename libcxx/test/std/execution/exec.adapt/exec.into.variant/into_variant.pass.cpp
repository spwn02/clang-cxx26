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
//   struct into_variant_t { ... };
//   inline constexpr into_variant_t into_variant{};
// }

#include <cassert>
#include <execution>
#include <type_traits>
#include <utility>
#include <variant>

using namespace std::execution;

// A sender with two distinct value-completion shapes (set_value_t(int) and
// set_value_t(double, char)), plus an error and a stopped completion, all runtime-switchable
// -- needed to exercise into_variant's variant-of-decayed-tuples mapping across more than one
// alternative, and its error/stopped passthrough, none of which just()/just_error()/
// just_stopped() alone (each advertising exactly one completion) can exercise together.
struct multi_value_sndr {
  using sender_concept = sender_tag;
  int mode; // 0: set_value(int), 1: set_value(double, char), 2: set_error(int), 3: set_stopped()

  template <class _Rcvr>
  struct __opstate {
    using operation_state_concept = operation_state_tag;
    int mode;
    _Rcvr rcvr;
    void start() & noexcept {
      switch (mode) {
      case 0:
        std::execution::set_value(std::move(rcvr), 42);
        break;
      case 1:
        std::execution::set_value(std::move(rcvr), 3.5, 'x');
        break;
      case 2:
        std::execution::set_error(std::move(rcvr), 99);
        break;
      default:
        std::execution::set_stopped(std::move(rcvr));
        break;
      }
    }
  };

  template <class _Rcvr>
  auto connect(_Rcvr&& __rcvr) && -> __opstate<std::remove_cvref_t<_Rcvr>> {
    return {mode, std::forward<_Rcvr>(__rcvr)};
  }

  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return completion_signatures<set_value_t(int), set_value_t(double, char), set_error_t(int), set_stopped_t()>{};
  }
};
static_assert(sender<multi_value_sndr>);

using __variant_t = std::variant<std::tuple<int>, std::tuple<double, char>>;

int main(int, char**) {
  // First value shape: variant holds the tuple<int> alternative.
  {
    auto r = std::this_thread::sync_wait(into_variant(multi_value_sndr{0}));
    assert(r.has_value());
    __variant_t& v = std::get<0>(*r);
    assert(v.index() == 0);
    assert(std::get<0>(v) == std::tuple(42));
  }
  // Second value shape: variant holds the tuple<double, char> alternative.
  {
    auto r = std::this_thread::sync_wait(into_variant(multi_value_sndr{1}));
    assert(r.has_value());
    __variant_t& v = std::get<0>(*r);
    assert(v.index() == 1);
    assert(std::get<1>(v) == std::tuple(3.5, 'x'));
  }
  // Error completions pass through unchanged -- not folded into the variant.
  {
    bool caught = false;
    try {
      (void)std::this_thread::sync_wait(into_variant(multi_value_sndr{2}));
    } catch (int __err) {
      caught = true;
      assert(__err == 99);
    }
    assert(caught);
  }
  // Stopped completions pass through unchanged -- the overall sender never synthesizes a value
  // out of a stopped completion the way stopped_as_optional does.
  {
    auto r = std::this_thread::sync_wait(into_variant(multi_value_sndr{3}));
    assert(!r.has_value());
  }
  // Call-syntax and pipe-syntax forms are equivalent.
  {
    auto r = std::this_thread::sync_wait(multi_value_sndr{0} | into_variant);
    assert(r.has_value());
    assert(std::get<0>(*r).index() == 0);
  }
  // Completion signatures: the two set_value shapes collapse into one set_value_t(variant),
  // set_error_t(int) passes through unchanged, set_stopped_t() passes through unchanged.
  {
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(into_variant(multi_value_sndr{0})), env<>>,
                                  completion_signatures<set_value_t(__variant_t), set_error_t(int), set_stopped_t()>>);
    static_assert(sends_stopped<decltype(into_variant(multi_value_sndr{0})), env<>>);
  }
  // A sender with no value completion at all still typechecks -- variant_type is
  // __variant_or_empty's zero-alternative empty-variant marker (never actually constructed,
  // since no set_value completion ever fires).
  static_assert(sender_in<decltype(into_variant(just_stopped())), env<>>);
  return 0;
}
