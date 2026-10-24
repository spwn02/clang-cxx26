// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-no-diagnostics

namespace std {
struct weak_ordering;
struct strong_ordering {
  signed char n;
  constexpr strong_ordering(signed char v) : n(v) {}
  static const strong_ordering less, equal, equivalent, greater;
  constexpr operator int() const { return n; }
  constexpr operator weak_ordering() const;
};
constexpr strong_ordering strong_ordering::less{-1}, strong_ordering::equal{0},
    strong_ordering::equivalent{0}, strong_ordering::greater{1};
struct weak_ordering {
  signed char n;
  constexpr weak_ordering(signed char v) : n(v) {}
  static const weak_ordering less, equivalent, greater;
  constexpr operator int() const { return n; }
};
constexpr weak_ordering weak_ordering::less{-1}, weak_ordering::equivalent{0},
    weak_ordering::greater{1};
constexpr strong_ordering::operator weak_ordering() const { return weak_ordering(n); }
} // namespace std

namespace cwg2437 { // cwg2437: yes
struct S {
  int a;
  std::weak_ordering operator<=>(const S &) const = default;
};
static_assert((S{1} <=> S{1}) == 0, "");
static_assert((S{1} <=> S{2}) < 0, "");
} // namespace cwg2437
