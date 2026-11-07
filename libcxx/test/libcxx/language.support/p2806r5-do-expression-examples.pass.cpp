//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23 || c++26

// The examples of P2806R5 (do expressions), run: the behaviour the paper states
// for control flow (4.5), copy elision (4.4), lifetimes (4.6, the init-hoist) and
// the places a do expression may appear (4.12).

// RUN: %{build}
// RUN: %{exec} %t.exe

#include <cassert>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// ---- the type and the value (4.3) ----
static_assert(__is_same(decltype(do { do_return 42; }), int));
static_assert(__is_same(decltype(do -> long { do_return 42; }), long));
static_assert(__is_same(decltype(do { do_return; }), void));
static_assert(__is_same(decltype(do { }), void));

// ---- control flow (4.5) ----
bool cond = false;

int example_a() {
  return do {
    if (cond) {
      do_return 1;
    } else {
      do_return 2;
    }
  };
}

int example_c() {
  return do {
    if (cond) {
      do_return 1;
    }
    do_return 2;
  };
}

int example_e() {
  int n = 0;
  (void)(do {
    if (cond) {
      do_return;
    }
    n = 5;
  }, 1);
  return n;
}

int example_f() {
  try {
    return do {
      if (cond) {
        do_return 1;
      }
      throw 2;
    };
  } catch (int e) {
    return 100 + e;
  }
}

int example_outer() {
  int g = do {
    if (cond) {
      do_return 1;
    }
    return 3;
  };
  return g + 10;
}

enum Color { Red, Green, Blue };
std::string_view name_of(Color c) {
  return do {
    switch (c) {
    case Red:   do_return "Red";
    case Green: do_return "Green";
    case Blue:  do_return "Blue";
    }
    do_return "?";
  };
}

// break out of the loop around the do expression, and yield from a loop in it
int example_loop(int limit) {
  int sum = 0;
  for (int i = 0;; ++i) {
    int j = do {
      if (i >= limit) {
        break;
      }
      for (int k = 0; k < 10; ++k) {
        if (k == i) {
          do_return k * 10;
        }
      }
      do_return -1;
    };
    sum += j;
  }
  return sum;
}

// continue, and goto out of two loops (4.5.3)
int example_goto() {
  int seen = 0;
  for (int a = 0; a < 3; ++a) {
    for (int b = 0; b < 3; ++b) {
      int i = do {
        if (b == 1) {
          continue;
        }
        if (a == 2) {
          goto done;
        }
        do_return a * 10 + b;
      };
      seen += i;
    }
  }
done:
  return seen;
}

// ---- copy elision (4.4): the local is implicitly moved, never copied ----
struct Counted {
  static inline int copies = 0, moves = 0;
  std::string s;
  Counted(std::string v) : s(std::move(v)) {}
  Counted(const Counted &o) : s(o.s) { ++copies; }
  Counted(Counted &&o) noexcept : s(std::move(o.s)) { ++moves; }
};

Counted counted_elision() {
  return do {
    Counted r("hello");
    r.s += "world";
    do_return r;
  };
}

// ---- lifetimes (4.6): the guards are destroyed at their nearest } ----
struct Mutex {
  bool held = false;
  int locks = 0;
};
Mutex mtx;
struct LockGuard {
  explicit LockGuard(Mutex &m) : m_(m) {
    assert(!m_.held && "deadlock: the other guard is still alive");
    m_.held = true;
    ++m_.locks;
  }
  ~LockGuard() { m_.held = false; }
  Mutex &m_;
};
int get(int i) { return 100 + i; }

int two_guards() {
  return do {
           LockGuard _(mtx);
           do_return get(0);
         } + do {
           LockGuard _(mtx);
           do_return get(1);
         };
}

// The init-hoist keeps the declaration (and the temporaries in its
// initializer) alive until the end of the full expression; a local of the body
// does not live that long.
std::vector<std::string> events;
struct Obs {
  std::string name;
  explicit Obs(std::string n) : name(std::move(n)) { events.push_back("make " + name); }
  Obs(const Obs &) = delete;
  ~Obs() { events.push_back("kill " + name); }
  int value() const { return 7; }
};
Obs make_obs(std::string n) { return Obs(std::move(n)); }
int use(int v) {
  events.push_back("use");
  return v;
}

int hoisted() {
  return use(do [h = make_obs("hoisted")] -> int { do_return h.value(); });
}
int not_hoisted() {
  return use(do -> int {
    Obs local("local");
    do_return local.value();
  });
}

