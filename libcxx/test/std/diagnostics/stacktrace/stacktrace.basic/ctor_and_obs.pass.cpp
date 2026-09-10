//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <stacktrace>

// class template basic_stacktrace: default/copy/move/allocator-extended
// constructors, and the container-like observers (begin/end/rbegin/rend,
// cbegin/cend/crbegin/crend, empty/size/max_size, operator[]/at,
// get_allocator).

#include <cassert>
#include <memory>
#include <stacktrace>
#include <stdexcept>

int main(int, char**) {
  // Default construction is an empty stacktrace, not a captured one --
  // current() is the only way to get a populated basic_stacktrace.
  std::stacktrace empty;
  assert(empty.empty());
  assert(empty.size() == 0);
  assert(empty.begin() == empty.end());
  assert(empty.rbegin() == empty.rend());
  assert(empty.cbegin() == empty.cend());
  assert(empty.crbegin() == empty.crend());
  assert(empty.max_size() > 0);

  std::allocator<std::stacktrace_entry> alloc;
  std::stacktrace with_alloc(alloc);
  assert(with_alloc.empty());
  assert(with_alloc.get_allocator() == alloc);

  std::stacktrace st = std::stacktrace::current();
  assert(!st.empty());
  assert(st.size() > 0);
  assert(static_cast<std::stacktrace::size_type>(st.end() - st.begin()) == st.size());

  // Copy construction/assignment.
  std::stacktrace copy = st;
  assert(copy.size() == st.size());
  assert(copy[0] == st[0]);

  // Allocator-extended copy/move.
  std::stacktrace copy_with_alloc(st, alloc);
  assert(copy_with_alloc.size() == st.size());
  std::stacktrace moved_source = st;
  std::stacktrace move_with_alloc(std::move(moved_source), alloc);
  assert(move_with_alloc.size() == st.size());

  // operator[] and at() agree; at() range-checks.
  for (std::stacktrace::size_type i = 0; i < st.size(); ++i)
    assert(st[i] == st.at(i));
  bool threw = false;
  try {
    (void)st.at(st.size());
  } catch (const std::out_of_range&) {
    threw = true;
  }
  assert(threw);

  // Move assignment/construction.
  std::stacktrace move_target;
  move_target = std::move(copy);
  assert(move_target.size() == st.size());

  std::stacktrace move_ctor(std::move(move_target));
  assert(move_ctor.size() == st.size());

  // Forward and reverse iteration visit the same elements in opposite order.
  std::stacktrace::size_type i = 0;
  for (auto it = st.begin(); it != st.end(); ++it, ++i)
    assert(*it == st[i]);
  i = st.size();
  for (auto it = st.rbegin(); it != st.rend(); ++it)
    assert(*it == st[--i]);

  return 0;
}
