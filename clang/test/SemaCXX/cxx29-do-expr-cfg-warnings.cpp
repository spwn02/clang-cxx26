// RUN: %clang_cc1 -std=c++29 -fsyntax-only -verify -Wimplicit-fallthrough -Wsometimes-uninitialized -Wconditional-uninitialized -Wreturn-type -Wunreachable-code %s

// A do expression is part of the function's control flow: do_return jumps to the
// end of the expression, so the CFG based warnings see its paths.

int no_fallthrough_after_do_return(int x) {
  return do {
    switch (x) {
    case 1:
      do_return 1; // leaves the switch and the body: no fall-through
    case 2:
      x++;
      [[fallthrough]];
    case 3:
      do_return 3;
    default:
      break;
    }
    do_return 0;
  };
}

int fallthrough_is_diagnosed(int x) {
  return do {
    switch (x) {
    case 1:
      x++;
    case 2: // expected-warning {{unannotated fall-through between switch labels}} expected-note {{insert '[[fallthrough]];' to silence this warning}} expected-note {{insert 'break;' to avoid fall-through}}
      do_return 2;
    }
    do_return 0;
  };
}

int sometimes_uninitialized(bool b) {
  int y; // expected-note {{initialize the variable 'y' to silence this warning}}
  int r = do {
    if (b) // expected-warning {{variable 'y' is used uninitialized whenever 'if' condition is false}} expected-note {{remove the 'if' if its condition is always true}}
      y = 1;
    do_return y; // expected-note {{uninitialized use occurs here}}
  };
  return r;
}

int initialized_on_every_path(bool b) {
  int y;
  int r = do {
    y = 1;
    do_return y;
  };
  return r + b;
}

int missing_return_after_do(bool b) {
  int r = do {
    if (b)
      do_return 1;
    do_return 2;
  };
  (void)r;
} // expected-warning {{non-void function does not return a value}}

int return_inside_do_is_a_function_return(bool b) {
  return do {
    if (b)
      return 1; // returns from the function
    do_return 2;
  };
}

int code_after_do_return(bool b) {
  return do {
    do_return 1;
    int z = 5; // expected-warning {{code will never be executed}}
    (void)z;
    do_return 3;
  };
}
