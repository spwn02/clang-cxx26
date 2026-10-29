//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// With the MSVC ABI the integral overloads of to_string and to_wstring and the members of bad_weak_ptr,
// bad_optional_access, bad_variant_access and bad_expected_access<void> are inline in every mode, and libc++.lib does
// not define them (the linker of COFF rejects both). This test selects that form on any platform and checks that the
// inline functions give the results of the library ones.

// The frozen C++03 headers do not have the form.
// UNSUPPORTED: c++03

// ADDITIONAL_COMPILE_FLAGS: -D_LIBCPP_FORCE_INLINE_CXX26_DEFINITIONS

#include <cassert>
#include <climits>
#include <cstring>
#include <memory>
#include <string>

#include "test_macros.h"
#if TEST_STD_VER >= 17
#  include <optional>
#  include <variant>
#endif

int main(int, char**) {
  assert(std::to_string(0) == "0");
  assert(std::to_string(-1) == "-1");
  assert(std::to_string(INT_MIN) == "-2147483648");
  assert(std::to_string(INT_MAX) == "2147483647");
  assert(std::to_string(UINT_MAX) == "4294967295");
  assert(std::to_string(LONG_MIN) == std::to_string(static_cast<long long>(LONG_MIN)));
  assert(std::to_string(ULONG_MAX) == std::to_string(static_cast<unsigned long long>(ULONG_MAX)));
  assert(std::to_string(LLONG_MIN) == "-9223372036854775808");
  assert(std::to_string(LLONG_MAX) == "9223372036854775807");
  assert(std::to_string(ULLONG_MAX) == "18446744073709551615");
#ifndef TEST_HAS_NO_WIDE_CHARACTERS
  assert(std::to_wstring(0) == L"0");
  assert(std::to_wstring(INT_MIN) == L"-2147483648");
  assert(std::to_wstring(ULLONG_MAX) == L"18446744073709551615");
  assert(std::to_wstring(-70000L) == L"-70000");
#endif

  assert(std::strcmp(std::bad_weak_ptr().what(), "bad_weak_ptr") == 0);
#if TEST_STD_VER >= 17
  assert(std::strcmp(std::bad_optional_access().what(), "bad_optional_access") == 0);
  assert(std::strcmp(std::bad_variant_access().what(), "bad_variant_access") == 0);
#endif
  return 0;
}
