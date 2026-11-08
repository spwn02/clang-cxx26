// Signature help on the injected class name inside a class template whose
// constructor templates are ordered while the enclosing template is still
// dependent used to assert or crash in partial ordering (#494).
template <unsigned long K> struct Tag {};
template <unsigned long N, bool D = false>
struct Box {
  template <unsigned long A>
  Box(Tag<A>, Tag<N>) requires(not D);
  template <unsigned long A, unsigned long B>
  Box(Tag<A>, Tag<B>) requires(D);
  void f() {
    Box{};
  }
};

// RUN: %clang_cc1 -std=c++20 -fsyntax-only -code-completion-at=%s:12:9 %s | FileCheck %s
// CHECK: OVERLOAD: Box{<#Tag<A>#>, Tag<N>}
// CHECK: OVERLOAD: Box{<#Tag<A>#>, Tag<B>}
