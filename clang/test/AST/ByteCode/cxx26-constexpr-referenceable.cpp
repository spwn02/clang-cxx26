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

#endif

// Automatic constexpr objects have distinct identities before C++26 too.
constexpr bool distinct_frames(int n, const int *parent = nullptr) {
  constexpr int value = 42;
  if (parent == &value)
    return false;
  return !n || distinct_frames(n - 1, &value);
}
static_assert(distinct_frames(3));
