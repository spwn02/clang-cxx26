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

// execution_context: service registry, ordered shutdown, frame allocator (P4003R3 4.4).

#include <io>

#include <cassert>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "test_io.h"

namespace io = std::io;
using namespace test_io;

static_assert(!std::is_copy_constructible_v<io::execution_context>);
static_assert(!std::is_copy_assignable_v<io::execution_context>);
static_assert(std::is_default_constructible_v<io::execution_context>);
static_assert(std::has_virtual_destructor_v<io::execution_context::service>);
static_assert(std::is_abstract_v<io::execution_context::service>);
static_assert(!std::is_default_constructible_v<io::execution_context::service>); // protected constructor
static_assert(!std::is_constructible_v<io::execution_context::service>);

std::vector<std::string> g_log;

struct plain_context : io::execution_context {
  ~plain_context() {
    shutdown();
    destroy();
  }
};

struct logging_service : io::execution_context::service {
  std::string name;
  explicit logging_service(io::execution_context&, std::string n = "logging") : name(n) {
    g_log.push_back("create " + name);
  }
  ~logging_service() override { g_log.push_back("destroy " + name); }
  void shutdown() override { g_log.push_back("shutdown " + name); }
};

struct service_a : io::execution_context::service {
  explicit service_a(io::execution_context&) { g_log.push_back("create A"); }
  ~service_a() override { g_log.push_back("destroy A"); }
  void shutdown() override { g_log.push_back("shutdown A"); }
};

// B depends on A: its constructor uses A from the same context (re-entrancy into use_service).
struct service_b : io::execution_context::service {
  service_a& a;
  explicit service_b(io::execution_context& c) : a(c.use_service<service_a>()) { g_log.push_back("create B"); }
  ~service_b() override { g_log.push_back("destroy B"); }
  void shutdown() override { g_log.push_back("shutdown B"); }
};

struct throwing_service : io::execution_context::service {
  explicit throwing_service(io::execution_context&) {
#ifndef TEST_HAS_NO_EXCEPTIONS
    throw 42;
#endif
  }
  void shutdown() override {}
};

int main(int, char**) {
  // use_service creates once and returns the same object afterwards.
  {
    plain_context ctx;
    logging_service& s1 = ctx.use_service<logging_service>();
    logging_service& s2 = ctx.use_service<logging_service>();
    assert(&s1 == &s2);
    assert(g_log.size() == 1 && g_log[0] == "create logging");
    g_log.clear();
  }
  assert((g_log == std::vector<std::string>{"shutdown logging", "destroy logging"}));
  g_log.clear();

  // make_service forwards the arguments, and refuses a second service of the same type.
  {
    plain_context ctx;
    logging_service& s = ctx.make_service<logging_service>(std::string("custom"));
    assert(s.name == "custom");
    assert(&ctx.use_service<logging_service>() == &s);
#ifndef TEST_HAS_NO_EXCEPTIONS
    try {
      ctx.make_service<logging_service>();
      assert(false);
    } catch (const std::invalid_argument&) {
    }
#endif
    g_log.clear();
  }
  g_log.clear();

  // Shutdown runs in reverse order of creation, then destruction in reverse order.
  {
    plain_context ctx;
    ctx.use_service<service_a>();
    ctx.use_service<logging_service>();
    ctx.use_service<service_b>(); // A already exists: B's constructor finds it
    g_log.clear();
  }
  assert((g_log == std::vector<std::string>{
                       "shutdown B", "shutdown logging", "shutdown A", "destroy B", "destroy logging", "destroy A"}));
  g_log.clear();

  // A service whose constructor creates another one: the dependency is created first, so it is shut down last.
  {
    plain_context ctx;
    service_b& b = ctx.use_service<service_b>();
    assert(&b.a == &ctx.use_service<service_a>());
    assert((g_log == std::vector<std::string>{"create A", "create B"}));
    g_log.clear();
  }
  assert((g_log == std::vector<std::string>{"shutdown B", "shutdown A", "destroy B", "destroy A"}));
  g_log.clear();

  // Distinct contexts have distinct services.
  {
    plain_context c1, c2;
    assert(&c1.use_service<service_a>() != &c2.use_service<service_a>());
    g_log.clear();
  }
  g_log.clear();

#ifndef TEST_HAS_NO_EXCEPTIONS
  // A constructor that throws leaves the registry unchanged.
  {
    plain_context ctx;
    try {
      ctx.use_service<throwing_service>();
      assert(false);
    } catch (int) {
    }
    g_log.clear();
    ctx.use_service<service_a>();
    g_log.clear();
  }
  g_log.clear();
#endif

  // A derived context may call shutdown() and destroy() itself (both idempotent), and the base destructor repeats them.
  {
    struct exposed : io::execution_context {
      using io::execution_context::destroy;
      using io::execution_context::shutdown;
    };
    exposed ctx;
    ctx.use_service<service_a>();
    g_log.clear();
    ctx.shutdown();
    ctx.shutdown();
    assert((g_log == std::vector<std::string>{"shutdown A"}));
    // A service created after shutdown starts shut down.
    ctx.use_service<logging_service>();
    assert((g_log == std::vector<std::string>{"shutdown A", "create logging", "shutdown logging"}));
    g_log.clear();
    ctx.destroy();
    ctx.destroy();
    assert((g_log == std::vector<std::string>{"destroy logging", "destroy A"}));
    g_log.clear();
  }
  assert(g_log.empty()); // nothing left for the destructor

  // Frame allocator.
  {
    plain_context ctx;
    assert(ctx.get_frame_allocator() == nullptr);
    counting_resource r1, r2;
    ctx.set_frame_allocator(&r1);
    assert(ctx.get_frame_allocator() == &r1);
    ctx.set_frame_allocator(&r2);
    assert(ctx.get_frame_allocator() == &r2);
    ctx.set_frame_allocator(nullptr);
    assert(ctx.get_frame_allocator() == nullptr);
    static_assert(noexcept(ctx.get_frame_allocator()));
    static_assert(noexcept(ctx.set_frame_allocator(nullptr)));
    const plain_context& cref = ctx;
    assert(cref.get_frame_allocator() == nullptr);
  }

  return 0;
}
