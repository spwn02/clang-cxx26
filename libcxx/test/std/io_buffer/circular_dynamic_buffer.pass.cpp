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

// circular_dynamic_buffer: a ring over caller-owned storage. The readable and the writable bytes may wrap around the
// end of the storage, which shows as a second, non-empty buffer of the pair.

#include <io_buffer>

#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

#include "test_macros.h"

namespace io = std::io;

// The readable bytes, gathered into one string.
static std::string contents(const io::circular_dynamic_buffer& db) {
  std::string s(db.size(), '?');
  assert(io::buffer_copy(io::mutable_buffer(s.data(), s.size()), db.data()) == db.size());
  return s;
}

static void put(io::circular_dynamic_buffer& db, const std::string& s) {
  auto out = db.prepare(s.size());
  assert(io::buffer_size(out) == s.size());
  assert(io::buffer_copy(out, io::const_buffer(s.data(), s.size())) == s.size());
  db.commit(s.size());
}

int main(int, char**) {
  char ring[8];
  std::memset(ring, '.', sizeof(ring));
  io::circular_dynamic_buffer db(ring, sizeof(ring));

  assert(db.size() == 0 && db.max_size() == 8 && db.capacity() == 8);
  assert(io::buffer_size(db.data()) == 0 && io::buffer_empty(db.data()));

  // No wrap: everything is in the first buffer of the pair.
  put(db, "abcde");
  {
    auto d = db.data();
    assert(d[0].data() == ring && d[0].size() == 5 && d[1].size() == 0);
    assert(contents(db) == "abcde");
  }

  // consume advances the read position without moving bytes.
  db.consume(3);
  {
    auto d = db.data();
    assert(d[0].data() == ring + 3 && d[0].size() == 2 && d[1].size() == 0);
    assert(contents(db) == "de");
  }

  // The writable bytes wrap: 3 bytes up to the end of the storage, then the rest at the front.
  {
    auto w = db.prepare(5);
    assert(io::buffer_size(w) == 5);
    assert(w[0].data() == ring + 5 && w[0].size() == 3);
    assert(w[1].data() == ring && w[1].size() == 2);
    assert(io::buffer_copy(w, io::const_buffer("12345", 5)) == 5);
    db.commit(5);
  }
  // Now the readable bytes wrap too: "de" + "12345" = 7 bytes starting at index 3.
  {
    assert(db.size() == 7);
    auto d = db.data();
    assert(d[0].data() == ring + 3 && d[0].size() == 5);
    assert(d[1].data() == ring && d[1].size() == 2);
    assert(contents(db) == "de12345");
    assert(std::memcmp(ring, "45", 2) == 0);
  }

  // consume across the wrap point.
  db.consume(5);
  {
    auto d = db.data();
    assert(db.size() == 2 && d[0].data() == ring && d[0].size() == 2 && d[1].size() == 0);
    assert(contents(db) == "45");
  }

  // The prepared space after a wrapped consume: starts at index 2.
  {
    auto w = db.prepare(6);
    assert(w[0].data() == ring + 2 && w[0].size() == 6 && w[1].size() == 0);
    db.commit(0);
    assert(db.size() == 2);
  }

  // Fill to capacity, then drain, repeatedly, so that the position travels around the ring many times.
  {
    std::string expect = contents(db); // "45"
    int next           = 0;
    for (int round = 0; round < 50; ++round) {
      std::string chunk;
      for (int i = 0; i < 1 + round % 6; ++i)
        chunk += static_cast<char>('A' + (next++ % 26));
      if (chunk.size() > db.max_size() - db.size()) {
        std::size_t drop = chunk.size() - (db.max_size() - db.size());
        db.consume(drop);
        expect.erase(0, drop);
      }
      put(db, chunk);
      expect += chunk;
      assert(db.size() == expect.size());
      assert(contents(db) == expect);
      assert(db.size() <= db.capacity());
    }
  }

  // Partial commit: the uncommitted tail of the prepared bytes is discarded.
  {
    db.consume(1000);
    assert(db.size() == 0);
    auto w = db.prepare(6);
    assert(io::buffer_copy(w, io::const_buffer("uvwxyz", 6)) == 6);
    db.commit(2);
    assert(db.size() == 2 && contents(db) == "uv");
    db.commit(3); // nothing prepared any more
    assert(db.size() == 2);
  }

  // prepare(0) gives an empty pair.
  {
    auto w = db.prepare(0);
    assert(io::buffer_size(w) == 0 && io::buffer_empty(w));
  }

  // Full buffer: no space; max_size() limit.
  {
    db.consume(1000);
    put(db, "01234567");
    assert(db.size() == 8);
    assert(io::buffer_size(db.prepare(0)) == 0);
#ifndef TEST_HAS_NO_EXCEPTIONS
    try {
      (void)db.prepare(1);
      assert(false);
    } catch (const std::length_error&) {
    }
    assert(db.size() == 8 && contents(db) == "01234567");
    db.consume(2);
    try {
      (void)db.prepare(3);
      assert(false);
    } catch (const std::length_error&) {
    }
    try {
      (void)db.prepare(std::size_t(-1));
      assert(false);
    } catch (const std::length_error&) {
    }
    (void)db.prepare(2); // exactly what is free
#endif
  }

  // Initial contents; a buffer with an empty-middle sequence is still a valid source for a gather.
  {
    char r2[6] = {'a', 'b', 'c', 'd', '.', '.'};
    io::circular_dynamic_buffer d2(r2, 6, 4);
    assert(d2.size() == 4 && contents(d2) == "abcd");
    auto w = d2.prepare(2);
    assert(w[0].data() == r2 + 4 && w[0].size() == 2 && w[1].size() == 0);
  }

  // Zero capacity.
  {
    io::circular_dynamic_buffer z;
    assert(z.size() == 0 && z.max_size() == 0 && z.capacity() == 0);
    assert(io::buffer_size(z.data()) == 0);
    assert(io::buffer_size(z.prepare(0)) == 0);
    z.commit(4);
    z.consume(4);
    assert(z.size() == 0);
#ifndef TEST_HAS_NO_EXCEPTIONS
    try {
      (void)z.prepare(1);
      assert(false);
    } catch (const std::length_error&) {
    }
#endif
  }

  // Moves carry the state.
  {
    char r3[4];
    io::circular_dynamic_buffer a(r3, 4);
    put(a, "xyz");
    io::circular_dynamic_buffer b(std::move(a));
    assert(b.size() == 3 && contents(b) == "xyz");
  }
  return 0;
}
