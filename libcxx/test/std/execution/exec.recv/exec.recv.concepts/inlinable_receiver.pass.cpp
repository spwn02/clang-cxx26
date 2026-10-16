//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.recv.concepts]: inlinable_receiver<Rcvr, ChildOp> is a receiver with a noexcept static make_receiver_for(ChildOp*)
// returning the receiver type (ChildOp may be incomplete).

#include <concepts>
#include <execution>

namespace ex = std::execution;

struct Rcvr {
  using receiver_concept = ex::receiver_tag;
  void set_value() && noexcept {}
};
static_assert(ex::receiver<Rcvr>);

struct Op; // incomplete
struct InlinableRcvr {
  using receiver_concept = ex::receiver_tag;
  void set_value() && noexcept {}
  static InlinableRcvr make_receiver_for(Op*) noexcept { return {}; }
};
static_assert(ex::inlinable_receiver<InlinableRcvr, Op>);
static_assert(ex::inlinable_receiver<const InlinableRcvr&, Op>);
static_assert(!ex::inlinable_receiver<Rcvr, Op>);

// not noexcept
struct ThrowingMake {
  using receiver_concept = ex::receiver_tag;
  void set_value() && noexcept {}
  static ThrowingMake make_receiver_for(Op*) { return {}; }
};
static_assert(!ex::inlinable_receiver<ThrowingMake, Op>);

// wrong result type
struct WrongResult {
  using receiver_concept = ex::receiver_tag;
  void set_value() && noexcept {}
  static Rcvr make_receiver_for(Op*) noexcept { return {}; }
};
static_assert(!ex::inlinable_receiver<WrongResult, Op>);

// not a receiver
struct NotReceiver {
  static NotReceiver make_receiver_for(Op*) noexcept { return {}; }
};
static_assert(!ex::inlinable_receiver<NotReceiver, Op>);

// a different child operation state type
struct OtherOp {};
static_assert(!ex::inlinable_receiver<InlinableRcvr, OtherOp>);
static_assert(!ex::inlinable_receiver<int, Op>);

int main(int, char**) { return 0; }
