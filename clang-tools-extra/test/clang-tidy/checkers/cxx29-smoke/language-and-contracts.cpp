// RUN: clang-tidy -checks='-*,modernize-*,readability-*,performance-*' %s -- -std=c++29 -fcontract-evaluation-semantic=enforce 2>&1 | FileCheck --allow-empty %s
// RUN: cp %s %t.cpp
// RUN: clang-tidy -checks='-*,modernize-*,readability-*,performance-*' -fix %t.cpp -- -std=c++29 -fcontract-evaluation-semantic=enforce
// RUN: clang++ -std=c++29 -fsyntax-only -fcontract-evaluation-semantic=enforce %t.cpp
// CHECK-NOT: warning:
// CHECK-NOT: Assertion
// CHECK-NOT: Stack dump
// NOLINTBEGIN(readability-identifier-naming)

namespace std::contracts::controls { // NOLINT(modernize-concat-nested-namespaces)
inline namespace __p4451 {
struct __enforced_t {};
inline constexpr __enforced_t enforced{};
}
}

struct Base {
  int first;
  int second;
};
struct Derived : Base { int third; };
constexpr Derived value{.first = 1, .second = 2, .third = 3};
static_assert(value.second == 2);

void may_throw();
void no_throw() noexcept;
template <class T> concept NoThrowCall = requires(T object) { { object.run() } noexcept(noexcept(object.run())); };

struct VirtualBase {
  virtual int run(int value) pre(value > 0) post(r: r > 0) { return value; }
  virtual ~VirtualBase() = default;
};
struct Override : VirtualBase { int run(int value) override { return value; } }; // NOLINT(modernize-use-trailing-return-type)

int enforce_contract(const int value) pre<std::contracts::controls::enforced>(value > 0) post(r: r > value) {
  contract_assert(value < 100); // NOLINT(readability-magic-numbers)
  return value + 1;
}

struct Assignment {
  Assignment& operator=(const Assignment&) & = default; // NOLINT(modernize-use-trailing-return-type)
};

void *operator new(unsigned long); // NOLINT(modernize-use-trailing-return-type)
void operator delete(void *) noexcept;

int diverging(bool stop) { // NOLINT(modernize-use-trailing-return-type)
  return stop ? throw 1 : 2;
}


// NOLINTEND(readability-identifier-naming)
