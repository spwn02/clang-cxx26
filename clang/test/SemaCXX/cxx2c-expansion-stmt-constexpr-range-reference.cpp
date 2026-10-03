// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -fsyntax-only -verify %s
// expected-no-diagnostics

// P1306R5 [stmt.expand]: with a constexpr expansion variable the range is a
// constexpr reference to the original object, so a constexpr reference
// expansion variable can denote an element of a static constexpr range (#188).
struct Range {
  int d[2]{1, 2};
  constexpr const int *begin() const { return d; }
  constexpr const int *end() const { return d + 2; }
};
static constexpr Range global;

void static_range() {
  template for (constexpr const int &x : global) { static_assert(x > 0); }
  template for (constexpr auto x : global) { static_assert(x > 0); }
}

struct Holder { Range r; };
static constexpr Holder holder;
static constexpr Range ranges[2];

void subobjects() {
  template for (constexpr const int &x : holder.r) { static_assert(x > 0); }
  template for (constexpr const int &x : ranges[1]) { static_assert(x > 0); }
}

// Since P2686R5 a local constexpr object of the same function can be bound by a
// constexpr reference; either way the iteration result is the same.
consteval int local_range() {
  constexpr Range local;
  int sum = 0;
  template for (constexpr int x : local) { sum += sizeof(char[x]); }
  return sum;
}
static_assert(local_range() == 3);
