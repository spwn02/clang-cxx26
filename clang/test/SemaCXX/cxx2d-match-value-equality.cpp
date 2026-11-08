// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s

// P2688R6 6.2: a value pattern is viable whenever 'bool(subject == E)' is
// well-formed, whatever the operand types, and 11: bool requires true and
// false. The comparison is the ordinary one, so an integer constant covers the
// state it equals.

namespace std {
template <class T>
struct alternative_traits;

struct alternative_info {
  decltype(^^int) info = {};
  bool empty = false;

  consteval alternative_info(decltype(^^int) info = {}, bool empty = false)
      : info(info), empty(empty) {}
};
} // namespace std

int bool_covered_by_integers(bool value) {
  return match (value) {
    case 1 => 0;
    case 0 => 1;
  };
}

int bool_covered_by_integer_and_bool(bool value) {
  return match (value) {
    case true => 0;
    case 0 => 1;
  };
}

int int_by_bool_is_not_exhaustive(int value) {
  return match (value) { // expected-error {{match expression is not exhaustive; example of a missing case: 2}}
    case true => 1;
    case false => 2;
  };
}

// 10.1: a dependent source case stays potentially useful, so a value that
// cannot occur for one specialization is not an error there.
template <class T>
int dependent_value(T value) {
  return match (value) {
    case 300 => 1;
    case _ => 0;
  };
}

template <class T>
int dependent_bool_cases(T value) {
  return match (value) {
    case true => 1;
    case false => 2;
    case _ => 0;
  };
}

template <class T>
int dependent_bool_exhaustive(T value) {
  return match (value) { // expected-error {{match expression is not exhaustive; example of a missing case: 2}}
    case true => 1;
    case false => 2;
  };
}

int instantiate() {
  return dependent_value((unsigned char)1) + dependent_value(300) +
         dependent_value(300L) + dependent_bool_cases(true) +
         dependent_bool_cases(1) + dependent_bool_cases(2L) +
         dependent_bool_exhaustive(true) +
         dependent_bool_exhaustive(0); // expected-note {{in instantiation of function template specialization 'dependent_bool_exhaustive<int>' requested here}}
}

// A braced value pattern covers every alternative it equals.
struct IntOrInt {
  unsigned state;
  int value;
};

template <>
struct std::alternative_traits<IntOrInt> {
  static constexpr alternative_info alternatives[] = {^^int, ^^int};
  static constexpr bool has_residual_states = false;

  struct names {
    __SIZE_TYPE__ value;

    template <__SIZE_TYPE__ I>
    static constexpr __SIZE_TYPE__ index = I;

    constexpr operator __SIZE_TYPE__() const noexcept { return value; }
    friend constexpr bool operator==(names, names) = default;
  };

  static constexpr names index(const IntOrInt &value) noexcept {
    return {value.state};
  }

  template <__SIZE_TYPE__ I, class Self>
  static constexpr decltype(auto) get(Self &&value) {
    return (static_cast<Self &&>(value).value);
  }
};

int braced_bool_on_integers(IntOrInt value) {
  return match (value) { // expected-error {{match expression is not exhaustive; example of a missing case: { 0 }}}
    case { true } => 1;
  };
}
