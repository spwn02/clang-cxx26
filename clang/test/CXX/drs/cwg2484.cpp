// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-no-diagnostics

namespace cwg2484 { // cwg2484: yes
#if __cplusplus >= 202002L
namespace p1 {
template<class A, class B> inline constexpr bool same = __is_same(A, B);
static_assert(same<decltype(+char8_t{}), int>);
static_assert(same<decltype(+char16_t{}), int>);
static_assert(same<decltype(+char32_t{}), unsigned>);
static_assert(same<decltype(+wchar_t{}), int>);
}
#endif
} // namespace cwg2484
