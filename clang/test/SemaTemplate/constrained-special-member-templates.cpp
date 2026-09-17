// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

// [class.mem.special]p6's "eligible special member function" computation
// (SetEligibleMethods/ComputeSpecialMemberFunctionsEligiblity in SemaDecl.cpp)
// must compare associated constraints between candidates. When a candidate is
// itself a constructor *template* made eligible via a default template
// argument (rather than an ordinary, non-template special member function),
// the comparison previously passed the template's raw templated-pattern
// FunctionDecl to Sema::IsAtLeastAsConstrained, which asserts its inputs are
// never that specific kind (TK_FunctionTemplate) -- see PR98671.cpp for the
// crash this produced. Constraint partial ordering between templates needs
// the FunctionTemplateDecl itself, matching the already-working
// getMostSpecialized()/DeduceTemplateArguments() partial-ordering call sites
// in SemaTemplateDeduction.cpp.

template <typename T>
concept Sized = sizeof(T) > 0;

// Genuine subsumption (Sized<T> && true syntactically subsumes Sized<T>,
// since both atomic constraints originate from the same concept
// expression): there is a real "more constrained" winner, so this must not
// be ambiguous, and the more-constrained overload must actually be chosen.
template <typename T>
struct Subsumes {
  int val = 0;

  template <typename U = T>
    constexpr Subsumes(U = {}) requires Sized<T> : val(1) {}

  template <typename U = T>
    constexpr Subsumes(U = {}) requires (Sized<T> && true) : val(2) {}
};

static_assert(Subsumes<int>{}.val == 2);

// Similar (not syntactically-equivalent) constraint expressions are
// correctly treated as incomparable -- genuinely ambiguous, not crashing.
template <typename T>
struct Incomparable {
  template <typename U = T>
    Incomparable(U = {}) requires (sizeof(T) > 0) {}
    // expected-note@-1 {{candidate constructor}}

  template <typename U = T>
    Incomparable(U = {}) requires (true) {}
    // expected-note@-1 {{candidate constructor}}
};

Incomparable<int> incomparable;
// expected-error@-1 {{call to constructor of 'Incomparable<int>' is ambiguous}}
