//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: no-exceptions

// <exception>

// [propagation]: current_exception and rethrow_exception are declared in every mode, also with exceptions disabled
// (they are not constexpr-capable then, so the C++26 constexpr declarations are not made).

#include <exception>

#include "test_macros.h"

std::exception_ptr (*current)() noexcept       = &std::current_exception;
void (*rethrow)(std::exception_ptr)            = &std::rethrow_exception;
std::exception_ptr (*make)(int) noexcept       = &std::make_exception_ptr<int>;
