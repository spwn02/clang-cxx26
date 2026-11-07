//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23 || c++26

// P4380R1 sections 3.1 (fwd), 3.2 (check), 3.3 (double evaluation) and 3.4
// (abbreviated lambdas), run. The text a failed check prints is the paper's
// (blocks 038 and 045); here it is collected in a string instead of printed.
// The derive_debug annotation is the paper's DeriveDebug class (block 009).

// RUN: %{build}
// RUN: %{exec} %t.exe

#include <algorithm>
#include <cassert>
#include <concepts>
#include <format>
#include <meta>
#include <optional>
#include <ranges>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

#include "test_macros.h"

static std::string g_out;

struct DeriveDebug {
    consteval auto on_complete(std::meta::info ty) const -> void {
        auto fmt_body = std::meta::list_builder();
        fmt_body += ^^{
            auto out = std::format_to(ctx.out(), "{}{{", \(display_string_of(ty)));
        };

        auto delim = [first=true, &fmt_body]() mutable {
            if (not first) {
                fmt_body += ^^{
                    *out++ = ',';
                    *out++ = ' ';
                };
            }
            first = false;
        };

        auto unchecked = std::meta::access_context::unchecked();
        for (auto nsdm : nonstatic_data_members_of(ty, unchecked)) {
            delim();
            fmt_body += ^^{
                out = std::format_to(out,
                    \(std::meta::str_lit(".", identifier_of(nsdm), "={}")),
                    object.\(nsdm)
                );
            };
        }

        fmt_body += ^^{
            *out++ = '}';
            return out;
        };

        queue_injection(^^std, ^^{
            template <>
            struct formatter<\(ty)> {
                constexpr auto parse(auto& ctx) { return ctx.begin(); }

                auto format(\(ty) const& object, auto& ctx) const {
                    \(fmt_body);
                }
            };
        });
    }
};

inline constexpr DeriveDebug derive_debug{};


// typed
template <class T>
__macro fwd(T&& t) {
    return ^^{ static_cast<\(type_of(t))&&>(\(t)) };
}


consteval auto is_comparison(std::meta::operators op) -> bool {
    switch (op) {
        using enum std::meta::operators;
        case op_equals_equals:
        case op_exclamation_equals:
        case op_less:
        case op_greater:
        case op_less_equals:
        case op_greater_equals:
            return true;
        default:
            return false;
    }
}

template <class T, class U>
auto check_fail(char const* text, std::source_location sloc, T const& lhs, char const* op, U const& rhs) -> void {
    g_out += std::format("{}:{}: FAILED:\n", sloc.file_name(), sloc.line());
    g_out += std::format("  CHECK( {} )\n", text);
    g_out += "with expansion:\n";
    g_out += std::format("  {} {} {}\n", lhs, op, rhs);
}

template <class T>
auto check_fail(char const* text, std::source_location sloc, T const& expr) -> void {
    g_out += std::format("{}:{}: FAILED:\n", sloc.file_name(), sloc.line());
    g_out += std::format("  CHECK( {} )\n", text);
    g_out += "with expansion:\n";
    g_out += std::format("  {}\n", expr);
}

template <class T>
__macro check(T&& cond) {
    auto const text = std::meta::str_lit(source_text_of(cond));
    auto const sloc = source_location_of(cond);

    if (is_binary_operation(cond) and is_comparison(operator_of(cond))) {
        auto ops = operands_of(cond);
        char const* op = stringize(^^{ \(operator_of(cond)) });

        return ^^{
            do {
                auto&& lhs = \(ops[0]);
                auto&& rhs = \(ops[1]);
                if (not (fwd!(lhs) \(operator_of(cond)) fwd!(rhs))) {
                    ::check_fail(\(text), \(sloc), lhs, \(op), rhs);
                }
            }
        };
    }

    return ^^{
        do {
            auto&& expr = \(cond);
            if (!static_cast<bool>(expr)) {
                ::check_fail(\(text), \(sloc), expr);
            }
        }
    };
}


consteval auto placeholder_index(std::string_view id) -> std::optional<int> {
    if (id.size() == 2 and id[0] == '_' and '1' <= id[1] and id[1] <= '9') {
        return id[1] - '0';
    }
    return std::nullopt;
}

__macro λ(std::meta::token_sequence body) {
    auto arity = std::optional<int>();
    for (std::meta::token tok : body) {
        if (token_kind_of(tok) == std::meta::token_kind::identifier) {
            arity = std::max(arity, placeholder_index(identifier_of(tok)));
        }
    }

    auto params = std::meta::list_builder(^^{ , });
    for (int i = 1; i <= arity.value_or(0); ++i) {
        params += ^^{ auto&& \(std::meta::id("_", i)) };
    }

    return ^^{ [&](\(params)) -> decltype(auto) { return \(body); } };
}


struct [[=derive_debug]] Point {
    int x;
    int y;
    auto operator==(Point const&) const -> bool = default;
};

void run_lambda_sort() {
    std::vector<Point> points = {{1, 2}, {3, 1}};
    std::ranges::sort(points, λ!(_1.y < _2.y));
    check!(points == std::vector<Point>{{3, 2}, {1, 2}});
}


int f() { return 1; }
int g() { return 2; }

int main(int, char**) {
  // block 038: a failed comparison prints both sides
  {
    g_out.clear();
    check!(f() == g());
    auto nl = g_out.find('\n');
    assert(nl != std::string::npos);
    assert(g_out.substr(0, nl).ends_with(": FAILED:"));
    assert(g_out.substr(nl + 1) == "  CHECK( f() == g() )\nwith expansion:\n  1 == 2\n");
  }
  // a passing check prints nothing
  {
    g_out.clear();
    check!(f() == 1);
    check!(f() != g());
    assert(g_out.empty());
  }
  // a non-comparison condition, converted to bool
  {
    g_out.clear();
    check!(f() - 1);
    auto nl = g_out.find('\n');
    assert(nl != std::string::npos);
    assert(g_out.substr(nl + 1) == "  CHECK( f() - 1 )\nwith expansion:\n  0\n");
  }
  // block 045: the abbreviated lambda sorts by y, and the check fails with the paper's output
  {
    g_out.clear();
    run_lambda_sort();
    auto nl = g_out.find('\n');
    assert(nl != std::string::npos);
    assert(g_out.substr(nl + 1) ==
           "  CHECK( points == std::vector<Point>{{3, 2}, {1, 2}} )\n"
           "with expansion:\n"
           "  [Point{.x=3, .y=1}, Point{.x=1, .y=2}] == [Point{.x=3, .y=2}, Point{.x=1, .y=2}]\n");
  }
  // λ with two placeholders in a call
  {
    auto add = λ!(_1 + _2);
    assert(add(2, 3) == 5);
    auto first = λ!(_1);
    assert(first(7) == 7);
    int k = 10;
    auto capture = λ!(_1 * k);
    assert(capture(4) == 40);
  }
  return 0;
}
