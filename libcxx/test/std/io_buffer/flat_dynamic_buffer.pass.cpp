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

// flat_dynamic_buffer: prepare/commit/consume cycles over caller-owned contiguous storage of fixed capacity.

#include <io_buffer>

#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "test_macros.h"

namespace io = std::io;

static std::string_view view(const io::const_buffer& b) {
  return std::string_view(static_cast<const char*>(b.data()), b.size());
}

// Writes s into the writable space and commits it.
static void put(io::flat_dynamic_buffer& db, std::string_view s) {
  io::mutable_buffer m = db.prepare(s.size());
  assert(m.size() == s.size());
  std::memcpy(m.data(), s.data(), s.size());
  db.commit(s.size());
}

int main(int, char**) {
  char storage[10];
  std::memset(storage, '.', sizeof(storage));

  // Fresh buffer.
  io::flat_dynamic_buffer db(storage, sizeof(storage));
  assert(db.size() == 0);
  assert(db.max_size() == 10 && db.capacity() == 10);
  assert(db.data().size() == 0);

  // prepare -> commit makes bytes readable, appended after the previous ones.
  put(db, "abcd");
  assert(db.size() == 4 && view(db.data()) == "abcd");
  assert(db.data().data() == storage);
  put(db, "ef");
  assert(db.size() == 6 && view(db.data()) == "abcdef");

  // The writable bytes start right behind the readable ones.
  {
    io::mutable_buffer m = db.prepare(3);
    assert(m.data() == storage + 6 && m.size() == 3);
    assert(db.size() == 6); // prepare alone does not change the readable bytes
    std::memcpy(m.data(), "XYZ", 3);
    db.commit(1); // only the first byte of the writable bytes becomes readable; the rest is discarded
    assert(db.size() == 7 && view(db.data()) == "abcdefX");
    db.commit(5); // nothing is prepared any more: no effect
    assert(db.size() == 7);
  }

  // commit(n) for n larger than the prepared bytes commits all of them.
  {
    io::mutable_buffer m = db.prepare(2);
    std::memcpy(m.data(), "gh", 2);
    db.commit(100);
    assert(db.size() == 9 && view(db.data()) == "abcdefXgh");
  }

  // consume removes from the front and does not move the bytes.
  db.consume(3);
  assert(db.size() == 6 && view(db.data()) == "defXgh");
  assert(db.data().data() == storage + 3);
  db.consume(0);
  assert(db.size() == 6);

  // prepare that fits only after moving the readable bytes to the front of the storage.
  {
    // 6 readable bytes at storage+3; 1 byte is free behind them; 4 are free in total.
    io::mutable_buffer m = db.prepare(4);
    assert(m.size() == 4);
    assert(db.size() == 6 && view(db.data()) == "defXgh"); // the readable bytes are unchanged
    assert(db.data().data() == storage);                    // and now at the front
    assert(m.data() == storage + 6);
    std::memcpy(m.data(), "ijkl", 4);
    db.commit(4);
    assert(db.size() == 10 && view(db.data()) == "defXghijkl");
  }

  // prepare(0) is valid, even when full.
  {
    io::mutable_buffer m = db.prepare(0);
    assert(m.size() == 0);
    db.commit(0);
    assert(db.size() == 10);
  }

  // consume of more than size() empties the buffer; the next prepare starts at the front.
  db.consume(1000);
  assert(db.size() == 0);
  assert(db.data().size() == 0);
  assert(db.prepare(10).data() == storage);
  db.commit(0);
  assert(db.size() == 0);

  // max_size() limit: prepare beyond it throws length_error and leaves the state alone.
#ifndef TEST_HAS_NO_EXCEPTIONS
  {
    put(db, "12345");
    try {
      (void)db.prepare(6);
      assert(false);
    } catch (const std::length_error&) {
    }
    assert(db.size() == 5 && view(db.data()) == "12345");
    (void)db.prepare(5); // exactly at the limit is fine
    db.commit(5);
    assert(db.size() == 10);
    try {
      (void)db.prepare(1);
      assert(false);
    } catch (const std::length_error&) {
    }
    try {
      (void)db.prepare(std::size_t(-1)); // no overflow in the check
      assert(false);
    } catch (const std::length_error&) {
    }
  }
#endif

  // An initial readable region.
  {
    char s2[6] = {'a', 'b', 'c', 'd', '.', '.'};
    io::flat_dynamic_buffer d2(s2, 6, 4);
    assert(d2.size() == 4 && view(d2.data()) == "abcd");
    assert(d2.prepare(2).data() == s2 + 4);
  }

  // Zero capacity and default construction.
  {
    io::flat_dynamic_buffer z;
    assert(z.size() == 0 && z.max_size() == 0 && z.capacity() == 0);
    assert(z.prepare(0).size() == 0);
    z.commit(1);
    z.consume(1);
    assert(z.size() == 0);
  }

  // Moves carry the state.
  {
    char s3[8];
    io::flat_dynamic_buffer a(s3, 8);
    put(a, "xyz");
    io::flat_dynamic_buffer b(std::move(a));
    assert(b.size() == 3 && view(b.data()) == "xyz");
  }
  return 0;
}
