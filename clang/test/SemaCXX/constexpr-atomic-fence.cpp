// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fexperimental-new-constant-interpreter -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx23 %s

// [atomics.fences] (P3309R3): fences are usable in constant expressions and have no effect there.
// expected-no-diagnostics

constexpr bool f() {
  __c11_atomic_thread_fence(5);
  __c11_atomic_signal_fence(5);
  __atomic_thread_fence(5);
  __atomic_signal_fence(5);
  return true;
}
static_assert(f()); // cxx23-error {{static assertion expression is not an integral constant expression}} \
                    // cxx23-note@9 {{subexpression not valid in a constant expression}} \
                    // cxx23-note {{in call to 'f()'}}
