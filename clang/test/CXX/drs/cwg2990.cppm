// RUN: %clang_cc1 -std=c++20 -emit-module-interface %s -o /dev/null -verify
// expected-no-diagnostics

export module M;

namespace cwg2990 { // cwg2990: yes
namespace N { void f(); }
namespace N { export void g(); }
} // namespace cwg2990
