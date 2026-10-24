// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-note@*:* 0+ {{}}

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

namespace cwg2432 { // cwg2432: yes
struct X {
  bool operator==(const X &) const;
  bool operator<(const X &) const;
};
struct S1 {
  X x;
  // The declared return type is not auto: the synthesized three-way
  // comparison of the member is converted to it.
  std::weak_ordering operator<=>(const S1 &) const = default;
};
bool b(S1 a, S1 c) { return a < c; }

struct S2 {
  X x;
  auto operator<=>(const S2 &) const = default;
  // expected-warning@-1 {{explicitly defaulted three-way comparison operator is implicitly deleted}}
};
void c(S2 a, S2 d) {
  (void)(a <=> d);
  // expected-error@-1 {{object of type 'S2' cannot be compared because its 'operator<=>' is implicitly deleted}}
}
} // namespace cwg2432
