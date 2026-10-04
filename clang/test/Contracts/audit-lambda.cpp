// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// [expr.prim.lambda.capture]: a capture-default is permitted if the lambda
// "appears within a contract assertion and its innermost enclosing scope is
// the corresponding contract-assertion scope".
void empty_copy() pre([=]{ return true; }());
void empty_reference() pre([&]{ return true; }());
void parameter_copy(int x) pre([=]() mutable { ++x; return true; }());
void parameter_reference(int x) pre([&]{ return x > 0; }());
int result_copy() post(r: [=]() mutable { ++r; return true; }());
int result_reference() post(r: [&]{ return r > 0; }());

// The exception is scoped to the contract assertion.
auto outside = [=]{ return true; }; // expected-error {{non-local lambda expression cannot have a capture-default}}

// [expr.prim.id.unqual], draft's result-binding example (h), verbatim:
struct Y {
  int h()
    post(r : ++r)   // expected-error {{cannot assign to variable 'r' because it is considered 'const' inside of a contract}}
    post(r: [=] mutable {
       ++r;         // OK, refers to member of closure type
       return true;
     }());
};
