// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -verify %s

constexpr int nested_switch() {
  int result = 0;
  template for (auto x : {1, 2}) {
    switch (x) {
    case 1: ++result; break;
    default: result += 2; break;
    }
  }
  return result;
}
static_assert(nested_switch() == 3);
template <class T> constexpr int dependent_switch() {
  int result = 0;
  template for (auto x : {T(1), T(2)}) {
    switch (x) {
    case 1: ++result; break;
    default: result += 2; break;
    }
  }
  return result;
}
static_assert(dependent_switch<int>() == 3);

void outside_switch(int k) {
  switch (k) {
  case 0:
    template for (auto x : {1}) {
      case 1:; // expected-error {{case and default labels are not allowed in expansion statements}} expected-error {{duplicate case value '1'}} expected-note {{previous case defined here}}
    }
  }
  switch (k) {
    template for (auto x : {1}) {
      default:; // expected-error {{case and default labels are not allowed in expansion statements}} expected-error {{multiple default labels in one switch}} expected-note {{previous case defined here}}
    }
  }
}
void identifier_labels() {
  template for (auto x : {1}) {
    if (x) { nested:; } // expected-error {{identifier labels are not allowed in expansion statements}}
  }
}
