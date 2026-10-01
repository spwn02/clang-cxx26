// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

#if !__has_builtin(__is_structural)
#error missing __is_structural builtin
#endif

struct Public { int i; };
class Private { int i; };
struct Mutable { mutable int i; };
struct NonLiteral { ~NonLiteral(); };
union PublicUnion { int i; float f; };
union PrivateUnion { private: int i; };
struct Base : Private {};
struct PrivateBase : private Public {};
template<class T> struct Wrapper { T value; };
struct Incomplete; // expected-note 3 {{forward declaration}}
union IncompleteUnion; // expected-note {{forward declaration}}

static_assert(__is_structural(int));
static_assert(__is_structural(const volatile int));
static_assert(__is_structural(int*));
static_assert(__is_structural(int&));
static_assert(__is_structural(Incomplete&));
static_assert(!__is_structural(int&&));
static_assert(!__is_structural(void));
static_assert(!__is_structural(const volatile void));
static_assert(!__is_structural(void()));
static_assert(!__is_structural(int[2]));
static_assert(!__is_structural(int[]));
static_assert(!__is_structural(Public[2]));
static_assert(__is_structural(Public));
static_assert(__is_structural(const volatile Public));
static_assert(__is_structural(PublicUnion));
static_assert(!__is_structural(Private));
static_assert(!__is_structural(Mutable));
static_assert(!__is_structural(NonLiteral));
static_assert(!__is_structural(PrivateUnion));
static_assert(!__is_structural(Base));
static_assert(!__is_structural(PrivateBase));
static_assert(__is_structural(Wrapper<Public>));
static_assert(!__is_structural(Wrapper<Private>));
bool a = __is_structural(Incomplete); // expected-error {{incomplete type}}
bool b = __is_structural(Incomplete[]); // expected-error {{incomplete type}}
bool c = __is_structural(Incomplete[2]); // expected-error {{incomplete type}}
bool d = __is_structural(IncompleteUnion); // expected-error {{incomplete type}}
