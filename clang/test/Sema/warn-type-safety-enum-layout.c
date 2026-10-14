// RUN: %clang_cc1 -std=c11 -fsyntax-only -verify=c %s
// RUN: %clang_cc1 -x c++ -std=c++17 -fsyntax-only -verify=cxx %s
// c-no-diagnostics

// Layout compatibility of enumerations: in C only the underlying type matters; in C++ ([dcl.enum], CWG3046) the
// values of the enumeration must be the same as well.
void F_func(void *ptr, int tag) __attribute__(( pointer_with_type_tag(f,1,2) ));

enum E1 { e1a = 0, e1b = 1 };
enum E2 { e2a = 0, e2b = 255 }; // the same underlying type as E1, but more values
struct S1 { enum E1 e; };
struct S2 { enum E2 e; };

static const int F_tag __attribute__(( type_tag_for_datatype(f, struct S1, layout_compatible) )) = 1;

void test(struct S2 *p) {
  F_func(p, F_tag); // cxx-warning {{doesn't match specified 'f' type tag}}
}
