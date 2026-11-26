// RUN: %clang_cc1 -std=c++26 -ast-print %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -ast-dump %s | FileCheck %s --check-prefix=DUMP

namespace std::contracts::controls {
inline namespace __p4451 {
struct __enforced_t {};
inline constexpr __enforced_t enforced{};
}
}

int f(int x) pre<std::contracts::controls::enforced>(x > 0)
    post<std::contracts::controls::enforced>(r : r > 0) {
  contract_assert<std::contracts::controls::enforced>(x > 0);
  return x;
}

// CHECK: pre<std::contracts::controls::enforced>(x > 0)
// CHECK: post<std::contracts::controls::enforced>(r: r > 0)
// CHECK: contract_assert<std::contracts::controls::enforced>(x > 0);
// DUMP: ContractStmt{{.*}}<std::contracts::controls::enforced>
// DUMP-COUNT-3: DeclRefExpr{{.*}}enforced
