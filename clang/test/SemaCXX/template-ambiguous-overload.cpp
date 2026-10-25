// RUN: %clang_cc1 -fsyntax-only -verify %s
// expected-no-diagnostics

namespace n {
template <int>
void f();

extern template void f<0>();
}

using namespace n;

template <int>
void f() {}

// Only the template nominable in this namespace is considered, not n::f.
template void f<0>();

