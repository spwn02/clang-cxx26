// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s
// expected-no-diagnostics

// #268: a generic lambda with a pack whose requires-clause names a local type alias of the enclosing function template
// (the alias type depends on the template parameters) is substituted after the instantiation of that function has
// finished; the alias is no longer in the local instantiation scope, so its underlying type is substituted instead.
// This used to abort with "declaration not instantiated in this scope" in assertion builds.

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

// An alias of an alias, used in more than one place of the constraint.
template <class S>
auto transform2(S&&) {
  using Func = typename S::type;
  using Ptr = Func*;
  return []<class... Vs>(Vs&... vs) requires invocable_with<Func, Vs&...> && (sizeof(Ptr) > 0) {
    return sizeof...(vs);
  };
}

template <class L>
concept call1 = requires(L& l, int& x) { l(x); };
template <class L>
concept call2 = requires(L& l, int& x) { l(x, x); };

int main() {
  auto l = transform(Holder{});
  auto m = transform2(Holder{});
  int x = 0;
  static_assert(call1<decltype(l)> && call1<decltype(m)>);
  static_assert(!call2<decltype(l)> && !call2<decltype(m)>);
  return static_cast<int>(l(x) + m(x)) - 2;
}
