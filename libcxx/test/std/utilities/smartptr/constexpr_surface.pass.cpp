//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS(has-fconstexpr-steps): -fconstexpr-steps=5000000

// P3037R6 (constexpr shared_ptr and make_shared), P2273R3 (constexpr unique_ptr), P3378R2 (constexpr exception
// types), P1132R8/P3037R6 (out_ptr, inout_ptr): every function the draft declares constexpr in [smartptr]
// ([unique.ptr], [util.smartptr]) used in a constant expression, and again at run time. The draft leaves
// owner_before, owner_hash, reinterpret_pointer_cast, hash, the stream inserters and the void** conversion of
// out_ptr/inout_ptr non-constexpr.

#include <compare>
#include <functional>
#include <cassert>
#include <memory>
#include <string_view>
#include <utility>

struct Del { constexpr void operator()(int* p) const { delete p; } };
struct ADel { constexpr void operator()(int* p) const { delete[] p; } };
struct B { virtual constexpr ~B() = default; int n = 7; };
struct D : B {};
struct E : std::enable_shared_from_this<E> { int v = 3; };

#define CASE(name, ...)                                                                                                \
  constexpr bool name() { return __VA_ARGS__; }                                                                        \
  static_assert(name(), #name);

CASE(up_default, []{ std::unique_ptr<int> p; return !p; }())
CASE(up_ptr, []{ std::unique_ptr<int> p(new int(3)); return *p == 3 && static_cast<bool>(p); }())
CASE(up_deleter, []{ std::unique_ptr<int, Del> p(new int(3), Del{}); return *p == 3; }())
CASE(up_move, []{ std::unique_ptr<int> a(new int(3)); std::unique_ptr<int> b(std::move(a)); return !a && *b == 3; }())
CASE(up_convert, []{ std::unique_ptr<D> a(new D); std::unique_ptr<B> b(std::move(a)); return b->n == 7; }())
CASE(up_assign, []{ std::unique_ptr<int> a(new int(3)), b; b = std::move(a); b = nullptr; return !b; }())
CASE(up_reset, []{ std::unique_ptr<int> a(new int(3)); a.reset(new int(4)); a.reset(); return a.get() == nullptr; }())
CASE(up_release, []{ std::unique_ptr<int> a(new int(3)); int* r = a.release(); bool ok = *r == 3; delete r; return ok; }())
CASE(up_swap, []{ std::unique_ptr<int> a(new int(1)), b(new int(2)); a.swap(b); std::swap(a, b); return *a == 1 && *b == 2; }())
CASE(up_getdel, []{ std::unique_ptr<int, Del> a(new int(1)); a.get_deleter()(nullptr); return true; }())
CASE(up_arrow, []{ std::unique_ptr<D> a(new D); return a->n == 7; }())
CASE(up_array, []{ std::unique_ptr<int[]> a(new int[3]{1,2,3}); return a[1] == 2; }())
CASE(up_array_del, []{ std::unique_ptr<int[], ADel> a(new int[2]{4,5}, ADel{}); return a[1] == 5; }())
CASE(up_array_reset, []{ std::unique_ptr<int[]> a; a.reset(new int[2]{1,2}); return a[0] == 1; }())
CASE(make_unique_, []{ auto p = std::make_unique<int>(4); return *p == 4; }())
CASE(make_unique_arr, []{ auto p = std::make_unique<int[]>(3); return p[2] == 0; }())
CASE(make_unique_ow, []{ auto p = std::make_unique_for_overwrite<int>(); *p = 5; return *p == 5; }())
CASE(make_unique_ow_arr, []{ auto p = std::make_unique_for_overwrite<int[]>(3); p[0] = 1; return p[0] == 1; }())
CASE(up_eq, []{ std::unique_ptr<int> a, b; return a == b && !(a != b) && a == nullptr && nullptr == a; }())
CASE(up_rel, []{ std::unique_ptr<int> a, b; return !(a < b) && !(a > b) && a <= b && a >= b; }())
CASE(up_rel_null, []{ std::unique_ptr<int> a; return !(a < nullptr) && !(nullptr < a) && a <= nullptr && nullptr >= a; }())
CASE(up_3way, []{ std::unique_ptr<int> a; std::unique_ptr<long> b; return (a <=> a) == 0 && (a <=> nullptr) == 0; }())
CASE(default_delete_, []{ std::default_delete<int>{}(new int(1)); std::default_delete<int[]>{}(new int[2]); return true; }())
CASE(sp_default, []{ std::shared_ptr<int> p; return !p && p.use_count() == 0; }())
CASE(sp_ptr, []{ std::shared_ptr<int> p(new int(7)); return *p == 7 && p.use_count() == 1; }())
CASE(sp_del, []{ std::shared_ptr<int> p(new int(7), Del{}); return *p == 7; }())
CASE(sp_del_alloc, []{ std::shared_ptr<int> p(new int(7), Del{}, std::allocator<int>{}); return *p == 7; }())
CASE(sp_default_del_alloc, []{ std::shared_ptr<int> p(new int(7), std::default_delete<int>{}, std::allocator<int>{}); return *p == 7; }())
CASE(sp_nullptr_del, []{ std::shared_ptr<int> p(nullptr, Del{}); return !p; }())
CASE(sp_nullptr_del_alloc, []{ std::shared_ptr<int> p(nullptr, Del{}, std::allocator<int>{}); return !p; }())
CASE(sp_alias, []{ std::shared_ptr<int> a(new int(1)); std::shared_ptr<int> b(a, a.get()); return b.use_count() == 2; }())
CASE(sp_copy, []{ std::shared_ptr<int> a(new int(1)); auto b = a; return a.use_count() == 2 && b == a; }())
CASE(sp_move, []{ std::shared_ptr<int> a(new int(1)); auto b = std::move(a); return !a && b.use_count() == 1; }())
CASE(sp_convert, []{ std::shared_ptr<D> a(new D); std::shared_ptr<B> b(a); return b->n == 7 && b.use_count() == 2; }())
CASE(sp_from_weak, []{ std::shared_ptr<int> a(new int(1)); std::weak_ptr<int> w(a); std::shared_ptr<int> b(w); return b.use_count() == 2; }())
CASE(sp_from_unique, []{ std::unique_ptr<int> u(new int(2)); std::shared_ptr<int> p(std::move(u)); return *p == 2; }())
CASE(sp_assign, []{ std::shared_ptr<int> a(new int(1)), b; b = a; b = std::move(a); std::shared_ptr<int> c; c = std::unique_ptr<int>(new int(3)); return *b == 1 && *c == 3; }())
CASE(sp_swap, []{ std::shared_ptr<int> a(new int(1)), b(new int(2)); a.swap(b); std::swap(a, b); return *a == 1; }())
CASE(sp_reset, []{ std::shared_ptr<int> a(new int(1)); a.reset(); a.reset(new int(2)); a.reset(new int(3), Del{}); a.reset(new int(4), Del{}, std::allocator<int>{}); return *a == 4; }())
CASE(sp_observers, []{ std::shared_ptr<D> a(new D); return a.get() == &*a && a->n == 7 && static_cast<bool>(a); }())
CASE(sp_array_index, []{ std::shared_ptr<int[]> a(new int[3]{1,2,3}); return a[2] == 3; }())
CASE(sp_owner_equal, []{ std::shared_ptr<int> a(new int(1)); auto b = a; std::weak_ptr<int> w(a); return std::owner_equal{}(a, b) && std::owner_equal{}(a, w) && std::owner_equal{}(w, a) && std::owner_equal{}(w, w); }())
CASE(make_shared_, []{ auto p = std::make_shared<int>(5); return *p == 5; }())
CASE(make_shared_arr, []{ auto p = std::make_shared<int[]>(3); return p[1] == 0; }())
CASE(make_shared_arr_init, []{ auto p = std::make_shared<int[]>(3, 9); return p[1] == 9; }())
CASE(make_shared_bound, []{ auto p = std::make_shared<int[3]>(); return p[2] == 0; }())
CASE(make_shared_bound_init, []{ auto p = std::make_shared<int[3]>(4); return p[2] == 4; }())
CASE(make_shared_md, []{ auto p = std::make_shared<int[2][3]>(); return p[1][2] == 0; }())
CASE(make_shared_md_init, []{ auto p = std::make_shared<int[2][3]>({4,5,6}); return p[1][2] == 6 && p[0][0] == 4; }())
CASE(make_shared_md_rt, []{ auto p = std::make_shared<int[][3]>(2); return p[1][2] == 0; }())
CASE(make_shared_ow, []{ auto p = std::make_shared_for_overwrite<int>(); *p = 7; return *p == 7; }())
CASE(make_shared_ow_arr, []{ auto p = std::make_shared_for_overwrite<int[]>(3); p[1] = 7; return p[1] == 7; }())
CASE(make_shared_ow_bound, []{ auto p = std::make_shared_for_overwrite<int[3]>(); p[1] = 7; return p[1] == 7; }())
CASE(alloc_shared, []{ auto p = std::allocate_shared<int>(std::allocator<int>{}, 5); return *p == 5; }())
CASE(alloc_shared_arr, []{ auto p = std::allocate_shared<int[]>(std::allocator<int>{}, 3); return p[0] == 0; }())
CASE(alloc_shared_md, []{ auto p = std::allocate_shared<int[2][3]>(std::allocator<int>{}); return p[1][1] == 0; }())
CASE(alloc_shared_ow, []{ auto p = std::allocate_shared_for_overwrite<int>(std::allocator<int>{}); *p = 1; return *p == 1; }())
CASE(alloc_shared_ow_arr, []{ auto p = std::allocate_shared_for_overwrite<int[]>(std::allocator<int>{}, 2); p[0] = 1; return p[0] == 1; }())
CASE(sp_cmp, []{ std::shared_ptr<int> a, b; return a == b && a == nullptr && !(a < b) && (a <=> b) == 0 && (a <=> nullptr) == 0 && a <= b; }())
CASE(sp_cast_static, []{ std::shared_ptr<D> a(new D); auto b = std::static_pointer_cast<B>(a); return b->n == 7; }())
CASE(sp_cast_dyn, []{ std::shared_ptr<B> a(new D); auto b = std::dynamic_pointer_cast<D>(a); return b != nullptr; }())
CASE(sp_cast_const, []{ std::shared_ptr<const int> a(new int(1)); auto b = std::const_pointer_cast<int>(a); return *b == 1; }())
CASE(sp_getdeleter, []{ std::shared_ptr<int> a(new int(1), Del{}); return std::get_deleter<Del>(a) != nullptr; }())
CASE(wp_default, []{ std::weak_ptr<int> w; return w.expired() && w.use_count() == 0 && !w.lock(); }())
CASE(wp_from_sp, []{ std::shared_ptr<int> a(new int(1)); std::weak_ptr<int> w(a); return !w.expired() && w.use_count() == 1 && *w.lock() == 1; }())
CASE(wp_copy_move, []{ std::shared_ptr<int> a(new int(1)); std::weak_ptr<int> w(a), x(w), y(std::move(x)); return y.use_count() == 1; }())
CASE(wp_assign, []{ std::shared_ptr<int> a(new int(1)); std::weak_ptr<int> w, x; w = a; x = w; x = std::move(w); x = a; return x.use_count() == 1; }())
CASE(wp_reset_swap, []{ std::shared_ptr<int> a(new int(1)); std::weak_ptr<int> w(a), x; w.swap(x); std::swap(w, x); w.reset(); return w.expired(); }())
CASE(wp_expire, []{ std::weak_ptr<int> w; { std::shared_ptr<int> a(new int(1)); w = a; } return w.expired(); }())
CASE(wp_convert, []{ std::shared_ptr<D> a(new D); std::weak_ptr<D> w(a); std::weak_ptr<B> x(w); return !x.expired(); }())
CASE(esft, []{ auto p = std::make_shared<E>(); auto q = p->shared_from_this(); auto w = p->weak_from_this(); return q.use_count() == 2 && !w.expired(); }())
CASE(bad_weak, []{ std::bad_weak_ptr e; return std::string_view(e.what()) == "bad_weak_ptr"; }())
CASE(out_ptr_, []{ std::unique_ptr<int> p; { auto o = std::out_ptr(p); int** pp = o; *pp = new int(5); } return *p == 5; }())
CASE(out_ptr_del, []{ std::shared_ptr<int> p; { auto o = std::out_ptr(p, Del{}); int** pp = o; *pp = new int(5); } return *p == 5; }())
CASE(inout_ptr_, []{ std::unique_ptr<int> p(new int(1)); { auto o = std::inout_ptr(p); int** pp = o; delete *pp; *pp = new int(6); } return *p == 6; }())

int main(int, char**) {
  assert(up_default());
  assert(up_ptr());
  assert(up_deleter());
  assert(up_move());
  assert(up_convert());
  assert(up_assign());
  assert(up_reset());
  assert(up_release());
  assert(up_swap());
  assert(up_getdel());
  assert(up_arrow());
  assert(up_array());
  assert(up_array_del());
  assert(up_array_reset());
  assert(make_unique_());
  assert(make_unique_arr());
  assert(make_unique_ow());
  assert(make_unique_ow_arr());
  assert(up_eq());
  assert(up_rel());
  assert(up_rel_null());
  assert(up_3way());
  assert(default_delete_());
  assert(sp_default());
  assert(sp_ptr());
  assert(sp_del());
  assert(sp_del_alloc());
  assert(sp_default_del_alloc());
  assert(sp_nullptr_del());
  assert(sp_nullptr_del_alloc());
  assert(sp_alias());
  assert(sp_copy());
  assert(sp_move());
  assert(sp_convert());
  assert(sp_from_weak());
  assert(sp_from_unique());
  assert(sp_assign());
  assert(sp_swap());
  assert(sp_reset());
  assert(sp_observers());
  assert(sp_array_index());
  assert(sp_owner_equal());
  assert(make_shared_());
  assert(make_shared_arr());
  assert(make_shared_arr_init());
  assert(make_shared_bound());
  assert(make_shared_bound_init());
  assert(make_shared_md());
  assert(make_shared_md_init());
  assert(make_shared_md_rt());
  assert(make_shared_ow());
  assert(make_shared_ow_arr());
  assert(make_shared_ow_bound());
  assert(alloc_shared());
  assert(alloc_shared_arr());
  assert(alloc_shared_md());
  assert(alloc_shared_ow());
  assert(alloc_shared_ow_arr());
  assert(sp_cmp());
  assert(sp_cast_static());
  assert(sp_cast_dyn());
  assert(sp_cast_const());
  assert(sp_getdeleter());
  assert(wp_default());
  assert(wp_from_sp());
  assert(wp_copy_move());
  assert(wp_assign());
  assert(wp_reset_swap());
  assert(wp_expire());
  assert(wp_convert());
  assert(esft());
  assert(bad_weak());
  assert(out_ptr_());
  assert(out_ptr_del());
  assert(inout_ptr_());
  return 0;
}
