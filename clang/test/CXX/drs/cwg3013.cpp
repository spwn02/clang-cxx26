// RUN: %clang_cc1 -std=c++2c -pedantic-errors -I %S/../../Preprocessor/Inputs %s -verify


// cwg3013: yes
// cwg3016: yes
// cwg3018: yes

// A macro named like an embed parameter makes the directive ill-formed.
#define limit 1
const unsigned char a[] = { // expected-error {{zero size arrays are an extension}}
#embed "jk.txt" limit(1) // expected-error {{expected identifier}}
};
#undef limit

#define prefix 1
#if __has_embed("jk.txt" prefix(0)) // expected-error {{expected identifier}} expected-error {{expected value in expression}}
#endif
#undef prefix

// The preprocessing tokens of __has_include shall form a header-name.
#if __has_include(<jk.txt> x) // expected-error {{missing ')' after '__has_include'}} expected-error {{expected end of line in preprocessor expression}} expected-note {{to match this}}
#endif

// 'defined' shall not appear in __has_include or __has_embed.
#define X 1
#if __has_embed("jk.txt" limit(defined(X))) // expected-error {{'defined' cannot appear within this context}} expected-error {{expected value in expression}}
#endif
