// RUN: %clang_cc1 -std=c++98 -triple x86_64-unknown-unknown %s -verify=expected,cxx98-17 -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++11 -triple x86_64-unknown-unknown %s -verify=expected,cxx98-17,since-cxx11 -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++14 -triple x86_64-unknown-unknown %s -verify=expected,cxx98-17,since-cxx11 -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++17 -triple x86_64-unknown-unknown %s -verify=expected,cxx98-17,since-cxx11 -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-unknown %s -verify=expected,since-cxx20,since-cxx11 -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++23 -triple x86_64-unknown-unknown %s -verify=expected,since-cxx20,since-cxx11 -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++2c -triple x86_64-unknown-unknown %s -verify=expected,since-cxx20,since-cxx11 -fexceptions -fcxx-exceptions -pedantic-errors

namespace cwg820 { // cwg820: 2.7
export template <class T> struct B {};
// cxx98-17-warning@-1 {{exported templates are unsupported}}
// since-cxx20-error@-2 {{export declaration can only be used within a module interface}}
export template<typename T> void f() {}
// cxx98-17-warning@-1 {{exported templates are unsupported}}
// since-cxx20-error@-2 {{export declaration can only be used within a module interface}}
} // namespace cwg820

namespace cwg873 { // cwg873: 3.0
#if __cplusplus >= 201103L
template <typename T> void f(T &&);
template <> void f(int &) = delete;  // #cwg873-lvalue-ref
template <> void f(int &&) = delete; // #cwg873-rvalue-ref
void g(int i) {
  f(i); // calls f<int&>(int&)
  // since-cxx11-error@-1 {{call to deleted function 'f'}}
  //   since-cxx11-note@#cwg873-lvalue-ref {{candidate function [with T = int &] has been implicitly deleted}}
  f(0); // calls f<int>(int&&)
  // since-cxx11-error@-1 {{call to deleted function 'f'}}
  //   since-cxx11-note@#cwg873-rvalue-ref {{candidate function [with T = int] has been implicitly deleted}}
}
#endif
} // namespace cwg873

// cwg882: 3.5
#if __cplusplus >= 201103L
int main() = delete;
// since-cxx11-error@-1 {{'main' is not allowed to be deleted}}
#endif
// cwg800: na
// cwg807: na
// cwg819: na
// cwg822: na
// cwg829: na
// cwg836: na
// cwg839: na
// cwg885: na
// cwg893: na
// cwg831: na
// cwg816: na
// cwg853: na
// cwg883: na
// cwg828: na
// cwg830: na
// cwg832: na
// cwg834: na
// cwg872: na
// cwg847: na
// cwg855: na
// cwg858: na

namespace cwg891 { // cwg891: 22
#if __cplusplus >= 201103L
typedef int *A[3];
typedef const int *const CA[3];
auto &&r2 = const_cast<A &&>(CA{});
struct S {};
const S f();
S &&r3 = const_cast<S &&>(f());
#endif
} // namespace cwg891
// cwg805: na
// cwg808: na
// cwg811: na
// cwg845: na

// cwg887: sup 2775
// cwg814: na
// cwg869: na
