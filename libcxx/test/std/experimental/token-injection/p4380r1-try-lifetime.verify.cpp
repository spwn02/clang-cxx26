//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23 || c++26

// P4380R1 section 3.5, try_ (blocks 047 and 048): the macro expands to a do
// expression with an init-hoist, and the compiler still warns about a dangling
// reference in the right place: bound to the result of try_!(...) (f1), not for
// a value consumed inside the full expression (f2). try_traits is not in the
// paper (it is in the Compiler Explorer demo); this is a minimal one.

#include <debugging>
#include <expected>
#include <meta>
#include <utility>

// typed
template <class T>
__macro fwd(T&& t) {
    return ^^{ static_cast<\(type_of(t))&&>(\(t)) };
}


template <class> struct try_traits;
template <class T, class E> struct try_traits<std::expected<T, E>> {
  static constexpr bool should_continue(std::expected<T, E> const& e) { return e.has_value(); }
  static constexpr auto extract_break(std::expected<T, E>&& e) { return std::move(e).error(); }
  // The reference into the expected is what makes the dangling reference of f1
  // detectable: it is bound to the hoisted __r, which ends with the full expression.
  static constexpr T& extract_continue(std::expected<T, E>&& e [[clang::lifetimebound]]) { return *e; }
  template <class U> static constexpr auto from_break(U&& u) { return std::unexpected(std::forward<U>(u)); }
};

template <class T>
__macro try_(T&& e) {
    // `try_` only means anything inside a function: it early-returns from one.
    std::meta::info where = std::meta::macro_expansion_context();
    if (not is_function(where)) {
        std::constexpr_error_str("bad-try-context",
                                 "try_ must be invoked inside a function");
    }

    // Computed in the macro body -- NOT injected as `using` aliases.
    std::meta::info CT = substitute(^^try_traits, {remove_cvref(type_of(e))});
    std::meta::info RT = substitute(^^try_traits, {return_type_of(where)});

    return ^^{
        do [__r=\(e)] -> decltype(auto) {
            if (not \(CT)::should_continue(__r)) [[unlikely]] {
                return \(RT)::from_break(\(CT)::extract_break(fwd!(__r)));
            }
            do_return \(CT)::extract_continue(fwd!(__r));
        }
    };
}


enum class E { };
template <class T>
auto get_data() -> std::expected<T, E>;

auto f1() -> std::expected<int, E> {
    auto&& data = try_!(get_data<int>()); // expected-warning {{temporary bound to local reference 'data' will be destroyed at the end of the full-expression}}
    return data;
}

auto consume(int x) -> int { return x; }

auto f2() -> std::expected<int, E> {
    auto&& data = consume(try_!(get_data<int>())); // no warning here
    return data;
}

