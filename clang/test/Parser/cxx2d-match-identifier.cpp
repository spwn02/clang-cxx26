// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify %s

// expected-no-diagnostics
// `match` begins a selection only where P2688R6 says so; everywhere else it is an
// ordinary identifier, in every mode.

void match(int);
struct matcher { void match(int); int match2; };
namespace ns { int match = 3; }
struct match_t {};
int f(matcher &m) {
  match(1);
  m.match(2);
  int r = ns::match + 1;
  auto match = [](int x) { return x; };
  r += match(3);
  return r;
}
int g() {
  int match = 4;
  match = 5;
  match++;
  return match;
}
template <class T> auto h(T t) { return match(t); }
