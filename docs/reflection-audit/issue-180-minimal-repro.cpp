// Minimal reproducer for upstream bloomberg/clang-p2996 issue #180
// ("static_assert(false) ignored"), narrowed down during the Reflection
// Closeup epic (2026-09-10) from the original, much larger issue report.
//
// This compiles with ZERO diagnostics, even though test_impl<int>'s body
// (instantiated via substitute()+extract()) contains an unconditional,
// non-dependent static_assert(false) that should fail.
//
// Confirmed NOT an expansion-statement bug (the item-5 plan's original
// assumption, inherited from an earlier investigation that treated #180
// and #181 as one item): this repro contains no 'template for' anywhere.
// Ordinary (non-reflection) instantiation of the exact same template
// correctly diagnoses the static_assert:
//
//   template <typename> void test_impl() { static_assert(false); }
//   int main() { test_impl<int>(); }  // correctly: error: static assertion failed
//
// The single discriminating factor found so far (see docs/REFLECTION_CLOSEUP.md's
// item 5a writeup for the two intermediate repros this was narrowed from):
// whether the *enclosing* function that calls substitute()+extract() is
// itself a function template. A non-template consteval member function
// doing the identical substitute()+extract() call diagnoses correctly; the
// only change needed to make the diagnostic vanish is adding an unused
// `template <bool E>` to that enclosing function (no `if constexpr`, no
// member function pointers, no reflected substitution of the enclosing
// function itself -- none of those upstream-repro details are load-bearing).
//
// This strongly suggests a Sema instantiation-context interaction bug:
// Sema::EnsureInstantiated (SemaReflect.cpp, ReflectionModifiableSemaActions)
// does call S.InstantiateFunctionDefinition(..., true, true) for a function
// template specialization reflection, and that call path works correctly
// when invoked from ordinary (non-template) consteval evaluation -- but
// something about the *nested* instantiation triggered while Sema's
// ActiveTemplateInstantiations stack already has a frame for the enclosing
// function template's own instantiation (maybe_init<true>) appears to
// suppress or defer the diagnostic that InstantiateFunctionDefinition would
// otherwise emit. Not yet root-caused to a specific line -- this needs the
// same class of investigation as item 1's escalation cluster (a targeted
// trace of Sema's instantiation-context/constant-evaluation interaction),
// not a narrow diagnostic fix. Escalated to the user per the epic's
// completion bar rather than continuing to guess between the untested
// candidate mechanisms (SFINAE-context misfire, instantiation-depth
// deferral, evaluator-side diagnostic suppression, or a nested-instantiation
// guard meant for mutual-recursion protection firing too eagerly here).

#include <meta>

template <typename>
void test_impl() {
  static_assert(false);
}

struct Foo {
  void (*test)() = nullptr;

  template <bool E>
  consteval void maybe_init() {
    test = extract<void(*)()>(substitute(^^test_impl, {^^int}));
  }
};

consteval Foo make() {
  Foo f;
  f.maybe_init<true>();
  return f;
}

int main() {
  auto f = make();
}
