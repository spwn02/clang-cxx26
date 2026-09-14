// RUN: %clang_cc1 -std=c++26 -fcxx-exceptions -fexceptions -verify %s

namespace std {
using size_t = decltype(sizeof(0));
}

struct owner {
  char *p;
  constexpr owner(const char *s) : p(new char[4]{s[0], s[1], s[2], 0}) {}
  constexpr owner(const owner &o) : owner(o.p) {}
  constexpr ~owner() { delete[] p; }
};

struct error {
  owner message;
  constexpr error(const char *s) : message(s) {}
};

consteval int throw_from_helper() { throw error("bad"); }

consteval bool direct_catch() {
  try {
    throw error("bad");
  } catch (const error &e) {
    return e.message.p[0] == 'b' && e.message.p[2] == 'd';
  }
  return false;
}

consteval bool call_catch() {
  try {
    (void)throw_from_helper();
  } catch (const error &e) {
    return e.message.p[0] == 'b' && e.message.p[2] == 'd';
  }
  return false;
}

static_assert(direct_catch());
static_assert(call_catch());

int main() { return 0; }

// expected-no-diagnostics
