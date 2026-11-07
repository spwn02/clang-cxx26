//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23 || c++26

// P4380R1 section 2.4, "Iterator Interface" (block 015): the paper's
// iterator_interface, which observes the members the user declared while the
// class is being completed and injects the rest of a random access iterator.
// The paper has no usage snippet (its demo is a Compiler Explorer link): the
// call is a consteval block in the body of the class.

// RUN: %{build}
// RUN: %{exec} %t.exe

#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <meta>
#include <string_view>

#include "test_macros.h"

template <class B> constexpr bool base_can_deref   = requires(B const& b) { *b; };
template <class B> constexpr bool base_can_pre_inc = requires(B& b) { ++b; };
template <class B> constexpr bool base_can_eq      =
    requires(B const& a, B const& b) { { a == b } -> std::convertible_to<bool>; };

struct IterConfig {
    std::meta::info iterator_concept;
    std::meta::info value_type;
    std::meta::info difference_type = ^^std::ptrdiff_t;
    std::meta::info adaptor = std::meta::info();
};

// Which operators has the user declared SO FAR in the class being defined?
// Caveat: We don't see friend declarations, that's still TBD for what the interface should be
consteval auto declares_op(std::meta::info cls, std::meta::operators which) -> bool {
    for (auto m : members_of(cls, std::meta::access_context::unchecked()))
        if (is_operator_function(m) && operator_of(m) == which)
            return true;
    return false;
}

consteval auto declares_fn(std::meta::info cls, std::string_view name) -> bool {
    for (auto m : members_of(cls, std::meta::access_context::unchecked()))
        if (has_identifier(m) && identifier_of(m) == name)
            return true;
    return false;
}

consteval auto iterator_interface(std::meta::info cls, IterConfig cfg) -> void {
    using namespace std::meta;
    auto ts = ^^{
        using iterator_concept = \(cfg.iterator_concept);
        using value_type = \(cfg.value_type);
        using difference_type = \(cfg.difference_type);
    };

    bool plus_eq  = declares_op(cls, op_plus_equals);
    bool minus    = declares_op(cls, op_minus);
    bool pre_inc  = declares_op(cls, op_plus_plus);
    bool pre_dec  = declares_op(cls, op_minus_minus);
    bool eq       = declares_op(cls, op_equals_equals);
    bool deref    = declares_op(cls, op_star);

    if (cfg.adaptor != std::meta::info()) {
        // Adaptor path: defer to cfg.adaptor for all operations
        info base_t = type_of(cfg.adaptor);
        bool base_deref   = extract<bool>(substitute(^^base_can_deref, {base_t}));
        bool base_pre_inc = extract<bool>(substitute(^^base_can_pre_inc, {base_t}));
        bool base_eq      = extract<bool>(substitute(^^base_can_eq, {base_t}));

        if (base_deref && !deref) {
            ts += ^^{ constexpr decltype(auto) operator*() const { return *this->\(cfg.adaptor); } };
        }
        if (base_pre_inc && !pre_inc) {
            ts += ^^{ constexpr auto operator++() -> \(cls)& { ++this->\(cfg.adaptor); return *this; } };
        }
        if (base_pre_inc) {
            ts += ^^{ constexpr auto operator++(int) -> \(cls) { auto tmp = *this; ++*this; return tmp; } };
        }
        if (base_eq && !eq) {
            ts += ^^{
                friend constexpr bool operator==(\(cls) const& a, \(cls) const& b) {
                    return a.\(cfg.adaptor) == b.\(cfg.adaptor);
                }
            };
        }
        queue_injection(ts);
        return;
    }

    // Direct path: decide based on visible members.
    // Wrong signatures error in the injected bodies at definition time — can't check these yet
    if (plus_eq) {
        ts += ^^{
            constexpr auto operator++() -> \(cls)& { *this += 1; return *this; }
            constexpr auto operator++(int) -> \(cls) { auto tmp = *this; ++*this; return tmp; }
            constexpr auto operator--() -> \(cls)& { *this += -1; return *this; }
            constexpr auto operator--(int) -> \(cls) { auto tmp = *this; --*this; return tmp; }
            constexpr auto operator-=(std::ptrdiff_t n) -> \(cls)& { *this += -n; return *this; }
            constexpr decltype(auto) operator[](std::ptrdiff_t n) const { auto tmp = *this; tmp += n; return *tmp; }
            friend constexpr auto operator+(\(cls) it, std::ptrdiff_t n) -> \(cls) { it += n; return it; }
            friend constexpr auto operator+(std::ptrdiff_t n, \(cls) it) -> \(cls) { it += n; return it; }
            friend constexpr auto operator-(\(cls) it, std::ptrdiff_t n) -> \(cls) { it += -n; return it; }
        };
    } else {
        if (pre_inc) {
            ts += ^^{ constexpr auto operator++(int) -> \(cls) { auto tmp = *this; ++*this; return tmp; } };
        }
        if (pre_dec) {
            ts += ^^{ constexpr auto operator--(int) -> \(cls) { auto tmp = *this; --*this; return tmp; } };
        }
    }
    if (minus) {
        if (!eq) {
            ts += ^^{
                friend constexpr bool operator==(\(cls) const& a, \(cls) const& b) { return (a - b) == 0; }
            };
        }
        ts += ^^{
            friend constexpr auto operator<=>(\(cls) const& a, \(cls) const& b) { return (a - b) <=> 0; }
        };
    }
    queue_injection(ts);
}


// Direct path: the user declares *, +=, - and the rest comes from them.
struct Counter {
  int pos = 0;
  constexpr int operator*() const { return pos; }
  constexpr auto operator+=(std::ptrdiff_t d) -> Counter& { pos += static_cast<int>(d); return *this; }
  constexpr auto operator-(Counter const& o) const -> std::ptrdiff_t { return pos - o.pos; }
  consteval { iterator_interface(^^Counter, {.iterator_concept = ^^std::random_access_iterator_tag, .value_type = ^^int}); }
};

static_assert(std::random_access_iterator<Counter>);

// Adaptor path: base_can_deref / base_can_pre_inc / base_can_eq are evaluated
// for the type of the adaptor member (substitute + extract<bool>), and every
// operation the base has is forwarded to it.
struct Wrapper {
  int const *base = nullptr;
  consteval { iterator_interface(^^Wrapper, {.iterator_concept = ^^std::forward_iterator_tag, .value_type = ^^int, .adaptor = ^^Wrapper::base}); }
};

static_assert(std::forward_iterator<Wrapper>);

constexpr bool run() {
  Counter c{3};
  assert(*c == 3);
  ++c;
  assert(*c == 4);
  c++;
  assert(*c == 5);
  --c;
  assert(*c == 4);
  c -= 2;
  assert(*c == 2);
  assert(c[3] == 5);
  assert(*(c + 4) == 6);
  assert(*(4 + c) == 6);
  assert(*(c - 1) == 1);
  assert(c == Counter{2});
  assert(c != Counter{3});
  assert(c < Counter{3});
  assert((Counter{7} - Counter{2}) == 5);

  static constexpr int data[] = {10, 20, 30};
  Wrapper w{data};
  assert(*w == 10);
  ++w;
  assert(*w == 20);
  Wrapper old = w++;
  assert(*old == 20 && *w == 30);
  assert(old == Wrapper{data + 1});
  assert(!(w == old));
  return true;
}
static_assert(run());

int main(int, char**) {
  assert(run());
  return 0;
}
