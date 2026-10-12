#ifndef REFLECTION_ENTITY_IDENTITY_H
#define REFLECTION_ENTITY_IDENTITY_H
using info = decltype(^^int);
struct sentinel {};
// Internal range entry points: bases_of=2, parameters_of=98, members_of=4/5.
consteval info base(info r, unsigned i = 0) {
  return __metafunction(2, r, ^^sentinel, i, ^^base);
}
consteval info parameter(info r, unsigned i = 0) {
  return __metafunction(98, r, ^^sentinel, i, ^^parameter);
}
consteval info member(info r) {
  return __metafunction(4, r, ^^sentinel, ^^member);
}
consteval info next_member(info r) {
  return __metafunction(5, r, ^^sentinel, ^^next_member);
}

enum E { first_enum = 1, second_enum = 1 };
enum class Scoped { first = 1, second = 1 };
enum { first_anonymous = 1, second_anonymous = 1 };
static_assert(^^first_enum != ^^second_enum);
static_assert(^^Scoped::first != ^^Scoped::second);
static_assert(^^first_anonymous != ^^second_anonymous);

struct B {};
struct B2 {};
struct D : B {};
struct E2 : B {};
struct Multiple : B, B2 {};
constexpr info d_base = base(^^D);
constexpr info e_base = base(^^E2);
constexpr info multiple_first = base(^^Multiple);
constexpr info multiple_second = base(^^Multiple, 1);
static_assert(d_base != e_base);
static_assert(multiple_first != multiple_second);

void first_function(int, double);
void second_function(int, double);
constexpr info first_parameter = parameter(^^first_function);
constexpr info second_parameter = parameter(^^first_function, 1);
constexpr info other_parameter = parameter(^^second_function);
static_assert(first_parameter != second_parameter);
static_assert(first_parameter != other_parameter);
inline void first_function(int renamed, double also_renamed) {
  static_assert(parameter(^^first_function) == first_parameter);
  static_assert(parameter(^^first_function, 1) == second_parameter);
}

struct C { C(int); C(double); };
constexpr info first_constructor = member(^^C);
constexpr info second_constructor = next_member(first_constructor);
constexpr info first_constructor_parameter = parameter(first_constructor);
constexpr info second_constructor_parameter = parameter(second_constructor);
static_assert(first_constructor_parameter != second_constructor_parameter);

template <info R> int code() {
  if constexpr (R == ^^first_enum) return 1;
  else if constexpr (R == ^^second_enum) return 2;
  else if constexpr (R == ^^Scoped::first) return 3;
  else if constexpr (R == ^^Scoped::second) return 4;
  else if constexpr (R == ^^first_anonymous) return 5;
  else if constexpr (R == ^^second_anonymous) return 6;
  else if constexpr (R == d_base) return 7;
  else if constexpr (R == e_base) return 8;
  else if constexpr (R == multiple_first) return 9;
  else if constexpr (R == multiple_second) return 10;
  else if constexpr (R == first_parameter) return 11;
  else if constexpr (R == second_parameter) return 12;
  else if constexpr (R == other_parameter) return 13;
  else if constexpr (R == first_constructor_parameter) return 14;
  else if constexpr (R == second_constructor_parameter) return 15;
  else return -1;
}
#endif
