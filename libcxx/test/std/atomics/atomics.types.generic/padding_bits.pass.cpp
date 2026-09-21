//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17
// UNSUPPORTED: gcc
// ADDITIONAL_COMPILE_FLAGS: -Wno-deprecated-declarations

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <type_traits>

struct Padded {
  char c;
  int i;
};

static_assert(std::is_trivially_copyable_v<Padded>);
static_assert(!std::has_unique_object_representations_v<Padded>);
static_assert(sizeof(Padded) > sizeof(char) + sizeof(int));

void set_padding(Padded& value, unsigned char byte) {
  unsigned char bytes[sizeof(Padded)];
  std::memcpy(bytes, &value, sizeof(value));
  for (std::size_t i = sizeof(char); i < offsetof(Padded, i); ++i)
    bytes[i] = byte;
  std::memcpy(&value, bytes, sizeof(value));
}

Padded make_padded(char c, int i, unsigned char padding) {
  Padded value{c, i};
  set_padding(value, padding);
  return value;
}

void expect_cas_succeeds(std::atomic<Padded>& value, unsigned char padding) {
  Padded expected = make_padded('x', 42, padding);
  assert(value.compare_exchange_strong(expected, Padded{'y', 43}, std::memory_order_relaxed));
}

int main(int, char**) {
  // Construction normalizes padding before establishing the stored value.
  std::atomic<Padded> value(make_padded('x', 42, 0x11));
  expect_cas_succeeds(value, 0x22);

  // Store and exchange normalize their incoming values too.
  value.store(make_padded('x', 42, 0x33), std::memory_order_relaxed);
  expect_cas_succeeds(value, 0x44);

  value.exchange(make_padded('x', 42, 0x55), std::memory_order_relaxed);
  expect_cas_succeeds(value, 0x66);

  // The deprecated initialization API follows the same representation rule.
  std::atomic_init(&value, make_padded('x', 42, 0x77));
  expect_cas_succeeds(value, 0x88);

  // Weak CAS and volatile overloads use the same padding-aware retry path.
  value.store(make_padded('x', 42, 0x99), std::memory_order_relaxed);
  Padded expected = make_padded('x', 42, 0xaa);
  assert(value.compare_exchange_weak(expected, Padded{'y', 43}, std::memory_order_relaxed));

  volatile std::atomic<Padded> volatile_value(make_padded('x', 42, 0xbb));
  expected = make_padded('x', 42, 0xcc);
  assert(volatile_value.compare_exchange_strong(expected, Padded{'y', 43}, std::memory_order_relaxed));

  return 0;
}
