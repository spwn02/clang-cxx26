//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

// P3248R5: std::intptr_t, std::uintptr_t and the macros INTPTR_MIN, INTPTR_MAX
// and UINTPTR_MAX are required, and the library facilities that used to depend on
// their presence (std::atomic_intptr_t, the pointer formatter) are always available.

#include <atomic>
#include <cstdint>
#include <format>
#include <limits>
#include <type_traits>

static_assert(std::is_signed_v<std::intptr_t>);
static_assert(std::is_unsigned_v<std::uintptr_t>);
static_assert(sizeof(std::intptr_t) >= sizeof(void*));
static_assert(sizeof(std::uintptr_t) >= sizeof(void*));
static_assert(std::is_same_v<std::make_unsigned_t<std::intptr_t>, std::uintptr_t>);

static_assert(INTPTR_MIN < 0);
static_assert(INTPTR_MAX > 0);
static_assert(UINTPTR_MAX > 0);
static_assert(static_cast<std::intptr_t>(INTPTR_MAX) == std::numeric_limits<std::intptr_t>::max());

static_assert(std::is_same_v<std::atomic_intptr_t, std::atomic<std::intptr_t>>);
static_assert(std::is_same_v<std::atomic_uintptr_t, std::atomic<std::uintptr_t>>);

// The pointer formatter is specified in terms of uintptr_t.
static_assert(std::formattable<void*, char>);
static_assert(std::formattable<const void*, char>);
static_assert(std::formattable<std::nullptr_t, char>);
