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

// The alias may name a local variable's type, or the constraint may name the variable directly.
template <class S>
auto transform3(S&&) {
  auto v = typename S::type{};
  using T = decltype(v);
  return []<class... Vs>(Vs&... vs) requires invocable_with<T, Vs&...> { return sizeof...(vs); };
}

template <class S>
auto transform4(S&&) {
  auto v = typename S::type{};
  return []<class... Vs>(Vs&... vs) requires invocable_with<decltype(v), Vs&...> { return sizeof...(vs); };
}

template <class L>
concept call1 = requires(L& l, int& x) { l(x); };
template <class L>
concept call2 = requires(L& l, int& x) { l(x, x); };

int main() {
  auto l = transform(Holder{});
  auto m = transform2(Holder{});
  auto n = transform3(Holder{});
  auto o = transform4(Holder{});
  int x = 0;
  static_assert(call1<decltype(l)> && call1<decltype(m)> && call1<decltype(n)> && call1<decltype(o)>);
  static_assert(!call2<decltype(l)> && !call2<decltype(m)> && !call2<decltype(n)> && !call2<decltype(o)>);
  return static_cast<int>(l(x) + m(x) + n(x) + o(x)) - 4;
}
