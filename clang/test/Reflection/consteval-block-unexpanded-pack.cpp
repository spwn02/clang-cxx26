// RUN: %clang_cc1 %s -std=c++2d -fsyntax-only -verify

// A parameter pack used in a consteval block without being expanded is
// diagnosed where the block is written, not crashed on when the class template
// is instantiated (the block is a lambda that is called at once, and a
// declaration has no enclosing full-expression that would expand the pack).

consteval void g(int) {}

template <class... Ts> struct Unexpanded {
  consteval { g(sizeof(Ts)); } // expected-error {{expression contains unexpanded parameter pack 'Ts'}}
};
Unexpanded<int, char> u;

// Used deeper in the block's body: reported at the block.
template <class... Ts> struct UnexpandedNested {
  consteval { // expected-error {{expression contains unexpanded parameter pack 'Ts'}}
    int n = 0;
    n += sizeof(Ts);
    g(n);
  }
};
UnexpandedNested<int> un;

// Expanded, it is fine.
template <class... Ts> struct Expanded {
  consteval { (g(sizeof(Ts)), ...); }
};
Expanded<int, char> e;

// A block that does not use the pack is fine.
template <class... Ts> struct Unrelated {
  consteval { g(1); }
};
Unrelated<int> unrelated;

// The block of a function template.
template <class... Ts> void f() {
  consteval { g(sizeof(Ts)); } // expected-error {{expression contains unexpanded parameter pack 'Ts'}}
}
