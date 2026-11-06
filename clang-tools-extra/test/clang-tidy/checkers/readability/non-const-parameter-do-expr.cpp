// RUN: %check_clang_tidy -std=c++29 %s readability-non-const-parameter %t -- --

// A pointer that leaves a do expression through do_return escapes just like a
// returned one, so it must not become a pointer to const.
int *escapes_plain(int *p) { return p; }
int *escapes_do_return(int *p) { return do { do_return p; }; }
int *escapes_via_variable(int *p) {
  int *r = do { do_return p; };
  return r;
}
int *escapes_on_one_path(int *p, bool b) {
  return do {
    if (b)
      do_return p;
    do_return static_cast<int *>(nullptr);
  };
}

int only_read(int *p) {
  // CHECK-MESSAGES: :[[@LINE-1]]:20: warning: pointer parameter 'p' can be pointer to const [readability-non-const-parameter]
  // CHECK-FIXES: {{^}}int only_read(const int *p) {{{$}}
  return do { do_return *p; };
}
