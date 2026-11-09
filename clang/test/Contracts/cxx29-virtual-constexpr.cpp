// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s

// P3097R3: in a constant evaluation, a virtual call through a class member
// access also evaluates the assertions of the statically chosen function,
// before and after those of the final overrider ([expr.call]).

struct B {
  virtual constexpr int f(const int x) pre(x > 0) post(r: r > 100) { return x; }
  virtual constexpr ~B() = default;
};
struct D : B { // no assertions of its own
  constexpr int f(const int x) override { return x + 200; }
};
struct E : B {
  constexpr int f(const int x) override { return x; }
};

constexpr int via_base(int x) { D d; B &b = d; return b.f(x); }
constexpr int via_base_e(int x) { E e; B &b = e; return b.f(x); }
constexpr int via_pointer(int x) { D d; B *p = &d; return p->f(x); }
constexpr int exact(int x) { B b; return b.f(x); }
constexpr int qualified(int x) { D d; B &b = d; return b.B::f(x); }
constexpr int named_overrider(int x) { D d; return d.f(x); }
constexpr int through_pmf(int x) { D d; B &b = d; int (B::*pmf)(int) = &B::f; return (b.*pmf)(x); }

static_assert(via_base(1) == 201);
static_assert(via_pointer(1) == 201);
static_assert(exact(200) == 200);     // callee and static function are the same
static_assert(qualified(200) == 200); // not a virtual call

// B::f's precondition fails, D::f has none.
constexpr int bad_pre = via_base(0);   // expected-error {{must be initialized by a constant expression}} \
                                       // expected-note {{in call to 'via_base(0)'}}
// B::f's postcondition fails (r > 100), E::f has none.
constexpr int bad_post = via_base_e(5); // expected-error {{must be initialized by a constant expression}} \
                                        // expected-note {{in call to 'via_base_e(5)'}}
constexpr int bad_ptr = via_pointer(-1); // expected-error {{must be initialized by a constant expression}} \
                                         // expected-note {{in call to 'via_pointer(-1)'}}
// expected-error@8 3 {{contract failed during execution of constexpr function}}
// expected-note@18 {{in call to 'b.f(0)'}}
// expected-note@19 {{in call to 'b.f(5)'}}
// expected-note@20 {{in call to 'p->f(-1)'}}

// The assertions of B::f are not those of the call when it names the final
// overrider, calls B::f non-virtually, or goes through a pointer to member.
static_assert(named_overrider(0) == 200);
static_assert(through_pmf(0) == 200);
