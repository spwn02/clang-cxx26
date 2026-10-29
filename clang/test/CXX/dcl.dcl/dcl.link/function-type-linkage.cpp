// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify -verify-ignore-unexpected=note %s

// [dcl.link]p1: two function types with different language linkages are
// distinct types even if they are otherwise identical.

template <typename T, typename U> struct same { static constexpr bool value = false; };
template <typename T> struct same<T, T> { static constexpr bool value = true; };

extern "C" typedef void CFN(int);
typedef void CXXFN(int);
static_assert(!same<CFN, CXXFN>::value, "");
static_assert(!same<CFN *, CXXFN *>::value, "");

// [dcl.link]p5 example.
extern "C"                      // f1 and its function type have C language linkage
  void f1(void(*pf)(int));      // pf is a pointer to a C function // #f1

extern "C" typedef void FUNC();
FUNC f2;                        // f2 has C++ language linkage and its type has C language linkage
extern "C" FUNC f3;             // f3 and its type have C language linkage

void (*pf2)(FUNC*);             // pointer to C++ function taking a pointer to C function

static_assert(same<decltype(f2), FUNC>::value, "");
static_assert(same<decltype(f3), FUNC>::value, "");
static_assert(!same<decltype(f2), void()>::value, "");

void cxx_cb(int);
extern "C" void c_cb(int);

void use() {
  f1(c_cb);
  f1(cxx_cb);
  // expected-error@-1 {{converting 'void (*)(int)' to 'extern "C" void (*)(int)' changes the language linkage of the function type}}
  CFN *pc = c_cb;
  CFN *pc2 = cxx_cb;
  // expected-error@-1 {{converting 'void (*)(int)' to 'CFN *' (aka 'extern "C" void (*)(int)') changes the language linkage of the function type}}
  CXXFN *px = cxx_cb;
  CXXFN *px2 = c_cb;
  // expected-error@-1 {{converting 'extern "C" void (*)(int)' to 'CXXFN *' (aka 'void (*)(int)') changes the language linkage of the function type}}
  CXXFN *px3 = reinterpret_cast<CXXFN *>(c_cb);
  (void)pc; (void)pc2; (void)px; (void)px2; (void)px3;
}

// [dcl.link]p5 example, static function with internal linkage.
extern "C" {
  static void f4();             // f4 has no language linkage; its type has C language linkage
}
extern "C" void f5() {
  extern void f4();             // OK, type linkage obtained from the previous declaration
}
extern void f4();               // OK
void f6() {
  extern void f4();             // OK
}
static_assert(same<decltype(f4), FUNC>::value, "");

// [dcl.link]p6 example: C language linkage is ignored for the function type of
// non-static class member functions.
extern "C" typedef void FUNC_c();
class C {
public:
  void mf1(FUNC_c *);           // the parameter has type pointer to C function
  FUNC_c mf2;                   // the function mf2 and its type have C++ language linkage
  static FUNC_c *q;             // q has C++ language linkage; its type is pointer to C function
};
static_assert(same<decltype(&C::mf2), void (C::*)()>::value, "");
static_assert(same<decltype(C::q), FUNC_c *>::value, "");

extern "C" {
  class X {
  public:
    void mf();                  // the function mf and its type have C++ language linkage
    void mf2(void (*)());       // the parameter has type pointer to C function
    static void sf();           // a static member function has the language linkage of the declaration
  };
}
static_assert(same<decltype(&X::mf), void (X::*)()>::value, "");
static_assert(same<decltype(&X::sf), FUNC *>::value, "");
static_assert(same<decltype(&X::mf2), void (X::*)(FUNC *)>::value, "");

// The linkage-specification applies to the declarators in the body of a
// function too; the pointer a closure type converts to has C++ language
// linkage ([expr.prim.lambda.closure]).
extern "C" void g() {
  auto l = [](int) {};
  void (*p)(int) = l;
  // expected-error@-1 {{converting 'void (*)(int)' to 'extern "C" void (*)(int)' changes the language linkage of the function type}}
  void (*p2)(int) = c_cb;
  CXXFN *p3 = l;
  (void)p; (void)p2; (void)p3;
}

// [temp.deduct]: a function type with C language linkage does not match a
// parameter of C++ function type (unlike a conversion, which is diagnosed
// with -Wfunction-type-language-linkage, template argument deduction fails).
template <typename T> int h(void (*)(T)); // #h
int a1 = h(cxx_cb);
int a2 = h(c_cb);
// expected-error@-1 {{no matching function for call to 'h'}}
// expected-note@#h {{candidate template ignored: deduced type 'void (*)(int)' of 1st parameter does not match adjusted type 'extern "C" void (*)(int)' of argument [with T = int]}}

// Redeclarations without a linkage-specification inherit the type's linkage.
extern "C" void r1(int);
void r1(int);
static_assert(same<decltype(r1), CFN>::value, "");
