//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// LWG4420: the Constraints of the stores, gathers and scatters on the element types.

#include <simd>
#include <array>
#include <complex>
#include <span>
namespace s = std::simd;
using V = s::vec<float, 4>;
using I = s::vec<int, 4>;
struct NoConv {};
template<class Vv, class R> concept can_pstore = requires(Vv v, R&& r){ s::partial_store(v, std::forward<R>(r)); };
template<class Vv, class R> concept can_ustore = requires(Vv v, R&& r){ s::unchecked_store(v, std::forward<R>(r)); };
template<class Vv, class R> concept can_pstore_cv = requires(Vv v, R&& r){ s::partial_store(v, std::forward<R>(r), s::flag_convert); };
template<class Vv, class R, class Ii> concept can_gather = requires(R&& r, Ii i){ s::partial_gather_from<Vv>(std::forward<R>(r), i); };
template<class Vv, class R, class Ii> concept can_scatter = requires(Vv v, R&& r, Ii i){ s::partial_scatter_to(v, std::forward<R>(r), i); };
static_assert(can_pstore<V, std::array<float,4>&>);
static_assert(can_ustore<V, std::array<float,4>&>);
static_assert(can_pstore<V, std::array<double,4>&>);
static_assert(!can_pstore<V, std::array<NoConv,4>&>);
static_assert(!can_ustore<V, std::span<const float>>);
static_assert(!can_pstore<s::vec<std::complex<float>,4>, std::array<float,4>&>);
static_assert(can_gather<V, std::array<float,4>&, I>);
static_assert(!can_gather<V, std::array<NoConv,4>&, I>);
static_assert(can_scatter<V, std::array<float,4>&, I>);
static_assert(!can_scatter<V, std::span<const float>, I>);
static_assert(!can_scatter<s::vec<float,8>, std::array<float,4>&, I>);
