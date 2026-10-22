//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <algorithm>, <memory>

// Every ranges:: overload with an execution policy ('Ep&& exec') of the synopses of N5050
// ([algorithm.syn], [memory.syn]) can be called with the arguments the declaration names (the
// parameters with a default are left out). The calls were generated from the declarations of the
// draft.

#include <algorithm>
#include <execution>
#include <functional>
#include <memory>
#include <numeric>
#include <ranges>
#include <span>

inline int a[8]{}, b[8]{}, c[8]{};
inline auto pred=[](int){return true;}; inline auto bpred=[](int,int){return true;}; inline auto bop=[](int x,int){return x;}; inline auto uop=[](int x){return x;}; inline auto fn=[](int){}; inline auto gen=[](){return 0;};
inline std::span<int> sp(int* p){return std::span<int>(p,8);}
inline constexpr auto& seq = std::execution::seq;
static_assert(requires { std::ranges::all_of(seq, a, a+8, pred); }, "all_of(I first, S last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::all_of(seq, sp(a), pred); }, "all_of(R&& r, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::any_of(seq, a, a+8, pred); }, "any_of(I first, S last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::any_of(seq, sp(a), pred); }, "any_of(R&& r, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::none_of(seq, a, a+8, pred); }, "none_of(I first, S last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::none_of(seq, sp(a), pred); }, "none_of(R&& r, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::contains(seq, a, a+8, 0); }, "contains(I first, S last, const T& value, Proj proj = {})");
static_assert(requires { std::ranges::contains(seq, sp(a), 0); }, "contains(R&& r, const T& value, Proj proj = {})");
static_assert(requires { std::ranges::contains_subrange(seq, a, a+8, b, b+8); }, "contains_subrange(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::contains_subrange(seq, sp(a), sp(b)); }, "contains_subrange(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::for_each(seq, a, a+8, fn); }, "for_each(I first, S last, Fun f, Proj proj = {})");
static_assert(requires { std::ranges::for_each(seq, sp(a), fn); }, "for_each(R&& r, Fun f, Proj proj = {})");
static_assert(requires { std::ranges::for_each_n(seq, a, 3, fn); }, "for_each_n(I first, iter_difference_t<I> n, Fun f, Proj proj = {})");
static_assert(requires { std::ranges::find(seq, a, a+8, 0); }, "find(I first, S last, const T& value, Proj proj = {})");
static_assert(requires { std::ranges::find(seq, sp(a), 0); }, "find(R&& r, const T& value, Proj proj = {})");
static_assert(requires { std::ranges::find_if(seq, a, a+8, pred); }, "find_if(I first, S last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::find_if(seq, sp(a), pred); }, "find_if(R&& r, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::find_if_not(seq, a, a+8, pred); }, "find_if_not(I first, S last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::find_if_not(seq, sp(a), pred); }, "find_if_not(R&& r, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::find_last(seq, a, a+8, 0); }, "find_last(I first, S last, const T& value, Proj proj = {})");
static_assert(requires { std::ranges::find_last(seq, sp(a), 0); }, "find_last(R&& r, const T& value, Proj proj = {})");
static_assert(requires { std::ranges::find_last_if(seq, a, a+8, pred); }, "find_last_if(I first, S last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::find_last_if(seq, sp(a), pred); }, "find_last_if(R&& r, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::find_last_if_not(seq, a, a+8, pred); }, "find_last_if_not(I first, S last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::find_last_if_not(seq, sp(a), pred); }, "find_last_if_not(R&& r, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::find_end(seq, a, a+8, b, b+8); }, "find_end(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::find_end(seq, sp(a), sp(b)); }, "find_end(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::find_first_of(seq, a, a+8, b, b+8); }, "find_first_of(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::find_first_of(seq, sp(a), sp(b)); }, "find_first_of(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::adjacent_find(seq, a, a+8); }, "adjacent_find(I first, S last, Pred pred = {}, Proj proj = {})");
static_assert(requires { std::ranges::adjacent_find(seq, sp(a)); }, "adjacent_find(R&& r, Pred pred = {}, Proj proj = {})");
static_assert(requires { std::ranges::count(seq, a, a+8, 0); }, "count(I first, S last, const T& value, Proj proj = {})");
static_assert(requires { std::ranges::count(seq, sp(a), 0); }, "count(R&& r, const T& value, Proj proj = {})");
static_assert(requires { std::ranges::count_if(seq, a, a+8, pred); }, "count_if(I first, S last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::count_if(seq, sp(a), pred); }, "count_if(R&& r, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::mismatch(seq, a, a+8, b, b+8); }, "mismatch(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::mismatch(seq, sp(a), sp(b)); }, "mismatch(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::equal(seq, a, a+8, b, b+8); }, "equal(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::equal(seq, sp(a), sp(b)); }, "equal(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::search(seq, a, a+8, b, b+8); }, "search(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::search(seq, sp(a), sp(b)); }, "search(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::search_n(seq, a, a+8, 3, 0); }, "search_n(I first, S last, iter_difference_t<I> count, const T& value, Pred pred = {}, Proj proj = {})");
static_assert(requires { std::ranges::search_n(seq, sp(a), 3, 0); }, "search_n(R&& r, range_difference_t<R> count, const T& value, Pred pred = {}, Proj proj = {})");
static_assert(requires { std::ranges::starts_with(seq, a, a+8, b, b+8); }, "starts_with(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::starts_with(seq, sp(a), sp(b)); }, "starts_with(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::ends_with(seq, a, a+8, b, b+8); }, "ends_with(I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::ends_with(seq, sp(a), sp(b)); }, "ends_with(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::copy(seq, a, a+8, c, c+8); }, "copy(I first, S last, O result, OutS result_last)");
static_assert(requires { std::ranges::copy(seq, sp(a), sp(c)); }, "copy(R&& r, OutR&& result_r)");
static_assert(requires { std::ranges::copy_n(seq, a, 3, c, c+8); }, "copy_n(I first, iter_difference_t<I> n, O result, OutS result_last)");
static_assert(requires { std::ranges::copy_if(seq, a, a+8, c, c+8, pred); }, "copy_if(I first, S last, O result, OutS result_last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::copy_if(seq, sp(a), sp(c), pred); }, "copy_if(R&& r, OutR&& result_r, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::move(seq, a, a+8, c, c+8); }, "move(I first, S last, O result, OutS result_last)");
static_assert(requires { std::ranges::move(seq, sp(a), sp(c)); }, "move(R&& r, OutR&& result_r)");
static_assert(requires { std::ranges::swap_ranges(seq, a, a+8, b, b+8); }, "swap_ranges(I1 first1, S1 last1, I2 first2, S2 last2)");
static_assert(requires { std::ranges::swap_ranges(seq, sp(a), sp(b)); }, "swap_ranges(R1&& r1, R2&& r2)");
static_assert(requires { std::ranges::transform(seq, a, a+8, c, c+8, uop); }, "transform(I first1, S last1, O result, OutS result_last, F op, Proj proj = {})");
static_assert(requires { std::ranges::transform(seq, sp(a), sp(c), uop); }, "transform(R&& r, OutR&& result_r, F op, Proj proj = {})");
static_assert(requires { std::ranges::transform(seq, a, a+8, b, b+8, c, c+8, bop); }, "transform(I1 first1, S1 last1, I2 first2, S2 last2, O result, OutS result_last, F binary_op, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::transform(seq, sp(a), sp(b), sp(c), bop); }, "transform(R1&& r1, R2&& r2, OutR&& result_r, F binary_op, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::replace(seq, a, a+8, 0, 0); }, "replace(I first, S last, const T1& old_value, const T2& new_value, Proj proj = {})");
static_assert(requires { std::ranges::replace(seq, sp(a), 0, 0); }, "replace(R&& r, const T1& old_value, const T2& new_value, Proj proj = {})");
static_assert(requires { std::ranges::replace_if(seq, a, a+8, pred, 0); }, "replace_if(I first, S last, Pred pred, const T& new_value, Proj proj = {})");
static_assert(requires { std::ranges::replace_if(seq, sp(a), pred, 0); }, "replace_if(R&& r, Pred pred, const T& new_value, Proj proj = {})");
static_assert(requires { std::ranges::replace_copy(seq, a, a+8, c, c+8, 0, 0); }, "replace_copy(I first, S last, O result, OutS result_last, const T1& old_value, const T2& new_value, Proj proj = {})");
static_assert(requires { std::ranges::replace_copy(seq, sp(a), sp(c), 0, 0); }, "replace_copy(R&& r, OutR&& result_r, const T1& old_value, const T2& new_value, Proj proj = {})");
static_assert(requires { std::ranges::replace_copy_if(seq, a, a+8, c, c+8, pred, 0); }, "replace_copy_if(I first, S last, O result, OutS result_last, Pred pred, const T& new_value, Proj proj = {})");
static_assert(requires { std::ranges::replace_copy_if(seq, sp(a), sp(c), pred, 0); }, "replace_copy_if(R&& r, OutR&& result_r, Pred pred, const T& new_value, Proj proj = {})");
static_assert(requires { std::ranges::fill(seq, a, a+8, 0); }, "fill(O first, S last, const T& value)");
static_assert(requires { std::ranges::fill(seq, sp(a), 0); }, "fill(R&& r, const T& value)");
static_assert(requires { std::ranges::fill_n(seq, a, 3, 0); }, "fill_n(O first, iter_difference_t<O> n, const T& value)");
static_assert(requires { std::ranges::remove(seq, a, a+8, 0); }, "remove(I first, S last, const T& value, Proj proj = {})");
static_assert(requires { std::ranges::remove(seq, sp(a), 0); }, "remove(R&& r, const T& value, Proj proj = {})");
static_assert(requires { std::ranges::remove_if(seq, a, a+8, pred); }, "remove_if(I first, S last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::remove_if(seq, sp(a), pred); }, "remove_if(R&& r, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::remove_copy(seq, a, a+8, c, c+8, 0); }, "remove_copy(I first, S last, O result, OutS result_last, const T& value, Proj proj = {})");
static_assert(requires { std::ranges::remove_copy(seq, sp(a), sp(c), 0); }, "remove_copy(R&& r, OutR&& result_r, const T& value, Proj proj = {})");
static_assert(requires { std::ranges::remove_copy_if(seq, a, a+8, c, c+8, pred); }, "remove_copy_if(I first, S last, O result, OutS result_last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::remove_copy_if(seq, sp(a), sp(c), pred); }, "remove_copy_if(R&& r, OutR&& result_r, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::unique(seq, a, a+8); }, "unique(I first, S last, C comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::unique(seq, sp(a)); }, "unique(R&& r, C comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::unique_copy(seq, a, a+8, c, c+8); }, "unique_copy(I first, S last, O result, OutS result_last, C comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::unique_copy(seq, sp(a), sp(c)); }, "unique_copy(R&& r, OutR&& result_r, C comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::reverse(seq, a, a+8); }, "reverse(I first, S last)");
static_assert(requires { std::ranges::reverse(seq, sp(a)); }, "reverse(R&& r)");
static_assert(requires { std::ranges::reverse_copy(seq, a, a+8, c, c+8); }, "reverse_copy(I first, S last, O result, OutS result_last)");
static_assert(requires { std::ranges::reverse_copy(seq, sp(a), sp(c)); }, "reverse_copy(R&& r, OutR&& result_r)");
static_assert(requires { std::ranges::rotate(seq, a, a+4, a+8); }, "rotate(I first, I middle, S last)");
static_assert(requires { std::ranges::rotate(seq, sp(a), std::span<int>(a).begin()+4); }, "rotate(R&& r, iterator_t<R> middle)");
static_assert(requires { std::ranges::rotate_copy(seq, a, a+4, a+8, c, c+8); }, "rotate_copy(I first, I middle, S last, O result, OutS result_last)");
static_assert(requires { std::ranges::rotate_copy(seq, sp(a), std::span<int>(a).begin()+4, sp(c)); }, "rotate_copy(R&& r, iterator_t<R> middle, OutR&& result_r)");
static_assert(requires { std::ranges::shift_left(seq, a, a+8, 3); }, "shift_left(I first, S last, iter_difference_t<I> n)");
static_assert(requires { std::ranges::shift_left(seq, sp(a), 3); }, "shift_left(R&& r, range_difference_t<R> n)");
static_assert(requires { std::ranges::shift_right(seq, a, a+8, 3); }, "shift_right(I first, S last, iter_difference_t<I> n)");
static_assert(requires { std::ranges::shift_right(seq, sp(a), 3); }, "shift_right(R&& r, range_difference_t<R> n)");
static_assert(requires { std::ranges::sort(seq, a, a+8); }, "sort(I first, S last, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::sort(seq, sp(a)); }, "sort(R&& r, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::stable_sort(seq, a, a+8); }, "stable_sort(I first, S last, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::stable_sort(seq, sp(a)); }, "stable_sort(R&& r, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::partial_sort(seq, a, a+4, a+8); }, "partial_sort(I first, I middle, S last, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::partial_sort(seq, sp(a), std::span<int>(a).begin()+4); }, "partial_sort(R&& r, iterator_t<R> middle, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::partial_sort_copy(seq, a, a+8, c, c+8); }, "partial_sort_copy(I1 first, S1 last, I2 result_first, S2 result_last, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::partial_sort_copy(seq, sp(a), sp(c)); }, "partial_sort_copy(R1&& r, R2&& result_r, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::is_sorted(seq, a, a+8); }, "is_sorted(I first, S last, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::is_sorted(seq, sp(a)); }, "is_sorted(R&& r, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::is_sorted_until(seq, a, a+8); }, "is_sorted_until(I first, S last, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::is_sorted_until(seq, sp(a)); }, "is_sorted_until(R&& r, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::nth_element(seq, a, a+4, a+8); }, "nth_element(I first, I nth, S last, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::nth_element(seq, sp(a), std::span<int>(a).begin()+4); }, "nth_element(R&& r, iterator_t<R> nth, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::is_partitioned(seq, a, a+8, pred); }, "is_partitioned(I first, S last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::is_partitioned(seq, sp(a), pred); }, "is_partitioned(R&& r, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::partition(seq, a, a+8, pred); }, "partition(I first, S last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::partition(seq, sp(a), pred); }, "partition(R&& r, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::stable_partition(seq, a, a+8, pred); }, "stable_partition(I first, S last, Pred pred, Proj proj = {})");
static_assert(requires { std::ranges::stable_partition(seq, sp(a), pred); }, "stable_partition(R&& r, Pred pred, Proj proj = {})");
// SKIP partition_copy(I first, S last, O1 out_true, OutS1 last_true, O2 out_false, OutS2 last_false, Pred pred, Proj proj = {})
// SKIP partition_copy(R&& r, OutR1&& out_true_r, OutR2&& out_false_r, Pred pred, Proj proj = {})
static_assert(requires { std::ranges::merge(seq, a, a+8, b, b+8, c, c+8); }, "merge(I1 first1, S1 last1, I2 first2, S2 last2, O result, OutS result_last, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::merge(seq, sp(a), sp(b), sp(c)); }, "merge(R1&& r1, R2&& r2, OutR&& result_r, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::inplace_merge(seq, a, a+4, a+8); }, "inplace_merge(I first, I middle, S last, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::inplace_merge(seq, sp(a), std::span<int>(a).begin()+4); }, "inplace_merge(R&& r, iterator_t<R> middle, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::includes(seq, a, a+8, b, b+8); }, "includes(I1 first1, S1 last1, I2 first2, S2 last2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::includes(seq, sp(a), sp(b)); }, "includes(R1&& r1, R2&& r2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::set_union(seq, a, a+8, b, b+8, c, c+8); }, "set_union(I1 first1, S1 last1, I2 first2, S2 last2, O result, OutS result_last, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::set_union(seq, sp(a), sp(b), sp(c)); }, "set_union(R1&& r1, R2&& r2, OutR&& result_r, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::set_intersection(seq, a, a+8, b, b+8, c, c+8); }, "set_intersection(I1 first1, S1 last1, I2 first2, S2 last2, O result, OutS result_last, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::set_intersection(seq, sp(a), sp(b), sp(c)); }, "set_intersection(R1&& r1, R2&& r2, OutR&& result_r, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::set_difference(seq, a, a+8, b, b+8, c, c+8); }, "set_difference(I1 first1, S1 last1, I2 first2, S2 last2, O result, OutS result_last, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::set_difference(seq, sp(a), sp(b), sp(c)); }, "set_difference(R1&& r1, R2&& r2, OutR&& result_r, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::set_symmetric_difference(seq, a, a+8, b, b+8, c, c+8); }, "set_symmetric_difference(I1 first1, S1 last1, I2 first2, S2 last2, O result, OutS result_last, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::set_symmetric_difference(seq, sp(a), sp(b), sp(c)); }, "set_symmetric_difference(R1&& r1, R2&& r2, OutR&& result_r, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::is_heap(seq, a, a+8); }, "is_heap(I first, S last, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::is_heap(seq, sp(a)); }, "is_heap(R&& r, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::is_heap_until(seq, a, a+8); }, "is_heap_until(I first, S last, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::is_heap_until(seq, sp(a)); }, "is_heap_until(R&& r, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::min(seq, sp(a)); }, "min(R&& r, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::max(seq, sp(a)); }, "max(R&& r, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::minmax(seq, sp(a)); }, "minmax(R&& r, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::min_element(seq, a, a+8); }, "min_element(I first, S last, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::min_element(seq, sp(a)); }, "min_element(R&& r, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::max_element(seq, a, a+8); }, "max_element(I first, S last, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::max_element(seq, sp(a)); }, "max_element(R&& r, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::minmax_element(seq, a, a+8); }, "minmax_element(I first, S last, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::minmax_element(seq, sp(a)); }, "minmax_element(R&& r, Comp comp = {}, Proj proj = {})");
static_assert(requires { std::ranges::lexicographical_compare(seq, a, a+8, b, b+8); }, "lexicographical_compare(I1 first1, S1 last1, I2 first2, S2 last2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");
static_assert(requires { std::ranges::lexicographical_compare(seq, sp(a), sp(b)); }, "lexicographical_compare(R1&& r1, R2&& r2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})");

