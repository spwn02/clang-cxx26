// RUN: %clang_cc1 -std=c++26 -fexperimental-new-constant-interpreter -verify %s
// RUN: %clang_cc1 -std=c++26 -verify %s
// RUN: %clang_cc1 -std=c++23 -fexperimental-new-constant-interpreter -verify %s
// RUN: %clang_cc1 -std=c++23 -verify %s
// expected-no-diagnostics

#if __cplusplus > 202302L
consteval int reference() {
  constexpr int value = 3;
  constexpr const int &ref = value;
  return ref;
}
static_assert(reference() == 3);

consteval int pointer() {
  constexpr int value = 3;
  constexpr const int *ptr = &value;
  return *ptr;
}
static_assert(pointer() == 3);

consteval int automatic() {
  int value = 3;
  constexpr int *ptr = &value;
  return *ptr;
}
static_assert(automatic() == 3);

// Each recursive invocation must read its own automatic object. A globally
// cached constexpr pointer instead refers to a dummy object or another frame.
constexpr int recursive(int n) {
  int value = n * 10;
  constexpr int *ptr = &value;
  return n ? recursive(n - 1) + *ptr : *ptr;
}
static_assert(recursive(3) == 60);

struct Holder { const int *ptr; };
constexpr int composite(int n) {
  int value = n * 10;
  constexpr Holder holder{&value};
  return n ? composite(n - 1) + *holder.ptr : *holder.ptr;
}
static_assert(composite(3) == 60);

// Standalone declaration evaluation preserves backing storage, while real
// invocations retain independent objects and destroy them at block exit.
struct D { int value; constexpr D(int n) : value(n) { value += 2; } constexpr ~D() {} };
constexpr int temporary_frames(int n) {
  constexpr const D &ref = D{7};
  static_assert(ref.value == 9);
  constexpr const int &primitive = 42;
  static_assert(primitive == 42);
  return ref.value + primitive + (n ? temporary_frames(n - 1) : 0);
}
static_assert(temporary_frames(2) == 153);
constexpr bool distinct_temporaries(int n, const D *parent = nullptr) {
  constexpr const D &ref = D{7};
  return &ref != parent && (!n || distinct_temporaries(n - 1, &ref));
}
static_assert(distinct_temporaries(3));
constexpr int destruction() {
  int destroyed = 0;
  struct T {
    int *counter;
    constexpr ~T() { ++*counter; }
  };
  {
    constexpr const T &ref = T{&destroyed};
    if (destroyed != 0 || ref.counter != &destroyed) return -1;
  }
  return destroyed;
}
static_assert(destruction() == 1);
constexpr int subobjects() {
  constexpr const int &member = D{7}.value;
  struct Base { int value; };
  struct Derived : Base { int other; };
  constexpr const Base &base = Derived{{11}, 13};
  static_assert(base.value == 11);
  return member + base.value;
}
static_assert(subobjects() == 20);

#endif

// Automatic constexpr objects have distinct identities before C++26 too.
constexpr bool distinct_frames(int n, const int *parent = nullptr) {
  constexpr int value = 42;
  if (parent == &value)
    return false;
  return !n || distinct_frames(n - 1, &value);
}
static_assert(distinct_frames(3));
