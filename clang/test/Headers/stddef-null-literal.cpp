// RUN: %clang_cc1 -internal-isystem %S/../../lib/Headers -fsyntax-only -verify -Wsentinel -Wundef -std=c++03 %s
// RUN: %clang_cc1 -internal-isystem %S/../../lib/Headers -fsyntax-only -verify -Wsentinel -Wundef -std=c++11 %s
// RUN: %clang_cc1 -internal-isystem %S/../../lib/Headers -fsyntax-only -verify -Wsentinel -Wundef -std=c++26 %s
// RUN: %clang_cc1 -internal-isystem %S/../../lib/Headers -fsyntax-only -std=c++26 -ast-dump %s | FileCheck %s

// LWG4182: NULL is an implementation-defined null pointer constant that is a literal; __null is a keyword.
// The NULL macro keeps the diagnostics that __null had (sentinel, null conversion, null arithmetic).

#include <stddef.h>

#if NULL != 0 // -Wundef diagnoses __null here: it is an identifier, not a literal
#error "NULL must be usable in a preprocessor expression, i.e. it is a literal"
#endif

typedef char null_has_the_width_of_a_pointer[sizeof(NULL) == sizeof(void *) ? 1 : -1];

int *p = NULL;
int i = NULL; // expected-warning {{implicit conversion of NULL constant to 'int'}}
void (*fp)() = NULL;
// CHECK: VarDecl {{.*}} p 'int *'
// CHECK-NEXT: ImplicitCastExpr {{.*}} <NullToPointer>
// CHECK-NEXT: IntegerLiteral {{.*}} 0
// CHECK-NOT: GNUNullExpr

int sum(int a) { return a + NULL; } // expected-warning {{use of NULL in arithmetic operation}}

void sentinel(const char *, ...) __attribute__((sentinel)); // expected-note {{function has been explicitly marked sentinel here}}
void use() {
  sentinel("a", NULL); // NULL from the macro stays a valid sentinel
  sentinel("a", 0L);   // expected-warning {{missing sentinel in function call}}
}
