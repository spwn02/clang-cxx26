// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++20 -emit-llvm -o /dev/null -verify %s

// The noexcept operand of a function template specialization whose substitution
// failed (an error diagnosed in a class instantiated while the operand was
// evaluated) is only a recovery expression. It must not make the function type
// dependent: that dependence leaked into the types built from the function (here
// the result of decltype of a call), and a class with a member of such a type
// reached the layout code with a dependent type (assertion in getTypeInfoImpl).

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

using P = decltype(dispatch(*static_cast<sender *>(nullptr), *static_cast<int *>(nullptr)));
struct Q { P p; };
Q q;
void local() {
  P x;
  (void)sizeof(P);
  (void)alignof(P);
}

int main() {
  sender s;
  int r;
  connect(s, r); // expected-error {{no matching function for call to object of type 'const connect_t'}} expected-note@* 1+ {{}}
}
