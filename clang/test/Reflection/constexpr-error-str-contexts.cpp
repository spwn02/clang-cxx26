// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=2 %s
// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=4 %s
// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=5 %s
// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=6 %s
// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=7 %s
// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=8 %s
// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=9 %s
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=1 %s 2>&1 | FileCheck %s --check-prefix=ONE
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=3 %s 2>&1 | FileCheck %s --check-prefix=ONE
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=10 %s 2>&1 | FileCheck %s --check-prefix=ONE
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=11 %s 2>&1 | FileCheck %s --check-prefix=ONE
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=12 %s 2>&1 | FileCheck %s --check-prefix=ONE
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=14 %s 2>&1 | FileCheck %s --check-prefix=ONE
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -DCASE=13 %s 2>&1 | FileCheck %s --check-prefix=TWO

// P2758R5 std::constexpr_error_str: the call is constexpr-erroneous, evaluation
// continues, and the program is ill-formed only when it is evaluated in a
// manifestly constant-evaluated context. One case per context (see #541). A
// context whose constant expression the compiler evaluates twice (if constexpr,
// a nested requirement) issues the message twice.
#include <debugging>
#include <type_traits>

constexpr int bad() { std::constexpr_error_str("tag", "boom"); return 1; }
constexpr int never() { std::constexpr_error_str("tag", "always"); return 0; }

#if CASE == 1 // initializer of a constexpr variable: error
constexpr int x = bad();
#elif CASE == 2 // call at run time: no effect
int main() { return bad(); }
#elif CASE == 3 // if constexpr condition: error
int f() { if constexpr (bad() == 1) return 1; else return 2; }
#elif CASE == 4 // unevaluated operand of noexcept: no evaluation
static_assert(noexcept(bad()) == false);
#elif CASE == 5 // a function that always reports is still a constexpr function
int main() { return never(); }
#elif CASE == 6 // unevaluated decltype operand (SFINAE): no evaluation
template <int N> auto g(int) -> decltype(bad(), 1) { return 1; }
int h() { return g<0>(0); }
#elif CASE == 7 // requires-expression operand is unevaluated
template <class T> concept C = requires { bad(); };
static_assert(C<int>);
#elif CASE == 8 // __builtin_constant_p is not a manifestly constant-evaluated context
constexpr bool p = __builtin_constant_p(bad());
#elif CASE == 9 // a run-time evaluation of a guarded call has no effect
constexpr int k() {
  if (std::is_constant_evaluated()) return 1;
  std::constexpr_error_str("t", "rt");
  return 2;
}
int main() { return k(); }
#elif CASE == 10 // non-type template argument: error
template <int N> struct S {};
S<bad()> s;
#elif CASE == 11 // nested requirement is evaluated: error
template <class T> concept C = requires { requires (bad() == 1); };
static_assert(C<int>);
#elif CASE == 12 // consteval call: error
consteval int ce() { return bad(); }
int main() { return ce(); }
#elif CASE == 13 // every call of one evaluation is issued
constexpr int two() {
  std::constexpr_error_str("first", "one");
  std::constexpr_error_str("second", "two");
  std::constexpr_warning_str("w", "warn after");
  return 3;
}
constexpr int y = two();
#elif CASE == 14 // evaluation continues after the call
constexpr int f() { std::constexpr_error_str("t", "m"); return 7; }
constexpr int g = (f(), 0);
#endif

// ONE: error: constexpr message with tag '{{[a-z]+}}': {{[a-z]+}}
// ONE: {{[12]}} error{{s?}} generated.
// TWO: error: constexpr message with tag 'first': one
// TWO: error: constexpr message with tag 'second': two
// TWO: warning: constexpr message with tag 'w': warn after
// TWO: 1 warning and 2 errors generated.
