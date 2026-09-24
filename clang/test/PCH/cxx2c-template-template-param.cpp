// RUN: %clang_cc1 -std=c++2c -emit-pch -o %t %s
// RUN: %clang_cc1 -std=c++2c -include-pch %t -verify %s
// RUN: %clang_cc1 -std=c++2c -emit-pch -fpch-instantiate-templates -o %t.inst %s
// RUN: %clang_cc1 -std=c++2c -include-pch %t.inst -verify %s

#ifndef HEADER
#define HEADER

template <typename T> concept Small = sizeof(T) <= 4;
template <typename T> constexpr auto Sz = sizeof(T);

template <template <typename...> concept C, typename T>
constexpr bool check_c = C<T>;
template <template <typename> auto V, typename T>
constexpr auto get_v = V<T>;

template <template <typename> concept C>
struct Holder {
  template <typename T> requires C<T> static constexpr int f() { return 1; }
  template <typename T> requires (!C<T>) static constexpr int f() { return 2; }
};
template <template <typename> auto V>
struct VHolder {
  template <typename T> static constexpr auto value = V<T>;
};

#else

static_assert(check_c<Small, int>);
static_assert(!check_c<Small, long long>);
static_assert(get_v<Sz, char> == 1);
static_assert(Holder<Small>::f<int>() == 1);
static_assert(Holder<Small>::f<long long>() == 2);
static_assert(VHolder<Sz>::value<short> == 2);

// expected-no-diagnostics
#endif
