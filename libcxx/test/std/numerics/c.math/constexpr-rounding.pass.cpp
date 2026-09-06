// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <cmath>
#include <cstdlib>

static_assert(std::floor(3.75) == 3.0);
static_assert(std::floor(-3.75) == -4.0);
static_assert(std::ceil(3.25) == 4.0);
static_assert(std::ceil(-3.25) == -3.0);
static_assert(std::trunc(3.75) == 3.0);
static_assert(std::trunc(-3.75) == -3.0);
static_assert(std::round(2.5) == 3.0);
static_assert(std::round(-2.5) == -3.0);
static_assert(std::nearbyint(2.5) == 2.0);
static_assert(std::nearbyint(-2.5) == -2.0);
static_assert(std::rint(2.5) == 2.0);
static_assert(std::rint(-2.5) == -2.0);
static_assert(std::fmod(-5.5, 2.0) == -1.5);
static_assert(std::remainder(5.5, 2.0) == -0.5);

static_assert(std::lround(2.5) == 3);
static_assert(std::lround(-2.5) == -3);
static_assert(std::llround(-2.5) == -3);
static_assert(std::llround(2.5) == 3);
static_assert(std::lrint(2.5) == 2);
static_assert(std::lrint(-2.5) == -2);
static_assert(std::llrint(-2.5) == -2);
static_assert(std::llrint(2.5) == 2);