// ---- the TRY macro of 4.6, expanded (block 28 with its early return) ----
struct Vec {
  std::vector<int> v;
  Vec(std::vector<int> x) : v(std::move(x)) { events.push_back("make Vec"); }
  Vec(Vec &&) = default;
  ~Vec() { events.push_back("kill Vec"); }
};
Vec get_data(bool ok) { return Vec(ok ? std::vector<int>{1, 2, 3} : std::vector<int>{}); }
std::expected<std::span<int const>, std::string> find_interesting(Vec const &d) {
  if (d.v.empty()) {
    return std::unexpected(std::string("nothing"));
  }
  return std::span<int const>(d.v);
}
int best_of(std::span<int const> s) {
  events.push_back("best_of");
  // (only the size: the span may dangle)
  return s.empty() ? -1 : static_cast<int>(s.size());
}

auto do_something(bool ok) -> std::expected<int, std::string> {
  int value = best_of(do -> std::span<int const> {
    auto __r = find_interesting(get_data(ok));
    if (not __r) {
      return std::unexpected(__r.error());
    }
    do_return *__r;
  });
  return value;
}

// The init-hoisted form keeps the temporary Vec until the end of the full
// expression, so the span does not dangle.
auto do_something_hoisted(bool ok) -> std::expected<int, std::string> {
  int value = best_of(do [__r = find_interesting(get_data(ok))] -> std::span<int const> {
    if (not __r) {
      return std::unexpected(__r.error());
    }
    do_return *__r;
  });
  return value;
}

// ---- everywhere an expression may appear (4.12) ----
int ns_scope = do { int j = 2; do_return j * 21; };
struct Member { int m = do { do_return 3; }; };
template <int N> struct Tpl { static constexpr int value = N; };
static_assert(Tpl<do { do_return 4; }>::value == 4);
constexpr int constexpr_do = do { int s = 0; for (int i = 1; i <= 4; ++i) s += i; do_return s; };
static_assert(constexpr_do == 10);

int main(int, char**) {
  cond = true;
  assert(example_a() == 1);
  assert(example_c() == 1);
  assert(example_e() == 0);
  assert(example_f() == 1);
  assert(example_outer() == 11);
  cond = false;
  assert(example_a() == 2);
  assert(example_c() == 2);
  assert(example_e() == 5);
  assert(example_f() == 102);
  assert(example_outer() == 3);

  assert(name_of(Red) == "Red" && name_of(Blue) == "Blue");

  // i = 0..2: j = 0, -1 (k==1 hit at k=1 -> 10)... computed by the definition
  {
    int expected = 0;
    for (int i = 0; i < 4; ++i) {
      int j = -1;
      for (int k = 0; k < 10; ++k) {
        if (k == i) {
          j = k * 10;
          break;
        }
      }
      expected += j;
    }
    assert(example_loop(4) == expected);
  }
  // a = 0: b = 0 -> 0, b = 1 continue, b = 2 -> 2; a = 1: 10, 12; a = 2: b = 0 goes to done
  assert(example_goto() == 0 + 2 + 10 + 12);

  {
    Counted::copies = Counted::moves = 0;
    Counted c = counted_elision();
    assert(c.s == "helloworld");
    assert(Counted::copies == 0);
  }

  assert(two_guards() == 100 + 101);
  assert(mtx.locks == 2 && !mtx.held);

  {
    events.clear();
    assert(hoisted() == 7);
    // the hoisted object is destroyed after the call that used the value
    std::vector<std::string> want = {"make hoisted", "use", "kill hoisted"};
    assert(events == want);
  }
  {
    events.clear();
    assert(not_hoisted() == 7);
    std::vector<std::string> want = {"make local", "kill local", "use"};
    assert(events == want);
  }

  {
    // the early return from inside the argument
    events.clear();
    auto r = do_something(false);
    assert(!r && r.error() == "nothing");
    std::vector<std::string> want = {"make Vec", "kill Vec"};
    assert(events == want); // no best_of: the return skipped it, the temporary was destroyed
  }
  {
    // Without the init-hoist (the paper's motivating problem): the temporary
    // Vec is destroyed at the end of the initialization of __r, inside the do
    // expression, so best_of sees a span whose vector is already gone.
    events.clear();
    auto r = do_something(true);
    assert(r);
    std::vector<std::string> want = {"make Vec", "kill Vec", "best_of"};
    assert(events == want);
  }
  {
    events.clear();
    auto r = do_something_hoisted(true);
    assert(r && *r == 3);
    // the temporary Vec lives until the end of the full expression: best_of
    // runs before it is destroyed
    std::vector<std::string> want = {"make Vec", "best_of", "kill Vec"};
    assert(events == want);
    events.clear();
    auto e = do_something_hoisted(false);
    assert(!e && e.error() == "nothing");
    std::vector<std::string> want2 = {"make Vec", "kill Vec"};
    assert(events == want2);
  }

  assert(ns_scope == 42);
  assert(Member{}.m == 3);
  return 0;
}
