// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// CWG3184: pointer subtraction needs pointers to *similar* complete object types, not only to cv-variants of the same type.

int *a[2];
const int *const *p = a;
int **q = a;
const int *const *const *r = nullptr;
int ***s = nullptr;

__PTRDIFF_TYPE__ d1 = q - p;
__PTRDIFF_TYPE__ d2 = p - q;
__PTRDIFF_TYPE__ d3 = s - r;

// Types that are not similar are still ill-formed.
long **l = nullptr;
__PTRDIFF_TYPE__ d4 = q - l; // expected-error {{are not pointers to compatible types}}
void *v = nullptr;
__PTRDIFF_TYPE__ d5 = q - v; // expected-error {{arithmetic on a pointer to void}} expected-error {{are not pointers to compatible types}}
