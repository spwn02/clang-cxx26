// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s
// expected-no-diagnostics

namespace std {
template<class> struct tuple_size;
}

struct Triple {
  int first;
  int second;
  int third;
};

constexpr int placeholder_pack(auto value) {
  return match (value) {
    case auto&& [first, ..._, last] => first + last;
  };
}

constexpr int unnamed_pack(auto value) {
  return match (value) {
    case auto&& [first, ..., last] => first + last;
  };
}

static_assert(placeholder_pack(Triple{1, 2, 3}) == 4);
static_assert(unnamed_pack(Triple{1, 2, 3}) == 4);

// P2688R6 13.6: the elements of a decomposition pack are forwarded with the
// value category and cv-qualification of the subject.
namespace forwarding_declaration_pack {
struct Pair {
  int first;
  int second;
};

template <class T>
constexpr T&& forward(__remove_reference_t(T)& value) noexcept {
  return static_cast<T&&>(value);
}

template <class...>
struct types {};

void accept(types<int&, int&>);
void accept(types<int&&, int&&>);
void accept(types<const int&, const int&>);
void accept(types<const int&&, const int&&>);

template <class T>
void match_case(T&& value) {
  match (static_cast<T&&>(value)) {
    case [auto&& ...args] =>
      accept(types<decltype(forward<decltype(args)>(args))...>{});
  }
}

void instantiate() {
  Pair value{};
  const Pair const_value{};
  match_case(value);
  match_case(static_cast<Pair&&>(value));
  match_case(const_value);
  match_case(static_cast<const Pair&&>(const_value));
}
} // namespace forwarding_declaration_pack
