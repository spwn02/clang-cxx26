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

// vector_dynamic_buffer and string_dynamic_buffer (N4771 dynamic_vector_buffer / dynamic_string_buffer): adapters
// that grow and shrink a std::vector / std::string of one-byte elements.

#include <io_buffer>

#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "test_macros.h"

namespace io = std::io;

static std::string_view view(const io::const_buffer& b) {
  return std::string_view(static_cast<const char*>(b.data()), b.size());
}

template <class DB>
void put(DB& db, std::string_view s) {
  io::mutable_buffer m = db.prepare(s.size());
  assert(m.size() == s.size());
  if (!s.empty())
    std::memcpy(m.data(), s.data(), s.size());
  db.commit(s.size());
}

template <class Container, class DB>
void common_cycle(Container& c, DB& db) {
  assert(db.size() == 0);
  assert(db.data().size() == 0);

  put(db, "hello");
  assert(db.size() == 5 && view(db.data()) == "hello");
  assert(c.size() == 5); // the container follows the readable bytes
  assert(db.capacity() == c.capacity() && db.capacity() >= 5);

  put(db, ", world");
  assert(db.size() == 12 && view(db.data()) == "hello, world");
  assert(c.size() == 12);

  // prepare grows the container behind the readable bytes; commit shrinks it back to what was committed.
  {
    io::mutable_buffer m = db.prepare(100);
    assert(m.size() == 100);
    assert(db.size() == 12);
    assert(c.size() == 112);
    std::memcpy(m.data(), "!!", 2);
    db.commit(2);
    assert(db.size() == 14 && c.size() == 14 && view(db.data()) == "hello, world!!");
  }

  // consume erases from the front.
  db.consume(7);
  assert(db.size() == 7 && c.size() == 7 && view(db.data()) == "world!!");
  db.consume(0);
  assert(db.size() == 7);
  db.consume(1000);
  assert(db.size() == 0 && c.size() == 0);

  // commit with nothing prepared (after consume) does nothing.
  db.commit(5);
  assert(db.size() == 0 && c.size() == 0);

  // prepare(0).
  assert(db.prepare(0).size() == 0);
  db.commit(0);
  assert(db.size() == 0);
}

int main(int, char**) {
  // ---- vector ----
  {
    std::vector<unsigned char> v;
    io::vector_dynamic_buffer db(v);
    assert(db.max_size() == v.max_size());
    common_cycle(v, db);
  }
  {
    std::vector<char> v;
    io::vector_dynamic_buffer<char> db(v);
    common_cycle(v, db);
  }

  // Existing contents are readable from the start.
  {
    std::vector<unsigned char> v{'a', 'b', 'c'};
    io::vector_dynamic_buffer db(v);
    assert(db.size() == 3 && view(db.data()) == "abc");
    put(db, "de");
    assert(view(db.data()) == "abcde" && v.size() == 5);
  }

  // max_size limits: prepare throws length_error when size() + n > max_size(); the container is left alone.
  {
    std::vector<unsigned char> v{'a', 'b'};
    io::vector_dynamic_buffer db(v, 6);
    assert(db.max_size() == 6 && db.size() == 2);
    (void)db.prepare(4); // exactly the limit
    db.commit(4);
    assert(db.size() == 6 && v.size() == 6);
#ifndef TEST_HAS_NO_EXCEPTIONS
    try {
      (void)db.prepare(1);
      assert(false);
    } catch (const std::length_error&) {
    }
    assert(v.size() == 6 && db.size() == 6);
    db.consume(1);
    try {
      (void)db.prepare(2);
      assert(false);
    } catch (const std::length_error&) {
    }
    assert(v.size() == 5);
    try {
      (void)db.prepare(std::size_t(-1)); // no overflow in the check
      assert(false);
    } catch (const std::length_error&) {
    }
#endif
  }

  // ---- string ----
  {
    std::string s;
    io::string_dynamic_buffer db(s);
    assert(db.max_size() == s.max_size());
    common_cycle(s, db);
  }
  {
    std::string s = "xy";
    io::string_dynamic_buffer db(s, 5);
    assert(db.size() == 2 && db.max_size() == 5);
    put(db, "zzz");
    assert(s == "xyzzz");
#ifndef TEST_HAS_NO_EXCEPTIONS
    try {
      (void)db.prepare(1);
      assert(false);
    } catch (const std::length_error&) {
    }
    assert(s == "xyzzz");
#endif
    db.consume(2);
    assert(s == "zzz");
    put(db, "ab");
    assert(s == "zzzab");
  }

  // Moving the adapter keeps the reference and the bookkeeping.
  {
    std::string s;
    io::string_dynamic_buffer a(s);
    put(a, "abc");
    io::string_dynamic_buffer b(std::move(a));
    assert(b.size() == 3);
    put(b, "d");
    assert(s == "abcd");
  }

  // Dynamic buffers are buffer sequences' producers: data() and prepare() are valid arguments to buffer_copy.
  {
    std::vector<unsigned char> v;
    io::vector_dynamic_buffer db(v);
    const char* src = "scatter";
    assert(io::buffer_copy(db.prepare(4), io::const_buffer(src, 7)) == 4);
    db.commit(4);
    char out[8] = {};
    assert(io::buffer_copy(io::mutable_buffer(out, 8), db.data()) == 4);
    assert(std::memcmp(out, "scat", 4) == 0);
  }
  return 0;
}
