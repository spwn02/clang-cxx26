// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// expected-no-diagnostics
// P3533R2: sharing of virtual bases, cross casts, dynamic_cast, construct_at, assignment, nesting, three levels.

namespace std { template<class T> constexpr T* construct_at(T *p) { return ::new (static_cast<void*>(p)) T(); } template<class T> constexpr void destroy_at(T *p) { p->~T(); } }
inline void *operator new(__SIZE_TYPE__, void *p) noexcept { return p; }
struct V { int n = 7; constexpr V() = default; constexpr V(int n): n(n) {} constexpr virtual int f() const { return n; } constexpr virtual ~V() = default; };
struct L : virtual V { constexpr L() : V(1) {} };
struct R : virtual V { constexpr R() : V(2) {} constexpr int g() const { return 5; } virtual constexpr int h() const { return 9; } };
struct D : L, R { constexpr D() : V(3) {} constexpr int h() const override { return 11; } };
// pointer identity of the shared virtual base
constexpr bool same() { D d; L *l = &d; R *r = &d; V *a = l; V *b = r; return a == b; }
static_assert(same());
// cross cast
constexpr bool cross() { D d; L *l = &d; R *r = dynamic_cast<R*>(l); return r != nullptr && r->h() == 11; }
static_assert(cross());
// failed dynamic_cast to pointer
struct Other : virtual V {};
constexpr bool failed() { D d; V *v = &d; return dynamic_cast<Other*>(v) == nullptr; }
static_assert(failed());
// down cast through virtual base via dynamic_cast
constexpr int down() { D d; V *v = &d; D *p = dynamic_cast<D*>(v); return p->n; }
static_assert(down() == 3);
// construct_at / destroy_at
constexpr int ca() { D d; std::destroy_at(&d); std::construct_at(&d); return d.n; }
static_assert(ca() == 3);
// assignment
constexpr int as() { D a; D b; a.n = 9; b = a; return b.n; }
static_assert(as() == 9);
// nested member
struct H { D d; int k = 1; };
constexpr int nested() { H h; return h.d.n + h.k; }
static_assert(nested() == 4);
// three levels
struct E : D { constexpr E() : V(4) {} };
struct F : E, Other { constexpr F() : V(5) {} };
constexpr int deep() { F f; const V &v = f; return v.f() + f.n; }
static_assert(deep() == 10);
// virtual base with own virtual base
struct Base0 { int z = 1; constexpr Base0() = default; };
struct Mid : virtual Base0 { int y = 2; constexpr Mid() = default; };
struct Top : virtual Mid { constexpr Top() = default; };
constexpr int tops() { Top t; return t.z * 10 + t.y; }
static_assert(tops() == 12);
// constexpr static storage
constexpr D gd;
static_assert(gd.n == 3 && gd.f() == 3);
// member function pointer through virtual base
constexpr int mfp() { D d; int (R::*p)() const = &R::g; return (d.*p)(); }
static_assert(mfp() == 5);
// copy through base reference
constexpr int cpb() { D d; V v = d; return v.n; }
static_assert(cpb() == 3);
// lambda capturing
constexpr int lam() { D d; auto f = [&] { return d.n; }; return f(); }
static_assert(lam() == 3);
// Destroying array elements (the last designator entry is an array index).
struct Dtor { int x = 1; constexpr ~Dtor() {} };
constexpr int destroy_elements() {
  Dtor a[2];
  std::destroy_at(&a[1]);
  std::construct_at(&a[1]);
  a[0].~Dtor();
  std::construct_at(&a[0]);
  return a[0].x + a[1].x;
}
static_assert(destroy_elements() == 2);
