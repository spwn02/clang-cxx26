// RUN: %clang_cc1 -std=c++98 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions

namespace cwg2616 { // cwg2616: yes
namespace p1 {
void f(){ for(;;){ switch(0){ case 0: continue; } } }
}
namespace p2 {
void f(){ break; }
// expected-error@-1 {{}}
// expected-note@*:* 0+ {{}}
}
namespace p3 {
void f(){ switch(0){ case 0: continue; } }
// expected-error@-1 {{}}
// expected-note@*:* 0+ {{}}
}
} // namespace cwg2616
