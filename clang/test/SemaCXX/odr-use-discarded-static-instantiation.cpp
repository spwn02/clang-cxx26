// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify -Wno-unused-value %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify -Wno-unused-value %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -Wno-unused-value %s

// [basic.def.odr]: a variable of non-reference type named by a potential result of a
// discarded-value expression to which the lvalue-to-rvalue conversion is not applied is
// not odr-used, so the definition of a static data member or variable template
// specialization need not exist and is not implicitly instantiated.

template <class T> struct Never { static int q; };
template <class T> int Never<T>::q = T::nope; // never instantiated

template <class T> inline int vt = T::nope; // never instantiated

void discarded() {
  Never<int>::q;
  (void)Never<int>::q;
  static_cast<void>(Never<int>::q);
  (void)(Never<int>::q, 0);
  (Never<int>::q, 0);
  (void)(true ? Never<int>::q : Never<int>::q);
  vt<int>;
  (void)vt<int>;
}

// Every kind of odr-use still instantiates the definition.
template <class T> struct Used { static int q; };
template <class T> int Used<T>::q = T::nope; // #q

int read() { return Used<int>::q; } // expected-note {{in instantiation of static data member 'Used<int>::q' requested here}} expected-error@#q {{type 'int' cannot be used prior to '::' because it has no members}}

template <class T> struct Ref { static int q; };
template <class T> int Ref<T>::q = T::nope; // #r
void bind() { int &r = Ref<int>::q; (void)r; } // expected-error@#r {{type 'int' cannot be used prior to '::' because it has no members}} expected-note {{in instantiation of static data member 'Ref<int>::q' requested here}}

// A discarded use next to an odr-use: the definition is instantiated (once).
template <class T> struct Both { static int q; };
template <class T> int Both<T>::q = T::nope; // #b
void both() { Both<int>::q; int &r = Both<int>::q; (void)r; } // expected-error@#b {{type 'int' cannot be used prior to '::' because it has no members}} expected-note {{in instantiation of static data member 'Both<int>::q' requested here}}

// A volatile glvalue is read when discarded ([expr.context]): odr-use.
template <class T> struct Vol { static volatile int q; };
template <class T> volatile int Vol<T>::q = T::nope; // #v
void vol() { Vol<int>::q; } // expected-error@#v {{type 'int' cannot be used prior to '::' because it has no members}} expected-note {{in instantiation of static data member 'Vol<int>::q' requested here}}

// The same inside a lambda body.
template <class T> struct InLambda { static int q; };
template <class T> int InLambda<T>::q = T::nope;
void lambda() { auto l = [] { InLambda<int>::q; (void)InLambda<int>::q; }; (void)l; }

// A discarded volatile local is read, so it is odr-used and must be captured.
void volatile_local() {
  volatile int x = 1;
  auto l = [] { x; }; // expected-error {{variable 'x' cannot be implicitly captured in a lambda with no capture-default specified}} expected-note@-1 {{'x' declared here}} expected-note {{lambda expression begins here}} expected-note {{capture 'x' by value}} expected-note {{capture 'x' by reference}} expected-note {{default capture by value}} expected-note {{default capture by reference}}
  (void)l;
}

// A const-qualified variable that is not constexpr is not needed for constant
// evaluation either: a discarded use does not instantiate it.
template <class T> struct ConstNotConstexpr { static const double v; };
template <class T> const double ConstNotConstexpr<T>::v = T::nope;
void const_double() { ConstNotConstexpr<int>::v; (void)ConstNotConstexpr<int>::v; }

// Inside templates: instantiating the function does not instantiate the variable.
template <class T> struct InTemplate { static int q; };
template <class T> int InTemplate<T>::q = T::nope;
template <class T> void discarded_in_template() { InTemplate<T>::q; (void)InTemplate<T>::q; }
template void discarded_in_template<int>();
void generic_lambda() {
  auto l = [](auto) { InTemplate<int>::q; (void)InTemplate<int>::q; };
  l(1);
}
