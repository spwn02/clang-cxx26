// RUN: %clang_cc1 %s -std=c++26 -freflection -fannotation-attributes -verify
namespace N { extern int v; void fn(); void fp(int); }
[[=1]] int N::v = 0; // expected-error {{host scope differs from its target scope}}
void N::fn [[=2]] () {} // expected-error {{host scope differs from its target scope}}
void N::fp([[=2]] int) {} // expected-error {{host scope differs from its target scope}}
struct F {
  friend void ff [[=1]] (); // expected-error {{non-defining friend declaration}}
  friend void fp([[=1]] int); // expected-error {{non-defining friend declaration}}
  friend void ok [[=1]] ([[=2]] int) {}
  void member [[=3]] ();
};
void F::member [[=4]] () {} // expected-error {{host scope differs from its target scope}}
void vp([[=1]] void); // expected-error {{void parameter}}
struct NoComma { [[=1 =2]] int x; }; // expected-error {{expected ','}}
namespace N { [[=5]] int w; [[=6]] void good([[=7]] int) {} }
namespace Types { struct S; }
struct [[=8]] Types::S {}; // expected-error {{host scope differs from its target scope}}
struct Outer { struct Inner; };
struct [[=9]] Outer::Inner {}; // expected-error {{host scope differs from its target scope}}
struct FriendType { friend struct [[=10]] Other; }; // expected-error {{non-defining friend declaration}}
