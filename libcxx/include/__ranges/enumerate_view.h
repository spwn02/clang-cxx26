// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Experimental C++23 enumerate view for the Bloomberg libc++ fork.
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___RANGES_ENUMERATE_VIEW_H
#define _LIBCPP___RANGES_ENUMERATE_VIEW_H

#include <__config>
#include <__ranges/iota_view.h>
#include <__ranges/zip_view.h>
#include <cstddef>
#include <utility>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

 #if !defined(_LIBCPP_BUILDING_STD_MODULE)

namespace ranges::views {

struct __enumerate_fn : range_adaptor_closure<__enumerate_fn> {
  template <viewable_range _Range>
  constexpr auto operator()(_Range&& __range) const
      noexcept(noexcept(zip(iota(size_t{0}), std::forward<_Range>(__range)))) {
    return zip(iota(size_t{0}), std::forward<_Range>(__range));
  }
};

inline constexpr auto enumerate = __enumerate_fn{};

} // namespace ranges::views

#endif

#endif

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___RANGES_ENUMERATE_VIEW_H
