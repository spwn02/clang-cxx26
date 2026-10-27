// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++17 -fsyntax-only -Wno-error=function-type-language-linkage -verify=warn %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -Wno-error=function-type-language-linkage -verify=warn %s
// RUN: %clang_cc1 -std=c++17 -fsyntax-only -Wno-function-type-language-linkage -verify=none %s
// none-no-diagnostics

// Function types with different language linkages are distinct types
// ([dcl.link]p1). Converting a pointer to a function to a pointer to a function
// that differs only in the language linkage of its type is diagnosed with
// -Wfunction-type-language-linkage, which is an error by default, so that code
// written for implementations that ignore the language linkage of function
// types can be compiled by downgrading it.

template <typename T, typename U> struct same { static constexpr bool value = false; };
template <typename T> struct same<T, T> { static constexpr bool value = true; };

extern "C" void cfn(int);
void xfn(int);
extern "C" { typedef void (*CPtr)(int); }
typedef void (*XPtr)(int);

// Initialization, assignment and argument passing.
XPtr a = cfn; // expected-error {{changes the language linkage of the function type}} warn-warning {{changes the language linkage of the function type}}
CPtr b = xfn; // expected-error {{converting 'void (*)(int)' to 'CPtr' (aka 'extern "C" void (*)(int)') changes the language linkage of the function type}} warn-warning {{changes the language linkage of the function type}}
CPtr c = cfn;
XPtr d = xfn;
void takes(XPtr);
void takes_c(CPtr);
void args() {
  takes(cfn); // expected-error {{changes the language linkage of the function type}} warn-warning {{changes the language linkage of the function type}}
  takes_c(xfn); // expected-error {{changes the language linkage of the function type}} warn-warning {{changes the language linkage of the function type}}
  takes_c(cfn);
}

// A cast is how the language linkage of a function pointer type is changed.
XPtr e1 = (XPtr)cfn;
XPtr e2 = static_cast<XPtr>(cfn);
XPtr e3 = reinterpret_cast<XPtr>(cfn);
CPtr e4 = CPtr(xfn);

// An overload with the exact type is better than the ill-formed conversion.
int which(XPtr);
long which(CPtr);
static_assert(same<decltype(which(cfn)), long>::value, "");
static_assert(same<decltype(which(xfn)), int>::value, "");

// Overload sets and function templates resolve against the target type.
void ov(int);
void ov(char *);
template <class T> void tf(int);
template <class T> void td(T);
template <class T> void tf(int) {}
void sets() {
  takes_c(ov); // expected-error {{changes the language linkage of the function type}} warn-warning {{changes the language linkage of the function type}}
  takes_c(tf<char>); // expected-error {{changes the language linkage of the function type}} warn-warning {{changes the language linkage of the function type}}
  takes_c(td); // expected-error {{changes the language linkage of the function type}} warn-warning {{changes the language linkage of the function type}}
  takes(ov);
}

// Non-type template arguments (converted constant expressions).
template <XPtr P> struct W {};
W<cfn> w1; // expected-error {{changes the language linkage of the function type}} warn-warning {{changes the language linkage of the function type}}
W<xfn> w2;

// Together with a function pointer conversion.
extern "C" void cnoexcept(int) noexcept;
XPtr n = cnoexcept; // expected-error {{changes the language linkage of the function type}} warn-warning {{changes the language linkage of the function type}}

// A function type written in a template argument in a linkage-specification
// matches a partial specialization written outside of it.
template <class T> struct Wrap;
template <class R, class... A> struct Wrap<R(A...)> {
  static constexpr int value = 1;
};
extern "C" {
int wrapped = Wrap<int(int)>::value;
}

// In an unevaluated operand the conversion stays an error, so traits and
// requires-expressions still see the function types as distinct.
static_assert(!__is_convertible(CPtr, XPtr), "");
static_assert(!__is_convertible(XPtr, CPtr), "");
static_assert(__is_convertible(CPtr, CPtr), "");
#if __cplusplus >= 202002L
template <class T>
concept CallsTakes = requires(T t) { takes(t); };
static_assert(!CallsTakes<CPtr>);
static_assert(CallsTakes<XPtr>);
#endif
