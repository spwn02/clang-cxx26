// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify -fexceptions -fcxx-exceptions %s

// Every example of P2806R5 that does not depend on pattern matching, with the
// verdict the paper gives ("OK" or "Ill-formed"). The block numbers refer to the
// order of the code blocks in the paper. The examples that use `match` (blocks
// 3-9, 32) are P2688 syntax and are covered by the pattern matching tests.

struct S;
[[noreturn]] void abort_now();
extern bool cond;
extern bool something;
int get(int);
struct mutex_t {};
extern mutex_t mtx;
struct lock_guard {
  explicit lock_guard(mutex_t &);
  ~lock_guard();
};

namespace sec_1_intro {
int S1();
int S2();
int E();
// block 1 and 2
auto x1 = [&] { S1(); S2(); return E(); }(); // expected-error {{non-local lambda expression cannot have a capture-default}}
auto x2 = do { S1(); S2(); do_return E(); };
} // namespace sec_1_intro

// block 0
int x = do { do_return 42; };

// block 5: an explicit trailing-return-type
auto ex5 = do -> long { do_return 42; };
static_assert(__is_same(decltype(do -> long { do_return 42; }), long));

// 4.3: the type is deduced from the do_return statements; none, or only
// "do_return;", gives void.
static_assert(__is_same(decltype(do { do_return 42; }), int));
static_assert(__is_same(decltype(do { }), void));
static_assert(__is_same(decltype(do { do_return; }), void));

namespace sec_4_4_copy_elision {
struct string {
  string(const char *);
  string(const string &);
  string(string &&);
  string &operator+=(const char *);
};
// block 10
string s = do {
  string r = "hello";
  r += "world";
  do_return r;
};
} // namespace sec_4_4_copy_elision

namespace sec_4_5_control_flow {
// block 11: OK
auto a = do {
  if (cond) {
    do_return 1;
  } else {
    do_return 2;
  }
};
// block 12: Ill-formed, different types and no trailing-return-type
auto b = do {
  if (cond) {
    do_return 1;
  } else {
    do_return 2.0; // expected-error {{'do_return' yields type 'double', conflicting with previously deduced type 'int'}}
  }
};
// ... and fine with a trailing-return-type
auto b_int = do -> int { if (cond) { do_return 1; } else { do_return 2.0; } };
auto b_double = do -> double { if (cond) { do_return 1; } else { do_return 2.0; } };
auto b_float = do -> float { if (cond) { do_return 1; } else { do_return 2.0; } };
// block 13: OK
auto c = do {
  if (cond) {
    do_return 1;
  }
  do_return 2;
};
// block 14: Ill-formed, falls off the end with a non-void type
auto d = do {
  if (cond) {
    do_return 1;
  }
}; // expected-error {{control may fall off the end of do-expression with non-void result type 'int'}}
// block 15: OK, both paths are "do_return;"
int e = (do {
  if (cond) {
    do_return;
  }
}, 1);
// block 16: OK, throw escapes
int f = do {
  if (cond) {
    do_return 1;
  }
  throw 2;
};
// block 17: OK, return escapes
int outer() {
  int g = do {
    if (cond) {
      do_return 1;
    }
    return 3;
  };
  (void)g;
  return 0;
}
// block 18: OK, a [[noreturn]] call escapes
int h = do {
  if (cond) {
    do_return 1;
  }
  abort_now();
};
// block 19: Ill-formed, the switch could still fall off the end
enum Color { Red, Green, Blue };
void func(Color c) {
  const char *name = do {
    switch (c) {
    case Red:   do_return "Red";
    case Green: do_return "Green";
    case Blue:  do_return "Blue";
    }
  }; // expected-error {{control may fall off the end of do-expression with non-void result type 'const char *'}}
  (void)name;
}
// block 20: OK, break escapes to the outer loop, otherwise both yields are int
void func2() {
  for (;;) {
    int j = do {
      if (cond) {
        break;
      }
      while (something) {
        if (cond) {
          do_return 1;
        }
      }
      do_return 2;
    };
    (void)j;
  }
}
// ... without the final "do_return 2;" it is ill-formed, because the loop may end
void func3() {
  for (;;) {
    int j = do {
      if (cond) {
        break;
      }
      while (something) {
        if (cond) {
          do_return 1;
        }
      }
    }; // expected-error {{control may fall off the end of do-expression with non-void result type 'int'}}
    (void)j;
  }
}
// ... and with "for (;;)" instead of the loop, no final yield is needed
void func4() {
  for (;;) {
    int j = do {
      if (cond) {
        break;
      }
      for (;;) {
        if (cond) {
          do_return 1;
        }
      }
    };
    (void)j;
  }
}
// block 21: OK, abort() never returns
int i5 = do {
  if (cond) {
    do_return 5;
  }
  abort_now();
};
// block 22: OK, always escaping with an explicit type
int i42 = do -> int { throw 42; };
} // namespace sec_4_5_control_flow

