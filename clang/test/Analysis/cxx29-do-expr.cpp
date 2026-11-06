// RUN: %clang_analyze_cc1 -std=c++29 -analyzer-checker=core,unix.Malloc,debug.ExprInspection -verify %s

typedef __SIZE_TYPE__ size_t;
void *malloc(size_t);
void free(void *);
void clang_analyzer_eval(int);
void clang_analyzer_warnIfReached();

int value_flows_out() {
  int x = do { do_return 0; };
  return 1 / x; // expected-warning{{Division by zero}}
}

int both_paths(bool b) {
  int x = do {
    if (b)
      do_return 1;
    do_return 2;
  };
  clang_analyzer_eval(x == 1 || x == 2); // expected-warning{{TRUE}}
  return x;
}

int code_after_do_return_is_dead() {
  int x = do {
    do_return 3;
    clang_analyzer_warnIfReached(); // no-warning
  };
  return x;
}

int do_return_leaves_the_loop() {
  int x = do {
    for (int i = 0; i < 3; ++i)
      if (i == 1)
        do_return i;
    do_return 9;
  };
  clang_analyzer_eval(x == 1); // expected-warning{{TRUE}}
  return x;
}

int guard_before_deref(int *p) {
  return do {
    if (!p)
      do_return 0;
    do_return *p; // no-warning
  };
}

int null_deref_in_operand() {
  int *p = nullptr;
  return do { do_return *p; }; // expected-warning{{Dereference of null pointer}}
}

int init_capture_value() {
  int x = do [k = 0] { do_return k; };
  return 1 / x; // expected-warning{{Division by zero}}
}

int explicit_result_type() {
  long x = do -> long { do_return 0; };
  return 1 / x; // expected-warning{{Division by zero}}
}

int nested() {
  int x = do { do_return do { do_return 0; }; };
  return 1 / x; // expected-warning{{Division by zero}}
}

int nested_inner_value_does_not_leak() {
  int x = do {
    int y = do { do_return 5; };
    do_return y + 1;
  };
  clang_analyzer_eval(x == 6); // expected-warning{{TRUE}}
  return x;
}

void leak() {
  void *p = do { do_return malloc(4); };
} // expected-warning{{Potential leak of memory pointed to by 'p'}}

void no_leak() {
  void *p = do { do_return malloc(4); };
  free(p);
}

void void_result(int *p) {
  (void)(do { do_return; });
  clang_analyzer_warnIfReached(); // expected-warning{{REACHABLE}}
}

struct S {
  int v;
  ~S();
};

int destructor_runs_once() {
  return do { S s{1}; do_return s.v; };
}

template <class T> T tmpl(T v) {
  T z = do { do_return 0; };
  return do { do_return v / z; }; // expected-warning{{Division by zero}}
}
int instantiate() { return tmpl(3); }
