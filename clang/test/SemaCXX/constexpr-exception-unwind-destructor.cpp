// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// expected-no-diagnostics

// A destructor that runs while an exception unwinds through its scope
// evaluates like any other function: the exception in flight is not one of
// its own.
struct E {};

struct Owner {
  int *p;
  struct Release {
    Owner &o;
    constexpr void operator()() {
      if (o.p != nullptr)
        delete o.p;
    }
  };
  constexpr Owner() : p(new int(1)) {}
  constexpr ~Owner() { Release{*this}(); }
};

constexpr bool unwinds() {
  try {
    Owner o;
    throw E{};
  } catch (...) {
    return true;
  }
  return false;
}
static_assert(unwinds());

struct Ignored {
  int *p;
  constexpr Ignored() : p(new int(1)) {}
  constexpr ~Ignored() {
    (void)0, (void)nullptr;
    delete p;
  }
};

constexpr bool unwindsIgnoredValues() {
  try {
    Ignored i;
    throw E{};
  } catch (...) {
    return true;
  }
  return false;
}
static_assert(unwindsIgnoredValues());
