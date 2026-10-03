// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx23 %s

#if __cplusplus <= 202302L
void old_rules() {
  constexpr int a = 3; // cxx23-note {{address of non-static constexpr variable 'a' may differ on each invocation}}
  constexpr const int &r = a; // cxx23-error {{must be initialized by a constant expression}} cxx23-note {{reference to 'a' is not a constant expression}}
}
#else
struct A {
  int m;
  const int &r;
};

void draft_objects() {
  static int sx;
  thread_local int tx;
  int ax; // expected-note 3 {{declared here}}
  A aa = {1, 2};
  static A sa = {3, 4};
  static constinit int &rss = sx;
  thread_local constinit int &rts = sx;
  constexpr int *psx = &sx;
  constexpr int *pax = &ax;
  constexpr int *paa = &aa.m;
  constexpr int *psa = &sa.m;
  constexpr int *ptx = &tx; // expected-error {{must be initialized by a constant expression}}
  constexpr int &rtx = tx; // expected-error {{must be initialized by a constant expression}}
  static constexpr int *bad = &ax; // expected-error {{must be initialized by a constant expression}} expected-note {{pointer to 'ax' is not a constant expression}}
  thread_local constexpr int *badthread = &ax; // expected-error {{must be initialized by a constant expression}} expected-note {{pointer to 'ax' is not a constant expression}}
  static int &rsa = ax; // dynamic initialization is valid
  thread_local int &rta = ax; // likewise
  static int &rst = tx;
  thread_local int &rtt = tx;
  int &rat = tx;
  auto lambda = [&] {
    int ay;
    constexpr int *py = &ay;
    constexpr int *px = &ax; // expected-error {{must be initialized by a constant expression}} expected-note {{pointer to 'ax' is not a constant expression}}
    constexpr int *ps = &sx;
    constexpr int *pm = &sa.m;
  };
}

constexpr int same_function(int n) {
  constexpr int a = 3;
  constexpr const int &r = a;
  constexpr const int *p = &a;
  static_assert(r == 3 && *p == 3);
  constexpr const int &t = 42;
  static_assert(t == 42);
  struct D { int v; constexpr ~D() {} };
  constexpr const D &d = D{9};
  static_assert(d.v == 9);
  struct E { const int &r = 8; };
  constexpr E e{};
  static_assert(e.r == 8);
  struct F { int v; };
  constexpr const auto &[bound] = F{6};
  constexpr const int &rb = bound;
  static_assert(rb == 6);
  constexpr A aa = {1, 2};
  constexpr const int *pm = &aa.m;
  constexpr const int *pt = &aa.r;
  static_assert(*pm == 1 && *pt == 2);
  static constexpr A sa = {3, 4};
  constexpr const int *pst = &sa.r;
  static_assert(*pst == 4);
  int arr[2] = {n, 4};
  constexpr int *q = arr + 1;
  constexpr int *const *qq = &q;
  *q += n;
  struct B { const int *p; };
  struct C : B { const int &r; };
  constexpr C c{{&a}, t};
  constexpr const int *ptrs[] = {&a, &t};
  union U { const int *p; };
  constexpr U u{&a};
  static_assert(*c.p == 3 && c.r == 42 && *ptrs[1] == 42 && *u.p == 3);
  return r + *p + t + *pm + *pt + **qq;
}
static_assert(same_function(5) == 60);

constexpr int recursive_frames(int n) {
  constexpr int *p = &n;
  if (*p == 0)
    return 0;
  return *p + recursive_frames(n - 1);
}
static_assert(recursive_frames(4) == 10);

constexpr int loop_frames() {
  int sum = 0;
  for (int i = 0; i != 4; ++i) {
    int a = i;
    constexpr int *p = &a;
    sum += ++*p;
  }
  return sum;
}
static_assert(loop_frames() == 10);

void different_functions() {
  constexpr int a = 3; // expected-note 2 {{address of non-static constexpr variable 'a' may differ on each invocation}}
  auto lambda = [&a] {
    constexpr const int &r = a; // expected-error {{must be initialized by a constant expression}} expected-note {{reference to 'a' is not a constant expression}}
  };
  auto nested = [p = &a] {
    constexpr const int *q = p; // expected-error {{must be initialized by a constant expression}} expected-note {{read of non-constexpr variable 'p' is not allowed in a constant expression}}
  }; // expected-note@-2 {{declared here}}
  static constexpr const int *p = &a; // expected-error {{must be initialized by a constant expression}} expected-note {{pointer to 'a' is not a constant expression}}
  constexpr auto local = [] {
    constexpr int ay = 7;
    constexpr const int &ry = ay;
    static_assert(ry == 7);
    return ry;
  }();
  static_assert(local == 7);
}

void reads_at_point_of_use(int x) {
  constexpr int a = 3; // expected-note 2 {{address of non-static constexpr variable 'a' may differ on each invocation}}
  constexpr const int *p = &a;
  constexpr const int &r = a; // expected-note {{declared here}}
  auto lambda = [&] {
    static_assert(*p == 3); // expected-error {{static assertion expression is not an integral constant expression}} expected-note {{pointer to 'a' is not a constant expression}}
    static_assert(r == 3); // expected-error {{static assertion expression is not an integral constant expression}} expected-note {{read of variable 'r' whose value is not known}}
  };
  constexpr int copy = [p] { return *p; }();
  static_assert(copy == 3);
  constexpr int by_reference = [&p] { return *p; }(); // expected-error {{must be initialized by a constant expression}} expected-note {{pointer to 'a' is not a constant expression}} expected-note {{in call to}}

  // [expr.const.init]'s example, with a numeric sibling that remains usable.
  struct B { int *const &r; int n; };
  constexpr B b = {&x, 7}; // expected-note {{temporary created here}}
  static_assert(b.r == &x);
  auto member = [&] {
    static_assert(b.r != nullptr); // expected-error {{static assertion expression is not an integral constant expression}} expected-note {{reference to temporary is not a constant expression}}
    static_assert(b.n == 7);
  };
}

template <const int *> struct Pointer {};
void non_variable_expressions() {
  constexpr int a = 3; // expected-note {{address of non-static constexpr variable 'a' may differ on each invocation}}
  constexpr const int *p = &a;
  Pointer<p> x; // expected-error {{non-type template argument is not a constant expression}} expected-note {{pointer to 'a' is not a constant expression}}
  static_assert(p); // Converting the pointer to bool leaves no constituent pointer.
}
#endif
