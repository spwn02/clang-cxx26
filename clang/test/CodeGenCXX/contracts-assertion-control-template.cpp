// RUN: %clang_cc1 -std=c++26 -fcontract-evaluation-semantic=ignore -emit-llvm -o - %s | FileCheck %s --check-prefix=IGNORE
// RUN: %clang_cc1 -std=c++26 -fcontract-evaluation-semantic=observe -emit-llvm -o - %s | FileCheck %s --check-prefix=OBSERVE
// RUN: %clang_cc1 -std=c++26 -ast-dump %s | FileCheck %s --check-prefix=AST

// P4451R2: the assertion-control-specifier of a function template (and of a
// member of a class template) survives instantiation (TreeTransform), so an
// instantiation is still a terminating assertion: it evaluates with the
// enforce semantic (kind 3) even where the plain one is ignored or observed.

namespace std::contracts::controls {
inline namespace __p4451 {
struct __enforced_t {};
inline constexpr __enforced_t enforced{};
}
}

template <class T> void tmpl(T x) pre<std::contracts::controls::enforced>(x > 0) {}
template <class T> void tmpl_plain(T x) pre(x > 0) {}
template <class T> struct Member {
  void f(T x) pre<std::contracts::controls::enforced>(x > 0) {}
  void g(T x) { contract_assert<std::contracts::controls::enforced>(x > 0); }
};

void use() {
  tmpl(1);
  tmpl_plain(1);
  Member<int>{}.f(1);
  Member<int>{}.g(1);
}

// The instantiations carry the control expression.
// AST: FunctionDecl{{.*}}tmpl 'void (int)'
// AST: ContractStmt{{.*}}<std::contracts::controls::enforced>
// AST: DeclRefExpr{{.*}}enforced

// IGNORE-LABEL: define{{.*}} @_Z4tmplIiEvT_(
// IGNORE: call void @__handle_contract_violation_v3(i32{{.*}} 3,
// IGNORE-LABEL: define{{.*}} @_Z10tmpl_plainIiEvT_(
// IGNORE-NOT: call void @__handle_contract_violation_v3
// IGNORE-LABEL: define{{.*}} @_ZN6MemberIiE1fEi(
// IGNORE: call void @__handle_contract_violation_v3(i32{{.*}} 3,
// IGNORE-LABEL: define{{.*}} @_ZN6MemberIiE1gEi(
// IGNORE: call void @__handle_contract_violation_v3(i32{{.*}} 3,

// OBSERVE-LABEL: define{{.*}} @_Z4tmplIiEvT_(
// OBSERVE: call void @__handle_contract_violation_v3(i32{{.*}} 3,
// OBSERVE-LABEL: define{{.*}} @_Z10tmpl_plainIiEvT_(
// OBSERVE: call void @__handle_contract_violation_v3(i32{{.*}} 2,
