// RUN: %clang_cc1 %s -std=c++26 -freflection -fannotation-attributes -verify
namespace N { extern int v; void fn(); void fp(int); }
[[=1]] int N::v = 0; // OK: a qualified definition inhabits and targets the same scope
void N::fn [[=2]] () {} // OK
void N::fp([[=2]] int) {} // OK
struct F {
  friend void ff [[=1]] (); // expected-error {{non-defining friend declaration}}
  friend void fp([[=1]] int); // expected-error {{non-defining friend declaration}}
  friend void ok [[=1]] ([[=2]] int) {}
  void member [[=3]] ();
};
void F::member [[=4]] () {} // OK
void vp([[=1]] void); // expected-error {{void parameter}}
struct NoComma { [[=1 =2]] int x; }; // expected-error {{expected ','}}
namespace N { [[=5]] int w; [[=6]] void good([[=7]] int) {} }
namespace Types { struct S; }
struct [[=8]] Types::S {}; // OK
struct Outer { struct Inner; };
struct [[=9]] Outer::Inner {}; // OK
struct FriendType { friend struct [[=10]] Other; }; // expected-error {{non-defining friend declaration}}

// [basic.scope.scope]: only a declaration that inhabits a block scope but targets
// a larger enclosing scope has a host scope that differs from its target scope.
void block_scope() {
  [[=11]] extern int ev; // expected-error {{host scope differs from its target scope}}
  [[=12]] void lf(); // expected-error {{host scope differs from its target scope}}
  [[=13]] int local = 0; // OK: a block-scope variable targets its own scope
  struct [[=14]] LocalClass {}; // OK
  (void)local;
}
