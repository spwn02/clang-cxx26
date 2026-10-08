// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// P4004R1 (Reconsider CWG1395, applied to C++26): partial ordering of function templates does not compare a function
// parameter pack of the argument template with each remaining parameter of the parameter template; only the trailing
// pack tie-breaker of [temp.deduct.partial]p11 remains. Existing practice, which this checks.

template <class A, class B> struct is_same { static const bool value = false; };
template <class A> struct is_same<A, A> { static const bool value = true; };

namespace first_example {
template <typename... T> char *f(T &...); // #1
template <typename T> int *f(T &&);       // #2
int i;
static_assert(is_same<decltype(f(i)), int *>::value, "the non-variadic template is preferred");
} // namespace first_example

namespace cwg1825 {
template <class... T> int f(T *...) { return 1; }  // #1
template <class T> int f(const T &) { return 2; }  // #2
int j = f((int *)0);                               // expected-error {{call to 'f' is ambiguous}}
                                                   // expected-note@-3 {{candidate function [with T = <int>]}}
                                                   // expected-note@-3 {{candidate function [with T = int *]}}
} // namespace cwg1825

namespace temp_deduct_partial_example {
template <class... Args> void f(Args... args);                // #1
template <class T1, class... Args> void f(T1 a1, Args... args); // #2
template <class T1, class T2> void f(T1 a1, T2 a2);           // #3
void test() {
  f();        // #1
  f(1, 2, 3); // #2
  f(1, 2);    // #3
}
} // namespace temp_deduct_partial_example

namespace cwg3154 {
template <typename... T> void f(T *...); // #1
template <typename U> void f(U, U);      // #2
void test() { f((int *)0, (int *)0); }   // expected-error {{call to 'f' is ambiguous}}
                                         // expected-note@-3 {{candidate function [with T = <int, int>]}}
                                         // expected-note@-3 {{candidate function [with U = int *]}}
} // namespace cwg3154
