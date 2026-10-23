//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <vector>
// [container.alloc.reqmts] requires allocator_type::value_type to match
// value_type, including for vector<bool>.

// UNSUPPORTED: c++03, c++11, c++14, c++17

#include <memory>
#include <vector>

std::vector<bool, std::allocator<int>> mismatched_allocator; // expected-error@*:* {{Allocator::value_type must be same type as value_type}} expected-note {{in instantiation of template class}}
