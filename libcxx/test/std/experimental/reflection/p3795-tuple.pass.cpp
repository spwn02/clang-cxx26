// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// UNSUPPORTED: no-reflection
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <array>
#include <meta>
#include <tuple>
#include <utility>

using namespace std::meta;

int add(int, long);
int add_noexcept(int, long) noexcept;

static_assert(is_applicable_type(^^decltype(&add), ^^std::tuple<int, long>));
static_assert(!is_applicable_type(^^decltype(&add), ^^std::tuple<int>));
static_assert(is_nothrow_applicable_type(^^decltype(&add_noexcept), ^^std::tuple<int, long>));
static_assert(!is_nothrow_applicable_type(^^decltype(&add), ^^std::tuple<int, long>));
static_assert(apply_result(^^decltype(&add), ^^std::tuple<int, long>) == ^^int);

struct sum {
  double operator()(int, double) const;
};
struct sum_noexcept {
  double operator()(int, double) const noexcept;
};
struct lvalues {
  int& operator()(int&, double&) const noexcept;
};
struct const_lvalues {
  const int& operator()(const int&, const double&) const noexcept;
};
struct rvalues {
  int&& operator()(int&&, double&&) && noexcept;
};

template <class F, class Tuple>
consteval bool check() {
  if (is_applicable_type(^^F, ^^Tuple) != std::is_applicable_v<F, Tuple> ||
      is_nothrow_applicable_type(^^F, ^^Tuple) != std::is_nothrow_applicable_v<F, Tuple>)
    return false;
  if constexpr (std::is_applicable_v<F, Tuple>) {
    return apply_result(^^F, ^^Tuple) == dealias(^^std::apply_result_t<F, Tuple>);
  } else {
    try { (void)apply_result(^^F, ^^Tuple); }
    catch (const exception&) { return true; }
    return false;
  }
}

template <class Tuple>
consteval bool check_tuple() {
  return check<sum, Tuple>() && check<sum_noexcept, Tuple>() &&
         check<lvalues, Tuple>() && check<const_lvalues, Tuple>() &&
         check<rvalues, Tuple>() && check<rvalues&, Tuple>() &&
         check<int, Tuple>();
}

template <class Tuple>
consteval bool check_cvref() {
  return check_tuple<Tuple>() && check_tuple<Tuple&>() && check_tuple<Tuple&&>() &&
         check_tuple<const Tuple>() && check_tuple<const Tuple&>() &&
         check_tuple<const Tuple&&>() && check_tuple<volatile Tuple>() &&
         check_tuple<volatile Tuple&>() && check_tuple<const volatile Tuple&&>();
}

static_assert(check_cvref<std::tuple<int, double>>());
static_assert(check_cvref<std::pair<int, double>>());
static_assert(check_cvref<std::array<int, 2>>());
static_assert(check_cvref<std::tuple<int>>());
static_assert(check_cvref<std::tuple<>>());

// Specializing tuple_size/tuple_element alone does not make a type tuple-like.
struct custom_tuple {};
namespace std {
template <> struct tuple_size<custom_tuple> : integral_constant<size_t, 2> {};
template <size_t I> struct tuple_element<I, custom_tuple> { using type = int; };
}
static_assert(!std::is_applicable_v<sum, custom_tuple>);
static_assert(check_cvref<custom_tuple>());
static_assert(apply_result(^^lvalues, ^^std::tuple<int, double>&) == ^^int&);
static_assert(apply_result(^^const_lvalues, ^^const std::tuple<int, double>&) == ^^const int&);
static_assert(apply_result(^^rvalues, ^^std::tuple<int, double>&&) == ^^int&&);

consteval bool rejects_non_type(info bad) {
  for (bool first : {false, true}) {
    info fn = first ? bad : ^^sum;
    info tuple = first ? ^^std::tuple<int, double> : bad;
    try { (void)is_applicable_type(fn, tuple); return false; }
    catch (const exception&) {}
    try { (void)is_nothrow_applicable_type(fn, tuple); return false; }
    catch (const exception&) {}
    try { (void)apply_result(fn, tuple); return false; }
    catch (const exception&) {}
  }
  return true;
}
static_assert(rejects_non_type(^^::));
static_assert(rejects_non_type(reflect_constant(42)));

// Arguments that are not tuple-like: [meta.rel] is_applicable requires
// tuple-like<Tuple>, so the trait is false (not a hard error) and the wrappers
// agree with it; apply_result has no 'type' member, so the wrapper throws.
struct not_a_tuple { int a; };
static_assert(!is_applicable_type(^^sum, ^^int));
static_assert(!is_applicable_type(^^sum, ^^not_a_tuple));
static_assert(!is_nothrow_applicable_type(^^sum_noexcept, ^^void*));
static_assert(check_tuple<int>());
static_assert(check_tuple<void*>());
static_assert(check_tuple<not_a_tuple>());
static_assert(check_tuple<not_a_tuple&>());

int main() {}
