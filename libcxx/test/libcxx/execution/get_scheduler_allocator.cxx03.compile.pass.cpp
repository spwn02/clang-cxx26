//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// RUN: %{cxx} %{flags} -nostdinc++ -I %{libcxx-dir}/include -I %{include-dir} -std=c++03 -fsyntax-only %s

// C++26-only internals must remain includable in older language modes.
#include <execution>
#include <__execution/get_scheduler_allocator.h>
