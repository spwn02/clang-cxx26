#include <meta>
#include <cstdio>
template <typename> void test_impl() { static_assert(false); std::puts("foo"); }
struct Foo {
  void (*test)() = nullptr;
  template <bool E> consteval void maybe_init() {
    if constexpr (E) test = std::meta::extract<void(*)()>(std::meta::substitute(^^test_impl, {^^int}));
  }
  consteval Foo(bool enabled) {
    (this->*std::meta::extract<void (Foo::*)()>(std::meta::substitute(^^maybe_init, {std::meta::reflect_constant(enabled)})))();
  }
};
int main() { auto f = Foo(true); f.test(); }
