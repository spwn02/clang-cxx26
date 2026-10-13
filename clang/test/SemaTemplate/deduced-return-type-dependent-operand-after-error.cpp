// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s
// RUN: not %clang_cc1 -std=c++20 -fsyntax-only -ast-dump -ast-dump-filter='operator()' %s 2>/dev/null | FileCheck %s

// A function with a deduced return type whose return operand is still type-dependent
// although the function is not dependent (the leftover of an error diagnosed in a
// nested instantiation) must not be deduced to a dependent 'auto': its callers took
// that for a deduced type and the dependence spread into the aliases, fields and
// locals of a non-dependent instantiation (an assertion in the layout code).
// The operator() specialization is invalid after the diagnosed error.

template <class T> struct rmref { using type = T; };
template <class T> struct rmref<T &> { using type = T; };
template <class T> struct rmref<T &&> { using type = T; };
template <class T> using rmref_t = typename rmref<T>::type;
template <class T> T &&forward(rmref_t<T> &x) { return static_cast<T &&>(x); }
template <class T> T &&forward(rmref_t<T> &&x) { return static_cast<T &&>(x); }

struct state {};
template <class T> concept op_state = __is_same(T, state);

template <class S, class R>
auto impl(S &&s, R &&r) noexcept(noexcept(forward<S>(s).connect(forward<R>(r))))
    -> decltype(forward<S>(s).connect(forward<R>(r)));
template <class S, class R>
constexpr auto dispatch(S &&s, R &&r) noexcept(noexcept(impl(forward<S>(s), forward<R>(r))))
    -> decltype(impl(forward<S>(s), forward<R>(r)));

struct connect_t {
  template <class S, class R>
    requires requires(S &&s, R &&r) { { dispatch(forward<S>(s), forward<R>(r)) } -> op_state; }
  constexpr auto operator()(S &&s, R &&r) const
      noexcept(noexcept(dispatch(forward<S>(s), forward<R>(r)))) {
    return dispatch(forward<S>(s), forward<R>(r));
  }
};
inline constexpr connect_t connect{};

template <class S, class R>
auto impl(S &&s, R &&r) noexcept(noexcept(forward<S>(s).connect(forward<R>(r))))
    -> decltype(forward<S>(s).connect(forward<R>(r))) {
  return forward<S>(s).connect(forward<R>(r));
}
template <class S, class R>
constexpr auto dispatch(S &&s, R &&r) noexcept(noexcept(impl(forward<S>(s), forward<R>(r))))
    -> decltype(impl(forward<S>(s), forward<R>(r))) {
  return impl(forward<S>(s), forward<R>(r));
}

struct bad {};
// The alias connects a value that cannot be connected: the call is ill-formed.
template <class S, class R> struct opstate {
  using child_op_t = decltype(connect(static_cast<bad &&>(*static_cast<bad *>(nullptr)), // expected-error {{no matching function for call to object of type 'const connect_t'}}
                                      static_cast<R &&>(*static_cast<rmref_t<R> *>(nullptr))));
};
struct sender {
  template <class R> auto connect(R &&) const -> opstate<sender, R>;
};

int main() {
  sender s;
  int r;
  connect(s, r); // expected-note@* 1+ {{}}
}

// CHECK: CXXMethodDecl {{.*}} invalid constexpr operator() 'auto (sender &, int &) const
