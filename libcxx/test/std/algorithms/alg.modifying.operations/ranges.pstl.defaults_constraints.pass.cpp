//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// <algorithm>

// [algorithm.syn]: the execution-policy overloads of the ranges algorithms have the default value-type
// template arguments `class T = projected_value_t<I, Proj>` (so `{2}` can initialize `const T&`) and the
// permutable / indirect_binary_predicate / indirect_equivalence_relation constraints of the synopsis.

#include <algorithm>
#include <execution>
#include <span>
struct Pred{bool operator()(int x)const{return x>0;}};inline constexpr Pred pred;
template<class R>concept C_find=requires(R a){std::ranges::find(std::execution::seq,a,{2});};
static_assert(C_find<std::span<int>>,"POLICY_BRACES_FIND_REJECTED");
template<class R>concept C_count=requires(R a){std::ranges::count(std::execution::seq,a,{2});};
static_assert(C_count<std::span<int>>,"POLICY_BRACES_COUNT_REJECTED");
template<class R>concept C_contains=requires(R a){std::ranges::contains(std::execution::seq,a,{2});};
static_assert(C_contains<std::span<int>>,"POLICY_BRACES_CONTAINS_REJECTED");
template<class R>concept C_search_n=requires(R a){std::ranges::search_n(std::execution::seq,a,2,{2});};
static_assert(C_search_n<std::span<int>>,"POLICY_BRACES_SEARCH_N_REJECTED");
template<class R>concept C_fill=requires(R a){std::ranges::fill(std::execution::seq,a,{2});};
static_assert(C_fill<std::span<int>>,"POLICY_BRACES_FILL_REJECTED");
template<class R>concept C_fill_n=requires(R a){std::ranges::fill_n(std::execution::seq,a.begin(),2,{2});};
static_assert(C_fill_n<std::span<int>>,"POLICY_BRACES_FILL_N_REJECTED");
template<class R>concept C_replace=requires(R a){std::ranges::replace(std::execution::seq,a,{2},{3});};
static_assert(C_replace<std::span<int>>,"POLICY_BRACES_REPLACE_REJECTED");
template<class R>concept C_replace_if=requires(R a){std::ranges::replace_if(std::execution::seq,a,pred,{2});};
static_assert(C_replace_if<std::span<int>>,"POLICY_BRACES_REPLACE_IF_REJECTED");
template<class R>concept C_remove=requires(R a){std::ranges::remove(std::execution::seq,a,{2});};
static_assert(C_remove<std::span<int>>,"POLICY_BRACES_REMOVE_REJECTED");

template<class I>concept Mutable_reverse=requires(I i){std::ranges::reverse(std::execution::seq,i,i);};
static_assert(Mutable_reverse<int*>);
static_assert(!Mutable_reverse<const int*>,"CONST_REVERSE_ACCEPTED");
template<class I>concept Mutable_rotate=requires(I i){std::ranges::rotate(std::execution::seq,i,i,i);};
static_assert(Mutable_rotate<int*>);
static_assert(!Mutable_rotate<const int*>,"CONST_ROTATE_ACCEPTED");
template<class I>concept Mutable_remove=requires(I i){std::ranges::remove(std::execution::seq,i,i,1);};
static_assert(Mutable_remove<int*>);
static_assert(!Mutable_remove<const int*>,"CONST_REMOVE_ACCEPTED");
template<class I>concept Mutable_unique=requires(I i){std::ranges::unique(std::execution::seq,i,i);};
static_assert(Mutable_unique<int*>);
static_assert(!Mutable_unique<const int*>,"CONST_UNIQUE_ACCEPTED");
template<class I>concept Mutable_shift_left=requires(I i){std::ranges::shift_left(std::execution::seq,i,i,1);};
static_assert(Mutable_shift_left<int*>);
static_assert(!Mutable_shift_left<const int*>,"CONST_SHIFT_LEFT_ACCEPTED");
template<class I>concept Mutable_shift_right=requires(I i){std::ranges::shift_right(std::execution::seq,i,i,1);};
static_assert(Mutable_shift_right<int*>);
static_assert(!Mutable_shift_right<const int*>,"CONST_SHIFT_RIGHT_ACCEPTED");

int main(int, char**) { return 0; }
