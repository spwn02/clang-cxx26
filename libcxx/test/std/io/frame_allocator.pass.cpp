//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library
// <io>

// Frame allocator slot (get/set_cached_frame_allocator) and safe_resume (P4003R3 4.5, P4172R1 8.2-8.3).

#include <io>

#include <cassert>
#include <coroutine>
#include <thread>

#include "test_io.h"

namespace io = std::io;
using namespace test_io;

static_assert(noexcept(io::get_cached_frame_allocator()));
static_assert(noexcept(io::set_cached_frame_allocator(nullptr)));
static_assert(noexcept(io::safe_resume(std::coroutine_handle<>{})));

counting_resource g_spoil_with;

// A coroutine that overwrites the slot, as a coroutine of an unrelated chain would on resumption.
struct spoiler {
  struct promise_type {
    spoiler get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {}
  };
  std::coroutine_handle<promise_type> h;
};

spoiler make_spoiler() {
  io::set_cached_frame_allocator(&g_spoil_with);
  co_return;
}

int main(int, char**) {
  // The slot starts empty and round-trips.
  assert(io::get_cached_frame_allocator() == nullptr);
  counting_resource r;
  io::set_cached_frame_allocator(&r);
  assert(io::get_cached_frame_allocator() == &r);
  io::set_cached_frame_allocator(nullptr);
  assert(io::get_cached_frame_allocator() == nullptr);

  // A plain resume lets the coroutine spoil the slot; safe_resume restores it.
  {
    spoiler s = make_spoiler();
    io::set_cached_frame_allocator(&r);
    s.h.resume();
    assert(io::get_cached_frame_allocator() == &g_spoil_with);
    s.h.destroy();

    spoiler t = make_spoiler();
    io::set_cached_frame_allocator(&r);
    io::safe_resume(t.h);
    assert(io::get_cached_frame_allocator() == &r);
    assert(t.h.done());
    t.h.destroy();
    io::set_cached_frame_allocator(nullptr);
  }

  // The slot is per thread.
  {
    io::set_cached_frame_allocator(&r);
    bool other_thread_saw_null = false;
    std::thread th([&] {
      other_thread_saw_null = io::get_cached_frame_allocator() == nullptr;
      io::set_cached_frame_allocator(&g_spoil_with);
    });
    th.join();
    assert(other_thread_saw_null);
    assert(io::get_cached_frame_allocator() == &r);
    io::set_cached_frame_allocator(nullptr);
  }

  return 0;
}
