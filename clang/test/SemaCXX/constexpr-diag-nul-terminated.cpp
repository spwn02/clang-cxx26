// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s

// libstdc++'s <meta> (GCC 16) calls __builtin_constexpr_diag with three
// arguments: kind, tag and message, the strings NUL-terminated. Clang's own
// form is five (the strings as pointer and length); both are accepted.
#if !__has_builtin(__builtin_constexpr_diag)
#error the builtin is missing
#endif

constexpr int f(int n) {
  if (n < 0)
    __builtin_constexpr_diag(2, "", "negative argument"); // expected-error {{constexpr message: negative argument}}
  return n;
}
constexpr int ok = f(1);
// (An error message does not fail the evaluation: the program is ill-formed.)
constexpr int bad = f(-1);

constexpr int g() {
  // The five-argument form still wants (pointer, length) pairs.
  __builtin_constexpr_diag(1, "tag", "five-argument form is separate", 0, 0); // expected-error {{argument 3 of '__builtin_constexpr_diag' must be an integer}}
  return 0;
}
