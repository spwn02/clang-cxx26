// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// A function with C++ language linkage that is passed where a pointer to a function with C language
// linkage is expected gets a note that says how to give it C language linkage, with a fix-it.

extern "C" { typedef void (*CPtr)(int); }
typedef void (*XPtr)(int);
void takes_c(CPtr);
void takes_x(XPtr);

void handler(int) {} // expected-note {{give 'handler' C language linkage by declaring it 'extern "C"' so that its type matches}}
static void local(int) {} // expected-note {{give 'local' C language linkage by declaring it inside an 'extern "C"' block so that its type matches}}
namespace {
void hidden(int) {} // expected-note {{give 'hidden' C language linkage by declaring it 'extern "C"' so that its type matches}}
}
void forward(int); // expected-note {{give 'forward' C language linkage by declaring it 'extern "C"' so that its type matches}}
void forward(int) {}
extern "C" void already_c(int);
struct S { static void member(int); }; // no note for a member function
template <class T> void tmpl(int) {}   // no note for a template

void use() {
  takes_c(handler); // expected-error {{changes the language linkage of the function type}}
  takes_c(&local);  // expected-error {{changes the language linkage of the function type}}
  takes_c(hidden);  // expected-error {{changes the language linkage of the function type}}
  takes_c(forward); // expected-error {{changes the language linkage of the function type}}
  takes_c(already_c);
  takes_c(S::member);  // expected-error {{changes the language linkage of the function type}}
  takes_c(tmpl<int>);  // expected-error {{changes the language linkage of the function type}}
  takes_x(already_c);  // expected-error {{changes the language linkage of the function type}}
}
