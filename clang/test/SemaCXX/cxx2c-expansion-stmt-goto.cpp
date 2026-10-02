// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -verify %s

int forward() {
  template for (auto x : {1, 2}) { if (x == 2) goto done; }
  return 1;
done:
  return 0;
}
int backward() {
  int result = 0;
again:
  template for (auto x : {1, 2}) {
    if (++result < 2) goto again;
  }
  return result;
}
template <class T> int forward_template() {
  template for (auto x : {T(1), T(2)}) { if (x == 2) goto done; }
  return 1;
done:
  return 0;
}
template <class T> int backward_template() {
  int result = 0;
again:
  template for (auto x : {T(1), T(2)}) {
    if (++result < 2) goto again;
  }
  return result;
}
int instantiate() { return forward_template<int>() + backward_template<int>(); }

void into() {
  goto inside;
  template for (auto x : {1}) {
    inside:; // expected-error {{identifier labels are not allowed in expansion statements}}
  }
}
void bypass_initialization() {
  template for (auto x : {1}) { goto done; } // expected-error {{cannot jump from this goto statement to its label}}
  int initialized = 1; // expected-note {{jump bypasses variable initialization}}
done:;
}
