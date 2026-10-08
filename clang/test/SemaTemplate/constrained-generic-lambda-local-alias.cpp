// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s
// expected-no-diagnostics

// #268: a generic lambda with a pack whose requires-clause names a local type alias of the enclosing function template
// (the alias type depends on the template parameters) reaches LocalInstantiationScope::findInstantiationOf with the
// alias after the scope of the function's instantiation is gone. Assertion builds abort with "declaration not
// instantiated in this scope".
// XFAIL: asserts

template <class T, class... A>
concept invocable_with = requires(T& t, A&... a) { t(a...); };

struct Callable {
  void operator()(int&) {}
};

struct Holder {
  using type = Callable;
};

template <class S>
auto transform(S&&) {
  using Func = typename S::type;
  return []<class... Vs>(Vs&... vs) requires invocable_with<Func, Vs&...> { return sizeof...(vs); };
}

int main() {
  auto l = transform(Holder{});
  int x = 0;
  return static_cast<int>(l(x)) - 1;
}
