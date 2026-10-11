// RUN: %clang_cc1 -std=c++26 -emit-llvm %s -o - | FileCheck %s

extern "C" {
template<class T> T identity(T value) { return value; }
int ordinary(int value) { return value; }
}

int main() { return identity(1) + (int)identity(2.0); }

// CHECK: define dso_local i32 @ordinary(
// CHECK: define linkonce_odr i32 @_Z8identityIiET_S0_(
// CHECK: define linkonce_odr double @_Z8identityIdET_S0_(
