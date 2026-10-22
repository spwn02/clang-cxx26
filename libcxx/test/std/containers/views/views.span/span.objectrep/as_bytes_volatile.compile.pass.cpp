//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <span>

// LWG4243: as_bytes and as_writable_bytes are constrained on is_volatile_v<ElementType> being false.

#include <span>

template <class S>
concept can_as_bytes = requires(S s) { std::as_bytes(s); };
template <class S>
concept can_as_writable_bytes = requires(S s) { std::as_writable_bytes(s); };

static_assert(can_as_bytes<std::span<int>>);
static_assert(can_as_bytes<std::span<const int>>);
static_assert(!can_as_bytes<std::span<volatile int>>);
static_assert(!can_as_bytes<std::span<const volatile int, 4>>);

static_assert(can_as_writable_bytes<std::span<int>>);
static_assert(!can_as_writable_bytes<std::span<const int>>);
static_assert(!can_as_writable_bytes<std::span<volatile int>>);
