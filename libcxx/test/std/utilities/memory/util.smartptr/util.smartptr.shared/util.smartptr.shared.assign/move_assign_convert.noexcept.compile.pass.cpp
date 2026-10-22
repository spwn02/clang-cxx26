//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <memory>

// template<class Y> shared_ptr& operator=(shared_ptr<Y>&& r) noexcept;

#include <memory>
#include <utility>

struct Base {};
struct Derived : Base {};

static_assert(noexcept(std::declval<std::shared_ptr<Base>&>() = std::declval<std::shared_ptr<Derived>&&>()));
