// RUN: %clang_cc1 -std=c++2d -fsyntax-only -fcxx-exceptions -Wdiverging-compat -verify %s
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -fcxx-exceptions -verify=quiet %s
// quiet-no-diagnostics

// P3549: -Wdiverging-compat (off by default) points at the places whose type
// changes once the bottom type std::noreturn_t exists.

void f() { throw 0; }
auto deduced() { throw 0; }  // expected-warning {{return type of function deduced as 'std::noreturn_t'; it was 'void' before P3549}}
auto lambda = [] { throw 0; }; // expected-warning {{return type of lambda deduced as 'std::noreturn_t'; it was 'void' before P3549}}

using T = decltype(throw 0);  // expected-warning {{'decltype' of a throw-expression is 'std::noreturn_t'; it was 'void' before P3549}}

void comma(bool b) {
  (void)(throw 0, b);         // expected-warning {{comma expression has type 'std::noreturn_t'; it had type 'bool' before P3549}}
  (void)(b ? throw 1 : throw 2); // expected-warning {{conditional expression has type 'std::noreturn_t'; it had type 'void' before P3549}}
}
