// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
// [expr.reflect] example: equal types do not merge alias target scopes.
namespace A { using T = int; } // expected-note {{candidate found by name lookup}}
namespace B { using T = int; } // expected-note {{candidate found by name lookup}}
using namespace A;
using namespace B;
constexpr auto r = ^^T; // expected-error {{reference to 'T' is ambiguous}}
T ordinary = 0;
constexpr auto ra = ^^A::T;
namespace Reopened { typedef int U; typedef int U; constexpr auto ru = ^^U; }
struct BaseA { using U = int; }; // expected-note {{member type 'int' found by ambiguous name lookup}}
struct BaseB { using U = int; }; // expected-note {{member type 'int' found by ambiguous name lookup}}
struct Derived : BaseA, BaseB {};
Derived::U ordinary_member = 0;
constexpr auto rd = ^^Derived::U; // expected-error {{member 'U' found in multiple base classes of different types}}
namespace MixedA { struct Q {}; } // expected-note {{candidate found by name lookup}}
namespace MixedB { using Q = MixedA::Q; } // expected-note {{candidate found by name lookup}}
using namespace MixedA;
using namespace MixedB;
Q ordinary_mixed;
constexpr auto mixed = ^^Q; // expected-error {{reference to 'Q' is ambiguous}}
