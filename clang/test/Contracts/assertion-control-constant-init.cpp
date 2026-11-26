// RUN: %clang_cc1 -std=c++26 -fcontract-evaluation-semantic=ignore -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++29 -fcontract-evaluation-semantic=ignore -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontract-evaluation-semantic=observe -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++29 -fcontract-evaluation-semantic=observe -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++29 -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontract-evaluation-semantic=quick_enforce -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++29 -fcontract-evaluation-semantic=quick_enforce -fsyntax-only -verify %s

namespace std::contracts::controls {
inline namespace __p4451 {
struct __enforced_t {};
inline constexpr __enforced_t enforced{};
}
}

constexpr int checked(int x)
    pre<std::contracts::controls::enforced>(x > 0) { // expected-error {{contract failed during execution of constexpr function}}
  return x;
}

const int checked_at_init = checked(0); // expected-error {{initialization of constant-initialized variable failed}} expected-note {{initializer is constant when evaluated with 'ignore' contract evaluation semantic}} expected-note {{in call to 'checked(0)'}} expected-note {{declared here}}
static_assert(checked_at_init == 0); // expected-error {{static assertion expression is not an integral constant expression}} expected-note {{initializer of 'checked_at_init' is not a constant expression}}
