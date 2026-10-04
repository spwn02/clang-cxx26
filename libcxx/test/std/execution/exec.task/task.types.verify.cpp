//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

#include <execution>
// [task.class]: "Otherwise a program that instantiates the definition of task<T, E> is ill-formed."
std::execution::task<const int> const_result; // expected-note {{in instantiation}}
std::execution::task<int[2]> array_result; // expected-note {{in instantiation}}
std::execution::task<int, int> nonclass_env; // expected-note {{in instantiation}}
// expected-error@*:* 2 {{task requires void, a reference, or a cv-unqualified non-array object type.}}
// expected-error@*:* {{task requires a class Environment.}}
struct invalid_errors { using error_types = std::execution::completion_signatures<std::execution::set_value_t()>; };
// [task.class]: "ill-formed if error_types is not a specialization ... or ... not of the form set_error_t(E)"
std::execution::task<int, invalid_errors> invalid; // expected-note {{in instantiation}}
// expected-error@*:* {{task error_types must contain only set_error_t(E) signatures.}}
