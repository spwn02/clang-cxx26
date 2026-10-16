// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=cxx26 %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=cxx26 -fexperimental-new-constant-interpreter -DNO_EVAL %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx23 %s

// A throw-expression and a try-block in a constexpr or consteval function are part of constant evaluation in C++26
// (P3068R6): they are accepted when the exception handling of the run time is disabled. Elsewhere they are still
// rejected.

// cxx26-no-diagnostics

struct E { int v; };

consteval int f(int x) {
  if (x < 0)
    throw E{x}; // cxx23-error {{cannot use 'throw' with exceptions disabled}}
  return x;
}

consteval int g(int x) {
  try { // cxx23-error {{cannot use 'try' with exceptions disabled}}
    return f(x);
  } catch (E) {
    return -7;
  }
}

constexpr int h(int x) {
  if (x < 0)
    throw E{x}; // cxx23-error {{cannot use 'throw' with exceptions disabled}}
  return x;
}

// A lambda in a constexpr function belongs to it.
constexpr int k(int x) {
  auto l = [&] {
    try { // cxx23-error {{cannot use 'try' with exceptions disabled}}
      return h(x);
    } catch (E) {
      return -1;
    }
  };
  return l();
}

#ifndef NO_EVAL // the bytecode interpreter does not evaluate throw
// A lambda in the initializer of a constexpr variable is evaluated at compile time.
consteval void thrower() { throw E{1}; } // cxx23-error {{cannot use 'throw' with exceptions disabled}}
constexpr bool in_variable = [] {
  try { // cxx23-error {{cannot use 'try' with exceptions disabled}}
    thrower();
  } catch (E &) {
    return true;
  }
  return false;
}();
#endif

template <class T> constexpr int tf(T x) {
  try { // a dependent context is not diagnosed before an instantiation
    if (x < 0)
      throw E{};
    return x;
  } catch (E &) {
    return -1;
  }
}

#if __cplusplus >= 202400L && !defined(NO_EVAL) // the bytecode interpreter does not evaluate throw
static_assert(g(-1) == -7);
static_assert(g(3) == 3);
static_assert(k(-2) == -1);
static_assert(in_variable);
static_assert(tf(-3) == -1);
#endif
