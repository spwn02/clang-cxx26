//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// [algorithm.syn]: every execution-policy overload of a ranges algorithm in the synopsis exists and is callable.

// Generated from the execution-policy declarations of [algorithm.syn]; every one is instantiated once and never run.
#include <algorithm>
#include <array>
#include <execution>
#include <functional>
#include <initializer_list>

void instantiate_all(int* ip, std::array<int, 8>& rv) {
  (void)ip; (void)rv;
  // all_of: bool all_of(Ep&& exec, I first, S last, Pred pred, Proj proj = {});
  (void)std::ranges::all_of(std::execution::seq, ip, ip + 4, [](int){ return true; });
  // all_of: bool all_of(Ep&& exec, R&& r, Pred pred, Proj proj = {});
  (void)std::ranges::all_of(std::execution::seq, rv, [](int){ return true; });
  // any_of: bool any_of(Ep&& exec, I first, S last, Pred pred, Proj proj = {});
  (void)std::ranges::any_of(std::execution::seq, ip, ip + 4, [](int){ return true; });
  // any_of: bool any_of(Ep&& exec, R&& r, Pred pred, Proj proj = {});
  (void)std::ranges::any_of(std::execution::seq, rv, [](int){ return true; });
  // none_of: bool none_of(Ep&& exec, I first, S last, Pred pred, Proj proj = {});
  (void)std::ranges::none_of(std::execution::seq, ip, ip + 4, [](int){ return true; });
  // none_of: bool none_of(Ep&& exec, R&& r, Pred pred, Proj proj = {});
  (void)std::ranges::none_of(std::execution::seq, rv, [](int){ return true; });
  // contains: requires indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, const T*> bool contains(Ep&& exec, I 
  (void)std::ranges::contains(std::execution::seq, ip, ip + 4, 1);
  // contains: requires indirect_binary_predicate<ranges::equal_to, projected<iterator_t<R>, Proj>, const T*> bool contains(E
  (void)std::ranges::contains(std::execution::seq, rv, 1);
  // contains_subrange: requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2> bool contains_subrange(Ep&& exec, I1 first1, S1 las
  (void)std::ranges::contains_subrange(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // contains_subrange: requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2> bool contains_subrange(Ep&&
  (void)std::ranges::contains_subrange(std::execution::seq, rv, rv);
  // for_each: I for_each(Ep&& exec, I first, S last, Fun f, Proj proj = {});
  (void)std::ranges::for_each(std::execution::seq, ip, ip + 4, [](int&) {});
  // for_each: borrowed_iterator_t<R> for_each(Ep&& exec, R&& r, Fun f, Proj proj = {});
  (void)std::ranges::for_each(std::execution::seq, rv, [](int&) {});
  // for_each_n: I for_each_n(Ep&& exec, I first, iter_difference_t<I> n, Fun f, Proj proj = {});
  (void)std::ranges::for_each_n(std::execution::seq, ip, 2, [](int&) {});
  // find: requires indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, const T*> I find(Ep&& exec, I first, 
  (void)std::ranges::find(std::execution::seq, ip, ip + 4, 1);
  // find: requires indirect_binary_predicate<ranges::equal_to, projected<iterator_t<R>, Proj>, const T*> borrowed_iterat
  (void)std::ranges::find(std::execution::seq, rv, 1);
  // find_if: I find_if(Ep&& exec, I first, S last, Pred pred, Proj proj = {});
  (void)std::ranges::find_if(std::execution::seq, ip, ip + 4, [](int){ return true; });
  // find_if: borrowed_iterator_t<R> find_if(Ep&& exec, R&& r, Pred pred, Proj proj = {});
  (void)std::ranges::find_if(std::execution::seq, rv, [](int){ return true; });
  // find_if_not: I find_if_not(Ep&& exec, I first, S last, Pred pred, Proj proj = {});
  (void)std::ranges::find_if_not(std::execution::seq, ip, ip + 4, [](int){ return true; });
  // find_if_not: borrowed_iterator_t<R> find_if_not(Ep&& exec, R&& r, Pred pred, Proj proj = {});
  (void)std::ranges::find_if_not(std::execution::seq, rv, [](int){ return true; });
  // find_last: requires indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, const T*> subrange<I> find_last(Ep&& 
  (void)std::ranges::find_last(std::execution::seq, ip, ip + 4, 1);
  // find_last: requires indirect_binary_predicate<ranges::equal_to, projected<iterator_t<R>, Proj>, const T*> borrowed_subran
  (void)std::ranges::find_last(std::execution::seq, rv, 1);
  // find_last_if: subrange<I> find_last_if(Ep&& exec, I first, S last, Pred pred, Proj proj = {});
  (void)std::ranges::find_last_if(std::execution::seq, ip, ip + 4, [](int){ return true; });
  // find_last_if: borrowed_subrange_t<R> find_last_if(Ep&& exec, R&& r, Pred pred, Proj proj = {});
  (void)std::ranges::find_last_if(std::execution::seq, rv, [](int){ return true; });
  // find_last_if_not: subrange<I> find_last_if_not(Ep&& exec, I first, S last, Pred pred, Proj proj = {});
  (void)std::ranges::find_last_if_not(std::execution::seq, ip, ip + 4, [](int){ return true; });
  // find_last_if_not: borrowed_subrange_t<R> find_last_if_not(Ep&& exec, R&& r, Pred pred, Proj proj = {});
  (void)std::ranges::find_last_if_not(std::execution::seq, rv, [](int){ return true; });
  // find_end: requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2> subrange<I1> find_end(Ep&& exec, I1 first1, S1 last
  (void)std::ranges::find_end(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // find_end: requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2> borrowed_subrange_t<R1> fin
  (void)std::ranges::find_end(std::execution::seq, rv, rv);
  // find_first_of: requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2> I1 find_first_of(Ep&& exec, I1 first1, S1 last1, I2
  (void)std::ranges::find_first_of(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // find_first_of: requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2> borrowed_iterator_t<R1> fin
  (void)std::ranges::find_first_of(std::execution::seq, rv, rv);
  // adjacent_find: I adjacent_find(Ep&& exec, I first, S last, Pred pred = {}, Proj proj = {});
  (void)std::ranges::adjacent_find(std::execution::seq, ip, ip + 4);
  // adjacent_find: borrowed_iterator_t<R> adjacent_find(Ep&& exec, R&& r, Pred pred = {}, Proj proj = {});
  (void)std::ranges::adjacent_find(std::execution::seq, rv);
  // count: requires indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, const T*> iter_difference_t<I> count(
  (void)std::ranges::count(std::execution::seq, ip, ip + 4, 1);
  // count: requires indirect_binary_predicate<ranges::equal_to, projected<iterator_t<R>, Proj>, const T*> range_differenc
  (void)std::ranges::count(std::execution::seq, rv, 1);
  // count_if: iter_difference_t<I> count_if(Ep&& exec, I first, S last, Pred pred, Proj proj = {});
  (void)std::ranges::count_if(std::execution::seq, ip, ip + 4, [](int){ return true; });
  // count_if: range_difference_t<R> count_if(Ep&& exec, R&& r, Pred pred, Proj proj = {});
  (void)std::ranges::count_if(std::execution::seq, rv, [](int){ return true; });
  // mismatch: requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2> mismatch_result<I1, I2> mismatch(Ep&& exec, I1 firs
  (void)std::ranges::mismatch(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // mismatch: requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2> mismatch_result<borrowed_it
  (void)std::ranges::mismatch(std::execution::seq, rv, rv);
  // equal: requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2> bool equal(Ep&& exec, I1 first1, S1 last1, I2 first
  (void)std::ranges::equal(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // equal: requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2> bool equal(Ep&& exec, R1&& 
  (void)std::ranges::equal(std::execution::seq, rv, rv);
  // search: requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2> subrange<I1> search(Ep&& exec, I1 first1, S1 last1,
  (void)std::ranges::search(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // search: requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2> borrowed_subrange_t<R1> sea
  (void)std::ranges::search(std::execution::seq, rv, rv);
  // search_n: requires indirectly_comparable<I, const T*, Pred, Proj> subrange<I> search_n(Ep&& exec, I first, S last, iter_
  (void)std::ranges::search_n(std::execution::seq, ip, ip + 4, 2, 1);
  // search_n: requires indirectly_comparable<iterator_t<R>, const T*, Pred, Proj> borrowed_subrange_t<R> search_n(Ep&& exec,
  (void)std::ranges::search_n(std::execution::seq, rv, 2, 1);
  // starts_with: requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2> bool starts_with(Ep&& exec, I1 first1, S1 last1, I2
  (void)std::ranges::starts_with(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // starts_with: requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2> bool starts_with(Ep&& exec,
  (void)std::ranges::starts_with(std::execution::seq, rv, rv);
  // ends_with: requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2> bool ends_with(Ep&& exec, I1 first1, S1 last1, I2 f
  (void)std::ranges::ends_with(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // ends_with: requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2> bool ends_with(Ep&& exec, R
  (void)std::ranges::ends_with(std::execution::seq, rv, rv);
  // copy: requires indirectly_copyable<I, O> copy_result<I, O> copy(Ep&& exec, I first, S last, O result, OutS result_la
  (void)std::ranges::copy(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // copy: requires indirectly_copyable<iterator_t<R>, iterator_t<OutR>> copy_result<borrowed_iterator_t<R>, borrowed_ite
  (void)std::ranges::copy(std::execution::seq, rv, rv);
  // copy_n: requires indirectly_copyable<I, O> copy_n_result<I, O> copy_n(Ep&& exec, I first, iter_difference_t<I> n, O re
  (void)std::ranges::copy_n(std::execution::seq, ip, 2, ip, ip + 4);
  // copy_if: requires indirectly_copyable<I, O> copy_if_result<I, O> copy_if(Ep&& exec, I first, S last, O result, OutS res
  (void)std::ranges::copy_if(std::execution::seq, ip, ip + 4, ip, ip + 4, [](int){ return true; });
  // copy_if: requires indirectly_copyable<iterator_t<R>, iterator_t<OutR>> copy_if_result<borrowed_iterator_t<R>, borrowed_
  (void)std::ranges::copy_if(std::execution::seq, rv, rv, [](int){ return true; });
  // move: requires indirectly_movable<I, O> move_result<I, O> move(Ep&& exec, I first, S last, O result, OutS result_las
  (void)std::ranges::move(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // move: requires indirectly_movable<iterator_t<R>, iterator_t<OutR>> move_result<borrowed_iterator_t<R>, borrowed_iter
  (void)std::ranges::move(std::execution::seq, rv, rv);
  // swap_ranges: requires indirectly_swappable<I1, I2> swap_ranges_result<I1, I2> swap_ranges(Ep&& exec, I1 first1, S1 last1, I
  (void)std::ranges::swap_ranges(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // swap_ranges: requires indirectly_swappable<iterator_t<R1>, iterator_t<R2>> swap_ranges_result<borrowed_iterator_t<R1>, borr
  (void)std::ranges::swap_ranges(std::execution::seq, rv, rv);
  // transform: requires indirectly_writable<O, indirect_result_t<F&, projected<I, Proj>>> unary_transform_result<I, O> transf
  (void)std::ranges::transform(std::execution::seq, ip, ip + 4, ip, ip + 4, [](int x) { return x; });
  // transform: requires indirectly_writable<iterator_t<OutR>, indirect_result_t<F&, projected<iterator_t<R>, Proj>>> unary_tr
  (void)std::ranges::transform(std::execution::seq, rv, rv, [](int x) { return x; });
  // transform: requires indirectly_writable<O, indirect_result_t<F&, projected<I1, Proj1>, projected<I2, Proj2>>> binary_tran
  (void)std::ranges::transform(std::execution::seq, ip, ip + 4, ip, ip + 4, ip, ip + 4, std::plus<>{});
  // transform: requires indirectly_writable<iterator_t<OutR>, indirect_result_t<F&, projected<iterator_t<R1>, Proj1>, project
  (void)std::ranges::transform(std::execution::seq, rv, rv, rv, std::plus<>{});
  // replace: requires indirectly_writable<I, const T2&> && indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, 
  (void)std::ranges::replace(std::execution::seq, ip, ip + 4, 1, 1);
  // replace: requires indirectly_writable<iterator_t<R>, const T2&> && indirect_binary_predicate<ranges::equal_to, projecte
  (void)std::ranges::replace(std::execution::seq, rv, 1, 1);
  // replace_if: requires indirectly_writable<I, const T&> I replace_if(Ep&& exec, I first, S last, Pred pred, const T& new_val
  (void)std::ranges::replace_if(std::execution::seq, ip, ip + 4, [](int){ return true; }, 1);
  // replace_if: requires indirectly_writable<iterator_t<R>, const T&> borrowed_iterator_t<R> replace_if(Ep&& exec, R&& r, Pred
  (void)std::ranges::replace_if(std::execution::seq, rv, [](int){ return true; }, 1);
  // replace_copy: requires indirectly_copyable<I, O> && indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, const T1
  (void)std::ranges::replace_copy(std::execution::seq, ip, ip + 4, ip, ip + 4, 1, 1);
  // replace_copy: requires indirectly_copyable<iterator_t<R>, iterator_t<OutR>> && indirect_binary_predicate<ranges::equal_to, p
  (void)std::ranges::replace_copy(std::execution::seq, rv, rv, 1, 1);
  // replace_copy_if: requires indirectly_copyable<I, O> && indirectly_writable<O, const T&> replace_copy_if_result<I, O> replace_co
  (void)std::ranges::replace_copy_if(std::execution::seq, ip, ip + 4, ip, ip + 4, [](int){ return true; }, 1);
  // replace_copy_if: requires indirectly_copyable<iterator_t<R>, iterator_t<OutR>> && indirectly_writable<iterator_t<OutR>, const T
  (void)std::ranges::replace_copy_if(std::execution::seq, rv, rv, [](int){ return true; }, 1);
  // fill: requires indirectly_writable<O, const T&> O fill(Ep&& exec, O first, S last, const T& value);
  (void)std::ranges::fill(std::execution::seq, ip, ip + 4, 1);
  // fill: requires indirectly_writable<iterator_t<R>, const T&> borrowed_iterator_t<R> fill(Ep&& exec, R&& r, const T& v
  (void)std::ranges::fill(std::execution::seq, rv, 1);
  // fill_n: requires indirectly_writable<O, const T&> O fill_n(Ep&& exec, O first, iter_difference_t<O> n, const T& value)
  (void)std::ranges::fill_n(std::execution::seq, ip, 2, 1);
  // remove: requires permutable<I> && indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, const T*> subrange<I
  (void)std::ranges::remove(std::execution::seq, ip, ip + 4, 1);
  // remove: requires permutable<iterator_t<R>> && indirect_binary_predicate<ranges::equal_to, projected<iterator_t<R>, Pro
  (void)std::ranges::remove(std::execution::seq, rv, 1);
  // remove_if: requires permutable<I> subrange<I> remove_if(Ep&& exec, I first, S last, Pred pred, Proj proj = {});
  (void)std::ranges::remove_if(std::execution::seq, ip, ip + 4, [](int){ return true; });
  // remove_if: requires permutable<iterator_t<R>> borrowed_subrange_t<R> remove_if(Ep&& exec, R&& r, Pred pred, Proj proj = {
  (void)std::ranges::remove_if(std::execution::seq, rv, [](int){ return true; });
  // remove_copy: requires indirectly_copyable<I, O> && indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, const T*
  (void)std::ranges::remove_copy(std::execution::seq, ip, ip + 4, ip, ip + 4, 1);
  // remove_copy: requires indirectly_copyable<iterator_t<R>, iterator_t<OutR>> && indirect_binary_predicate<ranges::equal_to, p
  (void)std::ranges::remove_copy(std::execution::seq, rv, rv, 1);
  // remove_copy_if: requires indirectly_copyable<I, O> remove_copy_if_result<I, O> remove_copy_if(Ep&& exec, I first, S last, O re
  (void)std::ranges::remove_copy_if(std::execution::seq, ip, ip + 4, ip, ip + 4, [](int){ return true; });
  // remove_copy_if: requires indirectly_copyable<iterator_t<R>, iterator_t<OutR>> remove_copy_if_result<borrowed_iterator_t<R>, bo
  (void)std::ranges::remove_copy_if(std::execution::seq, rv, rv, [](int){ return true; });
  // unique: requires permutable<I> subrange<I> unique(Ep&& exec, I first, S last, C comp = {}, Proj proj = {});
  (void)std::ranges::unique(std::execution::seq, ip, ip + 4);
  // unique: requires permutable<iterator_t<R>> borrowed_subrange_t<R> unique(Ep&& exec, R&& r, C comp = {}, Proj proj = {}
  (void)std::ranges::unique(std::execution::seq, rv);
  // unique_copy: requires indirectly_copyable<I, O> unique_copy_result<I, O> unique_copy(Ep&& exec, I first, S last, O result, 
  (void)std::ranges::unique_copy(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // unique_copy: requires indirectly_copyable<iterator_t<R>, iterator_t<OutR>> unique_copy_result<borrowed_iterator_t<R>, borro
  (void)std::ranges::unique_copy(std::execution::seq, rv, rv);
  // reverse: requires permutable<I> I reverse(Ep&& exec, I first, S last);
  (void)std::ranges::reverse(std::execution::seq, ip, ip + 4);
  // reverse: requires permutable<iterator_t<R>> borrowed_iterator_t<R> reverse(Ep&& exec, R&& r);
  (void)std::ranges::reverse(std::execution::seq, rv);
  // reverse_copy: requires indirectly_copyable<I, O> reverse_copy_truncated_result<I, O> reverse_copy(Ep&& exec, I first, S last
  (void)std::ranges::reverse_copy(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // reverse_copy: requires indirectly_copyable<iterator_t<R>, iterator_t<OutR>> reverse_copy_truncated_result<borrowed_iterator_
  (void)std::ranges::reverse_copy(std::execution::seq, rv, rv);
  // rotate: requires permutable<I> subrange<I> rotate(Ep&& exec, I first, I middle, S last);
  (void)std::ranges::rotate(std::execution::seq, ip, ip + 2, ip + 4);
  // rotate: requires permutable<iterator_t<R>> borrowed_subrange_t<R> rotate(Ep&& exec, R&& r, iterator_t<R> middle);
  (void)std::ranges::rotate(std::execution::seq, rv, rv.begin() + 2);
  // rotate_copy: requires indirectly_copyable<I, O> rotate_copy_truncated_result<I, O> rotate_copy(Ep&& exec, I first, I middle
  (void)std::ranges::rotate_copy(std::execution::seq, ip, ip + 2, ip + 4, ip, ip + 4);
  // rotate_copy: requires indirectly_copyable<iterator_t<R>, iterator_t<OutR>> rotate_copy_truncated_result<borrowed_iterator_t
  (void)std::ranges::rotate_copy(std::execution::seq, rv, rv.begin() + 2, rv);
  // shift_left: requires permutable<I> subrange<I> shift_left(Ep&& exec, I first, S last, iter_difference_t<I> n);
  (void)std::ranges::shift_left(std::execution::seq, ip, ip + 4, 2);
  // shift_left: requires permutable<iterator_t<R>> borrowed_subrange_t<R> shift_left(Ep&& exec, R&& r, range_difference_t<R> n
  (void)std::ranges::shift_left(std::execution::seq, rv, 2);
  // shift_right: requires permutable<I> subrange<I> shift_right(Ep&& exec, I first, S last, iter_difference_t<I> n);
  (void)std::ranges::shift_right(std::execution::seq, ip, ip + 4, 2);
  // shift_right: requires permutable<iterator_t<R>> borrowed_subrange_t<R> shift_right(Ep&& exec, R&& r, range_difference_t<R> 
  (void)std::ranges::shift_right(std::execution::seq, rv, 2);
  // sort: requires sortable<I, Comp, Proj> I sort(Ep&& exec, I first, S last, Comp comp = {}, Proj proj = {});
  (void)std::ranges::sort(std::execution::seq, ip, ip + 4);
  // sort: requires sortable<iterator_t<R>, Comp, Proj> borrowed_iterator_t<R> sort(Ep&& exec, R&& r, Comp comp = {}, Pro
  (void)std::ranges::sort(std::execution::seq, rv);
  // stable_sort: requires sortable<I, Comp, Proj> I stable_sort(Ep&& exec, I first, S last, Comp comp = {}, Proj proj = {});
  (void)std::ranges::stable_sort(std::execution::seq, ip, ip + 4);
  // stable_sort: requires sortable<iterator_t<R>, Comp, Proj> borrowed_iterator_t<R> stable_sort(Ep&& exec, R&& r, Comp comp = 
  (void)std::ranges::stable_sort(std::execution::seq, rv);
  // partial_sort: requires sortable<I, Comp, Proj> I partial_sort(Ep&& exec, I first, I middle, S last, Comp comp = {}, Proj pro
  (void)std::ranges::partial_sort(std::execution::seq, ip, ip + 2, ip + 4);
  // partial_sort: requires sortable<iterator_t<R>, Comp, Proj> borrowed_iterator_t<R> partial_sort(Ep&& exec, R&& r, iterator_t<
  (void)std::ranges::partial_sort(std::execution::seq, rv, rv.begin() + 2);
  // partial_sort_copy: requires indirectly_copyable<I1, I2> && sortable<I2, Comp, Proj2> && indirect_strict_weak_order<Comp, projecte
  (void)std::ranges::partial_sort_copy(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // partial_sort_copy: requires indirectly_copyable<iterator_t<R1>, iterator_t<R2>> && sortable<iterator_t<R2>, Comp, Proj2> && indir
  (void)std::ranges::partial_sort_copy(std::execution::seq, rv, rv);
  // is_sorted: bool is_sorted(Ep&& exec, I first, S last, Comp comp = {}, Proj proj = {});
  (void)std::ranges::is_sorted(std::execution::seq, ip, ip + 4);
  // is_sorted: bool is_sorted(Ep&& exec, R&& r, Comp comp = {}, Proj proj = {});
  (void)std::ranges::is_sorted(std::execution::seq, rv);
  // is_sorted_until: I is_sorted_until(Ep&& exec, I first, S last, Comp comp = {}, Proj proj = {});
  (void)std::ranges::is_sorted_until(std::execution::seq, ip, ip + 4);
  // is_sorted_until: borrowed_iterator_t<R> is_sorted_until(Ep&& exec, R&& r, Comp comp = {}, Proj proj = {});
  (void)std::ranges::is_sorted_until(std::execution::seq, rv);
  // nth_element: requires sortable<I, Comp, Proj> I nth_element(Ep&& exec, I first, I nth, S last, Comp comp = {}, Proj proj = 
  (void)std::ranges::nth_element(std::execution::seq, ip, ip + 2, ip + 4);
  // nth_element: requires sortable<iterator_t<R>, Comp, Proj> borrowed_iterator_t<R> nth_element(Ep&& exec, R&& r, iterator_t<R
  (void)std::ranges::nth_element(std::execution::seq, rv, rv.begin() + 2);
  // is_partitioned: bool is_partitioned(Ep&& exec, I first, S last, Pred pred, Proj proj = {});
  (void)std::ranges::is_partitioned(std::execution::seq, ip, ip + 4, [](int){ return true; });
  // is_partitioned: bool is_partitioned(Ep&& exec, R&& r, Pred pred, Proj proj = {});
  (void)std::ranges::is_partitioned(std::execution::seq, rv, [](int){ return true; });
  // partition: requires permutable<I> subrange<I> partition(Ep&& exec, I first, S last, Pred pred, Proj proj = {});
  (void)std::ranges::partition(std::execution::seq, ip, ip + 4, [](int){ return true; });
  // partition: requires permutable<iterator_t<R>> borrowed_subrange_t<R> partition(Ep&& exec, R&& r, Pred pred, Proj proj = {
  (void)std::ranges::partition(std::execution::seq, rv, [](int){ return true; });
  // stable_partition: requires permutable<I> subrange<I> stable_partition(Ep&& exec, I first, S last, Pred pred, Proj proj = {});
  (void)std::ranges::stable_partition(std::execution::seq, ip, ip + 4, [](int){ return true; });
  // stable_partition: requires permutable<iterator_t<R>> borrowed_subrange_t<R> stable_partition(Ep&& exec, R&& r, Pred pred, Proj p
  (void)std::ranges::stable_partition(std::execution::seq, rv, [](int){ return true; });
  // partition_copy: requires indirectly_copyable<I, O1> && indirectly_copyable<I, O2> partition_copy_result<I, O1, O2> partition_c
  (void)std::ranges::partition_copy(std::execution::seq, ip, ip + 4, ip, ip + 4, ip, ip + 4, [](int){ return true; });
  // partition_copy: requires indirectly_copyable<iterator_t<R>, iterator_t<OutR1>> && indirectly_copyable<iterator_t<R>, iterator_
  (void)std::ranges::partition_copy(std::execution::seq, rv, rv, rv, [](int){ return true; });
  // merge: requires mergeable<I1, I2, O, Comp, Proj1, Proj2> merge_result<I1, I2, O> merge(Ep&& exec, I1 first1, S1 last1
  (void)std::ranges::merge(std::execution::seq, ip, ip + 4, ip, ip + 4, ip, ip + 4);
  // merge: requires mergeable<iterator_t<R1>, iterator_t<R2>, iterator_t<OutR>, Comp, Proj1, Proj2> merge_result<borrowed
  (void)std::ranges::merge(std::execution::seq, rv, rv, rv);
  // inplace_merge: requires sortable<I, Comp, Proj> I inplace_merge(Ep&& exec, I first, I middle, S last, Comp comp = {}, Proj pr
  (void)std::ranges::inplace_merge(std::execution::seq, ip, ip + 2, ip + 4);
  // inplace_merge: requires sortable<iterator_t<R>, Comp, Proj> borrowed_iterator_t<R> inplace_merge(Ep&& exec, R&& r, iterator_t
  (void)std::ranges::inplace_merge(std::execution::seq, rv, rv.begin() + 2);
  // includes: bool includes(Ep&& exec, I1 first1, S1 last1, I2 first2, S2 last2, Comp comp = {}, Proj1 proj1 = {}, Proj2 pro
  (void)std::ranges::includes(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // includes: bool includes(Ep&& exec, R1&& r1, R2&& r2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {});
  (void)std::ranges::includes(std::execution::seq, rv, rv);
  // set_union: requires mergeable<I1, I2, O, Comp, Proj1, Proj2> set_union_result<I1, I2, O> set_union(Ep&& exec, I1 first1, 
  (void)std::ranges::set_union(std::execution::seq, ip, ip + 4, ip, ip + 4, ip, ip + 4);
  // set_union: requires mergeable<iterator_t<R1>, iterator_t<R2>, iterator_t<OutR>, Comp, Proj1, Proj2> set_union_result<borr
  (void)std::ranges::set_union(std::execution::seq, rv, rv, rv);
  // set_intersection: requires mergeable<I1, I2, O, Comp, Proj1, Proj2> set_intersection_result<I1, I2, O> set_intersection(Ep&& exe
  (void)std::ranges::set_intersection(std::execution::seq, ip, ip + 4, ip, ip + 4, ip, ip + 4);
  // set_intersection: requires mergeable<iterator_t<R1>, iterator_t<R2>, iterator_t<OutR>, Comp, Proj1, Proj2> set_intersection_resu
  (void)std::ranges::set_intersection(std::execution::seq, rv, rv, rv);
  // set_difference: requires mergeable<I1, I2, O, Comp, Proj1, Proj2> set_difference_truncated_result<I1, I2, O> set_difference(Ep
  (void)std::ranges::set_difference(std::execution::seq, ip, ip + 4, ip, ip + 4, ip, ip + 4);
  // set_difference: requires mergeable<iterator_t<R1>, iterator_t<R2>, iterator_t<OutR>, Comp, Proj1, Proj2> set_difference_trunca
  (void)std::ranges::set_difference(std::execution::seq, rv, rv, rv);
  // set_symmetric_difference: requires mergeable<I1, I2, O, Comp, Proj1, Proj2> set_symmetric_difference_result<I1, I2, O> set_symmetric_dif
  (void)std::ranges::set_symmetric_difference(std::execution::seq, ip, ip + 4, ip, ip + 4, ip, ip + 4);
  // set_symmetric_difference: requires mergeable<iterator_t<R1>, iterator_t<R2>, iterator_t<OutR>, Comp, Proj1, Proj2> set_symmetric_differe
  (void)std::ranges::set_symmetric_difference(std::execution::seq, rv, rv, rv);
  // is_heap: bool is_heap(Ep&& exec, I first, S last, Comp comp = {}, Proj proj = {});
  (void)std::ranges::is_heap(std::execution::seq, ip, ip + 4);
  // is_heap: bool is_heap(Ep&& exec, R&& r, Comp comp = {}, Proj proj = {});
  (void)std::ranges::is_heap(std::execution::seq, rv);
  // is_heap_until: I is_heap_until(Ep&& exec, I first, S last, Comp comp = {}, Proj proj = {});
  (void)std::ranges::is_heap_until(std::execution::seq, ip, ip + 4);
  // is_heap_until: borrowed_iterator_t<R> is_heap_until(Ep&& exec, R&& r, Comp comp = {}, Proj proj = {});
  (void)std::ranges::is_heap_until(std::execution::seq, rv);
  // min: requires indirectly_copyable_storable<iterator_t<R>, range_value_t<R>*> range_value_t<R> min(Ep&& exec, R&& r,
  (void)std::ranges::min(std::execution::seq, rv);
  // max: requires indirectly_copyable_storable<iterator_t<R>, range_value_t<R>*> range_value_t<R> max(Ep&& exec, R&& r,
  (void)std::ranges::max(std::execution::seq, rv);
  // minmax: requires indirectly_copyable_storable<iterator_t<R>, range_value_t<R>*> minmax_result<range_value_t<R>> minmax
  (void)std::ranges::minmax(std::execution::seq, rv);
  // min_element: I min_element(Ep&& exec, I first, S last, Comp comp = {}, Proj proj = {});
  (void)std::ranges::min_element(std::execution::seq, ip, ip + 4);
  // min_element: borrowed_iterator_t<R> min_element(Ep&& exec, R&& r, Comp comp = {}, Proj proj = {});
  (void)std::ranges::min_element(std::execution::seq, rv);
  // max_element: I max_element(Ep&& exec, I first, S last, Comp comp = {}, Proj proj = {});
  (void)std::ranges::max_element(std::execution::seq, ip, ip + 4);
  // max_element: borrowed_iterator_t<R> max_element(Ep&& exec, R&& r, Comp comp = {}, Proj proj = {});
  (void)std::ranges::max_element(std::execution::seq, rv);
  // minmax_element: minmax_element_result<I> minmax_element(Ep&& exec, I first, S last, Comp comp = {}, Proj proj = {});
  (void)std::ranges::minmax_element(std::execution::seq, ip, ip + 4);
  // minmax_element: minmax_element_result<borrowed_iterator_t<R>> minmax_element(Ep&& exec, R&& r, Comp comp = {}, Proj proj = {})
  (void)std::ranges::minmax_element(std::execution::seq, rv);
  // lexicographical_compare: bool lexicographical_compare(Ep&& exec, I1 first1, S1 last1, I2 first2, S2 last2, Comp comp = {}, Proj1 proj1 
  (void)std::ranges::lexicographical_compare(std::execution::seq, ip, ip + 4, ip, ip + 4);
  // lexicographical_compare: bool lexicographical_compare(Ep&& exec, R1&& r1, R2&& r2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {});
  (void)std::ranges::lexicographical_compare(std::execution::seq, rv, rv);
}
