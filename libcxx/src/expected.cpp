//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <expected>

_LIBCPP_BEGIN_NAMESPACE_STD
#if _LIBCPP_AVAILABILITY_HAS_BAD_EXPECTED_ACCESS_KEY_FUNCTION && !_LIBCPP_HAS_INLINE_CXX26_DEFINITIONS
const char* bad_expected_access<void>::what() const noexcept { return "bad access to std::expected"; }
#endif
_LIBCPP_END_NAMESPACE_STD
