// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify -fexceptions -fcxx-exceptions

// cwg1669: yes
auto main() { return 0; }
// expected-error@-1 {{'main' must return 'int'}}
