// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// expected-no-diagnostics

// [atomics.fences] (P3309R3): fences are usable in constant expressions and have no effect there.

constexpr bool f() {
  __c11_atomic_thread_fence(5);
  __c11_atomic_signal_fence(5);
  __atomic_thread_fence(5);
  __atomic_signal_fence(5);
  return true;
}
static_assert(f());
