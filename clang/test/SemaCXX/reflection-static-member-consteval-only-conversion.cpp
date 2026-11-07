// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s

// expected-no-diagnostics

// A static constexpr data member is initialized inside the evaluation context
// its initializer was parsed in, so an implicit conversion through a consteval
// constructor is an immediate invocation of that context (#503).

struct Info {
  decltype(^^int) info = {};
  bool empty = false;
  consteval Info(decltype(^^int) i = {}, bool e = false) : info(i), empty(e) {}
};

struct Implicit { static constexpr Info e = ^^long; };
struct Explicit { static constexpr Info e = Info(^^long); };
struct Braced { static constexpr Info e{^^long}; };
struct Array { static constexpr Info e[] = {{^^int, true}, ^^long}; };

template <class T> struct Dependent {
  static constexpr Info e[] = {{^^int, true}, ^^T};
};

constexpr Info ns = ^^long;

static_assert(Array::e[0].empty && !Array::e[1].empty);
static_assert(Dependent<char>::e[1].info == ^^char);
