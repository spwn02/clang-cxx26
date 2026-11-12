// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// An exception that escapes a constant expression is reported with the result
// of its what() member, when it has one that can be evaluated.

struct base {
  constexpr const char *what() const { return "message from base"; }
};
struct inherits : base {};
constexpr int from_base() { throw inherits{}; }
constexpr int v1 = from_base(); // expected-error {{must be initialized by a constant expression}} \
                                // expected-note@-1 {{what() returned "message from base"}} \
                                // expected-note {{in call to 'from_base()'}}

struct overrides : base {
  constexpr const char *what() const { return "message from derived"; }
};
constexpr int from_derived() { throw overrides{}; }
constexpr int v2 = from_derived(); // expected-error {{must be initialized by a constant expression}} \
                                   // expected-note@-1 {{what() returned "message from derived"}} \
                                   // expected-note {{in call to 'from_derived()'}}

// No what(): the note keeps its original wording.
constexpr int no_what() { throw 42; }
constexpr int v3 = no_what(); // expected-error {{must be initialized by a constant expression}} \
                              // expected-note@-1 {{exception thrown here was not caught within the constant expression}} \
                              // expected-note {{in call to 'no_what()'}}

// A what() that cannot be evaluated is not an error of its own.
struct opaque {
  const char *what() const; // not constexpr, no definition
};
constexpr int from_opaque() { throw opaque{}; }
constexpr int v4 = from_opaque(); // expected-error {{must be initialized by a constant expression}} \
                                  // expected-note@-1 {{exception thrown here was not caught within the constant expression}} \
                                  // expected-note {{in call to 'from_opaque()'}}
