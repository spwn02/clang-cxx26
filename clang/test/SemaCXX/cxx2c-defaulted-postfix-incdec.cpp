// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=pre26 %s

// [over.inc.default], P3668R4: defaulted postfix increment and decrement operators.

// expected-note@* 0+ {{}}


#if __cplusplus > 202302L

// A member with a user-declared prefix operator.
struct I {
  int v = 0;
  constexpr I& operator++() { ++v; return *this; }
  constexpr I operator++(int) = default;
  constexpr I& operator--() { --v; return *this; }
  constexpr I operator--(int) = default;
};

constexpr bool test_member() {
  I i;
  I a = i++;
  I b = i--;
  I c = ++i;
  return a.v == 0 && b.v == 1 && c.v == 1 && i.v == 1;
}
static_assert(test_member());

// A non-member function, defaulted where the class is complete.
struct N {
  int v = 5;
};
constexpr N& operator++(N& n) { ++n.v; return n; }
constexpr N& operator--(N& n) { --n.v; return n; }
constexpr N operator++(N&, int) = default;
constexpr N operator--(N&, int) = default;

constexpr bool test_nonmember() {
  N n;
  N a = n++;
  N b = n--;
  return a.v == 5 && b.v == 6 && n.v == 5;
}
static_assert(test_nonmember());

// A friend defined in the class.
struct F {
  int v = 1;
  constexpr F& operator++() { ++v; return *this; }
  friend constexpr F operator++(F&, int) = default;
};
static_assert([] { F f; F old = f++; return old.v == 1 && f.v == 2; }());

// volatile operand.
struct V {
  int v = 0;
  V() = default;
  V(const V&) = default;
  V& operator++() volatile;
  V operator++(int) volatile = default; // implicitly deleted: no copy from a volatile lvalue
};
void use_volatile(volatile V& x) {
  x++; // expected-error {{overload resolution selected deleted operator '++'}}
}

// The standard's examples.
struct S;
S operator++(S&, int) = default; // expected-error {{incomplete result type 'S' in function definition}}
struct S0 {
  S0(const S0&) = default;
  S0& operator++() { return *this; }
};
S0 operator++(S0, int) = default; // expected-error {{invalid first parameter type}}
struct T {
  T operator++(int) = default; // OK, defined as deleted
};
void use_deleted(T t) {
  t++; // expected-error {{overload resolution selected deleted operator '++'}}
}
enum class E1 {};
E1 operator++(E1&, int) = default; // OK, defined as deleted; there is no built-in operator++ for enumeration types
enum E2 {};
E2& operator++(E2& e);
E2 operator++(E2&, int) = default; // OK, not deleted
void use_enums(E1 a, E2 b) {
  a++; // expected-error {{overload resolution selected deleted operator '++'}}
  b++;
}

// Other requirements.
struct Ret {
  Ret& operator++();
  void operator++(int) = default; // expected-error {{must return 'Ret'}}
};
struct ConstObj {
  ConstObj& operator++();
  ConstObj operator++(int) const = default; // expected-error {{invalid first parameter type}}
};
struct DefaultArg {
  DefaultArg& operator++();
  DefaultArg operator++(int = 0) = default; // expected-error {{cannot have a default argument}}
};
struct NotFirst {
  NotFirst& operator++();
  friend NotFirst operator++(NotFirst&, int);
  friend NotFirst operator++(NotFirst&, int) = default; // expected-error {{must be the first declaration}}
};
template <class U> struct Templ {
  Templ& operator++() { return *this; }
  Templ operator++(int) = default;
};
static_assert(sizeof(Templ<int>) == 1);
void use_templ() { Templ<int> t; t++; }
struct FuncTemplate {
  FuncTemplate& operator++();
  template <class U> FuncTemplate operator++(U) = default; // expected-error {{template cannot be defaulted}}
};

#else

// Before C++26, only special member functions and comparison operators may be defaulted.
struct P {
  P& operator++();
  P operator++(int) = default; // pre26-error {{only special member functions and comparison operators may be defaulted}}
};

#endif
