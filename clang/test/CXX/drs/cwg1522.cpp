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

namespace cwg1522 { // cwg1522: yes
struct E {
  E();
private:
  E(const E &); // #cwg1522-copy
};
void f() {
  E e;
  std::initializer_list<E> l = {e};
  // expected-error@-1 {{calling a private constructor of class 'cwg1522::E'}}
  //   expected-note@#cwg1522-copy {{declared private here}}
}
} // namespace cwg1522
