// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -fcxx-exceptions %s

// P3068R6: allow throwing (and catching) exceptions during constant
// evaluation, as long as the thrown object is destroyed within the
// evaluation -- [expr.const.core]p2.24-2.25.

// Basic throw/catch by value.
constexpr int f1(int x) {
  try {
    if (x < 0) throw 1;
    return x;
  } catch (int) {
    return -1;
  }
}
static_assert(f1(5) == 5);
static_assert(f1(-5) == -1);

// Uncaught exception still makes the evaluation ill-formed.
constexpr int f2() {
  throw 1; // expected-note {{exception thrown here was not caught}}
  return 0;
}
static_assert(f2() == 0); // expected-error {{not an integral constant expression}} \
                          // expected-note {{in call to 'f2()'}}

// catch(...) matches anything.
constexpr int f3(int x) {
  try {
    if (x < 0) throw 1.5;
    return x;
  } catch (...) {
    return -2;
  }
}
static_assert(f3(5) == 5);
static_assert(f3(-5) == -2);

// Base/derived matching, catch by reference, and slicing.
struct Base { int tag = 1; };
struct Derived : Base { int tag2 = 2; };

constexpr int f4(int x) {
  try {
    if (x == 0) throw Derived{};
    return x;
  } catch (Base &b) {
    return b.tag * 100;
  }
}
static_assert(f4(0) == 100);
static_assert(f4(7) == 7);

constexpr int f5() {
  try {
    throw Derived{};
  } catch (Base b) {
    return b.tag * 10;
  }
}
static_assert(f5() == 10);

// Nested try/catch: an inner handler that doesn't match lets the exception
// propagate to an outer one.
constexpr int f6(int x) {
  try {
    try {
      if (x < 0) throw 1.0;
      return x;
    } catch (int) {
      return -1;
    }
  } catch (double) {
    return -2;
  }
}
static_assert(f6(5) == 5);
static_assert(f6(-5) == -2);

// Cross-function propagation: an exception thrown in a callee, uncaught
// there, is caught by the caller's try.
constexpr int callee(int x) {
  if (x < 0) throw 42;
  return x;
}
constexpr int f7(int x) {
  try {
    return callee(x);
  } catch (int e) {
    return e;
  }
}
static_assert(f7(5) == 5);
static_assert(f7(-5) == 42);

// Destructor-during-unwind: scope destructors must still run while an
// exception unwinds through them.
struct Counter {
  int &n;
  constexpr Counter(int &n) : n(n) {}
  constexpr ~Counter() { n = n + 1; }
};

constexpr int f8() {
  int n = 0;
  try {
    Counter c1(n);
    {
      Counter c2(n);
      throw 1;
    }
  } catch (int) {
    // Both c1 and c2 should have been destroyed during unwind.
  }
  return n;
}
static_assert(f8() == 2);

// Multiple handlers: the first matching one wins, in declaration order.
constexpr int f9(int x) {
  try {
    throw x;
  } catch (double) {
    return 1;
  } catch (int) {
    return 2;
  } catch (...) {
    return 3;
  }
}
static_assert(f9(0) == 2);

// Bare `throw;` (rethrow) is deliberately unsupported: rejecting a valid
// program is a safe under-approximation.
constexpr int f10() {
  try {
    throw 1;
  } catch (int) {
    throw; // expected-note {{subexpression not valid in a constant expression}}
  }
  return 0;
}
static_assert(f10() == 0); // expected-error {{not an integral constant expression}} \
                           // expected-note {{in call to 'f10()'}}

// catch(...) as a genuine fallback after real handlers don't match.
struct A {};
struct B {};
constexpr int f11(int x) {
  try {
    if (x == 0) throw A{};
    throw B{};
  } catch (A) {
    return 100;
  } catch (...) {
    return 200;
  }
}
static_assert(f11(0) == 100);
static_assert(f11(1) == 200);

// Pointer-to-base exception types are not matched (safe under-approximation):
// treated as "does not catch", so the exception keeps propagating uncaught
// rather than being wrongly caught.
struct PBase {};
struct PDerived : PBase {};
constexpr int f12() {
  PDerived d;
  try {
    throw &d; // expected-note {{exception thrown here was not caught}}
  } catch (PBase *) {
    return 1;
  }
  return 0;
}
static_assert(f12() == 1); // expected-error {{not an integral constant expression}} \
                           // expected-note {{in call to 'f12()'}}

// Throw from inside a loop body, caught outside the loop.
constexpr int loopThrow(int n) {
  try {
    for (int i = 0; i < n; ++i) {
      if (i == 3) throw i;
    }
    return -1;
  } catch (int caught) {
    return caught;
  }
}
static_assert(loopThrow(10) == 3);
static_assert(loopThrow(2) == -1);

// A new exception thrown from inside a catch handler, caught by an outer try.
constexpr int nestedHandlerThrow(int x) {
  try {
    try {
      throw 1;
    } catch (int) {
      throw 2.0; // a different exception, thrown while handling the first
    }
  } catch (double d) {
    return static_cast<int>(d) * 100;
  }
  return -1;
}
static_assert(nestedHandlerThrow(0) == 200);
