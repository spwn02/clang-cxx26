// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify -fcxx-exceptions -fexceptions %s

// The pattern matching examples of P2806R5 (blocks 3-9 and 32), which the
// do-expression test (cxx29-p2806r5-examples.cpp) leaves to this one: the paper
// writes them in the syntax of an earlier P2688 revision (`x match { 0 => ... }`),
// here in the syntax of P2688R6 (`match (x) { case 0 => ... }`).

namespace std {
template <class T, class U> struct pair { T first; U second; pair(T, U); };
template <class T, class U> pair(T, U) -> pair<T, U>;
template <class E> struct unexpected { E e; explicit unexpected(E x) : e(x) {} };
template <class E> unexpected(E) -> unexpected<E>;
struct string {};
template <class T, class E> struct expected {
  explicit operator bool() const;
  T &operator*();
  const E &error() const;
  expected(T);
  expected(unexpected<E>);
};
} // namespace std

#define FWD(x) static_cast<decltype(x)&&>(x)
struct ostream { ostream &operator<<(const char *); } cout;
void statements(int x) {
  match (x) {
    case 0 => { cout << "got zero"; }
    case 1 => { cout << "got one"; }
    case _ => { cout << "don't care"; }
  }
  match (x) {
    case 0 => (void)do { cout << "got zero"; };
    case 1 => (void)do { cout << "got one"; };
    case _ => (void)do { cout << "don't care"; };
  }
}

auto pair_ok(int i) {
  return match (i) -> std::pair<int, int> {
    case 0 => {1, 2};
    case _ => std::pair{3, 4};
  };
}
auto pair_bad(int i) {
  return match (i) {
    case 0 => {1, 2}; // expected-error {{cannot deduce return type from initializer list}}
    case _ => std::pair{3, 4};
  };
}

std::expected<int, std::string> source();
std::expected<int, std::string> try_it() {
  int v = match (source()) -> decltype(auto) {
    case auto&& __r => do -> decltype(auto) {
      if (not __r) {
        return std::unexpected(__r.error());
      }
      do_return *FWD(__r);
    };
  };
  return v + 1;
}
