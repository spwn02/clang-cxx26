//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: no-threads, no-filesystem
// UNSUPPORTED: libcpp-has-no-unicode
// UNSUPPORTED: windows

// XFAIL: availability-fp_to_chars-missing

// <print>

// void vprint_unicode(FILE* stream, string_view fmt, format_args args);
// void vprint_nonunicode(FILE* stream, string_view fmt, format_args args);

// [print.fun]: the functions hold the lock of the stream while they format and
// write ("Locks stream" ... "Unconditionally unlocks stream on function exit";
// "While holding the lock on stream, writes"). A formatter that waits inside
// the call observes that another thread cannot lock the stream.

#include <atomic>
#include <cassert>
#include <cstdio>
#include <format>
#include <print>
#include <string_view>
#include <thread>

std::atomic<bool> entered{false};
std::atomic<bool> release_formatter{false};

struct slow {};

template <>
struct std::formatter<slow, char> : std::formatter<std::string_view, char> {
  template <class Ctx>
  auto format(slow, Ctx& ctx) const {
    entered.store(true, std::memory_order_release);
    while (!release_formatter.load(std::memory_order_acquire))
      std::this_thread::yield();
    return std::formatter<std::string_view, char>::format("done", ctx);
  }
};

template <>
inline constexpr bool std::enable_nonlocking_formatter_optimization<slow> = true;

template <class Print>
void test(Print print) {
  entered.store(false);
  release_formatter.store(false);

  FILE* stream = std::tmpfile();
  assert(stream);
  slow value;
  std::thread writer([&] { print(stream, "{}", std::make_format_args(value)); });
  while (!entered.load(std::memory_order_acquire))
    std::this_thread::yield();

  // Another thread cannot lock the stream while the call is in progress.
  int lock_result = ftrylockfile(stream);
  if (lock_result == 0)
    funlockfile(stream);
  assert(lock_result != 0);

  release_formatter.store(true, std::memory_order_release);
  writer.join();

  // The stream is unlocked on function exit.
  lock_result = ftrylockfile(stream);
  assert(lock_result == 0);
  funlockfile(stream);
  std::fclose(stream);
}

int main(int, char**) {
  test([](FILE* s, std::string_view f, std::format_args a) { std::vprint_unicode(s, f, a); });
  test([](FILE* s, std::string_view f, std::format_args a) { std::vprint_nonunicode(s, f, a); });
  return 0;
}
