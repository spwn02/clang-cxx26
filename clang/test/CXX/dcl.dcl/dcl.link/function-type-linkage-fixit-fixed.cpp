// RUN: %clang_cc1 -std=c++26 -fsyntax-only -Werror %s

// The code that the notes of function-type-linkage-fixit-text.cpp lead to is well-formed: the first declaration of
// a function has the linkage-specification, a function with a storage class is declared in a block.

extern "C" { typedef void (*CFnPtr)(int); }
void takes_c(CFnPtr);

extern "C" void handler(int) {}
extern "C" {
static void local(int) {}
}
extern "C" void forward(int);
namespace {
extern "C" void hidden(int) {}
}
void forward(int) {} // a redeclaration without a linkage-specification keeps the linkage

void use() {
  takes_c(handler);
  takes_c(&local);
  takes_c(hidden);
  takes_c(forward);
}
