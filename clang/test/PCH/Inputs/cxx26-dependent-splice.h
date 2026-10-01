// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef CXX26_DEPENDENT_SPLICE_H
#define CXX26_DEPENDENT_SPLICE_H

using info = decltype(^^int);

struct P { using t = int; };
template <class T> struct TT {
  using type = T;
  static constexpr int s = 7;
};
namespace N {
constexpr int nv = 42;
using P = ::P;
template <class T> using TT = ::TT<T>;
namespace M { constexpr int nv = 42; }
}

// Exercise declaration, parentheses and braces with both splice NNS kinds.
template <info R> constexpr int names() {
  typename [:R:]::t x = 0;
  return x + typename [:R:]::t() + typename [:R:]::t{};
}
template <info R> constexpr int specs() {
  typename template [:R:]<int>::type x = 0;
  return x + typename template [:R:]<int>::type() +
         typename template [:R:]<int>::type{};
}

// Preserve the dependent target and the direct target of an alias to an alias.
template <info R> constexpr int alias_target() {
  namespace A = [:R:];
  namespace B = A;
  return A::nv + B::nv;
}

// #197 rebuilt these alias qualifiers as dependent splice NNSs.
template <info R> constexpr int alias_types() {
  namespace A = [:R:];
  typename A::P::t u = 0;
  typename A::template TT<int>::type v = 0;
  return u + v + A::template TT<int>::s;
}
template <info R> constexpr int qualified() {
  namespace A = [:R:]::M;
  namespace B = A;
  return A::nv + B::nv;
}

#endif
