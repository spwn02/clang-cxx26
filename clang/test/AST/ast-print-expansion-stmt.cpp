// RUN: %clang_cc1 -std=c++26 -fexpansion-statements -ast-print %s | FileCheck %s
// CHECK-LABEL: void iterable() {
// CHECK-NEXT:     template for (auto x : items) {
// CHECK-LABEL: void destructurable() {
// CHECK-NEXT:     template for (auto x : fields) {
// CHECK-LABEL: void init_list() {
// CHECK-NEXT:     template for (auto x : {5, 6}) {
// CHECK-LABEL: template <class T> void dependent(T range) {
// CHECK-NEXT:     template for (auto x : range) {
struct Iterable {
  constexpr const int *begin() const { return data; }
  constexpr const int *end() const { return data + 2; }
  int data[2];
};
constexpr Iterable items{{1, 2}};
struct Aggregate { int first; int second; };
constexpr Aggregate fields{3, 4};
void iterable() { template for (auto x : items) { (void)x; } }
void destructurable() { template for (auto x : fields) { (void)x; } }
void init_list() { template for (auto x : {5, 6}) { (void)x; } }
template<class T> void dependent(T range) { template for (auto x : range) { (void)x; } }
