// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: not %clang_cc1 -std=c++26 -fsyntax-only -fexperimental-new-constant-interpreter %s > /dev/null

// expected-no-diagnostics
// P3533R2: more constant-evaluation scenarios with virtual bases: copies, arrays,
// temporaries, up-casts, dynamic_cast, new/delete. The bytecode interpreter must
// reject these without crashing.

struct V { int n = 7; constexpr V() = default; constexpr V(int n): n(n) {} constexpr virtual int f() const { return n; } constexpr virtual ~V() = default; };
struct L : virtual V { constexpr L() : V(1) {} constexpr int f() const override { return 10 + V::f(); } };
struct R : virtual V { constexpr R() : V(2) {} };
struct D : L, R { constexpr D() : V(3) {} };
// copy construction
constexpr int cp() { D a; D b = a; return b.n; }
static_assert(cp() == 3);
// array of such
constexpr int arr() { D a[2]; return a[0].n + a[1].n; }
static_assert(arr() == 6);
// temporaries and reference binding
constexpr int tmp() { const V &v = D(); return v.f(); }
static_assert(tmp() == 13);
// dynamic type during construction of base
struct B1 { int x; constexpr B1() : x(0) {} };
struct Z : virtual B1 { constexpr Z() { x = 5; } };
static_assert(Z().x == 5);
// static member pointers and casts up
constexpr int up() { D d; L *l = &d; V *v = l; return v->f(); }
static_assert(up() == 13);
// down cast from virtual base is ill-formed, dynamic_cast
constexpr bool dc() { D d; V *v = &d; return dynamic_cast<D*>(v) == &d; }
static_assert(dc());
// typeid
namespace std { struct type_info { const char *n; constexpr bool operator==(const type_info &o) const { return this == &o; } }; }
constexpr bool ti() { D d; V &v = d; return typeid(v) == typeid(D); }
static_assert(ti());
// new / delete
constexpr int nd() { D *d = new D; V *v = d; int r = v->f(); delete d; return r; }
static_assert(nd() == 13);
constexpr int nd2() { V *v = new D; int r = v->f(); delete v; return r; }
static_assert(nd2() == 13);
