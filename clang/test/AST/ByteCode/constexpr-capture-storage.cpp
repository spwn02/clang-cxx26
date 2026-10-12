// RUN: %clang_cc1 -std=c++17 -fexperimental-new-constant-interpreter -verify %s
// RUN: %clang_cc1 -std=c++17 -verify %s
// RUN: %clang_cc1 -std=c++20 -fexperimental-new-constant-interpreter -verify %s
// RUN: %clang_cc1 -std=c++20 -verify %s
// RUN: %clang_cc1 -std=c++23 -fexperimental-new-constant-interpreter -verify %s
// RUN: %clang_cc1 -std=c++23 -verify %s
// RUN: %clang_cc1 -std=c++26 -fexperimental-new-constant-interpreter -verify %s
// RUN: %clang_cc1 -std=c++26 -verify %s
// expected-no-diagnostics

constexpr bool value_capture_identity() {
  constexpr int value = 42;
  auto captured = [value, original = &value] { return &value != original; };
  return captured();
}
static_assert(value_capture_identity());

constexpr bool reference_capture_identity() {
  constexpr int value = 42;
  auto captured = [&value, original = &value] { return &value == original; };
  return captured();
}
static_assert(reference_capture_identity());

constexpr int mutable_capture() {
  constexpr int value = 42;
  auto captured = [copy = value]() mutable { return ++copy; };
  return captured() + captured();
}
static_assert(mutable_capture() == 87);

constexpr bool nested_capture() {
  constexpr int value = 42;
  auto outer = [value, original = &value] {
    auto inner = [value, outer_copy = &value, original] {
      return &value != outer_copy && &value != original;
    };
    return inner();
  };
  return outer();
}
static_assert(nested_capture());
