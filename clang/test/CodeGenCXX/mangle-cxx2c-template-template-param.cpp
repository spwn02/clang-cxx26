// RUN: %clang_cc1 -std=c++2c -emit-llvm -triple x86_64-linux-gnu -o - %s | FileCheck %s

// Concept and variable template template parameters used to reach the
// mangler with a bogus (invalid) nested-name-specifier and abort.

template <typename T> concept Small = sizeof(T) <= 4;
template <typename T> constexpr auto Sz = sizeof(T);

template <template <typename> auto V> struct VHolder {};

template <template <typename> auto V, typename T>
auto var_tt(VHolder<V>, T) -> decltype(V<T>) { return V<T>; }

template <template <typename> concept C, typename T> requires C<T>
void concept_tt(T) {}

template <template <typename> concept C>
struct Holder {
  template <typename T> requires C<T> static int f() { return 1; }
};

// CHECK-LABEL: define {{.*}} @_Z6callerv(
void caller() {
  var_tt(VHolder<Sz>{}, 1);
  concept_tt<Small>(1);
  Holder<Small>::f<int>();
}

// CHECK: @_Z6var_ttI2SziEDT1VIT0_EE7VHolderIT_ES1_(
// CHECK: @_Z10concept_ttI5SmalliQ1CIT0_EEvS1_(
// CHECK: @_ZN6HolderI5SmallE1fIiQ1CITL0__EEEiv(