static_assert(requires { std::ranges::uninitialized_default_construct(seq, a, a+8); }, "uninitialized_default_construct(I first, S last)");
static_assert(requires { std::ranges::uninitialized_default_construct(seq, sp(a)); }, "uninitialized_default_construct(R&& r)");
static_assert(requires { std::ranges::uninitialized_default_construct_n(seq, a, 3); }, "uninitialized_default_construct_n(I first, iter_difference_t<I> n)");
static_assert(requires { std::ranges::uninitialized_value_construct(seq, a, a+8); }, "uninitialized_value_construct(I first, S last)");
static_assert(requires { std::ranges::uninitialized_value_construct(seq, sp(a)); }, "uninitialized_value_construct(R&& r)");
static_assert(requires { std::ranges::uninitialized_value_construct_n(seq, a, 3); }, "uninitialized_value_construct_n(I first, iter_difference_t<I> n)");
static_assert(requires { std::ranges::uninitialized_copy(seq, a, a+8, c, c+8); }, "uninitialized_copy(I ifirst, S1 ilast, O ofirst, S2 olast)");
static_assert(requires { std::ranges::uninitialized_copy(seq, sp(a), sp(c)); }, "uninitialized_copy(IR&& in_range, OR&& out_range)");
static_assert(requires { std::ranges::uninitialized_copy_n(seq, a, 3, c, c+8); }, "uninitialized_copy_n(I ifirst, iter_difference_t<I> n, O ofirst, S olast)");
static_assert(requires { std::ranges::uninitialized_move(seq, a, a+8, c, c+8); }, "uninitialized_move(I ifirst, S1 ilast, O ofirst, S2 olast)");
static_assert(requires { std::ranges::uninitialized_move(seq, sp(a), sp(c)); }, "uninitialized_move(IR&& in_range, OR&& out_range)");
static_assert(requires { std::ranges::uninitialized_move_n(seq, a, 3, c, c+8); }, "uninitialized_move_n(I ifirst, iter_difference_t<I> n, O ofirst, S olast)");
static_assert(requires { std::ranges::uninitialized_fill(seq, a, a+8, 0); }, "uninitialized_fill(I first, S last, const T& x)");
static_assert(requires { std::ranges::uninitialized_fill(seq, sp(a), 0); }, "uninitialized_fill(R&& r, const T& x)");
static_assert(requires { std::ranges::uninitialized_fill_n(seq, a, 3, 0); }, "uninitialized_fill_n(I first, iter_difference_t<I> n, const T& x)");
static_assert(requires { std::ranges::destroy(seq, a, a+8); }, "destroy(I first, S last)");
static_assert(requires { std::ranges::destroy(seq, sp(a)); }, "destroy(R&& r)");
static_assert(requires { std::ranges::destroy_n(seq, a, 3); }, "destroy_n(I first, iter_difference_t<I> n)");

int main(int, char**) { return 0; }
