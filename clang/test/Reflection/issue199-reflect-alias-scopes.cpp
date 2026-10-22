// RUN: %clang_cc1 -std=c++26 -verify %s
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

namespace TargetA {}
namespace TargetB {}
namespace AliasA { namespace N = TargetA; } // expected-note {{candidate found by name lookup}}
namespace AliasB { namespace N = TargetB; } // expected-note {{candidate found by name lookup}}
using namespace AliasA;
using namespace AliasB;
constexpr auto namespace_alias = ^^N; // expected-error {{reference to 'N' is ambiguous}}

// CWG3190: aliases of the same namespace declared in different scopes have
// different target scopes.
namespace SameTarget {}
namespace AliasC { namespace M = SameTarget; } // expected-note {{candidate found by name lookup}}
namespace AliasD { namespace M = SameTarget; } // expected-note {{candidate found by name lookup}}
using namespace AliasC;
using namespace AliasD;
namespace Ordinary = M;
constexpr auto same_target_alias = ^^M; // expected-error {{reference to 'M' is ambiguous}}

namespace Redeclared { namespace R = SameTarget; namespace R = SameTarget; }
using namespace Redeclared;
constexpr auto redeclared_alias = ^^R;

namespace SameAlias { using V = int; }
namespace SamePathA { using namespace SameAlias; }
namespace SamePathB { using namespace SameAlias; }
using namespace SamePathA;
using namespace SamePathB;
constexpr auto same_alias = ^^V;

template <class T>
constexpr auto reflect_parameter_alias() { return ^^T; }
static_assert(reflect_parameter_alias<int>() == ^^int);
