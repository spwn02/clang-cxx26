// RUN: %clang_analyze_cc1 -std=c++2d -analyzer-checker=core -verify %s

// The analyzer follows every case of a match selection (C++29, P2688): the
// patterns are not modeled, so any case may be the one taken.

int null_in_a_case(int x) {
  int *p = nullptr;
  return match (x) {
    case 0 => *p; // expected-warning {{Dereference of null pointer}}
    case _ => 1;
  };
}

int all_cases_assign(int x) {
  int y;
  match (x) {
    case 0 => y = 1;
    case _ => y = 2;
  }
  return y;
}

int a_case_leaves_it(int x) {
  int y;
  match (x) {
    case 0 => y = 1;
    case 1 => {}
    case _ => y = 2;
  }
  return y; // expected-warning {{Undefined or garbage value returned to caller}}
}

int pattern_condition(int x, int *p) {
  if (case auto y = x)
    return y + *p;
  return 0;
}

bool test_form(int x) { return match(x, case 1 || 2); }
