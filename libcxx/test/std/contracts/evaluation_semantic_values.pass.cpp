// ADDITIONAL_COMPILE_FLAGS: -std=c++26 -fcontracts

// [contracts.syn]: std::contracts::evaluation_semantic has the enumerators
// ignore = 1, observe = 2, enforce = 3 and quick_enforce = 4.

#include <contracts>
#include <type_traits>

using E = std::contracts::evaluation_semantic;

static_assert(std::is_enum_v<E> && !std::is_convertible_v<E, int>);
static_assert(static_cast<int>(E::ignore) == 1);
static_assert(static_cast<int>(E::observe) == 2);
static_assert(static_cast<int>(E::enforce) == 3);
static_assert(static_cast<int>(E::quick_enforce) == 4);

int main(int, char**) { return 0; }
