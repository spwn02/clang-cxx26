// RUN: %clang_cc1 -std=c++26 -fcontract-evaluation-semantic=ignore -emit-llvm -o - %s | FileCheck %s --check-prefix=IGNORE
// RUN: %clang_cc1 -std=c++29 -fcontract-evaluation-semantic=ignore -emit-llvm -o - %s | FileCheck %s --check-prefix=IGNORE
// RUN: %clang_cc1 -std=c++26 -fcontract-evaluation-semantic=observe -emit-llvm -o - %s | FileCheck %s --check-prefix=OBSERVE
// RUN: %clang_cc1 -std=c++29 -fcontract-evaluation-semantic=observe -emit-llvm -o - %s | FileCheck %s --check-prefix=OBSERVE
// RUN: %clang_cc1 -std=c++26 -fcontract-evaluation-semantic=enforce -emit-llvm -o - %s | FileCheck %s --check-prefix=ENFORCE
// RUN: %clang_cc1 -std=c++29 -fcontract-evaluation-semantic=enforce -emit-llvm -o - %s | FileCheck %s --check-prefix=ENFORCE
// RUN: %clang_cc1 -std=c++26 -fcontract-evaluation-semantic=quick_enforce -emit-llvm -o - %s | FileCheck %s --check-prefix=QUICK
// RUN: %clang_cc1 -std=c++29 -fcontract-evaluation-semantic=quick_enforce -emit-llvm -o - %s | FileCheck %s --check-prefix=QUICK
// RUN: %clang_cc1 -std=c++26 -DABI_LABELED -emit-llvm -o - %s | FileCheck %s --check-prefix=ABI
// RUN: %clang_cc1 -std=c++26 -emit-llvm -o - %s | FileCheck %s --check-prefix=ABI
// RUN: %clang_cc1 -std=c++26 -DABI_LABELED -fcontract-evaluation-semantic=ignore -emit-llvm -o - %s | FileCheck %s --check-prefix=ABI
// RUN: %clang_cc1 -std=c++26 -fcontract-evaluation-semantic=ignore -emit-llvm -o - %s | FileCheck %s --check-prefix=ABI
// RUN: %clang_cc1 -std=c++29 -DABI_LABELED -fcontract-evaluation-semantic=ignore -emit-llvm -o - %s | FileCheck %s --check-prefix=ABI
// RUN: %clang_cc1 -std=c++29 -fcontract-evaluation-semantic=ignore -emit-llvm -o - %s | FileCheck %s --check-prefix=ABI

namespace std::contracts::controls {
inline namespace __p4451 {
struct __enforced_t {};
inline constexpr __enforced_t enforced{};
}
}

void labeled(int x) { contract_assert<std::contracts::controls::enforced>(x > 0); }
void pre_labeled(int x)
    pre<std::contracts::controls::enforced>(x > 0) {}
int post_labeled(const int x)
    post<std::contracts::controls::enforced>(r : r > x) {
  return x + 1;
}
void plain(int x) { contract_assert(x > 0); }

// Compare the mangled name for equivalent plain and terminating declarations.
#ifdef ABI_LABELED
void abi(int x) pre<std::contracts::controls::enforced>(x > 0) {}
#else
void abi(int x) pre(x > 0) {}
#endif

// IGNORE-LABEL: define{{.*}} @_Z7labeledi(
// IGNORE: call void @__handle_contract_violation_v3(i32{{.*}} 3,
// IGNORE-LABEL: define{{.*}} @_Z11pre_labeledi(
// IGNORE: call void @__handle_contract_violation_v3(i32{{.*}} 3,
// IGNORE-LABEL: define{{.*}} @_Z12post_labeledi(
// IGNORE: call void @__handle_contract_violation_v3(i32{{.*}} 3,
// IGNORE-LABEL: define{{.*}} @_Z5plaini(
// IGNORE-NOT: call void @__handle_contract_violation_v3

// OBSERVE-LABEL: define{{.*}} @_Z7labeledi(
// OBSERVE: call void @__handle_contract_violation_v3(i32{{.*}} 3,
// OBSERVE-LABEL: define{{.*}} @_Z11pre_labeledi(
// OBSERVE: call void @__handle_contract_violation_v3(i32{{.*}} 3,
// OBSERVE-LABEL: define{{.*}} @_Z12post_labeledi(
// OBSERVE: call void @__handle_contract_violation_v3(i32{{.*}} 3,
// OBSERVE-LABEL: define{{.*}} @_Z5plaini(
// OBSERVE: call void @__handle_contract_violation_v3(i32{{.*}} 2,

// ENFORCE-LABEL: define{{.*}} @_Z7labeledi(
// ENFORCE: call void @__handle_contract_violation_v3(i32{{.*}} 3,
// ENFORCE-LABEL: define{{.*}} @_Z11pre_labeledi(
// ENFORCE: call void @__handle_contract_violation_v3(i32{{.*}} 3,
// ENFORCE-LABEL: define{{.*}} @_Z12post_labeledi(
// ENFORCE: call void @__handle_contract_violation_v3(i32{{.*}} 3,
// ENFORCE-LABEL: define{{.*}} @_Z5plaini(
// ENFORCE: call void @__handle_contract_violation_v3(i32{{.*}} 3,

// QUICK-LABEL: define{{.*}} @_Z7labeledi(
// QUICK: call void @llvm.trap()
// QUICK-LABEL: define{{.*}} @_Z11pre_labeledi(
// QUICK: call void @llvm.trap()
// QUICK-LABEL: define{{.*}} @_Z12post_labeledi(
// QUICK: call void @llvm.trap()
// QUICK-LABEL: define{{.*}} @_Z5plaini(
// QUICK: call void @llvm.trap()

// ABI-LABEL: define{{.*}} @_Z3abii(
