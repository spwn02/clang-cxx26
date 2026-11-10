//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_buffer>

// The DynamicBuffer concept: the four implementations model it, and types that miss a member do not.

#include <io_buffer>

#include <cassert>
#include <deque>
#include <string>
#include <vector>

namespace io = std::io;

static_assert(io::DynamicBuffer<io::flat_dynamic_buffer>);
static_assert(io::DynamicBuffer<io::circular_dynamic_buffer>);
static_assert(io::DynamicBuffer<io::vector_dynamic_buffer<unsigned char>>);
static_assert(io::DynamicBuffer<io::vector_dynamic_buffer<char>>);
static_assert(io::DynamicBuffer<io::string_dynamic_buffer<char>>);
static_assert(io::DynamicBuffer<io::string_dynamic_buffer<char, std::char_traits<char>, std::allocator<char>>>);

// The member types.
static_assert(std::is_same_v<io::flat_dynamic_buffer::const_buffers_type, io::const_buffer>);
static_assert(std::is_same_v<io::flat_dynamic_buffer::mutable_buffers_type, io::mutable_buffer>);
static_assert(std::is_same_v<io::circular_dynamic_buffer::const_buffers_type, io::const_buffer_pair>);
static_assert(std::is_same_v<io::circular_dynamic_buffer::mutable_buffers_type, io::mutable_buffer_pair>);
static_assert(std::is_same_v<io::vector_dynamic_buffer<char>::const_buffers_type, io::const_buffer>);
static_assert(std::is_same_v<io::string_dynamic_buffer<char>::mutable_buffers_type, io::mutable_buffer>);

// noexcept-ness: queries, data, consume of caller-owned storage and commit never throw; prepare may (length_error).
static_assert(noexcept(std::declval<const io::flat_dynamic_buffer&>().size()));
static_assert(noexcept(std::declval<const io::flat_dynamic_buffer&>().max_size()));
static_assert(noexcept(std::declval<const io::flat_dynamic_buffer&>().capacity()));
static_assert(noexcept(std::declval<const io::flat_dynamic_buffer&>().data()));
static_assert(noexcept(std::declval<io::flat_dynamic_buffer&>().commit(1)));
static_assert(noexcept(std::declval<io::flat_dynamic_buffer&>().consume(1)));
static_assert(!noexcept(std::declval<io::flat_dynamic_buffer&>().prepare(1)));
static_assert(noexcept(std::declval<io::circular_dynamic_buffer&>().commit(1)));
static_assert(noexcept(std::declval<io::circular_dynamic_buffer&>().consume(1)));
static_assert(!noexcept(std::declval<io::circular_dynamic_buffer&>().prepare(1)));
static_assert(noexcept(std::declval<const io::string_dynamic_buffer<char>&>().data()));
static_assert(noexcept(std::declval<io::string_dynamic_buffer<char>&>().commit(1)));
static_assert(noexcept(std::declval<const io::vector_dynamic_buffer<char>&>().size()));
static_assert(!noexcept(std::declval<io::vector_dynamic_buffer<char>&>().prepare(1)));

// The adapters are move-constructible and refer to the container; the constructors do not throw.
static_assert(std::is_nothrow_move_constructible_v<io::vector_dynamic_buffer<char>>);
static_assert(std::is_nothrow_move_constructible_v<io::string_dynamic_buffer<char>>);
static_assert(std::is_nothrow_constructible_v<io::vector_dynamic_buffer<char>, std::vector<char>&>);
static_assert(std::is_nothrow_constructible_v<io::string_dynamic_buffer<char>, std::string&, std::size_t>);
static_assert(!std::is_constructible_v<io::string_dynamic_buffer<char>, std::string>); // no temporaries
static_assert(!std::is_convertible_v<std::string&, io::string_dynamic_buffer<char>>);   // explicit

// Negative cases.
struct missing_commit {
  using const_buffers_type   = io::const_buffer;
  using mutable_buffers_type = io::mutable_buffer;
  std::size_t size() const;
  std::size_t max_size() const;
  std::size_t capacity() const;
  const_buffers_type data() const;
  mutable_buffers_type prepare(std::size_t);
  void consume(std::size_t);
};
struct complete : missing_commit {
  void commit(std::size_t);
};
struct wrong_types : complete {
  using const_buffers_type = int;
};
struct wrong_mutable : complete {
  using mutable_buffers_type = io::const_buffer; // not a MutableBufferSequence
};
struct wrong_data : complete {
  io::mutable_buffer data() const; // not const_buffers_type
};
struct no_types {
  std::size_t size() const;
  std::size_t max_size() const;
  std::size_t capacity() const;
  void commit(std::size_t);
  void consume(std::size_t);
};
struct not_movable : complete {
  not_movable(not_movable&&) = delete;
  not_movable()              = default;
};
static_assert(io::DynamicBuffer<complete>);
static_assert(!io::DynamicBuffer<missing_commit>);
static_assert(!io::DynamicBuffer<wrong_types>);
static_assert(!io::DynamicBuffer<wrong_mutable>);
static_assert(!io::DynamicBuffer<wrong_data>);
static_assert(!io::DynamicBuffer<no_types>);
static_assert(!io::DynamicBuffer<not_movable>);
static_assert(!io::DynamicBuffer<int>);
static_assert(!io::DynamicBuffer<std::string>);
static_assert(!io::DynamicBuffer<std::vector<char>>);
static_assert(!io::DynamicBuffer<io::mutable_buffer>);

// A generic algorithm written against the concept works with all four implementations.
template <io::DynamicBuffer DB>
std::size_t append(DB& db, const char* s, std::size_t n) {
  auto out = db.prepare(n);
  std::size_t copied = io::buffer_copy(out, io::const_buffer(s, n));
  db.commit(copied);
  return copied;
}

int main(int, char**) {
  char storage[16];
  io::flat_dynamic_buffer f(storage, sizeof(storage));
  assert(append(f, "abc", 3) == 3 && f.size() == 3);

  char ring[16];
  io::circular_dynamic_buffer c(ring, sizeof(ring));
  assert(append(c, "abc", 3) == 3 && c.size() == 3);

  std::vector<unsigned char> v;
  io::vector_dynamic_buffer vb(v); // class template argument deduction
  static_assert(std::is_same_v<decltype(vb), io::vector_dynamic_buffer<unsigned char>>);
  assert(append(vb, "abc", 3) == 3 && vb.size() == 3 && v.size() == 3);

  std::string s;
  io::string_dynamic_buffer sb(s, 100);
  static_assert(std::is_same_v<decltype(sb), io::string_dynamic_buffer<char>>);
  assert(append(sb, "abc", 3) == 3 && sb.size() == 3 && s == "abc");
  return 0;
}
