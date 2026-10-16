//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// P4159R0: receiver_of and sender_to are exposition-only (receiver-of, sender-to), they are not part of std::execution.

#include <execution>

namespace ex = std::execution;

template <class R>
concept uses_receiver_of = ex::receiver_of<R, ex::completion_signatures<>>; // expected-error {{no template named 'receiver_of' in namespace 'std::execution'}}

template <class S, class R>
concept uses_sender_to = ex::sender_to<S, R>; // expected-error {{no member named 'sender_to' in namespace 'std::execution'}}
// expected-error@-1 {{'S' does not refer to a value}}
