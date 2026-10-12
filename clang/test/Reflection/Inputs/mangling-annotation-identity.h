#ifndef LLVM_CLANG_TEST_REFLECTION_INPUTS_MANGLING_ANNOTATION_IDENTITY_H
#define LLVM_CLANG_TEST_REFLECTION_INPUTS_MANGLING_ANNOTATION_IDENTITY_H
[[=1]] void annotated_function();
[[=1]] int annotated_variable;
struct [[=1]] AnnotatedType {};
struct Derived : [[=1]] AnnotatedType {};
[[=1]] void header_repeated();
[[=1]] void header_repeated();
#endif
