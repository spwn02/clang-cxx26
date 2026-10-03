// RUN: %clang_cc1 -std=c++26 -triple x86_64-unknown-linux-gnu -fblocks -emit-llvm -o - %s | FileCheck %s --check-prefix=BLOCKS
// RUN: %clang_cc1 -std=c++26 -triple x86_64-unknown-linux-gnu -fopenmp -emit-llvm -o - %s | FileCheck %s --check-prefix=OMP
// RUN: %clang_cc1 -std=c++26 -triple x86_64-unknown-linux-gnu -fblocks -fopenmp -emit-llvm -debug-info-kind=limited -o - %s | FileCheck %s --check-prefix=BLOCKS

// Blocks and outlined OpenMP regions are separate functions after lowering,
// so a variable holding the address of an automatic object is not
// constexpr-representable inside them: it must be captured, never folded.

#ifdef _OPENMP
// OMP-LABEL: define{{.*}} @_Z3refv(
// OMP: call {{.*}} @__kmpc_fork_call
int ref() {
  int x = 1;
  int &r = x;
  int s = 0;
#pragma omp parallel
  { s += r; }
  return s;
}

// OMP-LABEL: define{{.*}} @_Z3ptrv(
// OMP: call {{.*}} @__kmpc_fork_call
int ptr() {
  int x = 1;
  constexpr int *p = &x;
  int s = 0;
#pragma omp parallel
  { s += *p; }
  return s;
}
#endif

#ifdef __BLOCKS__
// BLOCKS-LABEL: define{{.*}} @_Z9block_refv(
// BLOCKS: %block.captured
int block_ref() {
  int x = 1;
  int &r = x;
  return ^{ return r; }();
}

// BLOCKS-LABEL: define{{.*}} @_Z9block_ptrv(
// BLOCKS: %block.captured
int block_ptr() {
  int x = 1;
  constexpr int *p = &x;
  return ^{ return *p; }();
}
#endif
