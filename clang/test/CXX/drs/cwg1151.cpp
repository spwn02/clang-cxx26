// RUN: %clang_cc1 -std=c++11 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++14 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++17 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-no-diagnostics

namespace std {
  typedef decltype(sizeof(0)) size_t;
  template<typename E> class initializer_list {
    const E *begin_; size_t size_;
  public:
    const E *begin() const { return begin_; }
    const E *end() const { return begin_ + size_; }
  };
}

namespace cwg1151 { // cwg1151: yes
struct A {
  A(std::initializer_list<int>);
  A(int, int) = delete;
};
A a{1, 2};
} // namespace cwg1151
