// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -verify %s

constexpr int case_values[] = {1, 2, 3};
constexpr int default_values[] = {1};

int test(int x) {
  switch (x) {
    template for (constexpr auto I : case_values) {
      case I: // expected-error {{case and default labels are not allowed in expansion statements}}
        return I;
    }
  }
  return -1;
}

int test_default(int x) {
  switch (x) {
    template for (constexpr auto I : default_values) {
      default: // expected-error {{case and default labels are not allowed in expansion statements}} expected-error {{multiple default labels in one switch}} expected-note {{previous case defined here}}
        return I;
    }
  }
  return -1;
}
