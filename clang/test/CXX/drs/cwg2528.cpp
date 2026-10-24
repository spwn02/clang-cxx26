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

namespace cwg2528 { // cwg2528: yes
void f(unsigned short a, short b, signed char d) {
  (void)(a <=> b);
  (void)(d <=> a);
  (void)(1 <=> 2u);
}
void g(unsigned a, int b) {
  (void)(a <=> b);
  // expected-error@-1 {{argument to 'operator<=>' cannot be narrowed from type 'int' to 'unsigned int'}}
}
} // namespace cwg2528
