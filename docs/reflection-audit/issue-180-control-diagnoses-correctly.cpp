// Companion control for docs/reflection-audit/issue-180-minimal-repro.cpp.
//
// Identical to that repro except 'maybe_init' is an ordinary (non-template)
// consteval member function instead of a function template. This variant
// correctly diagnoses test_impl<int>'s static_assert(false):
//
//   error: static assertion failed
//   note: in instantiation of function template specialization
//         'test_impl<int>' requested here
//
// This is the discriminator: the ONLY structural difference between this
// file and the sibling repro is whether the function that calls
// substitute()+extract() is itself a function template. That's the one
// variable to hold onto when actually root-causing issue #180.

#include <meta>

template <typename>
void test_impl() {
  static_assert(false);
}

struct Foo {
  void (*test)() = nullptr;

  consteval void maybe_init() {
    test = extract<void(*)()>(substitute(^^test_impl, {^^int}));
  }
};

int main() {
  Foo f;
  f.maybe_init();
}
