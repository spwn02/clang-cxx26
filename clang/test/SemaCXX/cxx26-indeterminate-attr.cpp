// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %s

// P2795R5: the [[indeterminate]] attribute.

static_assert(__has_cpp_attribute(indeterminate) == 202403L);

void locals(int n) {
  int a [[indeterminate]];
  [[indeterminate]] int b;
  int c [[indeterminate]] = 0;
  int arr [[indeterminate]] [4];
  [[indeterminate]] int d, e; // the attribute applies to every declarator
  (void)a; (void)b; (void)c; (void)arr; (void)d; (void)e; (void)n;
}

void params(int x [[indeterminate]], [[indeterminate]] int y);
void params(int x, int y);           // later declarations may omit it (it is inherited)
void params(int x [[indeterminate]], // fine: the first declaration has it
            int y);
void params(int x, int y) {}

void params2(int x); // expected-note {{first declaration of the parameter is here}}
void params2(int x [[indeterminate]]) {} // expected-error {{'indeterminate' attribute on a function parameter must appear on the first declaration of the function}}

void definition(int z [[indeterminate]]) { (void)z; }

// Only local variables and function parameters.
[[indeterminate]] int global;      // expected-error {{'indeterminate' attribute only applies to local variables and function parameters}}
struct S {
  [[indeterminate]] int field;     // expected-error {{'indeterminate' attribute only applies to local variables and function parameters}}
  [[indeterminate]] void member(); // expected-error {{'indeterminate' attribute only applies to local variables and function parameters}}
};
[[indeterminate]] void function(); // expected-error {{'indeterminate' attribute only applies to local variables and function parameters}}
void statics() {
  static int s [[indeterminate]];  // expected-error {{'indeterminate' attribute only applies to local variables and function parameters}}
}

// No arguments.
void args() {
  int a [[indeterminate(1)]]; // expected-error {{'indeterminate' attribute takes no arguments}}
  (void)a;
}
