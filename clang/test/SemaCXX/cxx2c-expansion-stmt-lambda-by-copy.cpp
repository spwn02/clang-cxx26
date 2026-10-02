// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -fsyntax-only -verify %s
// expected-no-diagnostics

// Copy captures must be rebuilt after every enclosing expansion is substituted.
template <class T> int by_default_impl(T v) {
  int n = 0;
  template for (auto x : {v, v}) {
    template for (auto y : {x, x}) {
      auto add = [=, &n] { n += y; };
      add();
    }
  }
  return n;
}
int by_default() { return by_default_impl(1); }

int generic_three_levels() {
  auto g = [](auto v) {
    int n = 0;
    template for (auto x : {v, v}) {
      template for (auto y : {x, x, x}) {
        template for (auto z : {y, y}) {
          auto add = [&n, z] { n += z; };
          add();
        }
      }
    }
    return n;
  };
  return g(1);
}

template <class T> int init_capture_impl(T v) {
  int n = 0;
  template for (auto x : {v, v}) {
    template for (auto y : {x, x}) {
      auto add = [w = y, &n] { n += w; };
      add();
    }
  }
  return n;
}
int init_capture() { return init_capture_impl(1); }

template <class T> int mutable_copy_impl(T v) {
  int n = 0;
  template for (auto x : {v, v}) {
    template for (auto y : {x, x}) {
      auto add = [y, &n]() mutable { n += y++; };
      add();
      n += y - 1; // Mutating the copy must leave the expansion variable intact.
    }
  }
  return n;
}
int mutable_copy() { return mutable_copy_impl(1); }

int non_template() {
  int n = 0;
  template for (auto x : {1, 1}) {
    template for (auto y : {x, x}) {
      auto add = [y, &n] { n += y; };
      add();
    }
  }
  return n;
}

struct Copyable {
  int value;
  int *copies;
  Copyable(int value, int &copies) : value(value), copies(&copies) {}
  Copyable(const Copyable &other) : value(other.value), copies(other.copies) {
    ++*copies;
  }
};
template <class T> int class_copy_impl(T v) {
  int n = 0;
  template for (auto x : {v, v}) {
    template for (auto y : {x, x}) {
      int before = *y.copies;
      auto add = [y] { return y.value; };
      if (*y.copies != before + 1)
        return -100;
      n += add();
    }
  }
  return n;
}
int class_copy() {
  int copies = 0;
  return class_copy_impl(Copyable(1, copies));
}
