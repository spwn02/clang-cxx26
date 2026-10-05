// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=pre26 %s

// [over.inc.default], P3668R4: defaulted postfix increment and decrement operators.

// The overload candidates listed with a use of a deleted or incompatible operator:
// expected-note@* 0+ {{candidate function}}
// expected-note@* 0+ {{forward declaration of}}


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


// [dcl.fct.def.default]: a function explicitly defaulted on its first declaration is implicitly inline and implicitly
// constexpr if it is constexpr-suitable.
struct IC {
  int v = 0;
  constexpr IC& operator++() { ++v; return *this; }
  IC operator++(int) = default;
};
static_assert([] { IC i; IC o = i++; return o.v == 0 && i.v == 1; }());
struct NC {
  int v = 0;
};
constexpr NC& operator++(NC& n) { ++n.v; return n; }
NC operator++(NC&, int) = default;
static_assert([] { NC n; NC o = n++; return o.v == 0 && n.v == 1; }());
// not constexpr-suitable: the prefix operator is not constexpr, so the function is not implicitly constexpr
struct NS {
  int v = 0;
  NS& operator++() { ++v; return *this; }
  NS operator++(int) = default; // expected-note {{declared here}}
};
constexpr bool use_ns() { NS n; NS o = n++; return o.v == 0; } // expected-note {{non-constexpr function 'operator++' cannot be used in a constant expression}}
static_assert(use_ns()); // expected-error {{static assertion expression is not an integral constant expression}} \
                         // expected-note {{in call to 'use_ns()'}}

// An explicit object parameter.
struct XO {
  int v = 0;
  constexpr XO& operator++() { ++v; return *this; }
  constexpr XO operator++(this XO& self, int) = default;
};
static_assert([] { XO x; XO o = x++; return o.v == 0 && x.v == 1; }());
struct XV {
  XV& operator++();
  XV operator++(this XV self, int) = default; // expected-error {{invalid first parameter type}}
};

// Why the function is deleted: the copy, the destructor or the prefix operator.
struct NoCopy {
  NoCopy(const NoCopy&) = delete;
  NoCopy& operator++();
  NoCopy operator++(int) = default;
  // expected-note@-1 {{explicitly defaulted function was implicitly deleted here}}
  // expected-note@-2 {{implicitly deleted because 'NoCopy' cannot be copy-initialized from an lvalue}}
};
struct DeletedDtor {
  DeletedDtor(const DeletedDtor&) = default;
  ~DeletedDtor() = delete;
  DeletedDtor& operator++();
  DeletedDtor operator++(int) = default;
  // expected-note@-1 {{explicitly defaulted function was implicitly deleted here}}
  // expected-note@-2 {{implicitly deleted because 'DeletedDtor' has a deleted or inaccessible destructor}}
};
struct NoPrefix {
  NoPrefix operator++(int) = default;
  // expected-note@-1 {{explicitly defaulted function was implicitly deleted here}}
  // expected-note@-2 {{implicitly deleted because the prefix increment operator is not usable on an lvalue of type 'NoPrefix'}}
};
struct NoPrefixDec {
  NoPrefixDec operator--(int) = default;
  // expected-note@-1 {{explicitly defaulted function was implicitly deleted here}}
  // expected-note@-2 {{implicitly deleted because the prefix decrement operator is not usable on an lvalue of type 'NoPrefixDec'}}
};
void explain() {
  NoCopy (NoCopy::*a)(int) = &NoCopy::operator++;                   // expected-error {{attempt to use a deleted function}}
  DeletedDtor (DeletedDtor::*b)(int) = &DeletedDtor::operator++;    // expected-error {{attempt to use a deleted function}}
  NoPrefix (NoPrefix::*d)(int) = &NoPrefix::operator++;             // expected-error {{attempt to use a deleted function}}
  NoPrefixDec (NoPrefixDec::*e)(int) = &NoPrefixDec::operator--;    // expected-error {{attempt to use a deleted function}}
}
void calls(NoCopy& a, DeletedDtor& b) {
  a.operator++(0); // expected-error {{call to deleted member function 'operator++'}}
  b.operator++(0); // expected-error {{call to deleted member function 'operator++'}}
}

// A non-member cannot use a private destructor; a member can (access is checked from the function body).
class PrivateDtor {
  PrivateDtor(const PrivateDtor&) = default;
public:
  PrivateDtor() = default;
private:
  ~PrivateDtor() = default;
  friend PrivateDtor& operator++(PrivateDtor&);
};
PrivateDtor& operator++(PrivateDtor&);
PrivateDtor operator++(PrivateDtor&, int) = default; // OK, defined as deleted
// expected-note@-1 {{explicitly defaulted function was implicitly deleted here}}
// expected-note@-2 {{implicitly deleted because 'PrivateDtor' has a deleted or inaccessible destructor}}
void use_private_dtor() { PrivateDtor (*p)(PrivateDtor&, int) = &operator++; } // expected-error {{attempt to use a deleted function}}

// Access is checked from the function body.
class PrivCopy {
  PrivCopy(const PrivCopy&) = default;
public:
  PrivCopy() = default;
  PrivCopy& operator++();
  PrivCopy operator++(int) = default;                  // a member can use the private copy constructor
  friend PrivCopy& operator--(PrivCopy&);
  friend PrivCopy operator--(PrivCopy&, int) = default; // so can a friend
};
class PrivCopy2 {
  PrivCopy2(const PrivCopy2&) = default;
public:
  PrivCopy2() = default;
};
PrivCopy2& operator++(PrivCopy2&);
PrivCopy2 operator++(PrivCopy2&, int) = default; // OK, defined as deleted: a non-member cannot copy
// expected-note@-1 {{explicitly defaulted function was implicitly deleted here}}
// expected-note@-2 {{implicitly deleted because 'PrivCopy2' cannot be copy-initialized from an lvalue}}
void access(PrivCopy& p) {
  p.operator++(0);
  operator--(p, 0);
  PrivCopy2 (*q)(PrivCopy2&, int) = &operator++; // expected-error {{attempt to use a deleted function}}
}

// A function defaulted after its first declaration is user-provided: it is defined there and must not be deleted.
struct OutOfLine {
  OutOfLine& operator++();
  OutOfLine operator++(int);
};
OutOfLine OutOfLine::operator++(int) = default;
struct OutOfLineDeleted {
  OutOfLineDeleted(const OutOfLineDeleted&) = delete;
  OutOfLineDeleted& operator++();
  OutOfLineDeleted operator++(int);
};
OutOfLineDeleted OutOfLineDeleted::operator++(int) = default; // expected-error {{would delete it}} // expected-note {{cannot be copy-initialized}}

#else

// Before C++26, only special member functions and comparison operators may be defaulted.
struct P {
  P& operator++();
  P operator++(int) = default; // pre26-error {{only special member functions and comparison operators may be defaulted}}
};

#endif
