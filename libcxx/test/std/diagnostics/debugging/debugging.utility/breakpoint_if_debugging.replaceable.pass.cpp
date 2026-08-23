//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <debugging>

// void breakpoint_if_debugging() noexcept;
//
// [debugging.utility]p2: Effects: Equivalent to:
//   if (is_debugger_present()) breakpoint();
//
// breakpoint_if_debugging() itself is _LIBCPP_HIDE_FROM_ABI inline (it isn't
// one of P2810R4's replaceable functions), so its body is baked into this
// translation unit and calls whatever is_debugger_present() resolves to at
// that call site -- unlike a call routed entirely through a replaceable
// function, there's no guarantee that resolves to a program-supplied
// definition rather than one already inlined/cached from elsewhere. This
// test proves it does: is_debugger_present() is replaced with a
// call-counting stub that always returns false (so breakpoint() -- which
// traps -- is never reached), and the counter shows breakpoint_if_debugging()
// actually invoked the replacement rather than some other, unreplaceable
// copy of is_debugger_present().

#include <cassert>
#include <debugging>

int is_debugger_present_calls = 0;

bool std::is_debugger_present() noexcept {
  ++is_debugger_present_calls;
  return false;
}

int main(int, char**) {
  std::breakpoint_if_debugging();
  assert(is_debugger_present_calls == 1);
  return 0;
}
