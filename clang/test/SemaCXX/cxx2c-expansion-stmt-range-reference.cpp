// RUN: %clang_cc1 -std=c++26 -freflection-latest -fexpansion-statements -fsyntax-only -Wno-expansion-stmt-non-compound-body -verify %s
// expected-no-diagnostics

struct Range {
  int values[3] = {1, 2, 3};
  constexpr int *begin() { return values; }
  constexpr int *end() { return values + 3; }
  constexpr const int *begin() const { return values; }
  constexpr const int *end() const { return values + 3; }
};

struct NonConstBeginRange {
  int values[3] = {1, 2, 3};
  constexpr int *begin() { return values; }
  constexpr int *end() { return values + 3; }
};

struct CopyCountedRange {
  int values[3] = {1, 2, 3};
  int copies = 0;
  constexpr CopyCountedRange() = default;
  constexpr CopyCountedRange(const CopyCountedRange &other)
      : values{other.values[0], other.values[1], other.values[2]},
        copies(other.copies + 1) {}
  constexpr int *begin() const { return const_cast<int *>(values); }
  constexpr int *end() const { return const_cast<int *>(values) + 3; }
};

constexpr bool lvalue_mutates_original() {
  int values[3] = {1, 2, 3};
  template for (auto &x : values) x *= 10;
  return values[0] == 10 && values[1] == 20 && values[2] == 30;
}
static_assert(lvalue_mutates_original());

constexpr bool lvalue_ranges() {
  NonConstBeginRange nonconst;
  template for (auto x : nonconst) { (void)x; }

  Range range;
  template for (auto x : range) { (void)x; }
  const Range const_range{};
  template for (auto x : const_range) { (void)x; }

  CopyCountedRange copied;
  template for (auto &x : copied) x *= 10;
  return copied.values[0] == 10 && copied.values[1] == 20 &&
         copied.values[2] == 30 && copied.copies == 0;
}
static_assert(lvalue_ranges());

void lvalue_class_ranges() {
  NonConstBeginRange nonconst;
  template for (auto x : nonconst) { (void)x; }
  Range range;
  template for (auto x : range) { (void)x; }
  const Range const_range{};
  template for (auto x : const_range) { (void)x; }
}

void prvalue_range() {
  template for (auto x : Range{}) { (void)x; }
  template for (constexpr auto x : Range{}) { (void)x; }
}
