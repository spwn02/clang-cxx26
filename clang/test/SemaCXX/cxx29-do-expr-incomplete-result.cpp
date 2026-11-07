// RUN: %clang_cc1 -std=c++2d -fsyntax-only -fexceptions -fcxx-exceptions -verify %s

// The result type of a do-expression is a function return type in all but
// name: a non-reference, non-void type must be complete and not abstract.
// Found by compiling the code blocks of P2806R5 (block 28): with <span> not
// included the result type was incomplete and the compiler asserted in
// LookupSpecialMember while binding the temporary (#495).

struct Incomplete;       // expected-note 8{{forward declaration of 'Incomplete'}}
Incomplete make();       // expected-note 4{{'make' declared here}}
int use(int);
struct Abstract { virtual void f() = 0; }; // expected-note {{unimplemented pure virtual method 'f' in 'Abstract'}}
template <class T> struct Undefined;       // expected-note {{template is declared here}}

// The reduced case of the crash: a call that yields the incomplete type in
// the operand of do_return (an error of its own) inside a function argument.
int explicit_incomplete() {
  return use(do -> Incomplete { do_return make(); }); // expected-error {{calling 'make' with incomplete return type 'Incomplete'}} expected-error {{incomplete result type 'Incomplete' of do-expression}}
}

int explicit_incomplete_without_other_errors() {
  return use(do -> Incomplete { throw 1; }); // expected-error {{incomplete result type 'Incomplete' of do-expression}}
}

void explicit_incomplete_as_operand() {
  (void)(do -> Incomplete { do_return make(); }); // expected-error {{calling 'make' with incomplete return type 'Incomplete'}} expected-error {{incomplete result type 'Incomplete' of do-expression}}
}

int explicit_incomplete_template_id() {
  return use(do -> Undefined<int const> { throw 1; }); // expected-error {{implicit instantiation of undefined template 'Undefined<const int>'}}
}

void explicit_abstract() {
  (void)(do -> Abstract { throw 1; }); // expected-error {{return type 'Abstract' is an abstract class}}
}

// The deduced type is checked where the do_return operand is initialized: an
// incomplete class prvalue cannot be the operand, so there is no result type
// left to check.
void deduced_incomplete() {
  (void)(do { do_return make(); });          // expected-error {{calling 'make' with incomplete return type 'Incomplete'}}
  (void)(do -> auto { do_return make(); });  // expected-error {{calling 'make' with incomplete return type 'Incomplete'}}
}

// Neither an array nor a function type can be returned, so neither can be the
// result type of a do-expression (a function type asserted in CodeGen).
using Array = int[3];
using Fn = void();
void array_and_function() {
  (void)(do -> Array { throw 1; });  // expected-error {{do-expression cannot have array result type 'Array' (aka 'int[3]')}}
  (void)(do -> int[3] { throw 1; }); // expected-error {{do-expression cannot have array result type 'int[3]'}}
  (void)(do -> Fn { throw 1; });     // expected-error {{do-expression cannot have function result type 'Fn' (aka 'void ()')}}
  (void)(do -> void() { throw 1; }); // expected-error {{do-expression cannot have function result type 'void ()'}}
}
template <class T> void tmpl_array_and_function() {
  (void)(do -> T { throw 1; }); // expected-error {{do-expression cannot have array result type 'int[3]'}} expected-error {{do-expression cannot have function result type 'void ()'}}
}
template void tmpl_array_and_function<int[3]>();  // expected-note {{in instantiation of function template specialization 'tmpl_array_and_function<int[3]>' requested here}}
template void tmpl_array_and_function<void()>();  // expected-note {{in instantiation of function template specialization 'tmpl_array_and_function<void ()>' requested here}}

// References and void are fine: no object is created.
Incomplete &ref();
void ok() {
  Incomplete &r = do -> Incomplete & { do_return ref(); };
  (void)&r;
  (void)(do -> void { do_return; });
}

// In a template the type is checked on each instantiation.
template <class T> int tmpl() {
  return use(do -> T { throw 1; }); // expected-error {{incomplete result type 'Incomplete' of do-expression}}
}
template int tmpl<int>();
template int tmpl<Incomplete>(); // expected-note {{in instantiation of function template specialization 'tmpl<Incomplete>' requested here}}
