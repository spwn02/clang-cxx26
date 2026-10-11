// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// CWG2228: the operand of sizeof, alignof, alignas, a cast or a new-expression
// is a nofun-type-id, which is not a function type spelled with a function
// declarator in the outermost position.

namespace std { using size_t = decltype(sizeof(0)); struct type_info; }
void *operator new(std::size_t, void *p);

int a;
using T = int;

// [dcl.ambig.res]: these are expressions.
static_assert(sizeof(int()) == sizeof(int));
static_assert(sizeof(int(unsigned(a))) == sizeof(int));
static_assert(sizeof(T()) == sizeof(int));
auto x = (int())+1;
auto y = (int(unsigned(a)))+1;
auto z = (int())-1;
static_assert(__is_same(decltype((int())+1), int));

// Function types nested in a declarator are still types.
static_assert(sizeof(int (*)()) == sizeof(void *));
static_assert(sizeof(int (*)(int)) == sizeof(void *));
static_assert(alignof(void (*)(int)) == alignof(void *));
struct B { int f(); };
static_assert(sizeof(int (B::*)()) >= sizeof(void *));
void (*fp)() = (void (*)())nullptr;
auto *np = new (int (*)());
void (*fp2)(int) = (void (*)(int))(void (*)())nullptr;

// A function declarator with a trailing-return-type is a nofun-declarator only
// when it is preceded by a noptr-nofun-declarator.
static_assert(sizeof(auto (*)() -> int) == sizeof(void *));
static_assert(sizeof(auto(a)) == sizeof(int));
auto bad1 = sizeof(auto() -> int); // expected-error {{initializer for functional-style cast to 'auto' is empty}} expected-error {{expected unqualified-id}}
auto bad2 = (auto() -> int)a; // expected-error {{initializer for functional-style cast to 'auto' is empty}} expected-error {{expected unqualified-id}}

// alignas(nofun-type-id) has the effect of alignas(alignof(nofun-type-id)).
struct alignas(int()) AI { char c; };
static_assert(alignof(AI) == 1);
struct alignas(int (*)()) AP { char c; };
static_assert(alignof(AP) == alignof(int (*)()));

// A function type is not a nofun-type-id.
void f() {
  (void)sizeof(int(int)); // expected-error {{expected '(' for function-style cast or type construction}}
  int v = 0;
  (void)(int(int))v; // expected-error {{expected '(' for function-style cast or type construction}}
}

// The rules for type-ids in template arguments and typeid are unchanged.
template <class> struct Y {};
Y<int()> t1;
Y<int(1)> *t2 = nullptr; // expected-error {{must be a type}} expected-note@-2 {{template parameter is declared here}}
