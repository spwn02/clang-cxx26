//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// The local_time stream insertion participates only when insertion of the
// corresponding sys_time is well-formed.

#include <chrono>
#include <ostream>

template <class T>
concept Streamable = requires(std::ostream& os, const T& value) { os << value; };

static_assert(Streamable<std::chrono::local_time<std::chrono::seconds>>);
static_assert(!Streamable<std::chrono::local_time<std::chrono::duration<double>>>);

int main(int, char**) { return 0; }
