// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce %s -o %t
// RUN: %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -O2 %s -o %t
// RUN: %t

// [expr.prim.lambda.capture]: a capture-default is permitted if the lambda
// "appears within a contract assertion and its innermost enclosing scope is
// the corresponding contract-assertion scope".
// [expr.prim.id.unqual] permits modifying the copy: "++p; // OK, refers to
// member of closure type" in the mutable-copy example.
void f(int x) pre([x]() mutable { ++x; return x == 8; }()) {}
void default_copy(int x) pre([=]() mutable { ++x; return x == 8; }()) {}
void default_empty() pre([=]{ return true; }()) {}
int result_copy() post(r: [=]() mutable { ++r; return r == 8; }()) { return 7; }
int main() {
  f(7);
  default_copy(7);
  default_empty();
  return result_copy() != 7;
}
