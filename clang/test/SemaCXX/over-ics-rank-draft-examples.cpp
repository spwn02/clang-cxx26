// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify %s

// [over.ics.ref], [over.ics.rank] examples of N5050 (CWG233, CWG2803, CWG2815).

namespace noexcept_function_reference {
void g() noexcept;
void u();
int h(void (&)() noexcept); // #1
int h(void (&)());          // #2
int j = h(g);               // calls #1, an exact match
int j2 = h(u);              // calls #2
static_assert(__is_same(decltype(h(g)), int), "");
char k(void (&)());
char k2 = k(g);             // function pointer conversion: still viable
} // namespace noexcept_function_reference

namespace qualification_vs_reference {
int f(const volatile int *);
int f(const int *);
int i;
int j = f(&i);              // calls f(const int*)
int g(const int *);
int g(const volatile int *const &);
int *p;
int k = g(p);               // calls g(const int*)
int g2(const volatile int *const &);
int g2(const int *);
int k2 = g2(p);
} // namespace qualification_vs_reference

namespace reference_to_t2_compatible {
int f(const int &);
int f(int &);
int g(const int &);
int g(int);
int i;
int j = f(i);               // calls f(int &)
int k = g(i);               // expected-error {{call to 'g' is ambiguous}}
                            // expected-note@35 {{candidate function}}
                            // expected-note@36 {{candidate function}}
int h(int (&)[]);
int h(int (&)[1]);
void g2() {
  int a[1];
  h(a);                     // calls h(int (&)[1])
}
} // namespace reference_to_t2_compatible

namespace conversion_function_source_type {
struct Z {};
struct A {
  operator Z &();
  operator const Z &(); // #1
};
struct B {
  operator Z();
  operator const Z &&(); // #2
};
const Z &r1 = A();      // OK, uses #1
const Z &&r2 = B();     // OK, uses #2
} // namespace conversion_function_source_type
