// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions

namespace std {
  typedef decltype(sizeof(0)) size_t;
  template<typename E> class initializer_list {
    const E *begin_; size_t size_;
  public:
    const E *begin() const { return begin_; }
    const E *end() const { return begin_ + size_; }
  };
}

namespace cwg711 { // cwg711: yes
auto x = {1, 2, 3};
auto y{1};
static_assert(__is_same(decltype(y), int), "");
auto z = {1, 2.0};
// expected-error@-1 {{deduced conflicting types ('int' vs 'double') for initializer list element type}}
auto w{1, 2};
// expected-error@-1 {{initializer for variable 'w' with type 'auto' contains multiple expressions}}
} // namespace cwg711