namespace sec_4_5_3_goto {
extern int value;
// block 23: jumping out of a do-expression to a label outside is fine
void jump_out() {
  for (;;) {
    for (;;) {
      int i = do {
        if (cond) {
          goto done;
        }
        do_return value;
      };
      (void)i;
    }
  }
done:;
}
// a label in scope of the variable being initialized is not allowed
void jump_to_scope_of_init() {
  int i = do { // expected-note {{jump bypasses variable initialization}}
    goto inside; // expected-error {{}}
    do_return 1;
  };
inside:
  (void)i;
}
// jumping into a do-expression is not allowed
void jump_into() {
  goto in; // expected-error {{}}
  (void)do { in: do_return 1; }; // expected-note {{}}
}
} // namespace sec_4_5_3_goto

namespace sec_4_6_lifetime {
int get1(int);
// block 25: both guards are destroyed at their nearest }
int i = do {
    lock_guard _(mtx);
    do_return get1(0);
  } + do {
    lock_guard _(mtx);
    do_return get1(1);
  };
} // namespace sec_4_6_lifetime

namespace sec_4_6_init_hoist {
struct Vec { ~Vec(); int *begin(); };
Vec get_data();
struct Span { int *p; };
struct Expected {
  Span v;
  explicit operator bool() const;
  Span &operator*();
  Expected(Span);
};
Expected find_interesting(const Vec &);
int best_of(Span);
// block 28 (the paper's expansion of TRY, std::span and std::expected replaced
// by small stand-ins): compiles; the temporary Vec is destroyed at the end of
// the init-statement of __r
int use() {
  int value = best_of(do -> Span {
    auto __r = find_interesting(get_data());
    if (not __r) {
      do_return Span{};
    }
    do_return *__r;
  });
  return value;
}
// block 29/30: the init-hoist keeps the temporary alive until the end of the
// full-expression
int use_hoist() {
  int value = best_of(do [__r = find_interesting(get_data())] -> Span {
    if (not __r) {
      do_return Span{};
    }
    do_return *__r;
  });
  return value;
}
} // namespace sec_4_6_init_hoist

namespace sec_4_6_2 {
struct T { T(); T(const T &); ~T(); };
T prvalue();
const T &lvalue();
void f() {
  T const &r1 = prvalue();
  T const &r3 = do -> T const & { do_return prvalue(); }; // expected-error {{returning reference to local temporary object}}
  (void)r1; (void)r3;
}
void g(bool c) {
  T const &r4 = do -> T const & {
    if (c) {
      do_return lvalue();
    } else {
      do_return prvalue(); // expected-error {{returning reference to local temporary object}}
    }
  };
  T const &r5 = do -> T const & {
    if (c) {
      do_return lvalue();
    } else {
      T x = prvalue();
      do_return x; // expected-warning {{reference to stack memory associated with local variable 'x' returned}}
    }
  };
  (void)r4; (void)r5;
}
} // namespace sec_4_6_2

namespace sec_4_8_disambiguation {
// 4.8: in a statement context "do" begins a do-while loop; parenthesize to get
// a do-expression statement.
int w(int cond_) {
  do { cond_--; } while (cond_ > 0);
  (void)(do { do_return 42; });
  return cond_;
}
}

namespace sec_4_10_statement_expression {
extern int foo();
// block 40: the same computation as the gcc statement expression
int z = do {
  int y = foo();
  if (y > 0) {
    do_return y;
  } else {
    do_return -y;
  }
};
}

namespace sec_4_12_everywhere {
// block 41 (rewritten): do-expressions are usable in every expression context,
// at namespace scope, in default member initializers and template arguments.
int i = do {
  int j = 2;
  do_return j;
};
struct M { int m = do { do_return 3; }; };
template <int N> struct Tpl {};
Tpl<do { do_return 4; }> t4;
}
