// RUN: %clang_cc1 -std=c++2c -ast-print %s | FileCheck %s

template <typename T> concept C = true;
template <typename T> constexpr int V = 0;

// CHECK: template <template <typename ...> concept CC, template <typename> auto VV> struct S {
template <template <typename...> concept CC, template <typename> auto VV>
struct S {};

// CHECK: template <template <typename> typename TT> struct U {
template <template <typename> typename TT> struct U {};
// CHECK: template <template <typename> class TT> struct U2 {
template <template <typename> class TT> struct U2 {};
