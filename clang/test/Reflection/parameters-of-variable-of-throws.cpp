// RUN: %clang_cc1 %s -std=c++26 -freflection -fcxx-exceptions -fexceptions -Wall -Wextra -Werror -verify
// expected-no-diagnostics

using info = decltype(^^int);

// Minimal library entry point used by the evaluator to construct exceptions.
namespace std::meta {
struct exception {
  info from;
  const char *what;
};
constexpr info __make_exception(info from, const char *what) {
  throw exception{from, what};
}
}

struct sentinel {};
consteval info parameter(info r, unsigned index = 0) {
  return __metafunction(107, r, ^^sentinel, index, ^^parameter);
}
consteval info variable(info r) {
  return __metafunction(113, r, ^^variable);
}
constexpr info parameter_origin = ^^parameter;
constexpr info variable_origin = ^^variable;

consteval bool parameters_throw(info r) {
  try {
    (void)parameter(r);
  } catch (const std::meta::exception &e) {
    return e.from == parameter_origin && e.what[0] != '\0';
  }
  return false;
}
consteval bool variable_throws(info r) {
  try {
    (void)variable(r);
  } catch (const std::meta::exception &e) {
    return e.from == variable_origin && e.what[0] != '\0';
  }
  return false;
}

namespace ns {}
template <class> struct tmpl {};
struct S { int member; void fn(int); };
int object;
void fn(int);
template <class T> void tfn(T);
constexpr auto lambda = [](int) {};
using Closure = decltype(lambda);
using Fn = void(int);

static_assert(parameters_throw(info{}));
static_assert(parameters_throw(^^::));
static_assert(parameters_throw(^^ns));
static_assert(parameters_throw(^^tmpl));
static_assert(parameters_throw(^^tfn));
static_assert(parameters_throw(^^object));
static_assert(parameters_throw(^^S::member));
static_assert(parameters_throw(parameter(^^fn)));
static_assert(parameters_throw(^^int));
static_assert(parameters_throw(^^S));

static_assert(variable_throws(info{}));
static_assert(variable_throws(^^::));
static_assert(variable_throws(^^ns));
static_assert(variable_throws(^^tmpl));
static_assert(variable_throws(^^tfn));
static_assert(variable_throws(^^object));
static_assert(variable_throws(^^S::member));
static_assert(variable_throws(parameter(^^fn)));
static_assert(variable_throws(^^int));
static_assert(variable_throws(^^S));
static_assert(variable_throws(^^fn));
static_assert(variable_throws(^^tfn<int>));
static_assert(variable_throws(^^Closure::operator()));
static_assert(variable_throws(^^S::fn));
static_assert(variable_throws(^^Fn));

static_assert(parameter(^^fn) != ^^sentinel);
static_assert(parameter(^^tfn<int>) != ^^sentinel);
static_assert(parameter(^^Closure::operator()) != ^^sentinel);
static_assert(parameter(^^S::fn) != ^^sentinel);
static_assert(parameter(^^Fn) == ^^int);
static_assert(parameter(^^Fn, 1) == ^^sentinel);

consteval bool inside([[maybe_unused]] int c) {
  try {
    return variable(parameter(^^inside)) == ^^c;
  } catch (const std::meta::exception &) {
    return false;
  }
}
static_assert(inside(0));
consteval bool nested([[maybe_unused]] int c) {
  return []() consteval {
    try {
      (void)variable(parameter(^^nested));
    } catch (const std::meta::exception &e) {
      return e.from == variable_origin && e.what[0] != '\0';
    }
    return false;
  }();
}
static_assert(nested(0));

// Helpers preserve the evaluation context of the static_assert in g.
consteval bool variable_throws_twice(info r) {
  return variable_throws(r);
}
void g([[maybe_unused]] int c, [[maybe_unused]] int d) {
  constexpr auto p = parameter(^^g);
  static_assert(!variable_throws(p));
  static_assert(!variable_throws_twice(p));
  static_assert([](info r) consteval { return !variable_throws(r); }(p));
  static_assert(variable(p) == ^^c);
  static_assert([]() consteval { return !variable_throws_twice(p); }());
}
static_assert(variable_throws(parameter(^^g)));
static_assert(variable_throws_twice(parameter(^^g)));
